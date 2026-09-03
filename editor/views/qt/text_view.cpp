#include "text_view.h"

#include "diagnostic_style.h"
#include "editor_frame.h"

#include "row_text.h"

#include "../../services/diagnostics/diagnostic_colors.h"
#include "../../util/text_columns.h"
#include "../../util/utf8.h"

#include "../../../util/settings.h"
#include "find_bar.h"
#include "hover_tooltip.h"
#include "line_jump.h"
#include "ned_color.h"

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

#include <cmath>
#include <cstring>
#include <fstream>
#include <iostream>
#include <sstream>

qreal EditorFrame::charWidthF() const
{
	// Cached at font-set time; the single width source for the whole view.
	return cellWidth;
}
RowText EditorFrame::expandRow(int row, int segmentStart) const
{
	return expandRowText(state.line(row), segmentStart);
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
qreal EditorFrame::xAtByteColumn(int row, int byteColumn, int segmentStart) const
{
	// Visual columns are counted from segmentStart (wrapped rows restart
	// their tab stops at the segment edge, like the ImGui wrap layout).
	const RowText rt = expandRow(row, segmentStart);
	const int last = static_cast<int>(rt.byteToVisual.size() - 1);
	const int segBase = rt.byteToVisual[std::clamp(segmentStart, 0, last)];
	const int col = rt.byteToVisual[std::clamp(byteColumn, 0, last)];
	return static_cast<qreal>(std::max(0, col - segBase)) * charWidthF();
}
int EditorFrame::byteColumnAtX(int row, qreal x, int segmentStart) const
{
	const RowText rt = expandRow(row, segmentStart);
	int visual = static_cast<int>(std::round(x / charWidthF()));
	visual = std::clamp(visual, 0, static_cast<int>(rt.visualToByte.size() - 1));
	return rt.visualToByte[static_cast<size_t>(visual)];
}

void TextView::paintDiagnosticSquiggles(
	QPainter &painter, int firstVisual, int visualRows, qreal yBase, int textLeft)
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
	for (const Seg &seg : segs)
	{
		const std::string text = frame->state.line(seg.row);
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
						  : frame->state.lineLength(seg.row);
			if (toB < fromB)
				std::swap(fromB, toB);
			fromB = std::clamp(fromB, seg.fromB, seg.toB);
			toB = std::clamp(toB, seg.fromB, seg.toB);
			if (toB < fromB)
				continue;

			const int segStart = frame->wordWrapEnabled() ? seg.fromB : 0; // tab stops
			const qreal x0 = textLeft + frame->xAtByteColumn(seg.row, fromB, segStart);
			const qreal x1 = textLeft + frame->xAtByteColumn(seg.row, toB, segStart);
			drawSquiggle(painter,
						 std::min(x0, clipRight),
						 std::min(std::max(x1, x0), clipRight),
						 seg.y + frame->lineHeightPx - 3.0,
						 DiagnosticSeverityColorQ(d.severity));
		}
	}
}

void TextView::drawSquiggle(
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
// allowed back in.
void TextView::paintRow(
	QPainter &painter, int row, qreal y, int fromByte, int toByte, qreal textLeft)
{
	// fromByte doubles as the wrap-segment start (0 for whole rows), so the
	// byte->visual map carries segment-rebased tab stops — the same
	// coordinate space the shared wrap layout measured the segment in.
	const RowText rt = frame->expandRow(row, fromByte);
	const int lastByte = static_cast<int>(rt.byteToVisual.size() - 1);
	const int vFrom = rt.byteToVisual[std::clamp(fromByte, 0, lastByte)];
	const int vTo = rt.byteToVisual[std::clamp(toByte, 0, lastByte)];
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
				rt.cellText(vis));
	};
	for (const ColorSpan &span : spans)
	{
		const int sVis = rt.byteToVisual[std::clamp(span.start, 0, lastByte)];
		const int eVis = rt.byteToVisual[std::clamp(span.end, 0, lastByte)];
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
qreal TextView::paint(QPainter &painter, int firstRow, int rows, qreal yBase)
{
	const bool wrapping = frame->wordWrapEnabled();
	const int textLeft = frame->gutterWidthPx;
	// Text x origin: wrap-off text shifts with the horizontal scroll (the
	// gutter stays pinned); wrap-on segments always start at the gutter.
	const qreal textX0 = wrapping ? textLeft : textLeft - frame->scrollPxX;
	// Narrow the text clip to the text area: scrolled text cuts at the
	// gutter's right edge instead of sliding under the line numbers.
	painter.setClipRect(frame->gutterWidthPx,
						frame->topInset(),
						frame->width() - frame->gutterWidthPx - frame->minimapWidth(),
						frame->height() - frame->topInset());
	const auto drawRowSegment = [&](int row, int y, int fromByte, int toByte) {
		if (toByte <= fromByte)
			return;
		paintRow(painter, row, y, fromByte, toByte, textX0);
	};

	if (wrapping)
	{
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
			drawRowSegment(hit.row, y, startB, endB);
		}
	} else
	{
		for (int i = 0; i < rows; ++i)
			drawRowSegment(firstRow + i,
						   yBase + i * frame->lineHeightPx,
						   0,
						   frame->state.lineLength(firstRow + i));
	}
	return textX0;
}
