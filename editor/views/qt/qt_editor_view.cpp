#include "qt_editor_view.h"

#include "../../util/text_columns.h"
#include "../../util/utf8.h"

#include "../../../util/settings.h"
#include "ned_color_qt.h"
#include "qt_find_bar.h"
#include <QFontInfo>
#include <QFontMetricsF>

#include <QMenu>
#include <QShortcut>

#include "qt_fonts.h"
#include "qt_icons.h"
#include "qt_theme.h"
#include <QFontMetrics>
#include <QInputDialog>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QScrollBar>
#include <QTimer>
#include <QWheelEvent>

#include <chrono>
#include <cmath>
#include <fstream>
#include <iostream>
#include <sstream>

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
	setFontFromSettings();

	// Blink runs entirely off blinkClock in the service timer — a second
	// timer restarting the clock raced the sampler and could strand the
	// caret in the invisible phase permanently.

	scrollBar = new QScrollBar(Qt::Vertical, this);
	connect(scrollBar, &QScrollBar::valueChanged, this, [this](int) { update(); });

	// Services (async tree-sitter, autosave, git status) expect per-frame
	// polling; the Qt backend has no frame loop, so a short timer drives
	// them — plus smooth caret blink/rainbow animation.
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
		// Blink at ~1.9 Hz; repaint only when the caret flips visibility.
		const bool next = (blinkClock.elapsed() % 1060) < 530;
		if (next != caretVisible)
		{
			caretVisible = next;
			update();
		}
	});
	serviceTimer->start();

	setFocusPolicy(Qt::StrongFocus);
	setMouseTracking(true);

	// Backend glyph metrics for the shared wrap layout (QFontMetrics).
	WrapLayout::setGlyphWidthFn([this](const char *s, const char *e) {
		const QFontMetricsF m(font());
		return m.horizontalAdvance(QString::fromUtf8(s, static_cast<int>(e - s)));
	});
	WrapLayout::setSpaceWidthFn(
		[this](const char *, const char *) { return charWidthF(); });

	// DidEdit fan-out mirrors the ImGui Editor: highlight, autosave, git
	// gutter. Without this subscription the git marks never update.
	events.subscribeDidEdit([this](const EditorEvents::DidEdit &e) {
		highlight.highlightContent();
		save.onDidEdit();
		git.onDidEdit(e.firstRow, e.lastRow);
	});

	findBar = new QtFindBar(this, this);
	auto *lineJumpShortcut = new QShortcut(QKeySequence("Ctrl+;"), this);
	connect(lineJumpShortcut, &QShortcut::activated, this, &QtEditorView::goToLineDialog);
	auto *findShortcut = new QShortcut(QKeySequence("Ctrl+F"), this);
	connect(findShortcut, &QShortcut::activated, this, &QtEditorView::toggleFindBar);
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

void QtEditorView::setFontFromSettings()
{
	QFont font("Menlo"); // fallback; profile font wins when registered
#ifdef _WIN32
	font.setFamily("Consolas");
#endif
	// Profile font (registered from resources/fonts by the settings dialog).
	const std::string profileFont = appSettings.settings.value("font", std::string());
	const QString resolved =
		NedQtFonts::resolveFamily(QString::fromStdString(profileFont));
	if (!profileFont.empty() && profileFont != "System Default" && !resolved.isEmpty())
	{
		font.setFamily(resolved);
		font.setFixedPitch(true);
	}
	font.setStyleHint(QFont::Monospace);
	font.setFixedPitch(true);
	font.setPointSize(static_cast<int>(appSettings.settings.value("fontSize", 13)));
	setFont(font);

	const QFontMetrics metrics(font);
	lineHeightPx = metrics.height();
	// Monospace advance: '0' is reliably full-width; ' ' can be narrower.
	cellWidth = metrics.horizontalAdvance(QLatin1String("0000")) / 4.0;
	// Glyph-run rendering needs a raw font at the real pixel size.
	gutterWidthPx = metrics.horizontalAdvance('0') * 6 + 16;
}

void QtEditorView::openFile(const QString &path)
{
	std::string raw;
	if (!path.isEmpty())
	{
		std::ifstream file(path.toStdString(), std::ios::binary);
		std::stringstream buffer;
		buffer << file.rdbuf();
		raw = buffer.str();
		state.path = path.toStdString();
		state.languageId = EditorState::languageIdFromPath(state.path);
	} else
	{
		state.path = "";
	}

	fileIcon = QtIconSet::instance().forFile(path);
	state.setFromString(raw);
	ops.clearPending();
	ops.bumpGeneration();
	viewState.setBoth(0, 0);
	highlight.resetForDocument(static_cast<size_t>(state.lineCount()));
	highlight.highlightContent();
	refreshWrap();
	git.init();
	git.onDocumentOpened();
	lastVisualGen = highlight.visualGeneration();
	scrollBar->setRange(0, maxScrollLine());
	scrollBar->setValue(0);
	update();
}

QSize QtEditorView::sizeHint() const { return QSize(800, 600); }

int QtEditorView::visibleLines() const
{
	return std::max(1, (height() - titleBarPx) / lineHeightPx);
}

int QtEditorView::maxScrollLine() const
{
	// Last line fully visible at bottom: allow scrolling past it a little
	// (ImGui scrolls to keep the caret line plus context visible).
	return std::max(0, totalLines() - visibleLines() + 2);
}

int QtEditorView::rowAtY(int y) const
{
	const int v = scrollBar->value() + std::max(0, y - titleBarPx) / lineHeightPx;
	if (wordWrapEnabled())
	{
		const WrapLayout::Hit hit = wrap.yToRow(static_cast<float>(v) + 0.5f);
		return std::clamp(hit.row, 0, std::max(0, state.lineCount() - 1));
	}
	return std::clamp(v, 0, std::max(0, state.lineCount() - 1));
}

int QtEditorView::columnAtX(int row, int x, int segmentStart) const
{
	const int textX = x - gutterWidthPx;
	if (textX <= 0)
		return segmentStart;
	return std::clamp(byteColumnAtX(row, static_cast<qreal>(textX), segmentStart),
					  0,
					  state.lineLength(row));
}

// --- Tab-expanded rendering model ------------------------------------------

qreal QtEditorView::charWidthF() const
{
	// Cached at font-set time; the single width source for the whole view.
	return cellWidth;
}

QtEditorView::RowText QtEditorView::expandRow(int row) const
{
	RowText rt;
	const std::string line = state.line(row);
	rt.byteToVisual.assign(line.size() + 1, 0);

	int visual = 0;
	for (size_t i = 0; i < line.size();)
	{
		rt.byteToVisual[i] = visual;
		const unsigned char c = static_cast<unsigned char>(line[i]);
		if (c == '\t')
		{
			// Tab stops every kTabSize visual cells (matches ImGui layout).
			const int next = (visual / EditorUtils::kTabSize + 1) * EditorUtils::kTabSize;
			for (; visual < next; ++visual)
			{
				rt.expanded += QLatin1Char(' ');
				rt.visualToByte.push_back(static_cast<int>(i));
			}
			++i;
			continue;
		}
		int len = 1;
		while ((static_cast<unsigned char>(line[i + len]) & 0xC0) == 0x80 &&
			   i + len < line.size())
			++len;
		rt.expanded += QString::fromUtf8(line.data() + i, len);
		++visual;
		rt.visualToByte.push_back(static_cast<int>(i));
		i += static_cast<size_t>(len);
	}
	rt.byteToVisual[line.size()] = visual;
	rt.visualToByte.push_back(static_cast<int>(line.size()));
	return rt;
}

qreal QtEditorView::xAtByteColumn(int row, int byteColumn, int segmentStart) const
{
	// Visual columns are counted from segmentStart (wrapped rows restart
	// their tab stops at the segment edge, like the ImGui wrap layout).
	const RowText rt = expandRow(row);
	const int last = static_cast<int>(rt.byteToVisual.size() - 1);
	const int segBase = rt.byteToVisual[std::clamp(segmentStart, 0, last)];
	const int col = rt.byteToVisual[std::clamp(byteColumn, 0, last)];
	return static_cast<qreal>(std::max(0, col - segBase)) * charWidthF();
}

int QtEditorView::byteColumnAtX(int row, qreal x, int segmentStart) const
{
	const RowText rt = expandRow(row);
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
	return std::max(50, width() - gutterWidthPx - 14);
}

int QtEditorView::totalLines() const
{
	return wordWrapEnabled() ? std::max(1, wrap.totalVisualLines())
							 : std::max(1, state.lineCount());
}

void QtEditorView::refreshWrap()
{
	if (wordWrapEnabled())
		wrap.ensure(state, static_cast<float>(textAreaWidth()));
	else
		wrap.invalidate();
	scrollBar->setRange(0, maxScrollLine());
}

// y -> document row + wrap-segment start (byte column of the segment).
QtEditorView::RowHit QtEditorView::hitTestY(int y) const
{
	const int v = scrollBar->value() + std::max(0, y - titleBarPx) / lineHeightPx;
	if (wordWrapEnabled())
	{
		const WrapLayout::Hit hit = wrap.yToRow(static_cast<float>(v) + 0.5f);
		RowHit out;
		out.row = std::clamp(hit.row, 0, std::max(0, state.lineCount() - 1));
		out.segmentStart = wrap.segmentStartColumn(out.row, hit.segment);
		return out;
	}
	return {std::clamp(v, 0, std::max(0, state.lineCount() - 1)), 0};
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
	const qreal rowH = 2.0;
	const qreal charW = 1.0;
	const qreal dotH = rowH * 0.75;
	const qreal padX = 2.0;
	const int stripTop = titleBarPx;
	const qreal stripH = static_cast<qreal>(height() - stripTop);
	const int maxCols = std::max(1, static_cast<int>((mw - 2 * padX) / charW));

	// Visible-window strip math (mirrors makeStrip): fit rows in the
	// strip, anchor so the slider position stays continuous with scroll.
	const int lineCount = state.lineCount();
	const int fit = std::max(1, static_cast<int>(stripH / rowH));
	const int viewLines = visibleLines();
	const qreal sliderH = std::clamp(static_cast<qreal>(viewLines) * rowH, 4.0, stripH);
	const qreal maxTop = std::min(
		stripH - sliderH, std::max(0.0, static_cast<qreal>(lineCount) * rowH - sliderH));
	const qreal maxScroll = static_cast<qreal>(maxScrollLine());
	const qreal ratio = maxScroll > 1.0 ? maxTop / maxScroll : 0.0;
	const qreal sliderTop =
		std::clamp(static_cast<qreal>(scrollBar->value()) * ratio, 0.0, maxTop);
	int startRow = 0;
	int endRow = lineCount - 1;
	if (lineCount > fit)
	{
		startRow = std::clamp(
			static_cast<int>(scrollBar->value() - sliderTop / rowH), 0, lineCount - fit);
		endRow = std::min(lineCount - 1, startRow + fit - 1);
	}

	// Rebuild density runs only when the window/content/key changes.
	QString key = QString("mm|%1|%2|%3|%4|%5")
					  .arg(startRow)
					  .arg(endRow)
					  .arg(lineCount)
					  .arg(highlight.visualGeneration())
					  .arg(ops.generation())
					  .arg(width());
	if (key != minimapRuns.key)
	{
		minimapRuns.key = key;
		minimapRuns.runs.clear();
		constexpr qreal kDim = 0.72f;
		const auto dim = [&](const NedColor &c) {
			return QColor::fromRgbF(c.r * kDim, c.g * kDim, c.b * kDim);
		};
		std::string line;
		for (int row = startRow; row <= endRow; ++row)
		{
			const qreal y0 = stripTop + static_cast<qreal>(row - startRow) * rowH;
			line = state.line(row);
			const LineColorSpans &spans = highlight.spansForLine(row);
			size_t sp = 0;
			int runStart = -1;
			QColor runInk;
			const auto flush = [&](int col) {
				if (runStart >= 0 && col > runStart)
					minimapRuns.runs.push_back(
						{padX + static_cast<qreal>(runStart) * charW,
						 y0,
						 static_cast<qreal>(col - runStart) * charW,
						 dotH,
						 runInk});
				runStart = -1;
			};
			int col = 0;
			for (int i = 0; i < static_cast<int>(line.size()) && col < maxCols;)
			{
				const int byte = i;
				const unsigned char c = static_cast<unsigned char>(line[i++]);
				if ((c & 0xC0) == 0x80)
					continue;
				if (c == '\t')
				{
					flush(col);
					col = std::min(maxCols, col + (4 - col % 4));
					continue;
				}
				if (c <= ' ')
				{
					flush(col);
					++col;
					continue;
				}
				while (sp < spans.size() && spans[sp].end <= byte)
					++sp;
				QColor ink = dim(highlight.defaultTextColor());
				if (sp < spans.size() && spans[sp].start <= byte)
					ink = dim(highlight.colorForSlot(spans[sp].slot));
				if (runStart < 0 || ink != runInk)
				{
					flush(col);
					runStart = col;
					runInk = ink;
				}
				++col;
			}
			flush(col);
		}
	}

	// Strip background + density runs (flat rect blits).
	painter.fillRect(x0, 0, mw, height(), QColor(0x1a, 0x1a, 0x22));
	painter.setPen(Qt::NoPen);
	painter.save();
	painter.translate(x0, 0);
	for (const MRun &r : minimapRuns.runs)
	{
		painter.setBrush(r.ink);
		painter.drawRect(QRectF(r.x, r.y, r.w, r.h));
	}
	painter.restore();

	// Continuous slider (viewport indicator).
	painter.setPen(QColor(255, 255, 255, 36));
	painter.setBrush(QColor(255, 255, 255, 26));
	painter.drawRect(QRectF(x0, stripTop + sliderTop, mw, sliderH));
}

void QtEditorView::minimapScrollTo(int y)
{
	// y -> target scroll line via the strip's slider mapping (continuous).
	const qreal stripTop = titleBarPx;
	const qreal stripH = static_cast<qreal>(height() - stripTop);
	const qreal rowH = 2.0;
	const int lineCount = state.lineCount();
	const int fit = std::max(1, static_cast<int>(stripH / rowH));
	const qreal sliderH =
		std::clamp(static_cast<qreal>(visibleLines()) * rowH, 4.0, stripH);
	const qreal maxTop = std::min(
		stripH - sliderH, std::max(0.0, static_cast<qreal>(lineCount) * rowH - sliderH));
	const qreal maxScroll = static_cast<qreal>(maxScrollLine());
	const qreal ratio = maxScroll > 1.0 ? maxTop / maxScroll : 0.0;
	if (ratio <= 0.0)
		return;
	const int target =
		static_cast<int>(
			(std::clamp<qreal>(static_cast<qreal>(y) - stripTop, 0.0, maxTop)) / ratio) -
		visibleLines() / 2;
	scrollBar->setValue(std::clamp(target, 0, maxScrollLine()));
	update();
}

// Batched glyph rendering (ImGui draw-list parity): each visible row is
// converted once into per-color-run QGlyphRuns on the monospace grid and
// cached by edit generation — paints become a few drawGlyphRun calls.
int QtEditorView::gitDirtyLineCount() const
{
	int n = 0;
	for (int l = 1; l <= state.lineCount(); ++l)
		if (git.isLineEdited(state.path, l))
			++n;
	return n;
}

std::string QtEditorView::gitChangesSummary() const { return git.currentGitChanges; }

// Keep the caret inside the viewport after edits/navigation (ImGui:
// EditorViewState::revealCursor). Wrap-aware via visual lines.
void QtEditorView::revealCaret()
{
	const Selection &caret = viewState.selections[viewState.primaryIndex];
	const int v = wordWrapEnabled() ? wrap.rowStartVisualLine(caret.headRow) +
										  wrap.segmentOf(caret.headRow, caret.headColumn)
									: caret.headRow;
	const int first = scrollBar->value();
	const int visible = visibleLines();
	if (v < first)
		scrollBar->setValue(v);
	else if (v >= first + visible - 1)
		scrollBar->setValue(v - visible + 2);
}

// Direct per-glyph painting on the monospace grid. Simple by design:
// no glyph-run engines, no pixmap caches, nothing to go stale. Perf is
// measured (render-check repaint timing) before any optimization is
// allowed back in.
void QtEditorView::paintTextRow(
	QPainter &painter, int row, int y, int fromByte, int toByte, qreal textLeft)
{
	const RowText rt = expandRow(row);
	const int lastByte = static_cast<int>(rt.byteToVisual.size() - 1);
	const int vFrom = rt.byteToVisual[std::clamp(fromByte, 0, lastByte)];
	const int vTo = rt.byteToVisual[std::clamp(toByte, 0, lastByte)];
	if (vTo <= vFrom)
		return;

	const qreal cw = charWidthF();
	const LineColorSpans &spans = highlight.spansForLine(row);
	QColor ink = toQColor(highlight.defaultTextColor());

	int vis = vFrom;
	const auto flushTo = [&](int nextVis, const QColor &color) {
		painter.setPen(color);
		for (; vis < nextVis; ++vis)
			painter.drawText(QRectF(textLeft + vis * cw, y, cw, lineHeightPx),
							 Qt::AlignCenter,
							 rt.expanded.mid(vis, 1));
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

void QtEditorView::paintEvent(QPaintEvent *)
{
	QPainter painter(this);
	const QColor background = NedQtTheme::background(appSettings);
	painter.fillRect(rect(), background);

	const int firstRow = scrollBar->value();
	const int rows = std::min(visibleLines() + 2, state.lineCount() - firstRow);

	// Editor title bar: file icon, full path, git ±N (ImGui title-bar parity).
	if (!state.path.empty())
	{
		painter.fillRect(0, 0, width(), titleBarPx, NedQtTheme::raised(background));
		painter.setPen(NedQtTheme::text(appSettings).darker(130));
		QFont small = font();
		small.setPointSize(std::max(9, font().pointSize() - 3));
		painter.setFont(small);
		int tx = 10;
		if (!fileIcon.isNull())
		{
			painter.drawPixmap(QRect(tx, (titleBarPx - 16) / 2, 16, 16),
							   fileIcon.pixmap(16, 16));
			tx += 24;
		}
		painter.drawText(QRect(tx, 0, width() - tx - 160, titleBarPx),
						 Qt::AlignVCenter | Qt::AlignLeft,
						 QString::fromStdString(state.path));
		const std::string changes = git.currentGitChanges;
		if (!changes.empty())
		{
			painter.setPen(QColor(0x3f, 0xc1, 0x8c));
			painter.drawText(QRect(width() - 150, 0, 140, titleBarPx),
							 Qt::AlignVCenter | Qt::AlignRight,
							 QString::fromStdString(changes));
		}
	}

	// Gutter + current-line highlight.
	const Selection &primary = viewState.selections[viewState.primaryIndex];
	painter.setFont(font());
	for (int i = 0; i < rows; ++i)
	{
		const int row = firstRow + i;
		const int y = titleBarPx + i * lineHeightPx;
		if (row == primary.headRow)
			painter.fillRect(0, y, width(), lineHeightPx, QColor(0x2a, 0x2a, 0x2a));

		// Changed lines tint their number (green) like the ImGui gutter.
		if (git.isLineEdited(state.path, row + 1))
			painter.setPen(QColor(0x3f, 0xc1, 0x8c));
		else
			painter.setPen(QColor(0x88, 0x88, 0x88));
		painter.drawText(QRect(0, y, gutterWidthPx - 12, lineHeightPx),
						 Qt::AlignVCenter | Qt::AlignRight,
						 QString::number(row + 1));
	}

	// Git gutter marks (added/edited lines vs HEAD).
	const std::string docPath = state.path;
	for (int i = 0; i < rows; ++i)
	{
		const int row = firstRow + i;
		if (git.isLineEdited(docPath, row + 1))
			painter.fillRect(gutterWidthPx - 6,
							 titleBarPx + i * lineHeightPx,
							 3,
							 lineHeightPx,
							 QColor(0x3f, 0xc1, 0x8c));
	}

	// Text with syntax colors. Rows paint through the tab-expanded model so
	// glyphs, tabs, caret and selection share one coordinate space. With
	// word wrap on, each wrapped segment paints on its own visual line.
	const int textLeft = gutterWidthPx;
	const QColor defaultInk = toQColor(highlight.defaultTextColor());
	const bool wrapping = wordWrapEnabled();

	const auto drawRowSegment = [&](int row, int y, int fromByte, int toByte) {
		if (toByte <= fromByte)
			return;
		paintTextRow(painter, row, y, fromByte, toByte, textLeft);
	};

	if (wrapping)
	{
		for (int v = firstRow; v < totalLines() && v - firstRow < rows; ++v)
		{
			const WrapLayout::Hit hit = wrap.yToRow(static_cast<float>(v) + 0.5f);
			const int y = titleBarPx + (v - firstRow) * lineHeightPx;
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
			drawRowSegment(firstRow + i,
						   titleBarPx + i * lineHeightPx,
						   0,
						   state.lineLength(firstRow + i));
	}

	// Selection + carets on the shared tab-expanded coordinate space.
	const auto visualLineOf = [&](int row, int column) {
		if (!wrapping)
			return row;
		const int seg = wrap.segmentOf(row, column);
		return wrap.rowStartVisualLine(row) + seg;
	};

	painter.setPen(Qt::transparent);
	painter.setBrush(QColor(255, 30, 170, 70));
	for (const Selection &sel : viewState.selections)
	{
		int sr, sc, er, ec;
		sel.getOrdered(sr, sc, er, ec);
		for (int row = sr; row <= er; ++row)
		{
			const int vStart = visualLineOf(row, row == sr ? sc : 0);
			const int vEnd = visualLineOf(row, row == er ? ec : state.lineLength(row));
			for (int v = vStart; v <= vEnd; ++v)
			{
				const int i = v - firstRow;
				if (i < 0 || i >= rows)
					continue;
				const int fromB = (v == vStart && row == sr) ? sc : 0;
				const int toB =
					(v == vEnd && row == er)
						? ec
						: (wrapping ? (wrap.segmentOf(row, 0) + 1 < wrap.segmentCount(row)
										   ? wrap.segmentStartColumn(
												 row, wrap.segmentOf(row, 0) + 1)
										   : state.lineLength(row))
									: state.lineLength(row));
				const qreal x0 = textLeft + xAtByteColumn(row, fromB, fromB);
				const qreal x1 = textLeft + xAtByteColumn(row, toB, fromB);
				painter.drawRect(
					QRectF(x0, titleBarPx + i * lineHeightPx, x1 - x0, lineHeightPx));
			}
		}
	}

	if (caretVisible)
	{
		// 2px caret on the glyph boundary, never over the glyph.
		painter.setPen(QPen(QColor(255, 255, 255), 2));
		for (const Selection &sel : viewState.selections)
		{
			const int v = visualLineOf(sel.headRow, sel.headColumn);
			const int i = v - firstRow;
			if (i < 0 || i >= rows)
				continue;
			const int seg =
				wrapping ? wrap.segmentStartColumn(
							   sel.headRow, wrap.segmentOf(sel.headRow, sel.headColumn))
						 : 0;
			const qreal x = textLeft + xAtByteColumn(sel.headRow, sel.headColumn, seg);
			painter.drawLine(QPointF(x, titleBarPx + i * lineHeightPx + 2),
							 QPointF(x, titleBarPx + (i + 1) * lineHeightPx - 2));
		}
	}

	paintMinimap(painter);
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
	revealCaret();
	update();
}

void QtEditorView::toggleFindBar()
{
	if (findBar->isVisible())
		findBar->closeBar();
	else
		findBar->open();
}

void QtEditorView::goToLineDialog()
{
	// In-editor overlay (ImGui parity): small floating input, same window.
	if (!lineJumpInput)
	{
		lineJumpInput = new QLineEdit(this);
		lineJumpInput->setPlaceholderText("Go to line…");
		lineJumpInput->setFixedWidth(160);
		lineJumpInput->setAutoFillBackground(true);
		lineJumpInput->installEventFilter(this);
	}
	lineJumpInput->setText(
		QString::number(viewState.selections[viewState.primaryIndex].headRow + 1));
	lineJumpInput->show();
	lineJumpInput->setFocus();
	lineJumpInput->selectAll();
}

bool QtEditorView::eventFilter(QObject *watched, QEvent *event)
{
	if (watched == lineJumpInput && event->type() == QEvent::KeyPress)
	{
		auto *key = static_cast<QKeyEvent *>(event);
		if (key->key() == Qt::Key_Escape)
		{
			lineJumpInput->hide();
			setFocus();
			return true;
		}
		if (key->key() == Qt::Key_Return || key->key() == Qt::Key_Enter)
		{
			const int line = lineJumpInput->text().toInt();
			lineJumpInput->hide();
			setFocus();
			if (line >= 1 && line <= state.lineCount())
			{
				commands.goToLine(line - 1); // commands API is 0-based
				scrollBar->setValue(
					std::max(0,
							 viewState.selections[viewState.primaryIndex].headRow -
								 visibleLines() / 2));
				scheduleBlink();
				update();
			}
			return true;
		}
	}
	return QWidget::eventFilter(watched, event);
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
			appSettings.saveSettings();
			break;
		}
		case Qt::Key_Minus: {
			appSettings.settings["fontSize"] =
				std::max(8.0, appSettings.settings.value("fontSize", 13) - 2.0);
			applyProfileFont();
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
		commands.collapseSelection();
		break;
	case Qt::Key_Return:
	case Qt::Key_Enter:
		commands.insertNewline();
		break;
	case Qt::Key_Backspace:
		commands.deleteLeft(alt); // Alt deletes by word (ImGui: KeyAlt)
		break;
	case Qt::Key_Delete:
		commands.deleteRight(alt);
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
	if (findBar && findBar->isVisible())
		findBar->setGeometry(0, 0, width(), findBar->sizeHint().height());
	if (lineJumpInput && lineJumpInput->isVisible())
		lineJumpInput->move(width() - lineJumpInput->width() - 24, titleBarPx + 6);
	scrollBar->setGeometry(width() - 14, 0, 14, height());
	scrollBar->setPageStep(std::max(1, visibleLines() - 1));
	scrollBar->setRange(0, maxScrollLine());
}

void QtEditorView::wheelEvent(QWheelEvent *event)
{
	// Trackpads report small pixel-ish deltas; mice report 120/notch.
	// Over the minimap strip the wheel scrolls like the editor.
	const int notches = event->angleDelta().y() / 40;
	scrollBar->setValue(scrollBar->value() - notches);
	update();
}

void QtEditorView::mousePressEvent(QMouseEvent *event)
{
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
	const RowHit hit = hitTestY(static_cast<int>(event->position().y()));
	const int column =
		columnAtX(hit.row, static_cast<int>(event->position().x()), hit.segmentStart);
	commands.selectWordAt(hit.row, column);
	update();
}

void QtEditorView::showContextMenu(const QPoint &pos)
{
	QMenu menu(this);
	menu.addAction(
		"Cut",
		[this] {
			commands.cut();
			afterEdit();
		},
		QKeySequence("Ctrl+X"));
	menu.addAction("Copy", [this] { commands.copy(); }, QKeySequence("Ctrl+C"));
	menu.addAction(
		"Paste",
		[this] {
			commands.paste();
			afterEdit();
		},
		QKeySequence("Ctrl+V"));
	menu.addSeparator();
	menu.addAction(
		"Select All",
		[this] {
			commands.selectAll();
			update();
		},
		QKeySequence("Ctrl+A"));
	menu.exec(mapToGlobal(pos));
}

void QtEditorView::mouseMoveEvent(QMouseEvent *event)
{
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
