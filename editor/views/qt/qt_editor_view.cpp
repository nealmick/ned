#include "qt_editor_view.h"

#include "../../services/diagnostics/diagnostic_colors.h"
#include "../../util/text_columns.h"
#include "../../util/utf8.h"

#include "../../../util/settings.h"
#include "ned_color_qt.h"
#include "qt_find_bar.h"
#include "qt_hover_tip.h"
#include "qt_line_jump.h"

#include <QMenu>
#include <QShortcut>

#include "qt_fonts.h"
#include "qt_theme.h"
#include "util/qt_icons.h"
#include <QApplication>
#include <QFontMetrics>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QScrollBar>
#include <QTimer>
#include <QWheelEvent>

#include <cmath>
#include <cstring>
#include <fstream>
#include <iostream>
#include <sstream>

namespace {
// The shared WrapLayout stores its glyph metrics in PROCESS-WIDE statics.
// A functor capturing a VIEW pointer dangles once that view is closed
// while other splits keep wrapping — install value-owned copies instead
// (all editors share the profile font, so the static stays coherent).
// Metrics are the MONOSPACE GRID cell width, not per-glyph font advances:
// the Qt painter, caret, selection and hit-testing all position glyphs on
// the cell grid (charWidthF), so the wrap layout must break segments on
// the same grid or wrapped rows overflow/underflow their measured width.
// (The ImGui backend renders with real font advances, so IT installs
// ImFont advances — each backend matches its own renderer.)
void installWrapMetrics(qreal cellWidth)
{
	WrapLayout::setGlyphWidthFn([cellWidth](const char *, const char *) {
		return static_cast<float>(cellWidth);
	});
	WrapLayout::setSpaceWidthFn([cellWidth](const char *, const char *) {
		return static_cast<float>(cellWidth);
	});
}
} // namespace

QtEditorView::QtEditorView(Settings &settings, QWidget *parent)
	: QWidget(parent),
	  appSettings(settings),
	  projectUndo(projectRoot),
	  state(),
	  events(),
	  ops(state),
	  viewState(state),
	  save(state, events),
	  highlight(state, ops, &settings),
	  git(state, projectRoot, appSettings),
	  commands(state, viewState, ops, projectUndo, events, save)
{
	// Host check modes (NED_QT_*_CHECK in main.cpp) locate the editor by
	// object name.
	setObjectName(QStringLiteral("__ned_editor"));

	setFontFromSettings();

	// Blink runs entirely off blinkClock in the service timer — a second
	// timer restarting the clock raced the sampler and could strand the
	// caret in the invisible phase permanently.

	scrollBar = new QScrollBar(Qt::Vertical, this);
	connect(scrollBar, &QScrollBar::valueChanged, this, [this](int v) {
		// The bar moves in whole lines; internal sync drives scrollPx.
		if (!syncingScroll)
			scrollPx = v * lineHeightPx;
		update();
	});

	// Horizontal scrollbar (wrap off, ImGui HorizontalScrollbar parity):
	// thin overlay strip above the bottom edge, styled by #nedHScroll in the
	// app sheet (the global QScrollBar rule zeroes EVERY scrollbar; the id
	// selector outranks it). Pixel units — sub-cell precision like the wheel.
	hScrollBar = new QScrollBar(Qt::Horizontal, this);
	hScrollBar->setObjectName(QStringLiteral("nedHScroll"));
	hScrollBar->hide();
	connect(hScrollBar, &QScrollBar::valueChanged, this, [this](int v) {
		if (!syncingScrollX)
			setScrollXPixels(static_cast<qreal>(v));
	});

	// Services (async tree-sitter, autosave, git status) expect per-frame
	// polling; the Qt backend has no frame loop, so a short timer drives
	// them — plus the caret blink.
	serviceTimer = new QTimer(this);
	serviceTimer->setInterval(30);
	blinkClock.start();
	connect(serviceTimer, &QTimer::timeout, this, [this] {
		highlight.poll();
		git.poll();
		if (highlight.visualGeneration() != lastVisualGen)
		{
			lastVisualGen = highlight.visualGeneration();
			update();
		}
		if (git.currentGitChanges != lastGitChanges)
		{
			lastGitChanges = git.currentGitChanges;
			update();
		}
		// LSP diagnostics arrive on the client's reader thread; the store's
		// revision tells us a repaint is due (squiggles + gutter marks).
		if (diagStore && diagStore->revision() != diagRevisionSeen)
		{
			diagRevisionSeen = diagStore->revision();
			update();
		}
		// Hover delay elapse: let an armed trigger fire under the parked
		// mouse (rule 4 of HoverTrigger — no re-arm without movement).
		if (hoverTrigger.info().active || underMouse())
			updateHover(false, false, lastHoverPos);
		// Blink at ~1.9 Hz; repaint only when the caret flips visibility.
		// Unfocused dock siblings keep the caret hidden — skip their repaints.
		if (caretActive())
		{
			const bool next = (blinkClock.elapsed() % 1060) < 530;
			if (next != caretVisible)
			{
				caretVisible = next;
				update();
			}
		}
	});
	serviceTimer->start();

	setFocusPolicy(Qt::StrongFocus);
	setMouseTracking(true);

	// DidEdit fan-out mirrors the ImGui Editor: highlight, autosave, git
	// gutter, wrap dirty-span, h-scroll range. The wrap noteEdit is the
	// piece the Qt view used to miss: without it, edits that keep the line
	// count unchanged never re-wrapped (ensure() only rescans a dirty span
	// or rebuilds on line-count/width change) — wrap went stale mid-typing
	// and every row<->visual-line mapping drifted.
	events.subscribeDidEdit([this](const EditorEvents::DidEdit &e) {
		highlight.highlightContent();
		save.onDidEdit();
		git.onDidEdit(e.firstRow, e.lastRow);
		wrap.noteEdit(e.firstRow, e.lastRow);	   // re-wrap dirty span
		refreshLongestLine(e.firstRow, e.lastRow); // h-scroll range
	});

	findBar = new QtFindBar(this, this);
	// Wrap toggles change the bar's height — re-displace the text area.
	connect(findBar, &QtFindBar::heightChanged, this, [this] {
		if (!findBar->isVisible())
			return;
		findBarPx = findBar->sizeHint().height();
		findBar->setGeometry(0, titleBarPx, width(), findBarPx);
		setScrollPixels(scrollPx); // fewer visible lines — re-clamp
		update();
	});
	// NOTE: no per-view Ctrl+F / Ctrl+; QShortcuts here — one per view
	// means two live views (background tab, split) make the sequence
	// ambiguous and Qt fires NOTHING. The host owns the single shortcuts
	// and routes them to the active view (see NedQtHost's keybinds).
	// live views (background tab, split) make the sequence ambiguous and
	// Qt fires NOTHING. The host owns the single shortcut and routes it
	// to the active view (see NedQtHost's keybinds).
}

QtEditorView::~QtEditorView() = default;

void QtEditorView::inputMethodEvent(QInputMethodEvent *event)
{
	// IME preedit renders inline as hollow text; commit goes through the
	// normal typing path (CJK input, dead keys, dictation).
	if (!event->commitString().isEmpty())
	{
		commands.typeText(event->commitString().toUtf8().constData());
		afterEdit();
	}
	setAttribute(Qt::WA_InputMethodEnabled);
	update();
}

QVariant QtEditorView::inputMethodQuery(Qt::InputMethodQuery query) const
{
	if (query == Qt::ImEnabled)
		return true;
	return QWidget::inputMethodQuery(query);
}

void QtEditorView::openWorkspaceRoot(const std::string &root)
{
	projectRoot = root;
	git.init();
}

// --- LspEditor seam ---------------------------------------------------------

void QtEditorView::getCaret(int &row, int &column) const
{
	row = viewState.row;
	column = viewState.column;
}

std::string QtEditorView::line(int row) const { return state.line(row); }

const std::string &QtEditorView::path() const { return state.path; }

const std::string &QtEditorView::languageId() const { return state.languageId; }

NedColor QtEditorView::defaultTextColor() const { return highlight.defaultTextColor(); }

NedColor QtEditorView::syntaxColor(ThemeSlot slot) const
{
	return highlight.colorForSlot(slot);
}

void QtEditorView::requestCursorCenter(int row, int column)
{
	// Qt lays out synchronously — no deferred post-layout schedule needed
	// (ImGui's viewState::requestCursorCenter); center like goToLine does.
	commands.setCursor(row, column, false, EditorCommands::CursorReveal::ensure);
	ensureWrapFresh(); // LSP jumps arrive outside the paint/afterEdit paths
	setScrollPixels(std::max<qreal>(
		0.0,
		(static_cast<qreal>(visualLineOf(row, column) - visibleLines() / 2) *
		 static_cast<qreal>(lineHeightPx))));
	revealCaret();
	scheduleBlink();
}

void QtEditorView::setDiagnostics(const LSPDiagnostics *store)
{
	diagStore = store;
	diagRevisionSeen = store ? store->revision() : 0;
	updateGutterWidth(); // the severity column appears with the first store
	update();
}

void QtEditorView::setFontFromSettings()
{
	// One resolution rule (NedQtFonts::profileMonoFont) shared with the
	// terminal panel: profile family when registered, Menlo/Consolas
	// fallback, always fixed-pitch.
	const QFont font = NedQtFonts::profileMonoFont(appSettings);
	setFont(font);

	const QFontMetrics metrics(font);
	const int oldLineHeight = lineHeightPx;
	lineHeightPx = metrics.height();
	// Monospace advance: '0' is reliably full-width; ' ' can be narrower.
	cellWidth = metrics.horizontalAdvance(QLatin1String("0000")) / 4.0;
	// Font changed: the shared wrap layout's metric statics must follow —
	// grid cells (see installWrapMetrics); its cache keys on the space
	// width, so ensure() rebuilds itself.
	installWrapMetrics(cellWidth);
	// Cell width changed: the longest-line cache is now stale.
	widthDirty = true;

	updateGutterWidth();
	// Keep the viewport anchored when the line height changes (font zoom).
	if (oldLineHeight > 0)
	{
		scrollPx = scrollPx * lineHeightPx / oldLineHeight;
		// The rescale can push scrollPx past the (shrunk) range — clamp or
		// the first wheel-ups dead-clamp back to the bottom.
		scrollPx = std::clamp(scrollPx, 0.0, static_cast<qreal>(maxScrollPx()));
	}
}

void QtEditorView::updateGutterWidth()
{
	// ImGui parity (gutter_view.cpp): the number column only needs to fit
	// max(999, lineCount + 1) — three digits for most files, growing when
	// the document passes 999 lines. Numbers right-align inside it with a
	// 12px pad on the right and 4px of breathing room at the left edge.
	// With diagnostics bound, a severity-mark column (ImGui parity) sits
	// in front of the numbers.
	const int reference = std::max(999, state.lineCount() + 1);
	gutterWidthPx = fontMetrics().horizontalAdvance(QString::number(reference)) + 12 + 4 +
					diagnosticColumnWidth();
}

int QtEditorView::diagnosticColumnWidth() const
{
	if (!diagStore)
		return 0;
	return std::max(6, static_cast<int>(fontMetrics().height() * 0.55));
}

void QtEditorView::openFile(const QString &path)
{
	std::string raw;
	if (!path.isEmpty())
	{
		std::ifstream file(path.toStdString(), std::ios::binary);
		if (!file.is_open())
		{
			// Errors flow toward the user: an unreadable path opens as an
			// untitled buffer carrying the reason, never as a silent
			// "empty file" still bound to that path (git/LSP/save would
			// all act on a document we never actually read).
			state.path.clear();
			raw = "Could not open file:\n" + path.toStdString() + "\n" +
				  std::strerror(errno);
		} else
		{
			std::stringstream buffer;
			buffer << file.rdbuf();
			raw = buffer.str();
			state.path = path.toStdString();
			state.languageId = EditorState::languageIdFromPath(state.path);
		}
	} else
		state.path.clear();

	reloadFileIcon();
	state.setFromString(raw);
	updateGutterWidth();
	ops.clearPending();
	ops.bumpGeneration();
	viewState.setBoth(0, 0);
	highlight.resetForDocument(static_cast<size_t>(state.lineCount()));
	highlight.highlightContent();
	// New document: reset both axes and rebuild the longest-line cache
	// (refreshWrap re-ensures the wrap layout + scrollbars after it).
	widthDirty = true;
	scrollPxX = 0.0;
	refreshWrap();
	git.init();
	git.onDocumentOpened();
	lastVisualGen = highlight.visualGeneration();
	setScrollPixels(0);
	update();
}

QSize QtEditorView::sizeHint() const { return QSize(800, 600); }

int QtEditorView::visibleLines() const
{
	return std::max(1, (height() - topInset()) / lineHeightPx);
}

int QtEditorView::maxScrollLine() const
{
	// Last line fully visible at bottom: allow scrolling past it a little
	// (ImGui scrolls to keep the caret line plus context visible).
	return std::max(0, totalLines() - visibleLines() + 2);
}

int QtEditorView::maxScrollPx() const { return maxScrollLine() * lineHeightPx; }

int QtEditorView::firstVisualLine() const
{
	return std::max(0, static_cast<int>(scrollPx / lineHeightPx));
}

// Screen y of the first painted visual line. With fractional scroll the
// top row slides up under the top inset (paint clips there).
qreal QtEditorView::rowYBase() const
{
	return static_cast<qreal>(topInset()) - (scrollPx - firstVisualLine() * lineHeightPx);
}

void QtEditorView::setScrollPixels(qreal px)
{
	scrollPx = std::clamp(px, 0.0, static_cast<qreal>(maxScrollPx()));
	syncingScroll = true;
	scrollBar->setValue(
		std::clamp(static_cast<int>(scrollPx / lineHeightPx + 0.5), 0, maxScrollLine()));
	syncingScroll = false;
	update();
}

int QtEditorView::rowAtY(int y) const
{
	const qreal v = (scrollPx + std::max(0, y - topInset())) / lineHeightPx;
	if (wordWrapEnabled())
	{
		// v is CONTINUOUS (the pointer can be anywhere within a visual
		// line) — yToRow floors it directly. An extra +0.5 here pushed
		// everything in a row's lower half down to the next visual line.
		const WrapLayout::Hit hit = wrap.yToRow(static_cast<float>(v));
		return std::clamp(hit.row, 0, std::max(0, state.lineCount() - 1));
	}
	return std::clamp(static_cast<int>(v), 0, std::max(0, state.lineCount() - 1));
}

int QtEditorView::columnAtX(int row, int x, int segmentStart) const
{
	if (wordWrapEnabled())
	{
		// ImGui parity (editor_input.cpp rowColFromMouse): map through the
		// shared wrap layout so continuation segments resolve against their
		// own start column — the monospace byte-map below knows nothing
		// about segments and lands clicks on the wrong columns.
		const std::string line = state.line(row);
		if (line.empty())
			return 0;
		const int seg = wrap.segmentOf(row, segmentStart);
		const int column = wrap.columnAt(line, row, seg, x - gutterWidthPx);
		return EditorUtils::SnapToUtf8CharBoundary(line, column);
	}
	// Wrap off: x is screen space — shift by the horizontal scroll offset.
	const int textX = x - gutterWidthPx + static_cast<int>(scrollPxX);
	if (textX <= 0)
		return segmentStart;
	return std::clamp(
		byteColumnAtX(row, static_cast<qreal>(textX), 0), 0, state.lineLength(row));
}

// --- Tab-expanded rendering model (qt_row_text.h) ---------------------------

qreal QtEditorView::charWidthF() const
{
	// Cached at font-set time; the single width source for the whole view.
	return cellWidth;
}

QtEditorView::RowText QtEditorView::expandRow(int row, int segmentStart) const
{
	return expandRowText(state.line(row), segmentStart);
}

// Wrap-aware caret placement: the visual line holding (row, column) —
// the row's base visual line plus the wrap segment the column falls in.
int QtEditorView::visualLineOf(int row, int column) const
{
	if (!wordWrapEnabled())
		return row;
	return wrap.rowStartVisualLine(row) + wrap.segmentOf(row, column);
}

// Byte column where the wrap segment containing (row, column) starts —
// tab stops rebase there (the shared wrap layout measures flush-left).
int QtEditorView::caretSegmentStart(int row, int column) const
{
	return wrap.segmentStartColumn(row, wrap.segmentOf(row, column));
}

qreal QtEditorView::xAtByteColumn(int row, int byteColumn, int segmentStart) const
{
	// Visual columns are counted from segmentStart (wrapped rows restart
	// their tab stops at the segment edge, like the ImGui wrap layout).
	const RowText rt = expandRow(row, segmentStart);
	const int last = static_cast<int>(rt.byteToVisual.size() - 1);
	const int segBase = rt.byteToVisual[std::clamp(segmentStart, 0, last)];
	const int col = rt.byteToVisual[std::clamp(byteColumn, 0, last)];
	return static_cast<qreal>(std::max(0, col - segBase)) * charWidthF();
}

int QtEditorView::byteColumnAtX(int row, qreal x, int segmentStart) const
{
	const RowText rt = expandRow(row, segmentStart);
	int visual = static_cast<int>(std::round(x / charWidthF()));
	visual = std::clamp(visual, 0, static_cast<int>(rt.visualToByte.size() - 1));
	return rt.visualToByte[static_cast<size_t>(visual)];
}

bool QtEditorView::wordWrapEnabled() const
{
	return appSettings.settings.value("word_wrap", false);
}

int QtEditorView::textAreaWidth() const
{
	// Minimap-aware: wrapped rows must break BEFORE the minimap strip, or
	// they would paint underneath it.
	return std::max(50, width() - gutterWidthPx - minimapWidth() - 14);
}

int QtEditorView::totalLines() const
{
	return wordWrapEnabled() ? std::max(1, wrap.totalVisualLines())
							 : std::max(1, state.lineCount());
}

void QtEditorView::ensureWrapFresh()
{
	if (wordWrapEnabled())
		wrap.ensure(state, static_cast<float>(textAreaWidth()));
}

void QtEditorView::refreshWrap()
{
	if (wordWrapEnabled())
	{
		ensureWrapFresh();
		scrollPxX = 0.0; // no horizontal scroll in wrap mode
	} else
		wrap.invalidate();
	// Re-clamp: the scroll range can shrink (font zoom rescales scrollPx
	// unclamped, wrap re-measures, deletions) — a scrollPx past the max
	// makes the first wheel-ups clamp straight back to the bottom.
	scrollPx = std::clamp(scrollPx, 0.0, static_cast<qreal>(maxScrollPx()));
	scrollBar->setRange(0, maxScrollLine());
	syncHScrollBar();
}

// --- Horizontal scroll (wrap off) ------------------------------------------

qreal QtEditorView::maxScrollPxX()
{
	ensureLongestLine();
	// Two cells of overshoot past the longest line (ImGui scroll-range pad).
	return std::max<qreal>(0.0, widthMaxPx - textAreaWidth() + charWidthF() * 2.0);
}

void QtEditorView::setScrollXPixels(qreal px)
{
	scrollPxX = std::clamp(px, 0.0, maxScrollPxX());
	syncHScrollBar();
	update();
}

void QtEditorView::syncHScrollBar()
{
	// Overlay strip: only when wrap is off AND lines overflow. Range in
	// pixels so dragging matches the wheel's sub-cell precision.
	const int maxPx = static_cast<int>(maxScrollPxX());
	hScrollBar->setVisible(!wordWrapEnabled() && maxPx > 0);
	syncingScrollX = true;
	hScrollBar->setRange(0, maxPx);
	hScrollBar->setPageStep(std::max(1, textAreaWidth()));
	hScrollBar->setValue(static_cast<int>(scrollPxX));
	syncingScrollX = false;
}

void QtEditorView::ensureLongestLine()
{
	if (!widthDirty)
		return;
	widthDirty = false;
	refreshLongestLine(0, state.lineCount() - 1);
}

// Incremental longest-line cache (editor_frame.cpp scheme): scan the dirty
// span; keep the old max as a safe overestimate when the longest row
// shrank (avoids an O(n) rescan per keystroke).
void QtEditorView::refreshLongestLine(int lo, int hi)
{
	if (hi < lo)
		return;
	qreal localMax = 0.0;
	int localLongest = -1;
	for (int r = lo; r <= hi && r < state.lineCount(); ++r)
	{
		// Painted width = tab-expanded CELLS * cell width (the monospace
		// grid paintTextRow draws on) — visualCount counts glyphs, so an
		// astral-plane char is one cell, not two UTF-16 units.
		const qreal w = expandRow(r).visualCount() * charWidthF();
		if (w > localMax)
		{
			localMax = w;
			localLongest = r;
		}
	}
	const bool longestInDirty =
		widthLongestRow >= lo && widthLongestRow <= hi && widthLongestRow >= 0;
	if (localMax > widthMaxPx)
	{
		widthMaxPx = localMax;
		widthLongestRow = localLongest;
	} else if (longestInDirty && localMax >= widthMaxPx)
	{
		widthMaxPx = localMax;
		widthLongestRow = localLongest;
	} else if (longestInDirty)
		widthLongestRow = -1; // overestimate stays; range only shrinks lazily
}

// y -> document row + wrap-segment start (byte column of the segment).
QtEditorView::RowHit QtEditorView::hitTestY(int y) const
{
	const qreal v = (scrollPx + std::max(0, y - topInset())) / lineHeightPx;
	if (wordWrapEnabled())
	{
		// v is CONTINUOUS (a click lands anywhere within a visual line), so
		// yToRow floors it directly — the extra +0.5 here used to resolve
		// clicks in a row's lower half onto the NEXT visual line, putting
		// the caret a few columns past the click on wrapped rows.
		const WrapLayout::Hit hit = wrap.yToRow(static_cast<float>(v));
		RowHit out;
		out.row = std::clamp(hit.row, 0, std::max(0, state.lineCount() - 1));
		out.segmentStart = wrap.segmentStartColumn(out.row, hit.segment);
		return out;
	}
	return {std::clamp(static_cast<int>(v), 0, std::max(0, state.lineCount() - 1)), 0};
}

bool QtEditorView::minimapEnabled() const
{
	return appSettings.settings.value("minimap", true);
}

int QtEditorView::minimapWidth() const { return minimapEnabled() ? 70 : 0; }

void QtEditorView::paintMinimap(QPainter &painter)
{
	const int mw = minimapWidth();
	if (mw <= 0)
		return;
	const int x0 = width() - mw;

	// Density model (ImGui minimap_view parity): ~2px rows, 1px cols,
	// dots at 75% row height, colors dimmed to 72%.
	const qreal charW = 1.0;
	const qreal padX = 2.0;
	const int stripTop = topInset();
	const qreal stripH = static_cast<qreal>(height() - stripTop);
	const int maxCols = std::max(1, static_cast<int>((mw - 2 * padX) / charW));

	// Visible-window strip: geometry shared with minimapScrollTo (one
	// definition in QtMinimap — see the undershoot bug note there).
	const QtMinimap::Geometry geo =
		QtMinimap::geometry(stripH, visibleLines(), state.lineCount(), maxScrollLine());
	const qreal scrollLinesF = scrollPx / lineHeightPx;
	const qreal sliderTop = geo.sliderTopFor(scrollLinesF);
	int startRow = 0;
	int endRow = state.lineCount() - 1;
	if (state.lineCount() > geo.fit)
	{
		startRow = std::clamp(static_cast<int>(scrollLinesF - sliderTop / geo.rowH),
							  0,
							  state.lineCount() - geo.fit);
		endRow = std::min(state.lineCount() - 1, startRow + geo.fit - 1);
	}

	// Rebuild density runs only when the window/content/key changes.
	// stripTop MUST be in the key: the find bar changes it without any
	// other key member moving, and stale runs would paint over the bar
	// until a resize happened to rebuild them.
	minimap.ensureRuns(QString("mm|%1|%2|%3|%4|%5|%6|%7")
						   .arg(startRow)
						   .arg(endRow)
						   .arg(state.lineCount())
						   .arg(highlight.visualGeneration())
						   .arg(ops.generation())
						   .arg(width())
						   .arg(stripTop),
					   startRow,
					   endRow,
					   state,
					   highlight,
					   static_cast<qreal>(stripTop),
					   geo.rowH * 0.75,
					   padX,
					   charW,
					   maxCols);

	// No background fill and no separator: rows are clipped to the
	// minimap's left edge (paintEvent), so nothing paints underneath —
	// the window's single tint layer shows through the strip like
	// everywhere else, with no border.

	// Density runs (flat rect blits).
	painter.setPen(Qt::NoPen);
	painter.save();
	painter.translate(x0, 0);
	for (const QtMinimap::Run &r : minimap.runs())
	{
		painter.setBrush(r.ink);
		painter.drawRect(QRectF(r.x, r.y, r.w, r.h));
	}
	painter.restore();

	// Continuous slider (viewport indicator).
	painter.setPen(QColor(255, 255, 255, 36));
	painter.setBrush(QColor(255, 255, 255, 26));
	painter.drawRect(QRectF(x0, stripTop + sliderTop, mw, geo.sliderH));
}

void QtEditorView::minimapScrollTo(int y)
{
	// y -> target scroll line via the strip's slider mapping (continuous,
	// geometry shared with paintMinimap).
	const QtMinimap::Geometry geo =
		QtMinimap::geometry(static_cast<qreal>(height() - topInset()),
							visibleLines(),
							state.lineCount(),
							maxScrollLine());
	const qreal target = geo.scrollLinesForY(static_cast<qreal>(y) - topInset());
	if (geo.ratio <= 0.0)
		return;
	setScrollPixels(target * lineHeightPx);
}

int QtEditorView::gitDirtyLineCount() const
{
	int n = 0;
	for (int l = 1; l <= state.lineCount(); ++l)
		if (git.isLineEdited(state.path, l))
			++n;
	return n;
}

std::string QtEditorView::gitChangesSummary() const { return git.currentGitChanges; }

// Diagnostic: scroll via minimap at the very bottom; report resulting
// position vs the maximum (interact test).

void QtEditorView::reloadFileIcon()
{
	if (state.path.empty())
		return;
	const QFontMetrics fm(font());
	const int px = std::max(11, fm.height() - 2);
	fileIcon = QtIconSet::forFile(QString::fromStdString(state.path), px);
}

void QtEditorView::forceColorUpdate()
{
	highlight.forceColorUpdate();
	update();
}

// Keep the caret inside the viewport after edits/navigation (ImGui:
// EditorViewState::revealCursor). Wrap-aware via visual lines; wrap-off
// also reveals horizontally (ImGui revealCursor's x axis).
void QtEditorView::revealCaret()
{
	ensureWrapFresh(); // callers can arrive between an edit and the paint
	const Selection &caret = viewState.selections[viewState.primaryIndex];
	const int v = visualLineOf(caret.headRow, caret.headColumn);
	const int first = firstVisualLine();
	const int visible = visibleLines();
	if (v < first)
		setScrollPixels(v * lineHeightPx);
	else if (v >= first + visible - 1)
		setScrollPixels((v - visible + 2) * lineHeightPx);

	if (!wordWrapEnabled())
	{
		const qreal caretX = xAtByteColumn(caret.headRow, caret.headColumn);
		const qreal pad = charWidthF() * 2.0;
		const qreal right = scrollPxX + textAreaWidth();
		if (caretX > right - pad)
			setScrollXPixels(caretX - textAreaWidth() + pad);
		else if (caretX < scrollPxX + pad)
			setScrollXPixels(std::max<qreal>(0.0, caretX - pad));
	}
}

QPoint QtEditorView::caretWidgetPos() const
{
	const Selection &caret = viewState.selections[viewState.primaryIndex];
	const bool wrapping = wordWrapEnabled();
	const int segment = wrapping ? caretSegmentStart(caret.headRow, caret.headColumn) : 0;
	const qreal x = gutterWidthPx +
					xAtByteColumn(caret.headRow, caret.headColumn, segment) -
					(wrapping ? 0.0 : scrollPxX);
	const qreal y =
		rowYBase() + static_cast<qreal>(visualLineOf(caret.headRow, caret.headColumn) -
										firstVisualLine()) *
						 lineHeightPx;
	return QPoint(qRound(x), qRound(y));
}

// --- Diagnostics painting ---------------------------------------------------

void QtEditorView::paintDiagnosticSquiggles(
	QPainter &painter, int firstVisual, int visualRows, qreal yBase, int textLeft)
{
	if (!diagStore || state.path.empty())
		return;
	const std::vector<DiagnosticItem> items = diagStore->forDocument(state.path);
	if (items.empty())
		return;

	// Visible (row, y, byte-range) segments — the same enumeration the text
	// painter uses, so a diagnostic spanning wrapped rows marks each line.
	struct Seg
	{
		int row;
		qreal y;
		int fromB;
		int toB;
	};
	std::vector<Seg> segs;
	if (wordWrapEnabled())
	{
		for (int v = firstVisual; v < totalLines() && v - firstVisual < visualRows; ++v)
		{
			const WrapLayout::Hit hit = wrap.yToRow(static_cast<float>(v) + 0.5f);
			const int segCount = wrap.segmentCount(hit.row);
			segs.push_back({hit.row,
							yBase + (v - firstVisual) * lineHeightPx,
							wrap.segmentStartColumn(hit.row, hit.segment),
							hit.segment + 1 < segCount
								? wrap.segmentStartColumn(hit.row, hit.segment + 1)
								: state.lineLength(hit.row)});
		}
	} else
	{
		for (int i = 0; i < visualRows; ++i)
			segs.push_back({firstVisual + i,
							yBase + i * lineHeightPx,
							0,
							state.lineLength(firstVisual + i)});
	}

	const qreal clipRight = static_cast<qreal>(width() - minimapWidth());
	for (const Seg &seg : segs)
	{
		const std::string text = state.line(seg.row);
		for (const DiagnosticItem &d : items)
		{
			if (seg.row < d.startLine || seg.row > d.endLine)
				continue;
			// Wire columns are UTF-16; convert against this row's bytes.
			int fromB = seg.row == d.startLine
							? EditorUtils::Utf16ToUtf8ByteOffset(text, d.startCharacter)
							: 0;
			int toB = seg.row == d.endLine
						  ? EditorUtils::Utf16ToUtf8ByteOffset(text, d.endCharacter)
						  : state.lineLength(seg.row);
			if (toB < fromB)
				std::swap(fromB, toB);
			fromB = std::clamp(fromB, seg.fromB, seg.toB);
			toB = std::clamp(toB, seg.fromB, seg.toB);
			if (toB < fromB)
				continue;

			const int segStart = wordWrapEnabled() ? seg.fromB : 0; // tab stops
			const qreal x0 = textLeft + xAtByteColumn(seg.row, fromB, segStart);
			const qreal x1 = textLeft + xAtByteColumn(seg.row, toB, segStart);
			const DiagnosticSeverityRGB sev = DiagnosticSeverityColor(d.severity);
			drawSquiggle(painter,
						 std::min(x0, clipRight),
						 std::min(std::max(x1, x0), clipRight),
						 seg.y + lineHeightPx - 3.0,
						 QColor::fromRgbF(sev.r, sev.g, sev.b, sev.a));
		}
	}
}

void QtEditorView::drawSquiggle(
	QPainter &painter, qreal x0, qreal x1, qreal y, const QColor &color)
{
	// Sine wave, ImGui text_view parity: 1.25px amplitude, 2px steps,
	// degenerate/narrow ranges still get a visible minimum wave.
	if (x1 <= x0)
		x1 = x0 + 6.0;
	else if (x1 - x0 < 4.0)
		x1 = x0 + 8.0;
	painter.setPen(QPen(color, 1.4));
	QPointF prev(x0, y);
	for (qreal x = x0 + 2.0; x <= x1; x += 2.0)
	{
		const QPointF cur(x, y + std::sin((x - x0) * 1.2) * 1.25);
		painter.drawLine(prev, cur);
		prev = cur;
	}
	painter.drawLine(prev, QPointF(x1, y + std::sin((x1 - x0) * 1.2) * 1.25));
}

// --- Hover trigger (shared with LSP symbol hover) -----------------------------

HoverTrigger::Target QtEditorView::hoverTargetAt(const QPoint &pos) const
{
	HoverTrigger::Target target;
	if (pos.y() < topInset() || pos.y() > height() || pos.x() < 0)
		return target;
	if (pos.x() >= width() - minimapWidth())
		return target; // minimap strip is not a hover zone
	if (pos.x() < gutterWidthPx)
	{
		target.zone = HoverTrigger::Zone::Gutter;
		target.row = rowAtY(pos.y());
		return target;
	}
	const RowHit hit = hitTestY(pos.y());
	// Past-end-of-text guard: columnAtX snaps to the nearest glyph and
	// clamps to the line length, so a mouse far right of the last glyph
	// would still report the end-of-line column — and LSP servers answer
	// that with the last token's hover. Only count cells actually on the
	// rendered text (half a char of slack for the last glyph's edge).
	const int seg = wrap.segmentOf(hit.row, hit.segmentStart);
	const int segEnd = seg + 1 < wrap.segmentCount(hit.row)
						   ? wrap.segmentStartColumn(hit.row, seg + 1)
						   : state.lineLength(hit.row);
	const qreal textEndX = xAtByteColumn(hit.row, segEnd, hit.segmentStart);
	const qreal textX = pos.x() - gutterWidthPx + (wordWrapEnabled() ? 0.0 : scrollPxX);
	if (textX > textEndX + charWidthF() * 0.5)
		return target;
	target.zone = HoverTrigger::Zone::Text;
	target.row = hit.row;
	target.column = columnAtX(hit.row, pos.x(), hit.segmentStart);
	return target;
}

void QtEditorView::updateHover(bool mouseMoved, bool dismissed, const QPoint &pos)
{
	const HoverTrigger::Info prev = liveHoverInfo;
	const HoverTrigger::Target target = (dismissed || dragging || !underMouse())
											? HoverTrigger::Target{}
											: hoverTargetAt(pos);
	hoverTrigger.update(mouseMoved, dismissed, target);
	lastHoverPos = pos;

	const HoverTrigger::Info current = hoverTrigger.info();
	// Only state transitions act: arm→fire, active→inactive (dismiss),
	// retarget. Both-inactive ticks (the common case) do nothing.
	const bool unchanged =
		current.active == prev.active &&
		(!current.active || (current.row == prev.row && current.column == prev.column &&
							 current.zone == prev.zone));
	if (unchanged)
		return;
	liveHoverInfo = current;
	if (current.active)
		fireHover(current);
	else
		hideHoverTooltips();
}

void QtEditorView::fireHover(const HoverTrigger::Info &info)
{
	if (state.path.empty())
		return;

	// Diagnostics own the tooltip first (ImGui: squiggle/gutter claims win
	// over symbol hover) — Gutter zones match by row, Text by exact cell.
	if (diagStore)
	{
		std::vector<DiagnosticItem> matched;
		if (info.zone == HoverTrigger::Zone::Gutter)
		{
			matched = diagStore->forLine(state.path, info.row);
		} else if (info.zone == HoverTrigger::Zone::Text)
		{
			const int utf16 =
				EditorUtils::Utf8ByteOffsetToUtf16(state.line(info.row), info.column);
			for (const DiagnosticItem &d : diagStore->forLine(state.path, info.row))
				if (DiagnosticContains(d, info.row, utf16))
					matched.push_back(d);
		}
		if (!matched.empty())
		{
			showDiagnosticTooltip(matched, QCursor::pos());
			return;
		}
	}

	// No diagnostic there: Text cells fall through to the symbol hover.
	if (info.zone == HoverTrigger::Zone::Text && hoverObserver)
		hoverObserver(info);
}

void QtEditorView::showDiagnosticTooltip(const std::vector<DiagnosticItem> &items,
										 const QPoint &globalPos)
{
	if (!diagTip)
		diagTip = new QtHoverTip(this);

	// Severity card: colored dot + bold label (+ dim source), message
	// below, items separated by hairlines.
	const QColor ink = toQColor(highlight.defaultTextColor());
	QString html;
	for (size_t i = 0; i < items.size(); ++i)
	{
		if (i)
			html += "<hr>";
		const DiagnosticSeverityRGB c = DiagnosticSeverityColor(items[i].severity);
		const QColor sevColor = QColor::fromRgbF(c.r, c.g, c.b);
		html += QString("<b style=\"color:%1\">● %2</b>")
					.arg(sevColor.name(),
						 QString::fromUtf8(DiagnosticSeverityLabel(items[i].severity)));
		if (!items[i].source.empty())
			html += QStringLiteral("&nbsp;&nbsp;<span style=\"color:#888888\">") +
					QString::fromStdString(items[i].source).toHtmlEscaped() + "</span>";
		html += "<br>" + QString::fromStdString(items[i].message).toHtmlEscaped();
	}
	diagTip->present(html,
					 font(),
					 NedQtTheme::raised(NedQtTheme::background(appSettings)),
					 ink,
					 globalPos);
}

void QtEditorView::hideHoverTooltips()
{
	if (diagTip)
		diagTip->hide();
	if (hoverObserver)
		hoverObserver(HoverTrigger::Info{});
}

// Direct per-glyph painting on the monospace grid. Simple by design:
// no glyph-run engines, no pixmap caches, nothing to go stale. Perf is
// measured (render-check repaint timing) before any optimization is
// allowed back in.
void QtEditorView::paintTextRow(
	QPainter &painter, int row, qreal y, int fromByte, int toByte, qreal textLeft)
{
	// fromByte doubles as the wrap-segment start (0 for whole rows), so the
	// byte->visual map carries segment-rebased tab stops — the same
	// coordinate space the shared wrap layout measured the segment in.
	const RowText rt = expandRow(row, fromByte);
	const int lastByte = static_cast<int>(rt.byteToVisual.size() - 1);
	const int vFrom = rt.byteToVisual[std::clamp(fromByte, 0, lastByte)];
	const int vTo = rt.byteToVisual[std::clamp(toByte, 0, lastByte)];
	if (vTo <= vFrom)
		return;

	const qreal cw = charWidthF();
	const LineColorSpans &spans = highlight.spansForLine(row);
	QColor ink = toQColor(highlight.defaultTextColor());

	int vis = vFrom;
	// Segments draw REBASED: a wrapped continuation row starts at its own
	// left edge, so the x of a glyph is (vis - vFrom) cells in — not its
	// position from the line start (that pushed continuation segments off
	// past the clip; the wrap-rendering bug).
	const auto flushTo = [&](int nextVis, const QColor &color) {
		painter.setPen(color);
		for (; vis < nextVis; ++vis)
			painter.drawText(QRectF(textLeft + (vis - vFrom) * cw, y, cw, lineHeightPx),
							 Qt::AlignCenter,
							 rt.cellText(vis));
	};
	for (const ColorSpan &span : spans)
	{
		const int sVis = rt.byteToVisual[std::clamp(span.start, 0, lastByte)];
		const int eVis = rt.byteToVisual[std::clamp(span.end, 0, lastByte)];
		if (sVis > vis)
			flushTo(std::min(sVis, vTo), ink);
		ink = toQColor(highlight.colorForSlot(span.slot));
		flushTo(std::min(eVis, vTo), ink);
	}
	flushTo(vTo, ink);
}

// Editor title strip: file icon, full path, git ±N (ImGui title-bar
// parity). Painted first, above the clipped gutter/text area.
void QtEditorView::paintTitleStrip(QPainter &painter)
{
	// Editor title bar: file icon, full path, git ±N (ImGui title-bar parity).
	if (!state.path.empty())
	{
		painter.setPen(NedQtTheme::text(appSettings).darker(130));
		QFont small = font();
		small.setPointSize(std::max(9, font().pointSize() - 3));
		painter.setFont(small);
		int tx = 10;
		if (!fileIcon.isNull())
		{
			// Icon scales with the (small) title font. Fetch at device
			// resolution so the painter doesn't upscale a DPR-1 raster.
			const int px = std::max(11, painter.fontMetrics().height() - 2);
			const qreal dpr = painter.device()->devicePixelRatio();
			painter.drawPixmap(QRect(tx, (titleBarPx - px) / 2, px, px),
							   fileIcon.pixmap(QSize(qRound(px * dpr), qRound(px * dpr))));
			tx += px + 8;
		}
		const std::string changes = git.currentGitChanges;
		const QString changesText = QString::fromStdString(changes);
		// ImGui title-bar layout: path, then the ±N summary right after it
		// (SameLine). Elide the path only when both don't fit.
		const QFontMetrics fm = painter.fontMetrics();
		const int gap = 14;
		const int avail = width() - tx - 10;
		const int changesW =
			changesText.isEmpty() ? 0 : fm.horizontalAdvance(changesText);
		const QString shownPath =
			fm.elidedText(QString::fromStdString(state.path),
						  Qt::ElideMiddle,
						  std::max(40, avail - (changesW ? changesW + gap : 0)));
		const int pathW = fm.horizontalAdvance(shownPath);
		painter.drawText(
			QRect(tx, 0, pathW, titleBarPx), Qt::AlignVCenter | Qt::AlignLeft, shownPath);
		if (changesW > 0)
			painter.drawText(QRect(tx + pathW + gap, 0, changesW, titleBarPx),
							 Qt::AlignVCenter | Qt::AlignLeft,
							 changesText);
	}
}

// Gutter + current-line highlight + severity marks. `rows` counts VISUAL
// lines; only a row's first segment carries gutter decoration.
void QtEditorView::paintGutter(QPainter &painter,
							   int firstRow,
							   int rows,
							   qreal yBase,
							   const std::vector<int> &diagSeverity,
							   int diagColW)
{
	const QColor background = NedQtTheme::background(appSettings);
	const Selection &primary = viewState.selections[viewState.primaryIndex];
	// In wrap mode `rows` counts VISUAL lines: map each back to its
	// document row via the wrap layout (a 1:1 visual→row mapping printed
	// garbage numbers on continuation rows), and only the FIRST segment of
	// a row carries gutter decorations (ImGui gutter_view parity).
	const bool wrapping = wordWrapEnabled();
	for (int i = 0; i < rows; ++i)
	{
		int row = firstRow + i;
		int segment = 0;
		if (wrapping)
		{
			const WrapLayout::Hit hit = wrap.yToRow(firstRow + i + 0.5f);
			row = hit.row;
			segment = hit.segment;
		}
		const qreal y = yBase + i * lineHeightPx;
		// Current-line highlight — but NOT on rows the selection covers:
		// lighter-band + blue overlay stacks into a washed-out "inverted"
		// look (VSCode also hides it under selections).
		const bool inSelection = [&primary, row]() {
			int sr, sc, er, ec;
			primary.getOrdered(sr, sc, er, ec);
			return row >= sr && row <= er;
		}();
		if (row == primary.headRow && !inSelection)
			painter.fillRect(QRectF(0, y, width(), lineHeightPx), background.lighter(118));
		if (segment > 0)
			continue; // continuation visual line: no gutter decoration
		// Diagnostic severity mark (first visual line of the row only —
		// VSCode-style; continuation lines carry no gutter decoration).
		if (row < static_cast<int>(diagSeverity.size()) && diagSeverity[row] > 0)
		{
			const DiagnosticSeverityRGB sev = DiagnosticSeverityColor(diagSeverity[row]);
			const QColor sevColor = QColor::fromRgbF(sev.r, sev.g, sev.b, sev.a);
			const qreal markW = std::max<qreal>(3.0, diagColW * 0.45);
			painter.setPen(Qt::NoPen);
			painter.setBrush(sevColor);
			painter.drawRoundedRect(
				QRectF(4 + (diagColW - markW) / 2.0, y + 2, markW, lineHeightPx - 4),
				2.0,
				2.0);
			painter.setBrush(Qt::NoBrush);
		}
		// Line numbers match the ImGui gutter: current + edited lines are
		// white, everything else gray. No bars/marks.
		if (row == primary.headRow || git.isLineEdited(state.path, row + 1))
			painter.setPen(QColor(255, 255, 255));
		else
			painter.setPen(QColor(0x88, 0x88, 0x88));
		painter.drawText(QRectF(diagColW, y, gutterWidthPx - 12 - diagColW, lineHeightPx),
						 Qt::AlignVCenter | Qt::AlignRight,
						 QString::number(row + 1));
	}
}

// Text with syntax colors through the tab-expanded model. Returns the
// text x origin (wrap-off shifts with the horizontal scroll) so the
// squiggle/selection passes share it.
qreal QtEditorView::paintText(QPainter &painter, int firstRow, int rows, qreal yBase)
{
	const bool wrapping = wordWrapEnabled();
	const int textLeft = gutterWidthPx;
	// Text x origin: wrap-off text shifts with the horizontal scroll (the
	// gutter stays pinned); wrap-on segments always start at the gutter.
	const qreal textX0 = wrapping ? textLeft : textLeft - scrollPxX;
	// Narrow the text clip to the text area: scrolled text cuts at the
	// gutter's right edge instead of sliding under the line numbers.
	painter.setClipRect(gutterWidthPx,
						topInset(),
						width() - gutterWidthPx - minimapWidth(),
						height() - topInset());
	const auto drawRowSegment = [&](int row, int y, int fromByte, int toByte) {
		if (toByte <= fromByte)
			return;
		paintTextRow(painter, row, y, fromByte, toByte, textX0);
	};

	if (wrapping)
	{
		for (int v = firstRow; v < totalLines() && v - firstRow < rows; ++v)
		{
			const WrapLayout::Hit hit = wrap.yToRow(static_cast<float>(v) + 0.5f);
			const qreal y = yBase + (v - firstRow) * lineHeightPx;
			const int startB = wrap.segmentStartColumn(hit.row, hit.segment);
			const int segCount = wrap.segmentCount(hit.row);
			const int endB = hit.segment + 1 < segCount
								 ? wrap.segmentStartColumn(hit.row, hit.segment + 1)
								 : state.lineLength(hit.row);
			drawRowSegment(hit.row, y, startB, endB);
		}
	} else
	{
		for (int i = 0; i < rows; ++i)
			drawRowSegment(
				firstRow + i, yBase + i * lineHeightPx, 0, state.lineLength(firstRow + i));
	}
	return textX0;
}

// Selection rects (wrap-aware, per-segment) + carets on the shared
// tab-expanded coordinate space.
void QtEditorView::paintSelectionsAndCarets(
	QPainter &painter, int firstRow, int rows, qreal yBase, qreal textX0)
{
	const bool wrapping = wordWrapEnabled();
	// Selection + carets on the shared tab-expanded coordinate space.
	painter.setPen(Qt::transparent);
	// Text selection stays blue (the OS-default look); the grey accent is
	// chrome-only (sliders, tree pill, focus rings).
	painter.setBrush(QColor(0x0d, 0x6e, 0xfd, 90));
	for (const Selection &sel : viewState.selections)
	{
		int sr, sc, er, ec;
		sel.getOrdered(sr, sc, er, ec);
		for (int row = sr; row <= er; ++row)
		{
			// Per-segment rects: a wrapped row's selection spans several
			// visual lines — clamp each to its own segment's byte range
			// (previously continuation rects used whole-line bounds).
			const int vStart = visualLineOf(row, row == sr ? sc : 0);
			const int vEnd = visualLineOf(row, row == er ? ec : state.lineLength(row));
			for (int v = vStart; v <= vEnd; ++v)
			{
				const int i = v - firstRow;
				if (i < 0 || i >= rows)
					continue;
				// v is an ABSOLUTE visual line — resolve the segment index
				// from the row's visual-line base. (The old `v - vStart` base
				// was wrong whenever the selection starts mid-row on a
				// wrapped line: segments were misidentified and their rects
				// skipped/misclamped.)
				const int segIdx = wrapping ? v - wrap.rowStartVisualLine(row) : 0;
				const int segBase = wrapping ? wrap.segmentStartColumn(row, segIdx) : 0;
				const int segEnd = wrapping && segIdx + 1 < wrap.segmentCount(row)
									   ? wrap.segmentStartColumn(row, segIdx + 1)
									   : state.lineLength(row);
				const int fromB =
					(v == vStart && row == sr) ? std::max(sc, segBase) : segBase;
				const int toB = (v == vEnd && row == er) ? std::min(ec, segEnd) : segEnd;
				if (toB <= fromB)
					continue;
				const qreal x0 = textX0 + xAtByteColumn(row, fromB, segBase);
				const qreal x1 = textX0 + xAtByteColumn(row, toB, segBase);
				painter.drawRect(
					QRectF(x0, yBase + i * lineHeightPx, x1 - x0, lineHeightPx));
			}
		}
	}
	if (caretVisible && caretActive())
	{
		// 2px caret on the glyph boundary, never over the glyph.
		painter.setPen(QPen(QColor(255, 255, 255), 2));
		for (const Selection &sel : viewState.selections)
		{
			const int v = visualLineOf(sel.headRow, sel.headColumn);
			const int i = v - firstRow;
			if (i < 0 || i >= rows)
				continue;
			const int seg = wrapping ? caretSegmentStart(sel.headRow, sel.headColumn) : 0;
			const qreal x = textX0 + xAtByteColumn(sel.headRow, sel.headColumn, seg);
			painter.drawLine(QPointF(x, yBase + i * lineHeightPx + 2),
							 QPointF(x, yBase + (i + 1) * lineHeightPx - 2));
		}
	}
}

// Paint orchestration: title strip, gutter, text, squiggles, selections,
// minimap. Each pass is a private paint* helper above.
void QtEditorView::paintEvent(QPaintEvent *)
{
	QPainter painter(this);
	// NO whole-rect background fill: the main window paints the single
	// global tint (QSS QMainWindow rule), and stacked alpha fills would
	// double-darken the editor area vs the rest of the window chrome.
	// Highlights below paint over that one layer.

	// Track digit-count changes from edits (ImGui recomputes per frame).
	updateGutterWidth();

	// Wrap parity with the ImGui frame loop: re-ensure the wrap layout
	// before painting so a toggle/resize/edit can never paint stale
	// segments (cheap when clean).
	ensureWrapFresh();

	const int firstRow = firstVisualLine();
	const qreal yBase = rowYBase(); // rows slide under the title strip
	// `rows` is in VISUAL lines — with wrap on, the document spans
	// totalLines() visual rows (a physical row can occupy many), so the
	// clamp must use the visual total. Clamping against state.lineCount()
	// truncated the tail of every wrapped document (visual index outruns
	// the row count) and the bottom rendered blank.
	const int rows = std::max(0, std::min(visibleLines() + 2, totalLines() - firstRow));

	paintTitleStrip(painter);

	// Rows slide under the title strip with fractional scroll — clip the
	// text area to the strip's bottom, and to the minimap's left edge so
	// long lines are CUT before the strip instead of painting under it.
	painter.setClipRect(0, topInset(), width() - minimapWidth(), height() - topInset());
	painter.setFont(font());
	// One severity snapshot for the whole gutter pass (per-line queries
	// copy the full diagnostic set each call — store contract).
	const std::vector<int> diagSeverity =
		diagStore && !state.path.empty()
			? diagStore->maxSeverityByLine(state.path, state.lineCount())
			: std::vector<int>();
	paintGutter(painter, firstRow, rows, yBase, diagSeverity, diagnosticColumnWidth());

	const qreal textX0 = paintText(painter, firstRow, rows, yBase);

	// LSP diagnostics: squiggle underlines over the same coordinate space.
	paintDiagnosticSquiggles(painter, firstRow, rows, yBase, textX0);

	paintSelectionsAndCarets(painter, firstRow, rows, yBase, textX0);

	// The minimap strip spans the full height (over the title area).
	painter.setClipRect(0, 0, width(), height());
	paintMinimap(painter);
}

bool QtEditorView::caretActive() const
{
	const QWidget *fw = QApplication::focusWidget();
	return fw == nullptr || fw == this || isAncestorOf(fw);
}

void QtEditorView::focusInEvent(QFocusEvent *event)
{
	QWidget::focusInEvent(event);
	// Regained focus: solid caret, blink phase restarts.
	blinkClock.restart();
	caretVisible = true;
	update();
}

void QtEditorView::focusOutEvent(QFocusEvent *event)
{
	QWidget::focusOutEvent(event);
	// Repaint now so an unfocused dock sibling's caret disappears immediately
	// instead of lingering until the next blink flip.
	update();
}

void QtEditorView::afterEdit()
{
	highlight.poll();
	refreshWrap();
	scrollBar->setRange(0, maxScrollLine());
	revealCaret();
	caretVisible = true;
	scheduleBlink();
	update();
	Q_EMIT documentEdited();
}

void QtEditorView::repaintAndFollow()
{
	highlight.poll();
	highlight.highlightContent();
	refreshWrap(); // find/replace edits changed content before revealing
	revealCaret();
	update();
}

void QtEditorView::toggleFindBar()
{
	if (findBar->isVisible())
	{
		closeFindBar();
		return;
	}
	// Bar sits directly under the title strip; the text area starts below
	// it (topInset), so the document is displaced, not overlaid.
	findBarPx = findBar->sizeHint().height();
	findBar->setGeometry(0, titleBarPx, width(), findBarPx);
	findBar->open();
	setScrollPixels(scrollPx); // fewer visible lines — re-clamp
	update();
}

void QtEditorView::closeFindBar()
{
	findBarPx = 0;
	findBar->closeBar(); // hides + refocuses the editor
	setScrollPixels(scrollPx);
	update();
}

void QtEditorView::goToLineDialog()
{
	// Centered popup card, file-finder style (was: corner-anchored inline
	// input floating over the text).
	auto *dialog = new QtLineJumpDialog(
		viewState.selections[viewState.primaryIndex].headRow + 1, state.lineCount(), this);
	dialog->setAttribute(Qt::WA_DeleteOnClose);
	connect(dialog, &QtLineJumpDialog::jumpRequested, this, [this](int line) {
		commands.goToLine(line - 1); // commands API is 0-based
		setScrollPixels(std::max(0,
								 viewState.selections[viewState.primaryIndex].headRow -
									 visibleLines() / 2) *
						lineHeightPx);
		scheduleBlink();
		update();
	});
	dialog->show();
}

void QtEditorView::scheduleBlink()
{
	// Activity: caret solid, blink phase restarts (single-clock blink).
	blinkClock.restart();
	caretVisible = true;
	update();
}

bool QtEditorView::event(QEvent *event)
{
	// Qt routes Tab through focus traversal before keyPressEvent; claim it
	// first so the editor always receives it.
	if (event->type() == QEvent::KeyPress)
	{
		auto *key = static_cast<QKeyEvent *>(event);
		if (key->key() == Qt::Key_Tab || key->key() == Qt::Key_Backtab)
		{
			keyPressEvent(key);
			return true;
		}
	}
	return QWidget::event(event);
}

void QtEditorView::keyPressEvent(QKeyEvent *event)
{
	const bool shift = event->modifiers() & Qt::ShiftModifier;
	const bool ctrl = event->modifiers() & Qt::ControlModifier;
	const bool meta = event->modifiers() & Qt::MetaModifier; // Cmd
	const bool alt = event->modifiers() & Qt::AltModifier;	 // Option
	const bool primary = ctrl || meta;

	// Any keystroke dismisses hover popups (HoverTrigger rule 2).
	updateHover(false, true, lastHoverPos);

	// Option/Alt: word left/right, add caret above/below (not with Cmd/Ctrl).
	if (alt && !primary)
	{
		switch (event->key())
		{
		case Qt::Key_Left:
			commands.moveWordLeft(shift);
			break;
		case Qt::Key_Right:
			commands.moveWordRight(shift);
			break;
		case Qt::Key_Up:
			commands.addCursorAbove();
			break;
		case Qt::Key_Down:
			commands.addCursorBelow();
			break;
		case Qt::Key_Backspace:
			commands.deleteLeft(true); // Alt deletes by word (ImGui: KeyAlt)
			break;
		case Qt::Key_Delete:
			commands.deleteRight(true);
			break;
		default:
			QWidget::keyPressEvent(event);
			return;
		}
		afterEdit();
		return;
	}

	if (primary)
	{
		switch (event->key())
		{
		case Qt::Key_A:
			commands.selectAll();
			break;
		case Qt::Key_Plus:
		case Qt::Key_Equal: {
			appSettings.settings["fontSize"] =
				appSettings.settings.value("fontSize", 13) + 2;
			applyProfileFont();
			Q_EMIT fontZoomed();
			appSettings.saveSettings();
			break;
		}
		case Qt::Key_Minus: {
			appSettings.settings["fontSize"] =
				std::max(8.0, appSettings.settings.value("fontSize", 13) - 2.0);
			applyProfileFont();
			Q_EMIT fontZoomed();
			appSettings.saveSettings();
			break;
		}
		case Qt::Key_Z:
			commands.undo();
			break;
		case Qt::Key_Y:
			commands.redo();
			break;
		case Qt::Key_C:
			commands.copy();
			break;
		case Qt::Key_X:
			commands.cut();
			break;
		case Qt::Key_V:
			commands.paste();
			break;
		case Qt::Key_S:
			commands.save();
			break;
		case Qt::Key_Left:
			commands.moveLineStart(shift);
			break;
		case Qt::Key_Right:
			commands.moveLineEnd(shift);
			break;
		case Qt::Key_Up:
			commands.moveLines(-5, shift);
			break;
		case Qt::Key_Down:
			commands.moveLines(5, shift);
			break;
		default:
			QWidget::keyPressEvent(event);
			return;
		}
		afterEdit();
		return;
	}

	switch (event->key())
	{
	case Qt::Key_Left:
		commands.moveLeft(shift);
		break;
	case Qt::Key_Right:
		commands.moveRight(shift);
		break;
	case Qt::Key_Up:
		commands.moveUp(shift);
		break;
	case Qt::Key_Down:
		commands.moveDown(shift);
		break;
	case Qt::Key_Home:
		commands.moveLineStart(shift);
		break;
	case Qt::Key_End:
		commands.moveLineEnd(shift);
		break;
	case Qt::Key_Escape:
		// Find bar open: Escape closes it before anything else (the bar
		// itself handles Escape when its input has focus). A QShortcut
		// was tried here — its WidgetWithChildren context also swallowed
		// Escape for child popups like the line jump card.
		if (findBar->isVisible())
			closeFindBar();
		else
			commands.collapseSelection();
		break;
	case Qt::Key_Return:
	case Qt::Key_Enter:
		commands.insertNewline();
		break;
	case Qt::Key_Backspace:
		commands.deleteLeft(); // word-delete variant lives in the Alt branch
		break;
	case Qt::Key_Delete:
		commands.deleteRight();
		break;
	case Qt::Key_Tab:
		commands.indent();
		break;
	case Qt::Key_Backtab:
		commands.outdent();
		break;
	default: {
		const QString text = event->text();
		if (!text.isEmpty())
		{
			commands.typeText(text.toUtf8().constData());
			break;
		}
		QWidget::keyPressEvent(event);
		return;
	}
	}
	afterEdit();
}

void QtEditorView::resizeEvent(QResizeEvent *event)
{
	QWidget::resizeEvent(event);
	// Minimap replaces the scrollbar when enabled (ImGui parity).
	scrollBar->setVisible(!minimapEnabled());
	refreshWrap();
	// Geometry is kept current even while hidden so the first open() has
	// the right position without waiting for a resize (findBar is created
	// in the constructor — never null).
	findBar->setGeometry(0, titleBarPx, width(), findBar->sizeHint().height());
	scrollBar->setGeometry(width() - 14, 0, 14, height());
	scrollBar->setPageStep(std::max(1, visibleLines() - 1));
	scrollBar->setRange(0, maxScrollLine());
	// Horizontal strip spans the text area only (gutter to minimap), laid
	// over the bottom edge like the vertical bar overlays the right one.
	hScrollBar->setGeometry(
		gutterWidthPx, height() - 12, width() - gutterWidthPx - minimapWidth(), 12);
}

void QtEditorView::wheelEvent(QWheelEvent *event)
{
	// ImGui parity (editor_view_scroll.cpp): 120 delta units = 3 lines —
	// accumulated fractionally in PIXELS so trackpad micro-deltas and
	// momentum scroll with sub-line smoothness. The axes apply
	// INDEPENDENTLY: a diagonal trackpad gesture scrolls both. (The old
	// "any x-delta routes the whole event horizontal" rule ate the
	// vertical half of angled swipes — the wheel felt dead, e.g. trying
	// to scroll back up from the bottom of a file.)
	const bool shift = event->modifiers() & Qt::ShiftModifier;
	const bool wrap = wordWrapEnabled();
	const QPoint d = event->angleDelta();

	// Horizontal (wrap off): x-deltas; shift maps the y-wheel to horizontal
	// on platforms that deliver shift+wheel unswapped (macOS/Qt pre-swap it
	// into the x axis already). Same sign convention as the vertical axis —
	// content follows the gesture. 120 units = 3 cells.
	if (!wrap)
	{
		const int dx = d.x() != 0 ? d.x() : (shift ? d.y() : 0);
		if (dx != 0)
		{
			wheelCarryX -= dx * (3.0 * charWidthF() / 120.0);
			const int px = static_cast<int>(wheelCarryX);
			wheelCarryX -= px;
			if (px != 0)
				setScrollXPixels(scrollPxX + px);
		}
	}

	// Vertical: the y axis. Shift means horizontal intent on unswapped
	// wheels, so skip it there — unless wrapping, where ImGui's rule
	// scrolls vertically regardless.
	if (d.y() != 0 && (!shift || wrap))
	{
		wheelCarry += d.y() * (3.0 * lineHeightPx / 120.0);
		const int px = static_cast<int>(wheelCarry);
		wheelCarry -= px;
		if (px != 0)
			setScrollPixels(scrollPx - px);
	}
	// Scrolling shifts content under the mouse — dismiss hover (rule 2).
	updateHover(false, true, lastHoverPos);
}

void QtEditorView::leaveEvent(QEvent *event)
{
	QWidget::leaveEvent(event);
	// No zone outside the widget (the mouse may be resting ON a hover
	// tooltip — its own dismissal paths handle that case).
	updateHover(false, false, QPoint(-1, -1));
}

void QtEditorView::mousePressEvent(QMouseEvent *event)
{
	// Click/drag is a hover dismissal (rule 2) — before anything else.
	updateHover(false, true, lastHoverPos);

	if (event->button() == Qt::RightButton)
	{
		showContextMenu(event->pos());
		return;
	}
	if (event->position().x() >= width() - minimapWidth())
	{
		minimapDragging = true;
		minimapScrollTo(static_cast<int>(event->position().y()));
		return;
	}
	if (event->button() != Qt::LeftButton)
		return;
	const RowHit hit = hitTestY(static_cast<int>(event->position().y()));
	const int row = hit.row;
	const int column =
		columnAtX(row, static_cast<int>(event->position().x()), hit.segmentStart);
	dragging = true;
	caretVisible = true;
	scheduleBlink();
	if (event->modifiers() & Qt::ShiftModifier)
	{
		// Extend from the existing anchor (ImGui handleMouseClick).
		const Selection &p = viewState.selections[viewState.primaryIndex];
		commands.setSelection(p.anchorRow, p.anchorColumn, row, column);
	} else
	{
		commands.setCursor(row, column, false);
	}
	update();
}

void QtEditorView::mouseDoubleClickEvent(QMouseEvent *event)
{
	if (event->button() != Qt::LeftButton)
		return;
	// Qt fires the double-click INSTEAD of a second mousePressEvent — arm
	// dragging here too, or click-click-drag would never extend the
	// selection (setCursor select=true keeps the word anchor).
	dragging = true;
	caretVisible = true;
	scheduleBlink();
	const RowHit hit = hitTestY(static_cast<int>(event->position().y()));
	const int column =
		columnAtX(hit.row, static_cast<int>(event->position().x()), hit.segmentStart);
	commands.selectWordAt(hit.row, column);
	update();
}

void QtEditorView::showContextMenu(const QPoint &pos)
{
	QMenu menu(this);
	// Standard keys so the accelerators render platform-native (⌘ on macOS).
	menu.addAction(
		"Cut",
		[this] {
			commands.cut();
			afterEdit();
		},
		QKeySequence(QKeySequence::Cut));
	menu.addAction("Copy", [this] { commands.copy(); }, QKeySequence(QKeySequence::Copy));
	menu.addAction(
		"Paste",
		[this] {
			commands.paste();
			afterEdit();
		},
		QKeySequence(QKeySequence::Paste));
	menu.addSeparator();
	menu.addAction(
		"Select All",
		[this] {
			commands.selectAll();
			update();
		},
		QKeySequence(QKeySequence::SelectAll));
	menu.exec(mapToGlobal(pos));
}

void QtEditorView::mouseMoveEvent(QMouseEvent *event)
{
	// Real mouse movement (re)arms the hover delay; a drag doubles as a
	// dismissal signal — target resolution in updateHover drops the zone.
	updateHover(true, dragging, event->position().toPoint());

	// Heal drags whose release was consumed elsewhere (context-menu nested
	// loop, popup, window deactivate): with the left button up there is no
	// drag. A stuck minimap drag re-pinned the scroll to the pointer on
	// every move — "stuck at the bottom, can't scroll up".
	if ((dragging || minimapDragging) &&
		!(QGuiApplication::mouseButtons() & Qt::LeftButton))
	{
		dragging = false;
		minimapDragging = false;
		return;
	}

	if (minimapDragging)
	{
		minimapScrollTo(static_cast<int>(event->position().y()));
		return;
	}
	if (!dragging)
		return;
	const RowHit hit = hitTestY(static_cast<int>(event->position().y()));
	const int column =
		columnAtX(hit.row, static_cast<int>(event->position().x()), hit.segmentStart);
	commands.setCursor(hit.row, column, true);
	update();
}

void QtEditorView::mouseReleaseEvent(QMouseEvent *)
{
	dragging = false;
	minimapDragging = false;
}
