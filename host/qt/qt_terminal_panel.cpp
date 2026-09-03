/*
	File: host/qt/qt_terminal_panel.cpp
	Description: QtTerminalPanel implementation — multi-session QTermWidget
	tab group. Lives in the qtermwidget6 target (not ned_qt) because
	QTermWidget's headers use Qt keyword macros that ned_qt disables.
*/

#include "qt_terminal_panel.h"

#include "qt_tab_close.h"
#include "qtermwidget.h"

#include <QApplication>
#include <QDir>
#include <QFile>
#include <QFontDatabase>
#include <QHBoxLayout>
#include <QLabel>
#include <QStackedWidget>
#include <QStandardPaths>
#include <QTabBar>
#include <QTabWidget>
#include <QTextStream>
#include <QToolButton>

#include <algorithm>
#include <array>
#include <vector>

struct QtTerminalPanel::Impl
{
	struct Session
	{
		QTermWidget *term = nullptr;
		int id = 0; // stable "Terminal N" numbering across closes
		bool ended = false;
	};

	QtTerminalPanel *q = nullptr;
	QTabWidget *tabs = nullptr;
	QToolButton *plus = nullptr;
	std::vector<Session> sessions;
	int nextId = 1;
	QString projectRoot;
	QFont font = QFontDatabase::systemFont(QFontDatabase::FixedFont);
	QString schemePath; // generated Ned scheme file ("" until themed)
	int schemeRev = 0;	// name-bump: ColorSchemeManager caches by name
	qreal themeOpacity = 1.0;

	QTermWidget *termAt(int index) const
	{
		if (index < 0 || index >= static_cast<int>(sessions.size()))
			return nullptr;
		return sessions[static_cast<size_t>(index)].term;
	}

	Session *sessionFor(const QTermWidget *term)
	{
		for (auto &s : sessions)
			if (s.term == term)
				return &s;
		return nullptr;
	}

	// qtermwidget paints the scheme background with CompositionMode_Source
	// — it REPLACES the pixels behind the viewport (a panel-level fill
	// would be erased). So the theme color must ride IN the scheme, and
	// the opacity matches what the editor area ends up with (window tint
	// at alpha a, then the editor's own fill at alpha a → 1-(1-a)^2).
	void applyTheme(QTermWidget *term)
	{
		// Opaque paint breaks the window's frosted look. Cells only fill
		// when their color differs from palette().window() (synced to the
		// scheme), so the viewport's effective alpha IS _opacity: setting
		// it to the theme alpha makes the terminal read exactly like the
		// window tint everywhere else.
		if (!schemePath.isEmpty())
			term->setColorScheme(schemePath);
		term->setTerminalOpacity(themeOpacity);
	}

	void writeScheme(const QColor &bg)
	{
		const bool dark = bg.lightness() < 127;
		// Breeze-ish ANSI table; foreground follows the background.
		const std::array<QString, 16> ansi =
			dark ? std::array<QString, 16>{QStringLiteral("7,54,66"),
										   QStringLiteral("237,21,21"),
										   QStringLiteral("17,209,22"),
										   QStringLiteral("246,116,0"),
										   QStringLiteral("44,108,190"),
										   QStringLiteral("157,102,208"),
										   QStringLiteral("0,187,208"),
										   QStringLiteral("236,239,241"),
										   // Intense variants
										   QStringLiteral("102,119,136"),
										   QStringLiteral("255,108,96"),
										   QStringLiteral("140,220,120"),
										   QStringLiteral("255,200,87"),
										   QStringLiteral("120,170,255"),
										   QStringLiteral("220,160,255"),
										   QStringLiteral("120,240,255"),
										   QStringLiteral("255,255,255")}
				 : std::array<QString, 16>{QStringLiteral("0,0,0"),
										   QStringLiteral("197,34,29"),
										   QStringLiteral("0,128,0"),
										   QStringLiteral("196,160,0"),
										   QStringLiteral("20,85,204"),
										   QStringLiteral("106,27,154"),
										   QStringLiteral("13,99,140"),
										   QStringLiteral("128,128,128"),
										   // Intense variants
										   QStringLiteral("85,85,85"),
										   QStringLiteral("220,50,47"),
										   QStringLiteral("0,160,0"),
										   QStringLiteral("230,180,0"),
										   QStringLiteral("60,120,255"),
										   QStringLiteral("150,70,200"),
										   QStringLiteral("20,130,180"),
										   QStringLiteral("60,60,60")};
		const QString fg =
			dark ? QStringLiteral("215,218,224") : QStringLiteral("32,34,36");
		const QString fgIntense =
			dark ? QStringLiteral("255,255,255") : QStringLiteral("0,0,0");
		const QString bgStr =
			QStringLiteral("%1,%2,%3").arg(bg.red()).arg(bg.green()).arg(bg.blue());

		const QString dir =
			QStandardPaths::writableLocation(QStandardPaths::CacheLocation) +
			"/color-schemes";
		QDir().mkpath(dir);
		// New name per revision: ColorSchemeManager caches by name, so
		// live theme changes need a fresh entry.
		schemePath = dir + QStringLiteral("/NedTheme%1.colorscheme").arg(++schemeRev);
		QFile file(schemePath);
		if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate))
		{
			schemePath.clear();
			return;
		}
		QTextStream out(&file);
		out << "[General]\nDescription=Ned Theme\n\n";
		const char *const roles[10] = {"Foreground",
									   "Background",
									   "Color0",
									   "Color1",
									   "Color2",
									   "Color3",
									   "Color4",
									   "Color5",
									   "Color6",
									   "Color7"};
		const char *const rolesIntense[10] = {"ForegroundIntense",
											  "BackgroundIntense",
											  "Color0Intense",
											  "Color1Intense",
											  "Color2Intense",
											  "Color3Intense",
											  "Color4Intense",
											  "Color5Intense",
											  "Color6Intense",
											  "Color7Intense"};
		for (int i = 0; i < 10; ++i)
		{
			const QString color = (i == 0)	 ? fg
								  : (i == 1) ? bgStr
											 : ansi[static_cast<size_t>(i - 2)];
			out << '[' << roles[i] << "]\nColor=" << color << "\n\n";
			const QString intense = (i == 0)   ? fgIntense
									: (i == 1) ? bgStr
											   : ansi[static_cast<size_t>(i + 6)];
			out << '[' << rolesIntense[i] << "]\nColor=" << intense << "\n\n";
		}
	}

	void styleTerm(QTermWidget *term)
	{
		// The strips around the viewport (where scrollbars would live)
		// are painted ONCE with the scheme active at first show —
		// qtermwidget never repaints them. So the FIRST scheme must be
		// the theme (sessions are only created after setThemeBackground;
		// BreezeModified is the pre-theme fallback).
		term->setColorScheme(schemePath.isEmpty() ? QStringLiteral("BreezeModified")
												  : schemePath);
		term->setKeyBindings(QStringLiteral("default"));
		// No scrollbar — ImGui-terminal parity; its palette-filled trough
		// would also read as a mismatched translucent band.
		term->setScrollBarPosition(QTermWidget::NoScrollBar);
		term->setTerminalFont(font);
		applyTheme(term);
		if (!projectRoot.isEmpty())
			term->setWorkingDirectory(projectRoot);
	}

	// The "+" scales with the app font (13pt base) like the tab pills.
	void stylePlus()
	{
		const qreal z =
			qApp->font().pointSize() > 0 ? qApp->font().pointSize() / 13.0 : 1.0;
		plus->setFixedSize(std::max(26, qRound(26 * z)), std::max(22, qRound(22 * z)));
		plus->setStyleSheet(QString("font-size: %1px; font-weight: 600; border: none; "
									"background: transparent;")
								.arg(std::max(9, qRound(15 * z))));
	}

	void addSession()
	{
		Session s;
		s.id = nextId++;
		s.term = new QTermWidget(0, q);
		styleTerm(s.term);
		QTermWidget::connect(s.term, &QTermWidget::finished, q, [this, term = s.term] {
			if (Session *s2 = sessionFor(term))
			{
				s2->ended = true;
				if (int i = tabs->indexOf(term); i >= 0)
					tabs->setTabText(i, tabLabel(*s2));
			}
		});
		sessions.push_back(s);
		tabs->addTab(s.term, tabLabel(s));
		tabs->setCurrentWidget(s.term);
		s.term->startShellProgram();
		s.term->setFocus();
	}

	static QString tabLabel(const Session &s)
	{
		return s.ended ? QStringLiteral("Terminal %1 (ended)").arg(s.id)
					   : QStringLiteral("Terminal %1").arg(s.id);
	}

	void closeSession(int index)
	{
		if (index < 0 || index >= static_cast<int>(sessions.size()))
			return;
		QTermWidget *term = sessions[static_cast<size_t>(index)].term;
		sessions.erase(sessions.begin() + index);
		tabs->removeTab(index);
		delete term;
		// ImGui parity: closing the last tab respawns a fresh session.
		if (sessions.empty())
			addSession();
	}

	// Editor-tab parity: an ✕ tool button on the ACTIVE tab only — the
	// shared factory (qt_tab_close.h) keeps size/sheet/hover identical.
	void updateCloseButtons(int current)
	{
		updateTabCloseButtons(
			tabs->tabBar(), current, [this](int index) { closeSession(index); });
	}
};

QtTerminalPanel::QtTerminalPanel(QWidget *parent) : QWidget(parent), impl_(new Impl)
{
	impl_->q = this;

	auto *layout = new QHBoxLayout(this);
	layout->setContentsMargins(0, 0, 0, 0);
	layout->setSpacing(0);

	impl_->tabs = new QTabWidget(this);
	// Document mode draws a NATIVE macOS separator line over the tab bar
	// (reads black on light/translucent themes) — the QSS pill tabs are
	// the whole look here, so drop it like the editor groups do.
	impl_->tabs->setDocumentMode(false);
	// No pane hairline either: the splitter handle above the panel is the
	// one intended border (see the #terminalTabs rule in qt_theme.h).
	impl_->tabs->setObjectName(QStringLiteral("terminalTabs"));
	// The macOS document-mode close button is the red native pill — the
	// editor tabs use a plain ✕ tool button, so drop the built-in one
	// and install the same button (active tab only, QtWorkbench style).
	impl_->tabs->setTabsClosable(false);
	impl_->tabs->setMovable(false);

	// Trailing "+" — corner button spawns another session (ImGui parity).
	// Boxed with right padding so the glyph doesn't hug the pane edge.
	auto *plusBox = new QWidget(impl_->tabs);
	auto *plusLay = new QHBoxLayout(plusBox);
	plusLay->setContentsMargins(0, 2, 10, 2);
	auto *plus = new QToolButton(plusBox);
	plus->setText(QStringLiteral("+"));
	plus->setToolTip(QStringLiteral("New Terminal"));
	plus->setAutoRaise(true);
	impl_->plus = plus;
	impl_->stylePlus();
	plusLay->addWidget(plus);
	impl_->tabs->setCornerWidget(plusBox, Qt::TopRightCorner);
	connect(plus, &QToolButton::clicked, this, [this] { impl_->addSession(); });

	connect(impl_->tabs, &QTabWidget::currentChanged, this, [this](int current) {
		impl_->updateCloseButtons(current);
	});
	connect(impl_->tabs, &QTabWidget::tabCloseRequested, this, [this](int index) {
		impl_->closeSession(index);
	});

	layout->addWidget(impl_->tabs, 1);
	setMinimumHeight(80);
	// No session here — the host applies the theme BEFORE the first
	// setVisible(), and the first scheme must already be the theme (see
	// styleTerm): qtermwidget never repaints the viewport's edge strips.
}

QtTerminalPanel::~QtTerminalPanel() = default;

void QtTerminalPanel::setProjectRoot(const QString &root) { impl_->projectRoot = root; }

bool QtTerminalPanel::visible() const { return !isHidden(); }

void QtTerminalPanel::setVisible(bool on)
{
	QWidget::setVisible(on);
	if (!on || impl_->sessions.empty())
	{
		// First show (or respawn): create the session NOW — the theme is
		// already set, so the terminal's first paint uses it everywhere.
		if (on)
			impl_->addSession();
		return;
	}
	if (QTermWidget *term = impl_->termAt(impl_->tabs->currentIndex()))
		term->setFocus();
}

void QtTerminalPanel::toggle() { setVisible(!isVisible()); }

void QtTerminalPanel::setThemeBackground(const QColor &color)
{
	if (color.isValid())
	{
		impl_->writeScheme(color);
		// The window's one tint layer is background() at exactly this
		// alpha; the terminal paints its viewport at the same value so
		// it frosts like the rest of the window (opaque off-Mac, where
		// background() has no alpha).
		impl_->themeOpacity = color.alphaF();
	}
	for (auto &s : impl_->sessions)
		impl_->applyTheme(s.term);
	update();
}

void QtTerminalPanel::applyFont(const QFont &font)
{
	impl_->font = font;
	for (auto &s : impl_->sessions)
		s.term->setTerminalFont(font);
	// Called on every app font zoom: re-scale the tab chrome with it.
	impl_->stylePlus();
	impl_->updateCloseButtons(impl_->tabs->currentIndex());
}

void QtTerminalPanel::addSession() { impl_->addSession(); }

int QtTerminalPanel::sessionCount() const
{
	return static_cast<int>(impl_->sessions.size());
}
