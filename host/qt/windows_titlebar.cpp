/*
	File: host/qt/windows_titlebar.cpp
	Description: 1:1 port of the ImGui host's Workbench::renderWindowsTitlebar
	(host/imgui/workbench.cpp) to a QWidget — same layout, metrics, glyphs,
	hover fills, and click routing. See windows_titlebar.h.
*/

#ifdef _WIN32

#include "windows_titlebar.h"

#include "qt_icons.h"
#include "theme.h"

#include <QMouseEvent>
#include <QPainter>
#include <QToolTip>

// windows_chrome.cpp (native frameless layer).
extern void nedQtSetCaptionHeight(float physicalPx);
extern void nedQtClearCaptionExcludes();
extern void nedQtExcludeCaptionRect(float x0, float y0, float x1, float y1, int ht);
extern int nedQtCaptionHover();
extern bool nedQtWindowIsMaximized(void *hwnd);
extern void nedQtSetChromeNotify(void (*fn)(void *), void *ctx);

#ifndef HTCLIENT
#define HTCLIENT 1
#endif
#ifndef HTMINBUTTON
#define HTMINBUTTON 8
#endif
#ifndef HTMAXBUTTON
#define HTMAXBUTTON 9
#endif
#ifndef HTCLOSE
#define HTCLOSE 20
#endif

// ImGui GetFontSize() parity: the em size in logical px. QFontMetrics::
// height() is the whole line box (ascent + descent + internal leading,
// ~1.35x the em at typical DPIs) — deriving the bar from it inflated
// EVERY metric (height, hover rects, glyph strokes, the gear) at once,
// which is why all the buttons looked oversized together.
static float emFontSize(const QWidget *w)
{
	const QFont f = w->font();
	const qreal px =
		f.pixelSize() > 0 ? qreal(f.pixelSize()) : f.pointSizeF() * 96.0 / 72.0;
	return float(std::max(px, 9.0));
}

NedQtTitleBar::NedQtTitleBar(const Settings &settings, QWidget *parent)
	: QWidget(parent), m_settings(&settings)
{
	setMouseTracking(true);
	syncMetrics();
	nedQtSetChromeNotify(&NedQtTitleBar::chromeNotifyStatic, this);
}

NedQtTitleBar::~NedQtTitleBar() { nedQtSetChromeNotify(nullptr, nullptr); }

void NedQtTitleBar::chromeNotifyStatic(void *ctx)
{
	if (auto *bar = static_cast<NedQtTitleBar *>(ctx))
		bar->refreshChrome();
}

void NedQtTitleBar::refreshChrome()
{
	// Hover moved between native caption buttons, or maximize/restore
	// flipped (both change glyphs/fills).
	update();
	syncMetrics();
}

// Same height rule as renderWindowsTitlebar: max(fs * 1.7, 28).
void NedQtTitleBar::syncMetrics()
{
	const float fs = emFontSize(this);
	const int h = qRound(std::max(fs * 1.7f, 28.0f));
	if (h != minimumHeight() || h != maximumHeight())
		setFixedHeight(h);
}

QList<NedQtTitleBar::Btn> NedQtTitleBar::layoutButtons() const
{
	const float fs = emFontSize(this);
	const float h = height();
	// ImGui workbench.cpp parity: caption cluster wider (fs * 2.15) than
	// the action buttons (fs * 1.7) — the previous "match them up" shrink
	// was compensating for the wrong fs (line box vs em), not real widths.
	const float btnW = std::max(fs * 2.15f, 36.0f);
	const float actW = std::max(fs * 1.7f, 28.0f);

	QList<Btn> btns;
	const float titleX = fs * 0.7f;
	const float titleW =
		fontMetrics().horizontalAdvance(QStringLiteral("Ned Text Editor"));

	float ax = titleX + titleW + fs * 0.85f;
	auto action = [&](int role) {
		// HTCLIENT, NOT 0: the native hit-test treats 0 as "no exclusion",
		// which leaves the rect as HTCAPTION (drag) — Qt then never sees
		// hover/clicks on the button (the bug this fixed). A real HTCLIENT
		// exclusion wins over the caption fallback, exactly like the ImGui
		// bar's WindowsCaptionHit::Client.
		btns.push_back(Btn{QRectF(ax, 0, actW, h), role, HTCLIENT});
		ax += actW;
	};
	action(RoleSidebar);
#if NED_QT_TERMINAL
	action(RoleTerminal);
#endif
	// Gear sits immediately left of the caption cluster, same as ImGui.
	ax = width() - btnW * 3.0f - actW;
	action(RoleGear);

	float cx = width() - btnW * 3.0f;
	auto caption = [&](int role, int ht) {
		btns.push_back(Btn{QRectF(cx, 0, btnW, h), role, ht});
		cx += btnW;
	};
	caption(RoleMin, HTMINBUTTON);
	caption(RoleMax, HTMAXBUTTON);
	caption(RoleClose, HTCLOSE);
	return btns;
}

void NedQtTitleBar::paintEvent(QPaintEvent *)
{
	syncMetrics();

	const float fs = emFontSize(this);
	const float h = height();
	const QList<Btn> btns = layoutButtons();

	// Register the interactive rects with the native hit-test layer
	// (physical px — WM_NCHITTEST works in device coordinates).
	{
		const qreal dpr = devicePixelRatioF();
		const QPointF origin = mapTo(window(), QPointF(0, 0));
		nedQtSetCaptionHeight(h * dpr);
		nedQtClearCaptionExcludes();
		for (const Btn &b : btns)
			nedQtExcludeCaptionRect((origin.x() + b.rect.left()) * dpr,
									(origin.y() + b.rect.top()) * dpr,
									(origin.x() + b.rect.right()) * dpr,
									(origin.y() + b.rect.bottom()) * dpr,
									b.ht);
	}

	QPainter p(this);
	p.setRenderHint(QPainter::Antialiasing);

	const QColor ink = NedQtTheme::text(*m_settings);
	const float stroke = std::max(1.0f, fs * 0.07f);

	// Title left (same x inset as the ImGui bar).
	{
		p.setPen(ink);
		p.setFont(font());
		const QRectF tr(0, 0, width(), h);
		p.drawText(tr.adjusted(fs * 0.7f, 0, 0, 0),
				   Qt::AlignVCenter | Qt::AlignLeft,
				   QStringLiteral("Ned Text Editor"));
	}

	// Action glyph geometry (drawSidebar / drawBottomSplit parity).
	const float glyphPad = fs * 0.52f;
	auto glyphBox = [&](const QRectF &r) {
		const float side = std::min(r.width(), r.height()) - glyphPad * 2.0f;
		const QPointF c = r.center();
		return QRectF(c.x() - side * 0.5f, c.y() - side * 0.5f, side, side);
	};
	const float round = std::max(1.0f, fs * 0.06f);
	auto drawSidebar = [&](const QRectF &r) {
		const QRectF g = glyphBox(r);
		const float split = g.left() + g.width() * 0.38f;
		p.setPen(QPen(ink, stroke));
		p.setBrush(Qt::NoBrush);
		p.drawRoundedRect(g, round, round);
		p.drawLine(QPointF(split, g.top() + 0.5f), QPointF(split, g.bottom() - 0.5f));
	};
	auto drawBottomSplit = [&](const QRectF &r) {
		const QRectF g = glyphBox(r);
		const float split = g.top() + g.height() * 0.58f;
		p.setPen(QPen(ink, stroke));
		p.setBrush(Qt::NoBrush);
		p.drawRoundedRect(g, round, round);
		p.drawLine(QPointF(g.left() + 0.5f, split), QPointF(g.right() - 0.5f, split));
	};

	// Action buttons (Qt-handled): hover fill + glyph. The gear swaps to
	// gear-hover.svg like the ImGui bar.
	for (const Btn &b : btns)
	{
		if (b.ht != HTCLIENT)
			continue; // caption cluster below
		const bool hov = m_hoverAction == b.role;
		if (hov)
		{
			p.setPen(Qt::NoPen);
			p.setBrush(QColor(255, 255, 255, 28));
			p.drawRect(b.rect);
		}
		if (b.role == RoleGear)
		{
			const float pad = fs * 0.38f;
			const int px = qRound(b.rect.width() - pad * 2.0f);
			const QString key =
				hov ? QStringLiteral("gear-hover") : QStringLiteral("gear");
			const QIcon gear = QtIconSet::byKey(key, px);
			if (!gear.isNull())
				gear.paint(&p, b.rect.adjusted(pad, pad, -pad, -pad).toRect());
		} else if (b.role == RoleSidebar)
			drawSidebar(b.rect);
		else if (b.role == RoleTerminal)
			drawBottomSplit(b.rect);
	}

	// Caption cluster (native-handled): hover tracked through the OS state
	// exactly like windowsCaptionHover().
	const bool maximized = nedQtWindowIsMaximized((void *)window()->winId());
	for (const Btn &b : btns)
	{
		if (b.ht == HTCLIENT)
			continue;
		const bool hov = nedQtCaptionHover() == b.ht;
		if (hov)
		{
			p.setPen(Qt::NoPen);
			p.setBrush(b.role == RoleClose ? QColor(232, 17, 35)
										   : QColor(255, 255, 255, 28));
			p.drawRect(b.rect);
		}
		const QColor col = (hov && b.role == RoleClose) ? QColor(255, 255, 255) : ink;
		p.setPen(QPen(col, stroke));
		p.setBrush(Qt::NoBrush);
		const QPointF c = b.rect.center();
		if (b.role == RoleMin)
		{
			const float w = fs * 0.45f;
			p.drawLine(QPointF(c.x() - w, c.y()), QPointF(c.x() + w, c.y()));
		} else if (b.role == RoleMax)
		{
			const float s = fs * 0.42f;
			if (maximized)
			{
				// Restore pair — same offsets as the ImGui glyph.
				p.drawRect(QRectF(
					c.x() - s + 2.0f, c.y() - s, 2.0f * s - 2.0f, 2.0f * s - 2.0f));
				p.drawRect(QRectF(
					c.x() - s, c.y() - s + 3.0f, 2.0f * s - 2.0f, 2.0f * s - 3.0f));
			} else
				p.drawRect(QRectF(c.x() - s, c.y() - s, 2.0f * s, 2.0f * s));
		} else // RoleClose
		{
			const float s = fs * 0.38f;
			p.drawLine(QPointF(c.x() - s, c.y() - s), QPointF(c.x() + s, c.y() + s));
			p.drawLine(QPointF(c.x() + s, c.y() - s), QPointF(c.x() - s, c.y() + s));
		}
	}

	// Hairline under the bar (same as the ImGui ImGuiCol_Border strip).
	p.setPen(QPen(NedQtTheme::raised(NedQtTheme::background(*m_settings)), 1.0));
	p.drawLine(QPointF(0, h - 1.0), QPointF(width(), h - 1.0));
}

void NedQtTitleBar::mouseMoveEvent(QMouseEvent *event)
{
	// Hover + tooltips for the Qt-handled action buttons only — the caption
	// cluster is non-client (the wndproc owns it).
	const QList<Btn> btns = layoutButtons();
	int hit = -1;
	for (const Btn &b : btns)
		if (b.ht == HTCLIENT && b.rect.contains(event->position()))
		{
			hit = b.role;
			break;
		}
	if (hit == m_hoverAction)
		return;
	m_hoverAction = hit;
	update();
	if (hit == RoleSidebar)
		QToolTip::showText(QCursor::pos(), QStringLiteral("Toggle Explorer"), this);
	else if (hit == RoleTerminal)
		QToolTip::showText(QCursor::pos(), QStringLiteral("Toggle Terminal"), this);
	else if (hit == RoleGear)
		QToolTip::showText(QCursor::pos(), QStringLiteral("Settings"), this);
	else
		QToolTip::hideText();
}

void NedQtTitleBar::mousePressEvent(QMouseEvent *event)
{
	if (event->button() != Qt::LeftButton)
		return;
	const QList<Btn> btns = layoutButtons();
	for (const Btn &b : btns)
	{
		if (b.ht != HTCLIENT)
			continue;
		if (!b.rect.contains(event->position()))
			continue;
		if (b.role == RoleSidebar)
			Q_EMIT sidebarToggleRequested();
		else if (b.role == RoleTerminal)
			Q_EMIT terminalToggleRequested();
		else if (b.role == RoleGear)
			Q_EMIT settingsRequested();
		return;
	}
}

void NedQtTitleBar::leaveEvent(QEvent *event)
{
	QWidget::leaveEvent(event);
	if (m_hoverAction != -1)
	{
		m_hoverAction = -1;
		update();
	}
}

#endif // _WIN32
