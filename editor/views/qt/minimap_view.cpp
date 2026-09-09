#include "minimap_view.h"

#include "../../../util/settings.h"
#include "../../editor_state.h"
#include "../../services/highlight/highlight_service.h"
#include "editor_frame.h"
#include "ned_color.h"

#include <QPainter>

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

// --- MinimapView paint + interact (ImGui MinimapView parity) ---------------
// EditorFrame keeps minimapEnabled/minimapWidth (widget geometry queries
// used all over the frame) and forwards events that land in the strip.

bool EditorFrame::minimapEnabled() const
{
	return appSettings.settings.value("minimap", true);
}
int EditorFrame::minimapWidth() const { return minimapEnabled() ? 70 : 0; }

void MinimapView::paint(QPainter &painter, EditorFrame &frame)
{
	const int mw = frame.minimapWidth();
	if (mw <= 0)
		return;
	const int x0 = frame.width() - mw;

	// Density model (ImGui minimap_view parity): ~2px rows, 1px cols,
	// dots at 75% row height, colors dimmed to 72%.
	const qreal charW = 1.0;
	const qreal padX = 2.0;
	const int stripTop = frame.topInset();
	const qreal stripH = static_cast<qreal>(frame.height() - stripTop);
	const int maxCols = std::max(1, static_cast<int>((mw - 2 * padX) / charW));

	// Visible-window strip: geometry shared with scrollTo (one definition
	// in MinimapView — see the undershoot bug note there). NOTE: this must
	// be the static geometry() FUNCTION — `Geometry(...)` (the type) would
	// paren-aggregate-initialize the struct fields from these arguments.
	const Geometry geo = geometry(
		stripH, frame.visibleLines(), frame.state.lineCount(), frame.maxScrollLine());
	const qreal scrollLinesF = frame.viewState.scrollPx / frame.lineHeightPx;
	const qreal sliderTop = geo.sliderTopFor(scrollLinesF);
	int startRow = 0;
	int endRow = frame.state.lineCount() - 1;
	if (frame.state.lineCount() > geo.fit)
	{
		startRow = std::clamp(static_cast<int>(scrollLinesF - sliderTop / geo.rowH),
							  0,
							  frame.state.lineCount() - geo.fit);
		endRow = std::min(frame.state.lineCount() - 1, startRow + geo.fit - 1);
	}

	// Rebuild density runs only when the window/content/key changes.
	// stripTop MUST be in the key: the find bar changes it without any
	// other key member moving, and stale runs would paint over the bar
	// until a resize happened to rebuild them.
	ensureRuns(QString("mm|%1|%2|%3|%4|%5|%6|%7")
				   .arg(startRow)
				   .arg(endRow)
				   .arg(frame.state.lineCount())
				   .arg(frame.highlight.visualGeneration())
				   .arg(frame.ops.generation())
				   .arg(frame.width())
				   .arg(stripTop),
			   startRow,
			   endRow,
			   frame.state,
			   frame.highlight,
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
	for (const Run &r : runs())
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

bool MinimapView::press(EditorFrame &frame, const QPointF &pos)
{
	if (pos.x() < static_cast<qreal>(frame.width() - frame.minimapWidth()))
		return false;
	dragging_ = true;
	scrollTo(frame, pos.y());
	return true;
}

void MinimapView::move(EditorFrame &frame, const QPointF &pos)
{
	if (!dragging_)
		return;
	scrollTo(frame, pos.y());
}

void MinimapView::release() { dragging_ = false; }

void MinimapView::scrollTo(EditorFrame &frame, qreal y)
{
	// y -> target scroll line via the strip's slider mapping (continuous,
	// geometry shared with paint).
	const Geometry geo = geometry(static_cast<qreal>(frame.height() - frame.topInset()),
								  frame.visibleLines(),
								  frame.state.lineCount(),
								  frame.maxScrollLine());
	if (geo.ratio <= 0.0)
		return;
	const qreal target = geo.scrollLinesForY(y - static_cast<qreal>(frame.topInset()));
	frame.setScrollPixels(target * static_cast<qreal>(frame.lineHeightPx));
}
