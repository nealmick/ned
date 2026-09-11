/*
	File: host/qt/activity_bar.cpp
	Description: see activity_bar.h. Icon painting mirrors the status bar's
	branch icon: QSvgRenderer straight to a device-pixel QImage, tinted
	with SourceIn. States: muted ink / hover fill / active with VSCode's
	left accent bar.
*/

#include "activity_bar.h"

#include "theme.h"

#include <QMouseEvent>
#include <QPainter>
#include <QSvgRenderer>
#include <QToolTip>
#include <QVBoxLayout>

namespace {

constexpr int kBarWidth = 46;	// VSCode-ish activity strip
constexpr int kButtonSize = 46; // square touch targets
constexpr int kIconPx = 24;

// Render + tint one codicon at the exact device size (see status_bar's
// branch icon for why not QIcon/QPixmap round-trips).
QImage renderCodicon(const QString &key, int logical, qreal dpr, const QColor &tint)
{
	static const QString base =
		QString::fromStdString(Settings::getAppResourcesPath()) + "/resources/icons/";
	QSvgRenderer renderer(base + key + ".svg");
	if (!renderer.isValid() || logical <= 0)
		return {};
	QImage img(qRound(logical * dpr), qRound(logical * dpr), QImage::Format_ARGB32);
	img.fill(Qt::transparent);
	{
		QPainter p(&img);
		p.setRenderHint(QPainter::Antialiasing);
		renderer.render(&p);
	}
	QPainter tp(&img);
	tp.setCompositionMode(QPainter::CompositionMode_SourceIn);
	tp.fillRect(img.rect(), tint);
	tp.end();
	img.setDevicePixelRatio(dpr);
	return img;
}

QColor withAlpha(const QColor &c, int a)
{
	QColor r = c;
	r.setAlpha(a);
	return r;
}

} // namespace

// One square icon button. Hover/active/press states are painted (no QSS —
// the translucent macOS window never renders stylesheet backgrounds on
// plain widgets anyway).
class NedActivityBar::Button : public QWidget
{
  public:
	Button(const Settings &settings,
		   const QString &iconKey,
		   const QString &tip,
		   int panel,
		   NedActivityBar *owner)
		: QWidget(owner),
		  m_settings(&settings),
		  m_iconKey(iconKey),
		  m_tip(tip),
		  m_panel(panel),
		  m_owner(owner)
	{
		setFixedSize(kButtonSize, kButtonSize);
		setMouseTracking(true);
		setToolTip(tip);
	}

	int panel() const { return m_panel; }

	void setActive(bool on)
	{
		if (on == m_active)
			return;
		m_active = on;
		update();
	}

  protected:
	void paintEvent(QPaintEvent *) override
	{
		QPainter p(this);
		p.setRenderHint(QPainter::Antialiasing);

		// State colors: muted normally, full ink when active, hover lift
		// between (the progression VSCode uses).
		const QColor ink = NedQtTheme::text(*m_settings);
		const QColor color = m_active  ? ink
							 : m_hover ? withAlpha(ink, 220)
									   : withAlpha(ink, 150);

		if (m_hover && !m_active)
			p.fillRect(rect(), withAlpha(ink, 26));
		if (m_active)
			p.fillRect(0, 0, 2, height(), ink); // VSCode's left accent bar

		const QImage icon = renderCodicon(m_iconKey, kIconPx, devicePixelRatioF(), color);
		if (!icon.isNull())
			p.drawImage(QPointF((width() - kIconPx) / 2.0, (height() - kIconPx) / 2.0),
						icon);
	}

	void mouseMoveEvent(QMouseEvent *event) override
	{
		QWidget::mouseMoveEvent(event);
		const bool inside = rect().contains(event->position().toPoint());
		if (inside != m_hover)
		{
			m_hover = inside;
			update();
			if (m_hover)
				QToolTip::showText(QCursor::pos(), m_tip, this);
		}
	}

	void leaveEvent(QEvent *event) override
	{
		QWidget::leaveEvent(event);
		m_hover = false;
		update();
	}

	void mousePressEvent(QMouseEvent *event) override
	{
		if (event->button() == Qt::LeftButton)
			Q_EMIT m_owner->panelRequested(m_panel);
	}

  private:
	const Settings *m_settings;
	QString m_iconKey;
	QString m_tip;
	int m_panel = 0;
	bool m_hover = false;
	bool m_active = false;
	NedActivityBar *m_owner;
};

NedActivityBar::NedActivityBar(const Settings &settings, QWidget *parent)
	: QWidget(parent), m_settings(&settings)
{
	setObjectName(QStringLiteral("NedActivityBar"));
	setFixedWidth(kBarWidth);
	setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);

	auto *column = new QVBoxLayout(this);
	column->setContentsMargins(0, 2, 0, 0);
	column->setSpacing(0);

	m_buttons << makeButton("files", QObject::tr("Explorer"), PanelFiles)
			  << makeButton("git-branch", QObject::tr("Source Control"), PanelGit);
	m_buttons.front()->setActive(true);
	// Absorb the rest of the height — without this the VBox spreads the
	// buttons down the whole strip (git icon lands mid-screen).
	column->addStretch(1);
}

NedActivityBar::Button *
NedActivityBar::makeButton(const QString &iconKey, const QString &tip, int panel)
{
	auto *b = new Button(*m_settings, iconKey, tip, panel, this);
	static_cast<QVBoxLayout *>(layout())->addWidget(b, 0, Qt::AlignTop);
	return b;
}

void NedActivityBar::setActivePanel(int panel)
{
	if (panel == m_active)
		return;
	m_active = panel;
	for (Button *b : m_buttons)
		b->setActive(b->panel() == panel);
}

void NedActivityBar::paintEvent(QPaintEvent *event)
{
	QWidget::paintEvent(event);
	QPainter p(this);
	// Right-edge hairline — same separator the file tree uses.
	p.setPen(QPen(QColor(255, 255, 255, 23), 1.0));
	p.drawLine(QPointF(width() - 0.5, 0), QPointF(width() - 0.5, height()));
}
