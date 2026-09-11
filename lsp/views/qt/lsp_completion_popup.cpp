#include "lsp_completion_popup.h"

#include <QFontMetrics>
#include <QGuiApplication>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QScreen>
#include <QWheelEvent>

#include <algorithm>

namespace {

constexpr int kCardRadius = 9;

QRect screenAt(const QPoint &globalPos)
{
	const QScreen *screen = QGuiApplication::screenAt(globalPos);
	return screen ? screen->availableGeometry()
				  : QGuiApplication::primaryScreen()->availableGeometry();
}

// Kind badge: one Sublime-style letter, color-coded through the palette.
// Kind numbers are the LSP CompletionItemKind enum values.
enum class KindColor { Function, Type, Variable, Keyword, Number, Special, Op };

struct KindBadge
{
	const char *letter = "";
	KindColor color = KindColor::Keyword;
};

KindBadge kindBadge(int kind)
{
	switch (kind)
	{
	case 2:
		return {"F", KindColor::Function}; // Method
	case 3:
		return {"F", KindColor::Function}; // Function
	case 4:
		return {"F", KindColor::Function}; // Constructor
	case 5:
		return {"V", KindColor::Variable}; // Field
	case 6:
		return {"V", KindColor::Variable}; // Variable
	case 7:
		return {"C", KindColor::Type}; // Class
	case 8:
		return {"C", KindColor::Type}; // Interface
	case 9:
		return {"M", KindColor::Keyword}; // Module
	case 10:
		return {"V", KindColor::Variable}; // Property
	case 11:
		return {"N", KindColor::Number}; // Unit
	case 12:
		return {"N", KindColor::Number}; // Value
	case 13:
		return {"C", KindColor::Type}; // Enum
	case 14:
		return {"K", KindColor::Keyword}; // Keyword
	case 15:
		return {"S", KindColor::Special}; // Snippet
	case 16:
		return {"N", KindColor::Number}; // Color
	case 17:
		return {"D", KindColor::Special}; // File
	case 19:
		return {"D", KindColor::Special}; // Folder
	case 20:
		return {"C", KindColor::Type}; // EnumMember
	case 21:
		return {"N", KindColor::Number}; // Constant
	case 22:
		return {"C", KindColor::Type}; // Struct
	case 23:
		return {"E", KindColor::Special}; // Event
	case 24:
		return {"O", KindColor::Op}; // Operator
	case 25:
		return {"C", KindColor::Type}; // TypeParameter
	default:
		return {"", KindColor::Keyword}; // Text/unknown: no badge
	}
}

} // namespace

LSPCompletionPopup::LSPCompletionPopup(QWidget *parent)
	: QWidget(parent,
			  // Focusless tooltip-level window: the editor keeps keyboard
			  // focus while typing; only mouse presses/wheel reach us.
			  Qt::ToolTip | Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint)
{
	setAttribute(Qt::WA_TranslucentBackground, true);
	setAttribute(Qt::WA_ShowWithoutActivating, true);
	setFocusPolicy(Qt::NoFocus);

	// Like the hover tip: never float over another application after a
	// Cmd-Tab.
	connect(qGuiApp,
			&QGuiApplication::applicationStateChanged,
			this,
			[this](Qt::ApplicationState state) {
				if (state != Qt::ApplicationActive)
					hide();
			});
}

void LSPCompletionPopup::present(const std::vector<LSPCompletionEntry> &newEntries,
								 int newSelected,
								 const QFont &font,
								 const QColor &card,
								 const QColor &ink,
								 const QColor &accent,
								 const LSPCompletionKindColors &colors,
								 const QPoint &globalAnchor)
{
	const bool sameEntries =
		newEntries.size() == entries.size() &&
		std::equal(newEntries.begin(),
				   newEntries.end(),
				   entries.begin(),
				   [](const LSPCompletionEntry &a, const LSPCompletionEntry &b) {
					   return a.label == b.label && a.detail == b.detail &&
							  a.deprecated == b.deprecated && a.kind == b.kind;
				   });
	// Badge colors update unconditionally (paint-time state; a theme change
	// may move any single slot) — only the measured LAYOUT is cached.
	kindColors = colors;
	if (!sameEntries || rowFont != font || cardColor != card || inkColor != ink ||
		accentColor != accent)
	{
		entries = newEntries;
		rowFont = font;
		cardColor = card;
		inkColor = ink;
		accentColor = accent;

		const QFontMetrics fm(rowFont);
		rowHeight = fm.height() + 5;

		// Column layout (VSCode-style): [pad][kind tag][gap][label][gap]
		// [detail elided to fit]. Labels display their SHORT name —
		// scope-qualified server labels ("ns::Class::method") would both
		// bloat the window and starve the detail column.
		const bool anyDetail =
			std::any_of(entries.begin(), entries.end(), [](const LSPCompletionEntry &e) {
				return !e.detail.empty();
			});

		tagWidth = 0;
		labelWidth = 0;
		for (const LSPCompletionEntry &e : entries)
		{
			const KindBadge badge = kindBadge(e.kind);
			tagWidth =
				std::max(tagWidth,
						 badge.letter[0] == '\0'
							 ? 0
							 : fm.horizontalAdvance(QChar::fromLatin1(badge.letter[0])));
			labelWidth = std::max(
				labelWidth,
				fm.horizontalAdvance(QString::fromUtf8(lspCompletionShortLabel(e.label))));
		}
		labelX = kPadX + (tagWidth > 0 ? tagWidth + kColGap : 0);

		// Width budget: the label gets at most 60% of the space (elided
		// beyond that) so the detail column always keeps a real share —
		// the VSCode/Sublime look keeps the type column visible.
		const int avail = kMaxWidth - labelX - kPadX;
		labelWidth = std::min(labelWidth, anyDetail ? (avail * 3) / 5 : avail);
		labelWidth = std::max(labelWidth, 0);

		detailWidth = 0;
		if (anyDetail)
		{
			int rawDetail = 0;
			for (const LSPCompletionEntry &e : entries)
			{
				if (e.detail.empty())
					continue;
				rawDetail = std::max(
					rawDetail, fm.horizontalAdvance(QString::fromStdString(e.detail)));
			}
			detailWidth = std::min(rawDetail, avail - labelWidth - kColGap);
			detailWidth = std::max(detailWidth, 0);
		}
		contentWidth =
			labelX + labelWidth + (detailWidth > 0 ? kColGap + detailWidth : 0) + kPadX;
		contentWidth = std::clamp(contentWidth, kMinWidth, kMaxWidth);
		// Derive the detail column from the FINAL window width (the clamp
		// above can widen it past the measured content): the column is
		// anchored to the right edge, so every detail lines up there —
		// sizing it pre-clamp left it floating mid-row on narrow lists.
		if (detailWidth > 0)
			detailWidth =
				std::max(contentWidth - labelX - labelWidth - kColGap - kPadX, 0);
	}

	selected = std::clamp(newSelected, 0, std::max(rowCount() - 1, 0));
	firstVisible = std::clamp(firstVisible, 0, std::max(rowCount() - 1, 0));
	ensureVisible(selected);

	const int rows = std::min(rowCount(), kMaxVisibleRows);
	const int h = rows * rowHeight + 2 * kPadY;
	resize(contentWidth, h);

	QPoint pos = globalAnchor;
	const QRect screen = screenAt(pos);
	if (pos.y() + h > screen.bottom())
		pos.setY(globalAnchor.y() - h); // flip above the caret line
	// min/max (not clamp): on a screen narrower than the popup, clamp's
	// lo > hi would be undefined behavior.
	pos.setX(std::min(std::max(pos.x(), screen.left()),
					  std::max(screen.left(), screen.right() - width())));
	pos.setY(std::min(std::max(pos.y(), screen.top()),
					  std::max(screen.top(), screen.bottom() - height())));
	move(pos);

	if (!isVisible())
		show();
	update();
}

void LSPCompletionPopup::moveSelection(int delta)
{
	if (rowCount() == 0)
		return;
	selected = (selected + delta + rowCount()) % rowCount();
	ensureVisible(selected);
	update();
}

void LSPCompletionPopup::pageSelection(int deltaPages)
{
	moveSelection(deltaPages * kMaxVisibleRows);
}

void LSPCompletionPopup::ensureVisible(int row)
{
	if (row < firstVisible)
		firstVisible = row;
	else if (row >= firstVisible + kMaxVisibleRows)
		firstVisible = row - kMaxVisibleRows + 1;
	firstVisible = std::clamp(firstVisible, 0, std::max(rowCount() - kMaxVisibleRows, 0));
}

int LSPCompletionPopup::rowAt(const QPoint &pos) const
{
	const int row = firstVisible + (pos.y() - kPadY) / rowHeight;
	if (pos.y() < kPadY || row < firstVisible || row >= rowCount())
		return -1;
	return row;
}

void LSPCompletionPopup::paintEvent(QPaintEvent *)
{
	QPainter painter(this);
	painter.setRenderHint(QPainter::Antialiasing, true);

	// Card chrome like the hover tip (rounded translucent window).
	QPainterPath card;
	card.addRoundedRect(
		QRectF(0.5, 0.5, width() - 1.0, height() - 1.0), kCardRadius, kCardRadius);
	painter.fillPath(card, cardColor);
	painter.setPen(QPen(inkColor, 1, Qt::SolidLine));
	painter.drawPath(card);

	painter.setFont(rowFont);
	painter.setClipRect(card.toFillPolygon().boundingRect().toRect());
	const QFontMetrics fm(rowFont);
	const int rows = std::min(rowCount(), kMaxVisibleRows);
	for (int i = 0; i < rows; ++i)
	{
		const int row = firstVisible + i;
		const int y = kPadY + i * rowHeight;

		if (row == selected)
		{
			painter.fillRect(
				2,
				y,
				width() - 4,
				rowHeight,
				QColor(accentColor.red(), accentColor.green(), accentColor.blue(), 46));
			painter.fillRect(2, y, 3, rowHeight, accentColor);
		}

		const LSPCompletionEntry &e = entries[static_cast<size_t>(row)];

		// Kind badge: one Sublime-style letter in the kind's theme color.
		const KindBadge badge = kindBadge(e.kind);
		if (badge.letter[0] != '\0' && tagWidth > 0)
		{
			QColor badgeColor;
			switch (badge.color)
			{
			case KindColor::Function:
				badgeColor = kindColors.function;
				break;
			case KindColor::Type:
				badgeColor = kindColors.type;
				break;
			case KindColor::Variable:
				badgeColor = kindColors.variable;
				break;
			case KindColor::Keyword:
				badgeColor = kindColors.keyword;
				break;
			case KindColor::Number:
				badgeColor = kindColors.number;
				break;
			case KindColor::Special:
				badgeColor = kindColors.special;
				break;
			case KindColor::Op:
				badgeColor = kindColors.op;
				break;
			}
			if (!badgeColor.isValid())
				badgeColor = QColor(inkColor.red(),
									inkColor.green(),
									inkColor.blue(),
									150); // theme slot missing: dim ink
			painter.setPen(badgeColor);
			painter.drawText(QRect(kPadX, y, tagWidth, rowHeight),
							 Qt::AlignVCenter | Qt::AlignHCenter,
							 QChar::fromLatin1(badge.letter[0]));
		}

		QFont labelFont = rowFont;
		labelFont.setStrikeOut(e.deprecated);
		painter.setFont(labelFont);
		painter.setPen(
			e.deprecated ? QColor(inkColor.red(), inkColor.green(), inkColor.blue(), 120)
						 : inkColor);
		painter.drawText(QRect(labelX, y, labelWidth, rowHeight),
						 Qt::AlignVCenter | Qt::AlignLeft,
						 fm.elidedText(QString::fromUtf8(lspCompletionShortLabel(e.label)),
									   Qt::ElideRight,
									   labelWidth));

		// Detail column, anchored to the RIGHT edge (like VSCode's type
		// column) — every detail ends at the same x regardless of label
		// length; only rows that have one, in the column present() sized.
		if (!e.detail.empty() && detailWidth > 0)
		{
			painter.setFont(rowFont);
			painter.setPen(QColor(inkColor.red(), inkColor.green(), inkColor.blue(), 120));
			painter.drawText(
				QRect(width() - kPadX - detailWidth, y, detailWidth, rowHeight),
				Qt::AlignVCenter | Qt::AlignRight,
				// Elide LEFT like VSCode: the tail of a signature (return
				// type, closing parens) is what identifies it.
				fm.elidedText(
					QString::fromStdString(e.detail), Qt::ElideLeft, detailWidth));
		}
		painter.setFont(rowFont);
	}
}

void LSPCompletionPopup::mousePressEvent(QMouseEvent *event)
{
	const int row = rowAt(event->position().toPoint());
	if (row < 0)
	{
		hide();
		return;
	}
	selected = row;
	update();
	Q_EMIT itemAccepted(row);
}

void LSPCompletionPopup::wheelEvent(QWheelEvent *event)
{
	const int steps = event->angleDelta().y() > 0 ? -1 : 1;
	moveSelection(steps);
	event->accept();
}

void LSPCompletionPopup::hideEvent(QHideEvent *) { Q_EMIT dismissed(); }
