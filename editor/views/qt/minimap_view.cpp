#include "minimap_view.h"

#include "../../../util/settings.h"
#include "../../editor_state.h"
#include "../../services/highlight/highlight_service.h"
#include "editor_frame.h"
#include "find_bar.h"
#include "hover_tooltip.h"
#include "line_jump.h"
#include "ned_color.h"

#include <QMenu>
#include <QShortcut>

#include "host/qt/fonts.h"
#include "host/qt/theme.h"
#include "util/qt_icons.h"
#include <QApplication>
#include <QFontMetrics>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QScrollBar>
#include <QTimer>
#include <QWheelEvent>

#include <algorithm>

MinimapView::Geometry
MinimapView::geometry(qreal stripH, int viewLines, int lineCount, int maxScrollLine)
{
	Geometry g;
	const qreal sliderH = std::clamp(static_cast<qreal>(viewLines) * g.rowH, 4.0, stripH);
	const qreal maxTop =
		std::min(stripH - sliderH,
				 std::max(0.0, static_cast<qreal>(lineCount) * g.rowH - sliderH));
	const qreal maxScroll = static_cast<qreal>(std::max(0, maxScrollLine));
	g.sliderH = sliderH;
	g.maxTop = maxTop;
	g.ratio = maxScroll > 1.0 ? maxTop / maxScroll : 0.0;
	g.fit = std::max(1, static_cast<int>(stripH / g.rowH));
	return g;
}

qreal MinimapView::Geometry::sliderTopFor(qreal scrollLines) const
{
	return std::clamp(scrollLines * ratio, 0.0, maxTop);
}

qreal MinimapView::Geometry::scrollLinesForY(qreal yInStrip) const
{
	if (ratio <= 0.0)
		return 0.0;
	// The strip maps slider-top positions; the clicked point becomes the
	// viewport top directly (no centering offset — a centered offset
	// undershot the bottom, since clamp can't raise an undershot value).
	return std::clamp(yInStrip, 0.0, maxTop) / ratio;
}

void MinimapView::ensureRuns(const QString &newKey,
							 int startRow,
							 int endRow,
							 EditorState &state,
							 EditorHighlight &highlight,
							 qreal stripTop,
							 qreal dotH,
							 qreal padX,
							 qreal charW,
							 int maxCols)
{
	if (newKey == key)
		return;
	key = newKey;
	runs_.clear();
	constexpr qreal kDim = 0.72f;
	const auto dim = [&](const NedColor &c) {
		return QColor::fromRgbF(c.r * kDim, c.g * kDim, c.b * kDim);
	};
	std::string line;
	const qreal rowH = Geometry{}.rowH; // the one rowH definition (geometry())
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
				runs_.push_back({padX + static_cast<qreal>(runStart) * charW,
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

// --- EditorFrame minimap strip (painting + click/drag scrolling) ----------
// EditorFrame seam: the strip geometry + density-run cache live in the
// MinimapView helper above; these methods paint it inside the editor view.

#include "../../util/utf8.h"

#include <cmath>
#include <cstring>
#include <fstream>
#include <iostream>
#include <sstream>

bool EditorFrame::minimapEnabled() const
{
	return appSettings.settings.value("minimap", true);
}
int EditorFrame::minimapWidth() const { return minimapEnabled() ? 70 : 0; }
void EditorFrame::paintMinimap(QPainter &painter)
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
	// definition in MinimapView — see the undershoot bug note there).
	const MinimapView::Geometry geo =
		MinimapView::geometry(stripH, visibleLines(), state.lineCount(), maxScrollLine());
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
	for (const MinimapView::Run &r : minimap.runs())
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
void EditorFrame::minimapScrollTo(int y)
{
	// y -> target scroll line via the strip's slider mapping (continuous,
	// geometry shared with paintMinimap).
	const MinimapView::Geometry geo =
		MinimapView::geometry(static_cast<qreal>(height() - topInset()),
							  visibleLines(),
							  state.lineCount(),
							  maxScrollLine());
	const qreal target = geo.scrollLinesForY(static_cast<qreal>(y) - topInset());
	if (geo.ratio <= 0.0)
		return;
	setScrollPixels(target * lineHeightPx);
}
