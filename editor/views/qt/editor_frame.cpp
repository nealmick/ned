#include "editor_frame.h"

#include "../../../util/settings.h"
#include "find_bar.h"
#include "line_jump.h"
#include "ned_color.h"

#include "host/qt/fonts.h"
#include <QFontMetrics>
#include <QPainter>
#include <QScrollBar>
#include <QTimer>

#include <cstring>
#include <fstream>
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

EditorFrame::EditorFrame(Settings &settings, QWidget *parent)
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
	  commands(state, viewState, ops, projectUndo, events, save),
	  textView(*this),
	  gutterView(*this),
	  caretView(*this),
	  titleBarView(*this),
	  input(*this)
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
		// The bar moves in whole lines; internal sync drives viewState.scrollPx.
		if (!syncingScroll)
			viewState.scrollPx = v * lineHeightPx;
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
			input.updateHover(false, false, lastHoverPos);
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

	findBar = new FindBar(this, this);
	// Wrap toggles change the bar's height — re-displace the text area.
	connect(findBar, &FindBar::heightChanged, this, [this] {
		if (!findBar->isVisible())
			return;
		findBarPx = findBar->sizeHint().height();
		findBar->setGeometry(0, titleBarPx, width(), findBarPx);
		setScrollPixels(viewState.scrollPx); // fewer visible lines — re-clamp
		update();
	});
	// NOTE: no per-view Ctrl+F / Ctrl+; QShortcuts here — one per view
	// means two live views (background tab, split) make the sequence
	// ambiguous and Qt fires NOTHING. The host owns the single shortcuts
	// and routes them to the active view (see AppHost's keybinds).
}

EditorFrame::~EditorFrame() = default;

void EditorFrame::openWorkspaceRoot(const std::string &root)
{
	projectRoot = root;
	git.init();
}

// --- LSPEditor seam ---------------------------------------------------------

void EditorFrame::getCaret(int &row, int &column) const
{
	row = viewState.row;
	column = viewState.column;
}

std::string EditorFrame::line(int row) const { return state.line(row); }

const std::string &EditorFrame::path() const { return state.path; }

const std::string &EditorFrame::languageId() const { return state.languageId; }

NedColor EditorFrame::defaultTextColor() const { return highlight.defaultTextColor(); }

NedColor EditorFrame::syntaxColor(ThemeSlot slot) const
{
	return highlight.colorForSlot(slot);
}

void EditorFrame::requestCursorCenter(int row, int column)
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

void EditorFrame::setDiagnostics(const LSPDiagnostics *store)
{
	diagStore = store;
	diagRevisionSeen = store ? store->revision() : 0;
	gutterView.updateWidth(); // the severity column appears with the first store
	update();
}

void EditorFrame::setFontFromSettings()
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

	gutterView.updateWidth();
	// Keep the viewport anchored when the line height changes (font zoom).
	if (oldLineHeight > 0)
	{
		viewState.scrollPx = viewState.scrollPx * lineHeightPx / oldLineHeight;
		// The rescale can push viewState.scrollPx past the (shrunk) range — clamp or
		// the first wheel-ups dead-clamp back to the bottom.
		viewState.scrollPx =
			std::clamp(viewState.scrollPx, 0.0, static_cast<qreal>(maxScrollPx()));
	}
}

void EditorFrame::openFile(const QString &path)
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
	gutterView.updateWidth();
	ops.clearPending();
	ops.bumpGeneration();
	viewState.setBoth(0, 0);
	highlight.resetForDocument(static_cast<size_t>(state.lineCount()));
	highlight.highlightContent();
	// New document: reset both axes and rebuild the longest-line cache
	// (refreshWrap re-ensures the wrap layout + scrollbars after it).
	widthDirty = true;
	viewState.scrollPxX = 0.0;
	refreshWrap();
	git.init();
	git.onDocumentOpened();
	lastVisualGen = highlight.visualGeneration();
	setScrollPixels(0);
	update();
}

QSize EditorFrame::sizeHint() const { return QSize(800, 600); }

// Screen y of the first painted visual line. With fractional scroll the
// top row slides up under the top inset (paint clips there).

// --- Tab-expanded rendering model (row_text.h) ---------------------------

bool EditorFrame::wordWrapEnabled() const
{
	return appSettings.settings.value("word_wrap", false);
}

int EditorFrame::textAreaWidth() const
{
	// Minimap-aware: wrapped rows must break BEFORE the minimap strip, or
	// they would paint underneath it.
	return std::max(50, width() - gutterWidthPx - minimapWidth() - 14);
}

int EditorFrame::totalLines() const
{
	return wordWrapEnabled() ? std::max(1, wrap.totalVisualLines())
							 : std::max(1, state.lineCount());
}

void EditorFrame::ensureWrapFresh()
{
	if (wordWrapEnabled())
		wrap.ensure(state, static_cast<float>(textAreaWidth()));
}

void EditorFrame::refreshWrap()
{
	if (wordWrapEnabled())
	{
		ensureWrapFresh();
		viewState.scrollPxX = 0.0; // no horizontal scroll in wrap mode
	} else
		wrap.invalidate();
	// Re-clamp: the scroll range can shrink (font zoom rescales viewState.scrollPx
	// unclamped, wrap re-measures, deletions) — a viewState.scrollPx past the max
	// makes the first wheel-ups clamp straight back to the bottom.
	viewState.scrollPx =
		std::clamp(viewState.scrollPx, 0.0, static_cast<qreal>(maxScrollPx()));
	scrollBar->setRange(0, maxScrollLine());
	syncHScrollBar();
}

// --- Horizontal scroll (wrap off) ------------------------------------------

// Incremental longest-line cache (editor_frame.cpp scheme): scan the dirty
// span; keep the old max as a safe overestimate when the longest row
// shrank (avoids an O(n) rescan per keystroke).

void EditorFrame::forceColorUpdate()
{
	highlight.forceColorUpdate();
	update();
}

// Keep the caret inside the viewport after edits/navigation (ImGui:
// EditorViewState::revealCaret). Wrap-aware via visual lines; wrap-off
// also reveals horizontally (revealCaret's x axis).

void EditorFrame::paintEvent(QPaintEvent *)
{
	QPainter painter(this);
	// NO whole-rect background fill: the main window paints the single
	// global tint (QSS QMainWindow rule), and stacked alpha fills would
	// double-darken the editor area vs the rest of the window chrome.
	// Highlights below paint over that one layer.

	// Track digit-count changes from edits (ImGui recomputes per frame).
	gutterView.updateWidth();

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

	titleBarView.paint(painter);

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
	gutterView.paint(
		painter, firstRow, rows, yBase, diagSeverity, gutterView.diagColumnWidth());

	textView.paint(painter, firstRow, rows, yBase);

	// LSP diagnostics: squiggle underlines over the same coordinate space.
	textView.paintDiagnosticSquiggles(painter, firstRow, rows, yBase);

	caretView.paint(painter, firstRow, rows, yBase);

	// The minimap strip spans the full height (over the title area).
	painter.setClipRect(0, 0, width(), height());
	minimapView.paint(painter, *this);
}

void EditorFrame::afterEdit()
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

void EditorFrame::repaintAndFollow()
{
	highlight.poll();
	highlight.highlightContent();
	refreshWrap(); // find/replace edits changed content before revealing
	revealCaret();
	update();
}

void EditorFrame::toggleFindBar()
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
	setScrollPixels(viewState.scrollPx); // fewer visible lines — re-clamp
	update();
}

void EditorFrame::closeFindBar()
{
	findBarPx = 0;
	findBar->dismiss(); // hides + refocuses the editor
	setScrollPixels(viewState.scrollPx);
	update();
}

void EditorFrame::goToLineDialog()
{
	// Centered popup card, file-finder style (was: corner-anchored inline
	// input floating over the text).
	auto *dialog = new LineJump(viewState.selections[viewState.primaryIndex].headRow + 1,
								state.lineCount(),
								this);
	dialog->setAttribute(Qt::WA_DeleteOnClose);
	connect(dialog, &LineJump::jumpRequested, this, [this](int line) {
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

void EditorFrame::resizeEvent(QResizeEvent *event)
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
