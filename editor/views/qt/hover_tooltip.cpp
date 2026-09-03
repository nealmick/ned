#include "hover_tooltip.h"

#include <QGraphicsDropShadowEffect>
#include <QGuiApplication>
#include <QScreen>
#include <QVBoxLayout>

#include <algorithm>

namespace {

// Card chrome. The window is larger than the card so the drop shadow has
// room to fade; placeAt() targets the card and offsets by this margin.
constexpr int kShadowMargin = 14;
constexpr int kCardRadius = 9;
constexpr int kCardPad = 9; // label padding inside the card

QRect screenAt(const QPoint &globalPos)
{
	const QScreen *screen = QGuiApplication::screenAt(globalPos);
	return screen ? screen->availableGeometry()
				  : QGuiApplication::primaryScreen()->availableGeometry();
}

} // namespace

HoverTooltip::HoverTooltip(QWidget *parent)
	: QWidget(parent,
			  // Input-transparent: the window rect includes the invisible
			  // shadow margin, which would otherwise swallow clicks meant
			  // for the text under the tip (click must hide the tip AND
			  // land on the editor below in one press).
			  // NoDropShadowWindowHint: the card paints its own soft shadow;
			  // macOS's own rect-shaped window shadow would read as a hard
			  // black 1px outline around the translucent tip on light
			  // backgrounds.
			  Qt::ToolTip | Qt::FramelessWindowHint | Qt::WindowTransparentForInput |
				  Qt::NoDropShadowWindowHint)
{
	setObjectName("nedHoverTip");
	setAttribute(Qt::WA_TranslucentBackground); // real rounded corners
	setAttribute(Qt::WA_ShowWithoutActivating, true);

	// A ToolTip window lives at a macOS status level: above normal windows
	// of every application. It must never outlive the app switch itself —
	// otherwise it keeps floating over whatever the user Cmd+Tabs to.
	connect(qGuiApp,
			&QGuiApplication::applicationStateChanged,
			this,
			[this](Qt::ApplicationState state) {
				if (state != Qt::ApplicationActive)
					hide();
			});

	auto *layout = new QVBoxLayout(this);
	layout->setContentsMargins(kShadowMargin, kShadowMargin, kShadowMargin, kShadowMargin);

	card = new QWidget(this);
	card->setAttribute(Qt::WA_StyledBackground, true);

	// Soft elevation under the card (macOS popovers have one).
	auto *shadow = new QGraphicsDropShadowEffect(card);
	shadow->setBlurRadius(22);
	shadow->setOffset(0, 4);
	shadow->setColor(QColor(0, 0, 0, 130));
	card->setGraphicsEffect(shadow);

	label = new QLabel(card);
	label->setTextFormat(Qt::RichText);
	label->setWordWrap(true);
	label->setMargin(kCardPad);
	auto *cardLayout = new QVBoxLayout(card);
	cardLayout->setContentsMargins(0, 0, 0, 0);
	cardLayout->addWidget(label);

	layout->addWidget(card);
}

void HoverTooltip::present(const QString &html,
						   const QFont &font,
						   const QColor &bg,
						   const QColor &ink,
						   const QPoint &globalAnchor)
{
	label->setFont(font);
	// Card chrome scoped to the card: a selector-less declaration block
	// would also apply to the label child, drawing a second hairline box
	// inset inside the card. The ink rides a scoped QLabel rule instead
	// (it used to reach the label via propagation).
	// Card chrome scoped to the card (the shared popover pattern, see
	// NedQtTheme::popoverCardSheet); this card takes EXPLICIT bg/ink from
	// the caller (diagnostic vs symbol tooltips differ), so the sheet is
	// local. The ink rides a scoped QLabel rule — a selector-less color
	// would leak into the card chrome.
	card->setObjectName("nedPopoverCard");
	card->setStyleSheet(QString("#nedPopoverCard { background-color: %1; "
								"border: 1px solid rgba(128, 128, 128, 110); "
								"border-radius: %3; }"
								"QLabel { color: %2; background: transparent; }")
							.arg(bg.name())
							.arg(ink.name())
							.arg(kCardRadius));
	label->setText(html);

	// Size to content; wrap only when the natural width would overflow the
	// screen (ImGui tooltips grow and never wrap code).
	label->setMaximumWidth(QWIDGETSIZE_MAX);
	label->setWordWrap(false);
	const QSize natural = label->sizeHint();
	const QRect avail = screenAt(globalAnchor);
	const int maxW = avail.width() * 3 / 4;
	int w = natural.width() + 2 * kCardPad + 2; // +2: hairline border
	int h = natural.height() + 2 * kCardPad + 2;
	if (w > maxW)
	{
		label->setWordWrap(true);
		const int textW = maxW - 2 * kCardPad - 2;
		label->setMaximumWidth(textW);
		const int wrapped = label->heightForWidth(textW);
		w = maxW;
		h = (wrapped > 0 ? wrapped : natural.height()) + 2 * kCardPad + 2;
	}
	card->setFixedSize(w, h);
	resize(w + 2 * kShadowMargin, h + 2 * kShadowMargin);

	placeAt(globalAnchor);
	show();
	raise();
}

void HoverTooltip::replaceAt(const QPoint &globalAnchor)
{
	if (isVisible())
		placeAt(globalAnchor);
}

void HoverTooltip::placeAt(const QPoint &globalAnchor)
{
	// Card below-right of the anchor, flipped above/left when it would
	// overflow the screen, clamped on-screen; the window rides along with
	// the shadow margin.
	const QRect avail = screenAt(globalAnchor);
	QRect cardRect(globalAnchor + QPoint(6, 6), card->size());
	if (cardRect.right() > avail.right() - 8)
		cardRect.moveLeft(globalAnchor.x() - cardRect.width() - 6);
	if (cardRect.bottom() > avail.bottom() - 8)
		cardRect.moveTop(globalAnchor.y() - cardRect.height() - 4);
	cardRect.moveLeft(
		std::clamp(cardRect.left(),
				   avail.left() + 8,
				   std::max(avail.left() + 8, avail.right() - 8 - cardRect.width())));
	cardRect.moveTop(
		std::clamp(cardRect.top(),
				   avail.top() + 8,
				   std::max(avail.top() + 8, avail.bottom() - 8 - cardRect.height())));
	move(cardRect.topLeft() - QPoint(kShadowMargin, kShadowMargin));
}
