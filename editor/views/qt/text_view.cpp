#include "text_view.h"

#include "diagnostic_style.h"
#include "editor_frame.h"

#include "row_text.h"

#include "../../util/text_columns.h"
#include "../../util/utf8.h"

#include "ned_color.h"

#include <QPainter>

#include <cmath>

qreal EditorFrame::charWidthF() const
{
	// Cached at font-set time; the single width source for the whole view.
	return cellWidth;
}
const std::string &EditorFrame::lineRef(int row) const
{
	if (scratchRow != row || scratchGen != ops.generation())
	{
		state.lineInto(row, scratchLine);
		scratchRow = row;
		scratchGen = ops.generation();
	}
	return scratchLine;
}

// Wrap-aware caret placement: the visual line holding (row, column) —
// the row's base visual line plus the wrap segment the column falls in.
int EditorFrame::visualLineOf(int row, int column) const
{
	if (!wordWrapEnabled())
		return row;
	return wrap.rowStartVisualLine(row) + wrap.segmentOf(row, column);
}
// Byte column where the wrap segment containing (row, column) starts —
// tab stops rebase there (the shared wrap layout measures flush-left).
int EditorFrame::caretSegmentStart(int row, int column) const
{
	return wrap.segmentStartColumn(row, wrap.segmentOf(row, column));
}

EditorFrame::TextWindow EditorFrame::textWindow(int row, int segmentStart) const
{
	if (wordWrapEnabled())
	{
		// One wrap segment: bounded by the wrap width, so O(segment).
		const int seg = wrap.segmentOf(row, segmentStart);
		const int end = seg + 1 < wrap.segmentCount(row)
							? wrap.segmentStartColumn(row, seg + 1)
							: state.lineLength(row);
		return {segmentStart, end, 0};
	}
	// Viewport columns (plus a small over-scan), as a byte window. The
	// walk inside windowForVisual stops at the last visible column, so a
	// giant line costs O(scrolled-to column), never O(line).
	const qreal cw = charWidthF();
	if (cw <= 0.0)
		return {0, state.lineLength(row), 0};
	const ByteWindow w = windowForVisual(
		lineRef(row),
		static_cast<int>(viewState.scrollPxX / cw) - 4,
		static_cast<int>((viewState.scrollPxX + textAreaWidth()) / cw) + 4);
	return {w.from, w.to, w.visualBase};
}

RowText EditorFrame::expandWindow(int row, int fromByte, int toByte, int visualBase) const
{
	return expandRowText(lineRef(row), fromByte, toByte, visualBase);
}

qreal EditorFrame::xAtByteColumn(int row, int byteColumn, int segmentStart) const
{
	// Screen (widget-space) x of a byte column. Wrap-off columns are
	// ABSOLUTE (x = gutter + col*cw - viewState.scrollPxX), so horizontal scroll
	// lands exactly once — the window only limits which columns resolve
	// exactly; far-off ones clamp to its borders (row_text.h visAt),
	// which is where callers clip the rects/carets anyway. Wrapped rows
	// stay segment-relative (continuation rows restart at the gutter).
	const TextWindow w = textWindow(row, segmentStart);
	const RowText rt = expandWindow(row, w.from, w.to, w.visualBase);
	if (!wordWrapEnabled())
		return static_cast<qreal>(gutterWidthPx) +
			   static_cast<qreal>(rt.visAt(byteColumn)) * charWidthF() -
			   viewState.scrollPxX;
	const int segBase = rt.visAt(segmentStart);
	const int col = rt.visAt(byteColumn);
	return static_cast<qreal>(gutterWidthPx) +
		   static_cast<qreal>(std::max(0, col - segBase)) * charWidthF();
}
int EditorFrame::byteColumnAtX(int row, qreal x, int segmentStart) const
{
	// x is pixels from the segment start (wrap) or the line start
	// (wrap-off, scroll already added by the caller) — an ABSOLUTE visual
	// column; byteAt applies the window's visualBase shift itself.
	const TextWindow w = textWindow(row, segmentStart);
	const RowText rt = expandWindow(row, w.from, w.to, w.visualBase);
	const int visual = static_cast<int>(std::round(x / charWidthF()));
	return rt.byteAt(visual);
}

void TextView::paintDiagnosticSquiggles(QPainter &painter,
										int firstVisual,
										int visualRows,
										qreal yBase)
{
	if (!frame->diagStore || frame->state.path.empty())
		return;
	const std::vector<DiagnosticItem> items =
		frame->diagStore->forDocument(frame->state.path);
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
	if (frame->wordWrapEnabled())
	{
		for (int v = firstVisual; v < frame->totalLines() && v - firstVisual < visualRows;
			 ++v)
		{
			const WrapLayout::Hit hit = frame->wrap.yToRow(static_cast<float>(v) + 0.5f);
			const int segCount = frame->wrap.segmentCount(hit.row);
			segs.push_back({hit.row,
							yBase + (v - firstVisual) * frame->lineHeightPx,
							frame->wrap.segmentStartColumn(hit.row, hit.segment),
							hit.segment + 1 < segCount
								? frame->wrap.segmentStartColumn(hit.row, hit.segment + 1)
								: frame->state.lineLength(hit.row)});
		}
	} else
	{
		for (int i = 0; i < visualRows; ++i)
			segs.push_back({firstVisual + i,
							yBase + i * frame->lineHeightPx,
							0,
							frame->state.lineLength(firstVisual + i)});
	}

	const qreal clipRight = static_cast<qreal>(frame->width() - frame->minimapWidth());
	// Segs arrive in visual order, so a row's segments are consecutive:
	// convert each diagnostic's UTF-16 columns to bytes ONCE per row (a
	// giant wrapped row fills the whole seg list, and the conversion walks
	// the line — per-seg repetition cost O(line) per visible segment).
	int curRow = -1;
	std::vector<std::pair<int, int>> rowRanges; // byte range per matching item
	std::vector<const DiagnosticItem *> rowItems;
	for (const Seg &seg : segs)
	{
		if (seg.row != curRow)
		{
			curRow = seg.row;
			rowRanges.clear();
			rowItems.clear();
			const std::string &text = frame->lineRef(seg.row);
			for (const DiagnosticItem &d : items)
			{
				if (seg.row < d.startLine || seg.row > d.endLine)
					continue;
				// Wire columns are UTF-16; convert against this row's bytes.
				int fromB = seg.row == d.startLine ? EditorUtils::Utf16ToUtf8ByteOffset(
														 text, d.startCharacter)
												   : 0;
				int toB = seg.row == d.endLine
							  ? EditorUtils::Utf16ToUtf8ByteOffset(text, d.endCharacter)
							  : frame->state.lineLength(seg.row);
				if (toB < fromB)
					std::swap(fromB, toB);
				rowRanges.push_back({fromB, toB});
				rowItems.push_back(&d);
			}
		}
		for (size_t k = 0; k < rowItems.size(); ++k)
		{
			int fromB = std::clamp(rowRanges[k].first, seg.fromB, seg.toB);
			int toB = std::clamp(rowRanges[k].second, seg.fromB, seg.toB);
			if (toB < fromB)
				continue;

			const int segStart = frame->wordWrapEnabled() ? seg.fromB : 0;	 // tab stops
			const qreal x0 = frame->xAtByteColumn(seg.row, fromB, segStart); // widget x
			const qreal x1 = frame->xAtByteColumn(seg.row, toB, segStart);
			paintSquiggle(painter,
						  std::min(x0, clipRight),
						  std::min(std::max(x1, x0), clipRight),
						  seg.y + frame->lineHeightPx - 3.0,
						  DiagnosticSeverityColorQ(rowItems[k]->severity));
		}
	}
}

void TextView::paintSquiggle(
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

// Direct per-glyph painting on the monospace grid. Simple by design:
// no glyph-run engines, no pixmap caches, nothing to go stale. Perf is
// measured (render-check repaint timing) before any optimization is
// allowed back in. Only the byte WINDOW [fromByte, toByte) is expanded
// and drawn — the viewport's columns (wrap off) or one wrap segment —
// so a giant single-line file paints a few hundred glyphs, not millions.
void TextView::paintRow(QPainter &painter,
						int row,
						qreal y,
						int fromByte,
						int toByte,
						int visualBase,
						qreal textLeft)
{
	// fromByte doubles as the wrap-segment start (0 for whole rows), so the
	// byte->visual map carries segment-rebased tab stops — the same
	// coordinate space the shared wrap layout measured the segment in.
	const RowText rt = frame->expandWindow(row, fromByte, toByte, visualBase);
	const int vFrom = rt.visAt(fromByte);
	const int vTo = rt.visAt(toByte);
	if (vTo <= vFrom)
		return;

	const qreal cw = frame->charWidthF();
	const LineColorSpans &spans = frame->highlight.spansForLine(row);
	QColor ink = toQColor(frame->highlight.defaultTextColor());

	int vis = vFrom;
	// Segments draw REBASED: a wrapped continuation row starts at its own
	// left edge, so the x of a glyph is (vis - vFrom) cells in — not its
	// position from the line start (that pushed continuation segments off
	// past the clip; the wrap-rendering bug).
	const auto flushTo = [&](int nextVis, const QColor &color) {
		painter.setPen(color);
		for (; vis < nextVis; ++vis)
			painter.drawText(
				QRectF(textLeft + (vis - vFrom) * cw, y, cw, frame->lineHeightPx),
				Qt::AlignCenter,
				rt.cellText(vis - rt.visualBase));
	};
	for (const ColorSpan &span : spans)
	{
		if (span.end <= rt.base) // entirely left of the window
			continue;
		if (span.start >= toByte) // spans are start-ordered: rest is right
			break;
		const int sVis = rt.visAt(span.start);
		const int eVis = rt.visAt(span.end);
		if (sVis > vis)
			flushTo(std::min(sVis, vTo), ink);
		ink = toQColor(frame->highlight.colorForSlot(span.slot));
		flushTo(std::min(eVis, vTo), ink);
	}
	flushTo(vTo, ink);
}

// Text with syntax colors through the tab-expanded model. Returns the
// text x origin (wrap-off shifts with the horizontal scroll) so the
// squiggle/selection passes share it.
void TextView::paint(QPainter &painter, int firstRow, int rows, qreal yBase)
{
	const bool wrapping = frame->wordWrapEnabled();
	const int textLeft = frame->gutterWidthPx;
	// Narrow the text clip to the text area: scrolled text cuts at the
	// gutter's right edge instead of sliding under the line numbers.
	painter.setClipRect(frame->gutterWidthPx,
						frame->topInset(),
						frame->width() - frame->gutterWidthPx - frame->minimapWidth(),
						frame->height() - frame->topInset());
	const auto drawRowSegment =
		[&](int row, int y, int fromByte, int toByte, int visualBase, qreal originX) {
			if (toByte <= fromByte)
				return;
			paintRow(painter, row, y, fromByte, toByte, visualBase, originX);
		};

	if (wrapping)
	{
		// Wrap-on segments always start at the gutter.
		for (int v = firstRow; v < frame->totalLines() && v - firstRow < rows; ++v)
		{
			const WrapLayout::Hit hit = frame->wrap.yToRow(static_cast<float>(v) + 0.5f);
			const qreal y = yBase + (v - firstRow) * frame->lineHeightPx;
			const int startB = frame->wrap.segmentStartColumn(hit.row, hit.segment);
			const int segCount = frame->wrap.segmentCount(hit.row);
			const int endB =
				hit.segment + 1 < segCount
					? frame->wrap.segmentStartColumn(hit.row, hit.segment + 1)
					: frame->state.lineLength(hit.row);
			drawRowSegment(hit.row, y, startB, endB, 0, static_cast<qreal>(textLeft));
		}
	} else
	{
		for (int i = 0; i < rows; ++i)
		{
			// Only the viewport's byte window of each row — never the
			// whole line (a minified single-line file would otherwise
			// push millions of drawText calls through the clip per frame).
			// The window origin is screen space: the scroll offset lands
			// HERE (once), and paintRow positions glyphs relative to it.
			const EditorFrame::TextWindow w = frame->textWindow(firstRow + i, 0);
			const qreal originX =
				static_cast<qreal>(textLeft) +
				(static_cast<qreal>(w.visualBase) * frame->charWidthF() -
				 frame->viewState.scrollPxX);
			drawRowSegment(firstRow + i,
						   yBase + i * frame->lineHeightPx,
						   w.from,
						   w.to,
						   w.visualBase,
						   originX);
		}
	}
}
