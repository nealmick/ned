#include "caret_view.h"

#include "editor_frame.h"

#include <QPainter>
#include <QPainterPath>

#include <algorithm>

QPoint CaretView::caretWidgetPos() const
{
	const Selection &caret = frame->viewState.selections[frame->viewState.primaryIndex];
	const bool wrapping = frame->wordWrapEnabled();
	const int segment =
		wrapping ? frame->caretSegmentStart(caret.headRow, caret.headColumn) : 0;
	// xAtByteColumn already returns widget-space x (gutter + absolute
	// column, horizontal scroll applied inside).
	const qreal x = frame->xAtByteColumn(caret.headRow, caret.headColumn, segment);
	const qreal y =
		frame->rowYBase() +
		static_cast<qreal>(frame->visualLineOf(caret.headRow, caret.headColumn) -
						   frame->firstVisualLine()) *
			frame->lineHeightPx;
	return QPoint(qRound(x), qRound(y));
}

namespace {
// Selection rounding, VSCode/Zed-style: exposed (convex) corners get a
// quadratic-arc radius; reflex corners — where a neighbouring line's fill
// ends strictly inside this line's fill, 3 of 4 quadrants selected — get a
// fillet wedge in the empty quadrant so staircases read as one blob.
// Interior corners (aligned full-line blocks) stay square so consecutive
// lines merge into a seamless region.
//
// Everything in this namespace is pure geometry on collected runs; the
// frame-touching collection happens in CaretView::paint (friend access).

constexpr qreal kSelectionRadius = 7.5;

// Selected fill on one visual line, [x0, x1] in widget space. Before
// merging, one entry per selection segment (a wrapped row contributes one
// per visual line), gathered across ALL selections so stacked multi-cursor
// selections read as one block.
struct SelectionRun
{
	int line; // absolute visual line
	qreal x0;
	qreal x1;
};

// Sorts runs by visual line and merges overlapping/touching ones (abutting
// fills are continuous; multi-cursor spans on one line union).
void mergeRuns(std::vector<SelectionRun> &runs)
{
	std::sort(runs.begin(), runs.end(), [](const SelectionRun &a, const SelectionRun &b) {
		return a.line != b.line ? a.line < b.line : a.x0 < b.x0;
	});
	std::size_t out = 0;
	for (const SelectionRun &r : runs)
	{
		if (out > 0 && runs[out - 1].line == r.line && r.x0 <= runs[out - 1].x1)
			runs[out - 1].x1 = qMax(runs[out - 1].x1, r.x1);
		else
			runs[out++] = r;
	}
	runs.resize(out);
}

// One rounding site along a run edge: either the run's own endpoint
// (convex corner, rounded when exposed) or a neighbouring run's endpoint
// landing inside the run (reflex corner -> fillet wedge).
struct CornerSite
{
	qreal x;			  // corner point x on the edge
	qreal radius = 0.0;	  // assigned below; 0 keeps the corner square
	bool spanEnd = false; // the run's own endpoint vs a reflex fillet
	int dx = 0;			  // fillet only: wedge x direction, away from the neighbour's
	int dy = 0;			  // fill and toward its line
};

// Caps each site's radius at half the distance to its neighbours on the
// same edge, so adjacent arcs can never cross — a 1-char step or a narrow
// span shrinks its rounding instead of producing overlapping curves.
void allocateRadii(std::vector<CornerSite> &sites, qreal base)
{
	std::sort(sites.begin(), sites.end(), [](const CornerSite &a, const CornerSite &b) {
		return a.x < b.x;
	});
	for (std::size_t i = 0; i < sites.size(); ++i)
	{
		qreal r = base;
		if (i > 0)
			r = qMin(r, (sites[i].x - sites[i - 1].x) / 2.0);
		if (i + 1 < sites.size())
			r = qMin(r, (sites[i + 1].x - sites[i].x) / 2.0);
		sites[i].radius = qMax(r, 0.0);
	}
}

// Rounding sites along one horizontal edge of `run` — its top edge for
// dy < 0 (neighbour line above), bottom edge for dy > 0. Neighbour runs
// without horizontal overlap are separate blocks: no corner interaction.
std::vector<CornerSite> edgeSites(const std::vector<SelectionRun> &runs,
								  const SelectionRun &run,
								  int neighbourLine,
								  int dy,
								  qreal base)
{
	std::vector<CornerSite> sites;
	const auto covered = [&](qreal x) {
		for (const SelectionRun &n : runs)
			if (n.line == neighbourLine && n.x0 <= x && n.x1 >= x)
				return true;
		return false;
	};
	for (const SelectionRun &n : runs)
	{
		if (n.line != neighbourLine || n.x0 >= run.x1 || n.x1 <= run.x0)
			continue;
		if (n.x0 > run.x0)
			sites.push_back({n.x0, 0.0, false, -1, dy}); // neighbour starts inside
		if (n.x1 < run.x1)
			sites.push_back({n.x1, 0.0, false, +1, dy}); // neighbour ends inside
	}
	// Closed interval at the endpoints: an exactly-aligned edge continues
	// straight through — no corner at all.
	if (!covered(run.x0))
		sites.push_back({run.x0, 0.0, true, 0, dy});
	if (!covered(run.x1))
		sites.push_back({run.x1, 0.0, true, 0, dy});
	allocateRadii(sites, base);
	return sites;
}

// Radius assigned to the run's own corner at `x` on this edge (0 = square).
qreal radiusAt(const std::vector<CornerSite> &sites, qreal x)
{
	for (const CornerSite &c : sites)
		if (c.spanEnd && c.x == x)
			return c.radius;
	return 0.0;
}

// Appends one run to the path with independent per-corner radii (0 keeps
// the corner square). Rounded corners are quadratic curves through the
// corner point.
void appendRoundedRect(
	QPainterPath &path, const QRectF &r, qreal rTL, qreal rTR, qreal rBR, qreal rBL)
{
	const qreal clamp = qMin(r.width(), r.height()) / 2.0;
	rTL = qMin(rTL, clamp);
	rTR = qMin(rTR, clamp);
	rBR = qMin(rBR, clamp);
	rBL = qMin(rBL, clamp);
	const QPointF tl = r.topLeft(), tr = r.topRight();
	const QPointF br = r.bottomRight(), bl = r.bottomLeft();
	path.moveTo(rTL > 0 ? tl + QPointF(rTL, 0) : tl);
	path.lineTo(rTR > 0 ? tr - QPointF(rTR, 0) : tr);
	if (rTR > 0)
		path.quadTo(tr, tr + QPointF(0, rTR));
	path.lineTo(rBR > 0 ? br - QPointF(0, rBR) : br);
	if (rBR > 0)
		path.quadTo(br, br - QPointF(rBR, 0));
	path.lineTo(rBL > 0 ? bl + QPointF(rBL, 0) : bl);
	if (rBL > 0)
		path.quadTo(bl, bl - QPointF(0, rBL));
	path.lineTo(rTL > 0 ? tl + QPointF(0, rTL) : tl);
	if (rTL > 0)
		path.quadTo(tl, tl + QPointF(rTL, 0));
	path.closeSubpath();
}

// Fills the empty quadrant at one reflex corner: two straight edges
// through the corner point plus a quad arc hugging it.
void appendFilletWedge(QPainterPath &path, const QPointF &corner, const CornerSite &c)
{
	const QPointF verticalEnd = corner + QPointF(0, c.dy * c.radius);
	const QPointF horizontalEnd = corner + QPointF(c.dx * c.radius, 0);
	path.moveTo(verticalEnd);
	path.lineTo(corner);
	path.lineTo(horizontalEnd);
	path.quadTo(corner, verticalEnd);
	path.closeSubpath();
}

// Draws the merged runs as one path: abutting subpaths rasterize without
// the hairline seams separate fills would leave.
void paintSelection(QPainter &painter,
					const std::vector<SelectionRun> &runs,
					int firstRow,
					int rows,
					qreal yBase,
					qreal lineHeight)
{
	// Text selection stays blue (the OS-default look); the grey accent is
	// chrome-only (sliders, tree pill, focus rings).
	painter.setBrush(QColor(0x0d, 0x6e, 0xfd, 90));
	if (runs.empty())
		return;

	QPainterPath path;
	// All subpaths wind the same way and never overlap, so winding fill
	// unions them.
	path.setFillRule(Qt::WindingFill);
	const qreal base = qMin(kSelectionRadius, lineHeight / 3.5);
	for (const SelectionRun &run : runs)
	{
		const std::vector<CornerSite> top = edgeSites(runs, run, run.line - 1, -1, base);
		const std::vector<CornerSite> bottom =
			edgeSites(runs, run, run.line + 1, +1, base);
		const qreal y = yBase + static_cast<qreal>(run.line - firstRow) * lineHeight;
		if (run.line >= firstRow && run.line < firstRow + rows)
			appendRoundedRect(path,
							  QRectF(run.x0, y, run.x1 - run.x0, lineHeight),
							  radiusAt(top, run.x0),
							  radiusAt(top, run.x1),
							  radiusAt(bottom, run.x1),
							  radiusAt(bottom, run.x0));
		// Wedges are appended for offscreen runs too: a context row's wedge
		// can poke into view (the painter clip discards the rest).
		for (const CornerSite &c : top)
			if (!c.spanEnd && c.radius >= 0.5)
				appendFilletWedge(path, QPointF(c.x, y), c);
		for (const CornerSite &c : bottom)
			if (!c.spanEnd && c.radius >= 0.5)
				appendFilletWedge(path, QPointF(c.x, y + lineHeight), c);
	}
	painter.setRenderHint(QPainter::Antialiasing, true);
	painter.fillPath(path, painter.brush());
	painter.setRenderHint(QPainter::Antialiasing, false);
}
} // namespace

// Selection rects (wrap-aware, per-segment) + carets on the shared
// tab-expanded coordinate space.
void CaretView::paint(QPainter &painter, int firstRow, int rows, qreal yBase)
{
	const bool wrapping = frame->wordWrapEnabled();

	// Pass 1 (a lambda because it needs EditorFrame's private accessors,
	// reachable only from the friend class): one run per visual-line
	// segment per selection. One visual line beyond the painted range per
	// side is kept (undrawn) so runs at the viewport edge still know
	// whether the selection continues offscreen — without it, scrolling
	// mid-selection would round corners against open air.
	std::vector<SelectionRun> runs;
	const auto appendSelectionRuns = [&](const Selection &sel) {
		int sr, sc, er, ec;
		sel.getOrdered(sr, sc, er, ec);
		for (int row = sr; row <= er; ++row)
		{
			// A wrapped row's selection spans several visual lines — clamp
			// each to its own segment's byte range.
			const int vStart = frame->visualLineOf(row, row == sr ? sc : 0);
			const int vEnd =
				frame->visualLineOf(row, row == er ? ec : frame->state.lineLength(row));
			for (int v = vStart; v <= vEnd; ++v)
			{
				if (v < firstRow - 1 || v > firstRow + rows)
					continue;
				// v is an ABSOLUTE visual line — resolve the segment index
				// from the row's visual-line base (`v - vStart` misidentifies
				// segments when the selection starts mid-row on a wrapped
				// line).
				const int segIdx = wrapping ? v - frame->wrap.rowStartVisualLine(row) : 0;
				const int segBase =
					wrapping ? frame->wrap.segmentStartColumn(row, segIdx) : 0;
				const int segEnd = wrapping && segIdx + 1 < frame->wrap.segmentCount(row)
									   ? frame->wrap.segmentStartColumn(row, segIdx + 1)
									   : frame->state.lineLength(row);
				const int fromB =
					(v == vStart && row == sr) ? std::max(sc, segBase) : segBase;
				const int toB = (v == vEnd && row == er) ? std::min(ec, segEnd) : segEnd;
				if (toB <= fromB)
					continue;
				runs.push_back({v,
								frame->xAtByteColumn(row, fromB, segBase),
								frame->xAtByteColumn(row, toB, segBase)});
			}
		}
	};
	for (const Selection &sel : frame->viewState.selections)
		appendSelectionRuns(sel);
	mergeRuns(runs);
	paintSelection(painter, runs, firstRow, rows, yBase, frame->lineHeightPx);

	if (!frame->caretVisible || !frame->caretActive())
		return;
	// 2px caret on the glyph boundary, never over the glyph.
	painter.setPen(QPen(QColor(255, 255, 255), 2));
	for (const Selection &sel : frame->viewState.selections)
	{
		const int v = frame->visualLineOf(sel.headRow, sel.headColumn);
		const int i = v - firstRow;
		if (i < 0 || i >= rows)
			continue;
		const int seg =
			wrapping ? frame->caretSegmentStart(sel.headRow, sel.headColumn) : 0;
		const qreal x = frame->xAtByteColumn(sel.headRow, sel.headColumn, seg);
		painter.drawLine(QPointF(x, yBase + i * frame->lineHeightPx + 2),
						 QPointF(x, yBase + (i + 1) * frame->lineHeightPx - 2));
	}
}
