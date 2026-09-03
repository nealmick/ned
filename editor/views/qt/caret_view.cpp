#include "caret_view.h"

#include "editor_frame.h"

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

QPoint CaretView::caretWidgetPos() const
{
	const Selection &caret = frame->viewState.selections[frame->viewState.primaryIndex];
	const bool wrapping = frame->wordWrapEnabled();
	const int segment =
		wrapping ? frame->caretSegmentStart(caret.headRow, caret.headColumn) : 0;
	const qreal x = frame->gutterWidthPx +
					frame->xAtByteColumn(caret.headRow, caret.headColumn, segment) -
					(wrapping ? 0.0 : frame->scrollPxX);
	const qreal y =
		frame->rowYBase() +
		static_cast<qreal>(frame->visualLineOf(caret.headRow, caret.headColumn) -
						   frame->firstVisualLine()) *
			frame->lineHeightPx;
	return QPoint(qRound(x), qRound(y));
}

// Selection rects (wrap-aware, per-segment) + carets on the shared
// tab-expanded coordinate space.
void CaretView::paint(QPainter &painter, int firstRow, int rows, qreal yBase, qreal textX0)
{
	const bool wrapping = frame->wordWrapEnabled();
	// Selection + carets on the shared tab-expanded coordinate space.
	painter.setPen(Qt::transparent);
	// Text selection stays blue (the OS-default look); the grey accent is
	// chrome-only (sliders, tree pill, focus rings).
	painter.setBrush(QColor(0x0d, 0x6e, 0xfd, 90));
	for (const Selection &sel : frame->viewState.selections)
	{
		int sr, sc, er, ec;
		sel.getOrdered(sr, sc, er, ec);
		for (int row = sr; row <= er; ++row)
		{
			// Per-segment rects: a wrapped row's selection spans several
			// visual lines — clamp each to its own segment's byte range
			// (previously continuation rects used whole-line bounds).
			const int vStart = frame->visualLineOf(row, row == sr ? sc : 0);
			const int vEnd =
				frame->visualLineOf(row, row == er ? ec : frame->state.lineLength(row));
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
				const qreal x0 = textX0 + frame->xAtByteColumn(row, fromB, segBase);
				const qreal x1 = textX0 + frame->xAtByteColumn(row, toB, segBase);
				painter.drawRect(QRectF(
					x0, yBase + i * frame->lineHeightPx, x1 - x0, frame->lineHeightPx));
			}
		}
	}
	if (frame->caretVisible && frame->caretActive())
	{
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
			const qreal x =
				textX0 + frame->xAtByteColumn(sel.headRow, sel.headColumn, seg);
			painter.drawLine(QPointF(x, yBase + i * frame->lineHeightPx + 2),
							 QPointF(x, yBase + (i + 1) * frame->lineHeightPx - 2));
		}
	}
}
