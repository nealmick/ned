#include "title_bar_view.h"

#include "editor_frame.h"

#include "../../../../host/qt/qt_icons.h"
#include "host/qt/theme.h"
#include <QFontMetrics>
#include <QPainter>

std::string TitleBarView::gitSummary() const { return frame->git.currentGitChanges; }

void TitleBarView::reloadIcon()
{
	if (frame->state.path.empty())
		return;
	const QFontMetrics fm(frame->font());
	const int px = std::max(11, fm.height() - 2);
	frame->fileIcon = QtIconSet::forFile(QString::fromStdString(frame->state.path), px);
}

// Editor title strip: file icon, full path, git ±N (ImGui title-bar
// parity). Painted first, above the clipped gutter/text area.
void TitleBarView::paint(QPainter &painter)
{
	// Editor title bar: file icon, full path, git ±N (ImGui title-bar parity).
	if (!frame->state.path.empty())
	{
		painter.setPen(NedQtTheme::text(frame->appSettings).darker(130));
		QFont small = frame->font();
		small.setPointSize(std::max(9, frame->font().pointSize() - 3));
		painter.setFont(small);
		int tx = 10;
		if (!frame->fileIcon.isNull())
		{
			// Icon scales with the (small) title font. Fetch at device
			// resolution so the painter doesn't upscale a DPR-1 raster.
			const int px = std::max(11, painter.fontMetrics().height() - 2);
			const qreal dpr = painter.device()->devicePixelRatio();
			painter.drawPixmap(
				QRect(tx, (frame->titleBarPx - px) / 2, px, px),
				frame->fileIcon.pixmap(QSize(qRound(px * dpr), qRound(px * dpr))));
			tx += px + 8;
		}
		const std::string changes = frame->git.currentGitChanges;
		const QString changesText = QString::fromStdString(changes);
		// ImGui title-bar layout: path, then the ±N summary right after it
		// (SameLine). Elide the path only when both don't fit.
		const QFontMetrics fm = painter.fontMetrics();
		const int gap = 14;
		const int avail = frame->width() - tx - 10;
		const int changesW =
			changesText.isEmpty() ? 0 : fm.horizontalAdvance(changesText);
		const QString shownPath =
			fm.elidedText(QString::fromStdString(frame->state.path),
						  Qt::ElideMiddle,
						  std::max(40, avail - (changesW ? changesW + gap : 0)));
		const int pathW = fm.horizontalAdvance(shownPath);
		painter.drawText(QRect(tx, 0, pathW, frame->titleBarPx),
						 Qt::AlignVCenter | Qt::AlignLeft,
						 shownPath);
		if (changesW > 0)
			painter.drawText(QRect(tx + pathW + gap, 0, changesW, frame->titleBarPx),
							 Qt::AlignVCenter | Qt::AlignLeft,
							 changesText);
	}
}
