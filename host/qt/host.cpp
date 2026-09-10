#include "host.h"

#include "app_shortcuts.h"

#include "editor/platform/clipboard.h"
#include "editor/services/highlight/highlight_service.h"
#include "editor/views/qt/clipboard.h"
#include "editor/views/qt/editor_frame.h"
#include "files/views/qt/file_finder_view.h"
#include "files/views/qt/file_sidebar_view.h"
#include "fonts.h"
#include "lsp/lsp_client.h"
#include "lsp/views/qt/lsp_dashboard.h"
#include "lsp/views/qt/lsp_view.h"
#include "settings_view.h"
#include "terminal_panel.h"
#include "theme.h"
#include "util/macos_window.h"
#include "welcome.h"
#include "workbench.h"

#include <QApplication>
#include <QComboBox>
#include <QDialog>
#include <QDockWidget>
#include <QEvent>
#include <QFileDialog>
#include <QFileInfo>
#include <QLabel>
#include <QPainter>
#include <QProxyStyle>
#include <QShortcut>
#include <QSplitter>
#include <QStyleFactory>
#include <QTimer>
#include <QUrl>
#include <QVBoxLayout>

#ifdef __APPLE__
// Defined in mac_chrome.mm (ObjC++).
extern void configureNedQtChrome(void *nsWindow, float opacity, bool blurEnabled);
extern void applyNedQtWindowColor(void *nsWindow, float r, float g, float b);
extern void nedQtChromeWatch(void *winId);
#endif
#ifdef _WIN32
#include "windows_titlebar.h"

// Defined in windows_chrome.cpp.
extern void configureNedQtChromeWindows(void *hwnd);
#endif

namespace {
AppHost *gQtHost = nullptr; // macOS titlebar accessory callbacks

// Fusion with the tab-bar "tear" indicator suppressed. When a pill tab is
// cut at a scroll arrow, QCommonStyle draws PE_IndicatorTabTear — a jagged
// torn-paper zigzag in palette.dark() (a near-black squiggle on dark
// themes) at each end of an overflowing bar. A clean straight cut reads
// better with the flat pill styling.
class NedChromeStyle : public QProxyStyle
{
  public:
	explicit NedChromeStyle(QStyle *base) : QProxyStyle(base) {}

	int pixelMetric(PixelMetric metric,
					const QStyleOption *option = nullptr,
					const QWidget *widget = nullptr) const override
	{
		// 1px dock separator (Files sidebar ↔ editors) — QSS can't size
		// QMainWindow separators, only the style can.
		if (metric == PM_DockWidgetSeparatorExtent)
			return 1;
		return QProxyStyle::pixelMetric(metric, option, widget);
	}

	void drawPrimitive(PrimitiveElement element,
					   const QStyleOption *option,
					   QPainter *painter,
					   const QWidget *widget) const override
	{
		if (element == PE_IndicatorTabTear || element == PE_IndicatorTabTearLeft ||
			element == PE_IndicatorTabTearRight)
			return;
		QProxyStyle::drawPrimitive(element, option, painter, widget);
	}
};
} // namespace

AppHost::AppHost(QWidget *parent) : QMainWindow(parent)
{
	setWindowTitle("Ned Text Editor");
	resize(1200, 750);
#ifdef __APPLE__
	// Window vibrancy support: keeps the surface's alpha channel and stops
	// Qt from re-asserting NSWindow opacity at show time. MUST be set
	// before winId() forces native window creation. The actual alpha comes
	// from the explicit tint fill in paintEvent (Qt skips its background
	// machinery for translucent windows — stylesheet/palette fills never
	// paint, which is why the tint MUST be an explicit QPainter fill).
	setAttribute(Qt::WA_TranslucentBackground);
#if QT_VERSION >= QT_VERSION_CHECK(6, 9, 0)
	// Own the custom chrome through Qt itself: with these hints,
	// QCocoaWindow's style-mask computation includes
	// NSWindowStyleMaskFullSizeContentView (and titlebarAppearsTransparent),
	// matching the manual chrome from applyNativeChrome(). Without them,
	// every QTabBar show/hide/move — i.e. every tab open/close — makes Qt
	// recompute the NSWindow style mask WITHOUT the full-size-content bit:
	// the window resizes by a title bar height until the chrome watcher
	// repairs it, and a maximized window ends up shorter and pushed down.
	setWindowFlag(Qt::ExpandedClientAreaHint, true);
	setWindowFlag(Qt::NoTitleBarBackgroundHint, true);
#endif
#endif

	// System clipboard for editor copy/cut/paste (ImGui host does this in
	// Workbench::initialize — without it commands.paste() no-ops).
	static Clipboard clipboardForEditors;
	setEditorClipboard(&clipboardForEditors);

	connect(this, &AppHost::sidebarToggleRequested, this, [this] {
		for (QDockWidget *dock : findChildren<QDockWidget *>())
		{
			// The Windows caption strip lives in a dock too — never a
			// sidebar-toggle target.
			if (dock->objectName() == QLatin1String("NedTitleDock"))
				continue;
			dock->setVisible(!dock->isVisible());
		}
	});
	connect(this, &AppHost::settingsRequested, this, [this] {
		// Toggle: an open popup closes instead of stacking another.
		if (settingsPopup)
			closeSettingsPopup();
		else
			showSettingsPopup();
	});

	// Register ned's bundled fonts before editors load the profile family.
	NedQtFonts::registerBundledFonts();

	// Warm the async tree-sitter parser pool (ImGui host does this in
	// Workbench::initialize) — without it highlighting never starts.
	EditorHighlight::startBackgroundPrewarm();

	// Whole-app palette from the profile theme (sidebar, welcome, title
	// bars match the editor like the ImGui build). Fusion style: the
	// macOS native style overrides palette roles with system colors.
	// Wrapped in NedChromeStyle to drop the tab-bar tear zigzag.
	QApplication::setStyle(new NedChromeStyle(QStyleFactory::create("Fusion")));
	applyAppFontAndPalette();

	// Sidebar (hidden until a workspace opens). The 1px right-edge hairline
	// is the app stylesheet's #FileSidebar rule (theme.h) — the object
	// name is what activates it.
	sidebar = new FileSidebarView(this);
	sidebar->setObjectName(QStringLiteral("FileSidebar"));
	auto *dock = new QDockWidget("Files", this);
	// No dock title bar ("Files" strip) — tree flush like the ImGui sidebar.
	dock->setTitleBarWidget(new QWidget(dock));
	dock->setWidget(sidebar);
	dock->setFeatures(QDockWidget::DockWidgetMovable);
	dock->hide();
	addDockWidget(Qt::LeftDockWidgetArea, dock);

#ifdef _WIN32
	// Custom caption strip (ImGui renderWindowsTitlebar parity, see
	// windows_titlebar.h). Top dock area: QMainWindow lays side docks out
	// BELOW it, so the strip spans the full window width above the sidebar
	// like the ImGui caption. Empty dock title bar (same trick as the
	// sidebar), fixed-height content, nothing dockable/movable.
	titleBar = new NedQtTitleBar(settings, this);
	auto *titleDock = new QDockWidget(this);
	titleDock->setTitleBarWidget(new QWidget(titleDock));
	titleDock->setWidget(titleBar);
	titleDock->setFeatures(QDockWidget::NoDockWidgetFeatures);
	titleDock->setObjectName(QStringLiteral("NedTitleDock"));
	addDockWidget(Qt::TopDockWidgetArea, titleDock);
	connect(titleBar,
			&NedQtTitleBar::sidebarToggleRequested,
			this,
			&AppHost::sidebarToggleRequested);
	connect(
		titleBar, &NedQtTitleBar::settingsRequested, this, &AppHost::settingsRequested);
	connect(titleBar, &NedQtTitleBar::terminalToggleRequested, this, [this] {
		toggleTerminalPanel();
	});
#endif

	// Workbench (ImGui counterpart): tab groups
	// in a splitter tree, drag-a-tab-to-split, welcome page when empty.
	workbench = new Workbench(this);
	// Editor tree on top, fixed terminal panel below (ImGui workbench
	// parity: terminal is a split, not a dockable window).
#if NED_QT_TERMINAL
	terminalPanel = new TerminalPanel(this);
	rethemeTerminal();
	terminalPanel->setProjectRoot(workspaceRoot);
	// Welcome-screen state: no file tree, no terminal (even when the
	// toggle is on) — both appear once a folder is opened.
	terminalPanel->setVisible(!workspaceRoot.isEmpty() && settings.terminalVisible);
	mainSplit = new NedSplitter(Qt::Vertical, this);
	mainSplit->setContentsMargins(0, 0, 0, 0);
	mainSplit->setChildrenCollapsible(false);
	mainSplit->setHandleWidth(1); // painted hairline like the editor splits
	mainSplit->addWidget(workbench);
	mainSplit->addWidget(terminalPanel);
	mainSplit->setStretchFactor(0, 1);
	mainSplit->setStretchFactor(1, 0);
	mainSplit->setSizes({height() - 240, 240});
	setCentralWidget(mainSplit);
#else
	setCentralWidget(workbench);
#endif
	connect(sidebar, &FileSidebarView::fileActivated, this, [this](const QString &path) {
		openPath(path, true);
	});
	connect(workbench, &Workbench::editorClosed, this, [this](EditorFrame *view) {
		// Runs before the view is deleted — filePath still valid.
		if (lspClient && !view->filePath().isEmpty())
			lspClient->didClose(view->filePath().toStdString());
		if (lspView)
			lspView->rebind(workbench->activeView());
	});

	// Keybinds (lsp_symbol_info / lsp_find_def / lsp_find_ref, …) come from
	// keybinds.json like the ImGui host; watch the file for live edits.
	settings.keybinds.loadKeybinds();
	keybindsWatch = new QTimer(this);
	keybindsWatch->setInterval(2000);
	connect(keybindsWatch, &QTimer::timeout, this, [this] {
		settings.keybinds.checkKeybindsFile();
	});
	keybindsWatch->start();

	// Notification toast (ImGui SettingsView::renderNotification parity):
	// Settings::showNotification sets text + countdown from anywhere
	// (keybinds load errors, settings apply messages); without this the
	// Qt host swallows them silently.
	toast = new QLabel(this);
	toast->setObjectName(QStringLiteral("NedToast"));
	toast->setWordWrap(true);
	toast->setMaximumWidth(420);
	toast->hide();
	toastTick = new QTimer(this);
	toastTick->setInterval(100);
	connect(toastTick, &QTimer::timeout, this, &AppHost::tickNotificationToast);
	toastTick->start();

	installAppShortcuts(*this);

	// Force native window creation now so the titlebar chrome is in place
	// before the first paint (no layout flash at open).
#ifdef _WIN32
	// Frameless in Qt's eyes too: windows_chrome.cpp keeps the
	// WS_OVERLAPPEDWINDOW styles but strips the frame via WM_NCCALCSIZE,
	// so the CLIENT is the whole window rect. Without this hint Qt still
	// assumes a standard frame: it sizes the window to requested-client +
	// frame (~26x71px at 200% DPI) and lays out only for the smaller
	// client — leaving dead background strips on the right/bottom (the
	// launch-time gap that a maximize/restore cycle appeared to fix).
	setWindowFlag(Qt::FramelessWindowHint, true);
#endif
	(void)winId();
	applyNativeChrome();
	chromeApplied = true;

	// The welcome page is ALWAYS installed — it is the surface the editor
	// area returns to when the last document closes. Opening a document
	// swaps it out for the editor tree. CLI files are opened by main()
	// after construction (entry-point responsibility).
	showWelcome();

	gQtHost = this;
#ifdef __APPLE__
	setMacOSTitlebarActions(
		[] {
			if (gQtHost)
				Q_EMIT gQtHost->sidebarToggleRequested();
		},
		[] {
			if (gQtHost)
				gQtHost->toggleTerminalPanel();
		},
		[] {
			if (gQtHost)
				Q_EMIT gQtHost->settingsRequested();
		});
#endif
}

AppHost::~AppHost()
{
	// Drop the chrome-callback global before anything else unwinds — the
	// macOS titlebar accessory buttons must never touch a dead host.
	gQtHost = nullptr;
	// Stop the language server before the members unwind (workbench views
	// outlive the client's shutdown sequence by being QObject children).
	if (lspClient)
		lspClient->shutdown();
}

void AppHost::toggleTerminalPanel()
{
	// Welcome screen: no terminal to toggle yet.
	if (workspaceRoot.isEmpty())
		return;
	settings.toggleTerminal(); // persists terminal_visible (ImGui parity)
#if NED_QT_TERMINAL
	if (terminalPanel)
		terminalPanel->setVisible(settings.terminalVisible);
#endif
}

void AppHost::tickNotificationToast()
{
	if (settings.notificationRemaining() <= 0.0f)
	{
		toast->hide();
		return;
	}
	if (!toast->isVisible() || toast->text() != settings.notificationMessage().c_str())
	{
		const QColor bg = NedQtTheme::background(settings);
		toast->setText(settings.notificationMessage().c_str());
		// rgba() needs the theme background baked in (stylesheet has no
		// access to NedQtTheme).
		toast->setStyleSheet(QString("QLabel#NedToast { background: rgba(%1,%2,%3,230);"
									 " color: white; border: 1px solid white;"
									 " border-radius: 8px; padding: 12px; }")
								 .arg(bg.red())
								 .arg(bg.green())
								 .arg(bg.blue()));
		toast->adjustSize();
	}
	// Bottom-left overlay, above every child (docks, splits, terminal).
	toast->move(20, height() - toast->height() - 20);
	toast->raise();
	toast->show();
	settings.decayNotification(0.1f); // tick interval, in seconds
}

void AppHost::showEvent(QShowEvent *event)
{
	QMainWindow::showEvent(event);
	// Qt re-asserts window flags on show and can clobber the custom
	// titlebar style mask — re-apply chrome after every show. Idempotent:
	// the constructor already applied it once before the first show.
	if (chromeApplied)
		applyNativeChrome();
}

void AppHost::resizeEvent(QResizeEvent *event)
{
	QMainWindow::resizeEvent(event);
	// First real resize: give the terminal its share (the constructor's
	// setSizes ran before the window had its final size).
#if NED_QT_TERMINAL
	static bool sizedOnce = false;
	if (!sizedOnce && mainSplit && height() > 400)
	{
		sizedOnce = true;
		mainSplit->setSizes({height() * 3 / 4, height() / 4});
	}
#endif
	// Keep the settings popup centered over the window.
	if (settingsPopup || settingsScrim)
		repositionSettingsPopup();
}

void AppHost::paintEvent(QPaintEvent *event)
{
	QMainWindow::paintEvent(event);
	// THE single global tint: theme background with the window opacity as
	// alpha (opaque on non-macOS). An explicit QPainter fill — Qt skips
	// its background machinery (stylesheet/palette fills) for translucent
	// windows, so this is the only reliable place. Every child surface
	// (tab strips, dock gaps, editor, welcome) is transparent over this
	// one layer, giving the whole window a uniform shade over vibrancy.
	QPainter painter(this);
	painter.fillRect(rect(), NedQtTheme::background(settings));
}

bool AppHost::eventFilter(QObject *watched, QEvent *event)
{
	// Click on the scrim (outside the settings popup) dismisses it.
	if (watched == settingsScrim && event->type() == QEvent::MouseButtonPress)
		closeSettingsPopup();
	return QMainWindow::eventFilter(watched, event);
}

void AppHost::ensureLsp(EditorFrame &editor)
{
	if (lspClient)
		return;
	lspClient = std::make_unique<LSPClient>(editor, settings);
	lspView = std::make_unique<LSPView>(
		*lspClient,
		settings,
		this,
		// Open (or focus) a document and return the editor showing it —
		// goto-definition jumps across files through this.
		[this](const std::string &path) -> EditorFrame * {
			const QString qPath = QString::fromStdString(path);
			openPath(qPath, true);
			return workbench->viewForPath(qPath);
		});
	if (!workspaceRoot.isEmpty())
		lspClient->setWorkspace(workspaceRoot.toStdString());
}

// LSP document-sync notifications for one editor (ImGui wireTabEditor
// parity). The subscriptions live in the editor's own events, so they die
// with the tab.
static void wireLspDocumentSync(LSPClient *client, EditorFrame *editor)
{
	if (!client)
		return;
	editor->editorEvents().subscribeDidEdit(
		[client, editor](const EditorEvents::DidEdit &e) {
			client->didChange(
				editor->filePath().toStdString(), e.version, e.changes, [editor] {
					return editor->documentText();
				});
		});
	editor->editorEvents().subscribeDidSave(
		[client, editor](const EditorEvents::DidSave &e) {
			client->didSave(e.path, [editor] { return editor->documentText(); });
		});
}

void AppHost::openPath(const QString &path, bool focus)
{
	// One tab per file (matches the ImGui workbench behavior): focus the
	// existing tab wherever it lives, in whichever split group.
	if (EditorFrame *existing = workbench->viewForPath(path))
	{
		if (focus)
		{
			EditorGroup *group = workbench->groupForView(existing);
			if (group)
			{
				group->setCurrentIndex(group->indexOf(existing));
				existing->setFocus(Qt::OtherFocusReason);
			}
		}
		// Re-open on an existing tab keeps sync parity with the ImGui
		// DidOpenDocument path (LSPDocumentSync turns it into didChange).
		notifyLspOpen(existing, path);
		return;
	}

	auto *editor = new EditorFrame(settings, this);
	// Git needs the workspace root before the document opens.
	if (!workspaceRoot.isEmpty())
		editor->openWorkspaceRoot(workspaceRoot.toStdString());
	editor->openFile(path);
	editor->setFocusPolicy(Qt::StrongFocus);
	const QString tabName = path.isEmpty() ? QString("Untitled %1").arg(untitledCounter++)
										   : QFileInfo(path).fileName();
	connect(editor, &EditorFrame::documentEdited, this, [this, editor] {
		refreshTabTitle(editor);
	});
	connect(editor, &EditorFrame::fontZoomed, this, &AppHost::applyProfileAppWide);
	// Opens into the focused split group (ImGui's preferredDockNodeId).
	workbench->addEditor(editor, tabName, focus);

	// LSP session (created with the first editor), diagnostics + hover
	// wiring, and the document-open notification (ImGui: DidOpenDocument).
	ensureLsp(*editor);
	editor->setDiagnostics(&lspClient->diagnostics());
	wireLspDocumentSync(lspClient.get(), editor);
	lspView->editorOpened(*editor);
	notifyLspOpen(editor, path);
}

// init + the 4-arg didOpen notification for one editor's current document
// (re-opened tab, fresh tab, and the post-workspace retry send the same pair).
void AppHost::notifyLspOpen(EditorFrame *view, const QString &path)
{
	if (!lspClient || path.isEmpty())
		return;
	lspClient->init(path.toStdString());
	lspClient->didOpen(path.toStdString(),
					   view->documentText(),
					   view->documentVersion(),
					   view->languageId());
}

// Terminal background + mono font from the profile (construction and every
// later re-apply run the identical pair).
void AppHost::rethemeTerminal()
{
#if NED_QT_TERMINAL
	if (terminalPanel)
	{
		terminalPanel->setThemeBackground(NedQtTheme::background(settings));
		terminalPanel->applyFont(NedQtFonts::terminalFont(settings));
	}
#endif
}

// One place for the app-wide font/palette/stylesheet application — the
// constructor's initial apply and every later re-apply (settings OK, font
// zoom, word-wrap toggle, opacity change) run the identical sequence.
void AppHost::applyAppFontAndPalette()
{
	QApplication::setPalette(NedQtTheme::palette(settings));
	QFont appFont = QApplication::font();
	appFont.setPointSize(static_cast<int>(settings.settings.value("fontSize", 13)));
	QApplication::setFont(appFont); // base font for new widgets / metrics
	// Only install the sheet when it CHANGED: setStyleSheet re-polishes
	// every widget in the app (QStyleSheetStyle::repolish), and a
	// redundant re-polish of the same string mid-dialog has crashed in
	// updateObjects (sidebar-toggle segfault). Toggles that don't touch a
	// theme/font token now skip the storm entirely.
	const QString sheet = NedQtTheme::appStyleSheet(
		settings, appFont.pointSize(), NedQtFonts::terminalFont(settings).pointSize());
	if (sheet != qApp->styleSheet())
	{
		// Two-step swap (clear, then install): a direct swap has hit a
		// QStyleSheetStyle::repolish crash in updateObjects (theme-switch
		// segfault); clearing first tears down the old rule tables.
		qApp->setStyleSheet(QString());
		qApp->setStyleSheet(sheet); // drives ALL existing widgets
	}
}

void AppHost::applyProfileAppWide()
{
	applyAppFontAndPalette();
#ifdef __APPLE__
	{
		// Only cocoa hands out real NSView winIds — offscreen test runs
		// would cast a fake id into objc and crash.
		const QColor bg = NedQtTheme::background(settings);
		if (QGuiApplication::platformName() == QLatin1String("cocoa"))
			applyNedQtWindowColor(
				reinterpret_cast<void *>(winId()), bg.redF(), bg.greenF(), bg.blueF());
	}
#endif
#ifdef _WIN32
	// No native caption to recolor (windows_chrome.cpp strips it) — the
	// hand-drawn bar re-reads the theme on repaint; only its height rule
	// depends on the (possibly changed) app font.
	titleBar->syncMetrics();
	titleBar->update();
#endif
	sidebar->refreshIconScale();
	workbench->refreshTabChrome(); // tab ✕ at the new chrome scale
	rethemeTerminal();
	// Settings popup is open during live edits: re-fit it so the pickers
	// grow with the new app font (adjustSize + recenter — the dialog was
	// sized for the font in effect when it opened).
	if (settingsPopup)
		repositionSettingsPopup();
	for (EditorFrame *editor : workbench->views())
	{
		editor->applyProfileFont();
		editor->reloadFileIcon();
		editor->refreshWrap(); // word_wrap toggle
		// Theme / syntax colors changed: re-cache tree-sitter theme colors
		// and re-highlight — without this, syntax stays stale on profile
		// switches (ImGui parity: EditorApi::forceColorUpdate on apply).
		editor->forceColorUpdate();
	}
	// Settings may have changed the macOS chrome (opacity / blur).
	applyNativeChrome();
	update(); // background tint in paintEvent reads the profile live
}

void AppHost::showSettingsPopup()
{
	if (settingsPopup)
		return;

	auto *dialog = new SettingsView(settings, this);
	// Embed as a child of the main window instead of spawning a second OS
	// window — settings render inside the window like the ImGui build.
	dialog->setWindowFlags(Qt::Widget);
	dialog->setObjectName("settingsPopup");
	// Solid card fill + border come from the app stylesheet
	// (#settingsPopup) — a palette(window) fill here would carry the
	// window's opacity alpha and go see-through.
	dialog->setAttribute(Qt::WA_StyledBackground, true);

	// Scrim: dims the app behind the popup and swallows clicks (click
	// outside dismisses, Cancel semantics — nothing is applied).
	settingsScrim = new QWidget(this);
	settingsScrim->setObjectName("settingsScrim");
	settingsScrim->setAttribute(Qt::WA_StyledBackground, true);
	settingsScrim->setStyleSheet("#settingsScrim { background: rgba(0, 0, 0, 110); }");
	settingsScrim->setGeometry(rect());
	settingsScrim->installEventFilter(this);
	settingsScrim->show();

	settingsPopup = dialog;
	dialog->adjustSize();
	repositionSettingsPopup();
	settingsScrim->raise();
	dialog->raise();
	dialog->show();
	if (QComboBox *profile = dialog->findChild<QComboBox *>(QStringLiteral("profileBox")))
		profile->setFocus(); // profile picker first — theme switching is the hot path
	else
		dialog->setFocus();

	// Ok/Cancel and Esc all funnel into finished(); the dialog mutates
	// `settings` in place, so re-apply the theme app-wide on the way out.
	connect(dialog, &QDialog::finished, this, [this] {
		closeSettingsPopup();
		applyProfileAppWide();
	});
	// Live-apply parity with the ImGui settings window: every control
	// commits as it changes (no OK step). The re-apply is QUEUED: running
	// it synchronously re-polishes every widget (qApp->setStyleSheet)
	// from INSIDE the checkbox's toggle emission — mid mouse-release —
	// which wedges the button's event state and can segfault. One turn of
	// the event loop later is still "instant" to a user.
	connect(dialog, &SettingsView::applied, this, [this] {
		QMetaObject::invokeMethod(
			this, [this] { applyProfileAppWide(); }, Qt::QueuedConnection);
	});
	// LSP server management lives behind the settings popup like the ImGui
	// build's settings window button.
	connect(dialog, &SettingsView::lspDashboardRequested, this, [this] {
		closeSettingsPopup();
		if (lspView && lspView->dashboard())
			lspView->dashboard()->show();
	});
	// File explorer / terminal checkboxes: persist through the same
	// Settings::toggle* path the title-bar buttons use, then apply. Live,
	// not staged (ImGui settings parity). The visibility changes are QUEUED
	// — hiding a dock / touching the terminal panel re-layouts the main
	// window and moves focus, which must not happen inside the checkbox's
	// toggle emission (mid mouse-release; see applied() above).
	connect(dialog,
			&SettingsView::panelTogglesChanged,
			this,
			[this](bool sidebar, bool terminal) {
				if (settings.sidebarVisible != sidebar)
					settings.toggleSidebar();
				if (settings.terminalVisible != terminal)
					settings.toggleTerminal();
				QMetaObject::invokeMethod(
					this,
					[this] {
						const bool hasWorkspace = !workspaceRoot.isEmpty();
						for (QDockWidget *dock : findChildren<QDockWidget *>())
							dock->setVisible(settings.sidebarVisible && hasWorkspace);
#if NED_QT_TERMINAL
						if (terminalPanel)
							terminalPanel->setVisible(settings.terminalVisible &&
													  hasWorkspace);
#endif
					},
					Qt::QueuedConnection);
			});
}

void AppHost::closeSettingsPopup()
{
	if (!settingsPopup)
		return;
	QDialog *dialog = settingsPopup;
	QWidget *scrim = settingsScrim;
	settingsPopup = nullptr;
	settingsScrim = nullptr;
	dialog->hide();
	dialog->deleteLater();
	if (scrim)
	{
		scrim->hide();
		scrim->deleteLater();
	}
	setFocus(); // hand keyboard focus back to the window
}

void AppHost::repositionSettingsPopup()
{
	if (settingsScrim)
		settingsScrim->setGeometry(rect());
	if (!settingsPopup)
		return;
	settingsPopup->adjustSize();
	settingsPopup->move((width() - settingsPopup->width()) / 2,
						std::max(40, (height() - settingsPopup->height()) / 2));
}

void AppHost::openWorkspace(const QString &root)
{
	workspaceRoot = root;
	// Folder opened: the welcome page is done for good — the editor
	// area is the surface even with no documents open.
	workbench->setWorkspaceActive(true);
#if NED_QT_TERMINAL
	if (terminalPanel)
	{
		terminalPanel->setProjectRoot(root);
		terminalPanel->setVisible(settings.terminalVisible);
	}
#endif
	sidebar->openWorkspace(root);
	for (EditorFrame *editor : workbench->views())
		editor->openWorkspaceRoot(root.toStdString());
	for (QDockWidget *dock : findChildren<QDockWidget *>())
		dock->show();
	// New workspace = new language-server scope (client resets itself).
	// Documents opened BEFORE the workspace existed could not start a
	// server (init needs a workspace root) — the queued didOpens flush
	// once it starts, so retry init for the focused document now.
	if (lspClient)
	{
		lspClient->setWorkspace(root.toStdString());
		if (EditorFrame *view = workbench->activeView())
			notifyLspOpen(view, view->filePath());
	}
}

void AppHost::refreshTabTitle(EditorFrame *editor)
{
	const QString name = QFileInfo(editor->filePath()).fileName();
	workbench->setTabText(editor, name);
}

void AppHost::showWelcome()
{
	// The welcome page is owned by the workbench (swapped out for the
	// editor tree on the first document/workspace); the folder dialog is
	// host chrome, so it stays here.
	auto *welcome = new WelcomePage(this);
	connect(welcome, &WelcomePage::openFolderRequested, this, [this] {
		const QString root = QFileDialog::getExistingDirectory(this, "Open Folder");
		if (!root.isEmpty())
			openWorkspace(root);
	});
	workbench->setWelcomePage(welcome);
}

void AppHost::applyNativeChrome()
{
#if defined(__APPLE__)
	// Only the cocoa platform hands out NSView winIds (offscreen/other
	// platforms would be garbage pointers).
	if (QGuiApplication::platformName() != "cocoa")
		return;
#endif
#ifdef __APPLE__
	// Same treatment as the GLFW host: transparent title bar, vibrancy
	// blur behind, and the content extending under the title bar.
	configureNedQtChrome(reinterpret_cast<void *>(winId()),
						 settings.settings.value("mac_background_opacity", 0.5f),
						 settings.settings.value("mac_blur_enabled", true));
	{
		const QColor bg = NedQtTheme::background(settings);
		applyNedQtWindowColor(
			reinterpret_cast<void *>(winId()), bg.redF(), bg.greenF(), bg.blueF());
	}
	nedQtChromeWatch(reinterpret_cast<void *>(winId()));
#endif
#ifdef _WIN32
	// windows_chrome.cpp: borderless client area + DWM shadow/snap with the
	// hand-drawn NedQtTitleBar caption (same recipe as the GLFW host's
	// util/windows_window.cpp). Idempotent — safe to re-run on show.
	configureNedQtChromeWindows(reinterpret_cast<void *>(winId()));
#endif
}
