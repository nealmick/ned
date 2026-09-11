#include "gutter_view.h"

#include "diagnostic_style.h"
#include "editor_frame.h"

#include "host/qt/theme.h"
#include <QPainter>

void GutterView::updateWidth()
{
	// ImGui parity (gutter_view.cpp): the number column only needs to fit
	// max(999, lineCount + 1) — three digits for most files, growing when
	// the document passes 999 lines. Numbers right-align inside it with a
	// 12px pad on the right and 4px of breathing room at the left edge.
	// With diagnostics bound, a severity-mark column (ImGui parity) sits
	// in front of the numbers.
	// Diff views number rows by the OLD/NEW side (both columns' max digits
	// apply) and add a +/- marker column in front, mirroring the severity
	// column's sizing.
	int reference = std::max(999, frame->state.lineCount() + 1);
	if (frame->diffActive)
	{
		int maxLine = 0;
		for (const DiffOp &op : frame->diffRows)
			maxLine = std::max(maxLine, std::max(op.oldLine, op.newLine));
		reference = std::max(reference, maxLine + 1);
	}
	frame->gutterWidthPx =
		frame->fontMetrics().horizontalAdvance(QString::number(reference)) + 12 + 4 +
		diagColumnWidth() + diffColumnWidth();
}

int GutterView::diagColumnWidth() const
{
	if (!frame->diagStore)
		return 0;
	return std::max(6, static_cast<int>(frame->fontMetrics().height() * 0.55));
}

int GutterView::diffColumnWidth() const
{
	if (!frame->diffActive)
		return 0;
	return std::max(10, static_cast<int>(frame->fontMetrics().height() * 0.7));
}

// Gutter + current-line highlight + severity marks. `rows` counts VISUAL
// lines; only a row's first segment carries gutter decoration.
void GutterView::paint(QPainter &painter,
					   int firstRow,
					   int rows,
					   qreal yBase,
					   const std::vector<int> &diagSeverity,
					   int diagColW)
{
	const QColor background = NedQtTheme::background(frame->appSettings);
	const Selection &primary = frame->viewState.selections[frame->viewState.primaryIndex];
	// In wrap mode `rows` counts VISUAL lines: map each back to its
	// document row via the wrap layout (a 1:1 visual→row mapping printed
	// garbage numbers on continuation rows), and only the FIRST segment of
	// a row carries gutter decorations (ImGui gutter_view parity).
	const bool wrapping = frame->wordWrapEnabled();
	for (int i = 0; i < rows; ++i)
	{
		int row = firstRow + i;
		int segment = 0;
		if (wrapping)
		{
			const WrapLayout::Hit hit = frame->wrap.yToRow(firstRow + i + 0.5f);
			row = hit.row;
			segment = hit.segment;
		}
		const qreal y = yBase + i * frame->lineHeightPx;
		// Current-line highlight — but NOT on rows the selection covers:
		// lighter-band + blue overlay stacks into a washed-out "inverted"
		// look (VSCode also hides it under selections).
		const bool inSelection = [&primary, row]() {
			int sr, sc, er, ec;
			primary.getOrdered(sr, sc, er, ec);
			return row >= sr && row <= er;
		}();
		if (row == primary.headRow && !inSelection)
			painter.fillRect(QRectF(0, y, frame->width(), frame->lineHeightPx),
							 background.lighter(118));
		if (segment > 0)
			continue; // continuation visual line: no gutter decoration
		// Diagnostic severity mark (first visual line of the row only —
		// VSCode-style; continuation lines carry no gutter decoration).
		if (row < static_cast<int>(diagSeverity.size()) && diagSeverity[row] > 0)
		{
			const QColor sevColor = DiagnosticSeverityColorQ(diagSeverity[row]);
			const qreal markW = std::max<qreal>(3.0, diagColW * 0.45);
			painter.setPen(Qt::NoPen);
			painter.setBrush(sevColor);
			painter.drawRoundedRect(
				QRectF(
					4 + (diagColW - markW) / 2.0, y + 2, markW, frame->lineHeightPx - 4),
				2.0,
				2.0);
			painter.setBrush(Qt::NoBrush);
		}
		// Diff views: +/- marker column, then the side-appropriate number
		// (new-side for context/added rows, dim old-side for removed rows).
		if (frame->diffActive)
		{
			const int diffColW = diffColumnWidth();
			if (row >= static_cast<int>(frame->diffRows.size()))
				continue;
			const DiffOp &op = frame->diffRows[size_t(row)];
			const int number = op.kind == DiffOp::Kind::Delete ? op.oldLine : op.newLine;
			if (op.kind == DiffOp::Kind::Add)
			{
				painter.setPen(QColor(0x3f, 0xb9, 0x50));
				painter.drawText(QRectF(diagColW, y, diffColW, frame->lineHeightPx),
								 Qt::AlignVCenter | Qt::AlignHCenter,
								 QStringLiteral("+"));
			} else if (op.kind == DiffOp::Kind::Delete)
			{
				painter.setPen(QColor(0xf8, 0x51, 0x49));
				painter.drawText(QRectF(diagColW, y, diffColW, frame->lineHeightPx),
								 Qt::AlignVCenter | Qt::AlignHCenter,
								 QStringLiteral("\u2212"));
			}
			painter.setPen(op.kind == DiffOp::Kind::Delete ? QColor(0x98, 0x6b, 0x6b)
														   : QColor(0x88, 0x88, 0x88));
			if (op.kind == DiffOp::Kind::Add)
				painter.setPen(QColor(255, 255, 255));
			painter.drawText(QRectF(diagColW + diffColW,
									y,
									frame->gutterWidthPx - 12 - diagColW - diffColW,
									frame->lineHeightPx),
							 Qt::AlignVCenter | Qt::AlignRight,
							 QString::number(number));
			continue;
		}
		// Line numbers match the ImGui gutter: current + edited lines are
		// white, everything else gray. No bars/marks.
		if (row == primary.headRow || frame->git.isLineEdited(frame->state.path, row + 1))
			painter.setPen(QColor(255, 255, 255));
		else
			painter.setPen(QColor(0x88, 0x88, 0x88));
		painter.drawText(
			QRectF(diagColW, y, frame->gutterWidthPx - 12 - diagColW, frame->lineHeightPx),
			Qt::AlignVCenter | Qt::AlignRight,
			QString::number(row + 1));
	}
}
