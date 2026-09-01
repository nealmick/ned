#include "qt_host.h"

#include "editor/services/highlight/highlight_service.h"
#include "editor/views/qt/qt_editor_view.h"
#include "editor/views/qt/qt_finder.h"
#include "editor/views/qt/qt_fonts.h"
#include "editor/views/qt/qt_settings_dialog.h"
#include "editor/views/qt/qt_sidebar.h"
#include "editor/views/qt/qt_theme.h"
#include "util/macos_window.h"

#include <QDockWidget>

#include <QApplication>
#include <QCommandLineParser>
#include <QDockWidget>
#include <QFileDialog>
#include <QFileInfo>
#include <QFontDatabase>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QShortcut>
#include <QTabWidget>
#include <QVBoxLayout>

#ifdef __APPLE__
// Defined in qt_mac_chrome.mm (ObjC++).
extern void configureNedQtChrome(void *nsWindow, float opacity, bool blurEnabled);
extern void applyNedQtWindowColor(void *nsWindow, float r, float g, float b);
extern void nedQtChromeWatch(void *winId);
#endif

namespace {
NedQtHost *gQtHost = nullptr; // macOS titlebar accessory callbacks
}

NedQtHost::NedQtHost(QWidget *parent) : QMainWindow(parent)
{
	setWindowTitle("Ned Text Editor");
	resize(1200, 750);

	connect(this, &NedQtHost::sidebarToggleRequested, this, [this] {
		for (QDockWidget *dock : findChildren<QDockWidget *>())
			dock->setVisible(!dock->isVisible());
	});
	connect(this, &NedQtHost::settingsRequested, this, [this] {
		QtSettingsDialog dialog(settings, this);
		dialog.exec();
	});

	// Register ned's bundled fonts before editors load the profile family.
	{
		const QDir fontsDir(QString::fromStdString(Settings::getAppResourcesPath()) +
							"/resources/fonts");
		for (const QString &file : fontsDir.entryList({"*.ttf", "*.otf"}, QDir::Files))
			NedQtFonts::registerFontFile(
				fontsDir.filePath(file),
				QFontDatabase::addApplicationFont(fontsDir.filePath(file)));
	}

	// Warm the async tree-sitter parser pool (ImGui host does this in
	// Workbench::initialize) — without it highlighting never starts.
	EditorHighlight::startBackgroundPrewarm();

	// Whole-app palette from the profile theme (sidebar, welcome, title
	// bars match the editor like the ImGui build). Fusion style: the
	// macOS native style overrides palette roles with system colors.
	QApplication::setStyle("Fusion");
	QApplication::setPalette(NedQtTheme::palette(settings));

	// Sidebar (hidden until a workspace opens).
	sidebar = new QtFileSidebar(this);
	auto *dock = new QDockWidget("Files", this);
	// No dock title bar ("Files" strip) — tree flush like the ImGui sidebar.
	dock->setTitleBarWidget(new QWidget(dock));
	dock->setWidget(sidebar);
	dock->setFeatures(QDockWidget::DockWidgetMovable);
	dock->hide();
	addDockWidget(Qt::LeftDockWidgetArea, dock);

	tabs = new QTabWidget(this);
	tabs->setTabsClosable(true);
	tabs->setDocumentMode(true);
	setCentralWidget(tabs);

	connect(tabs, &QTabWidget::tabCloseRequested, this, [this](int index) {
		QWidget *page = tabs->widget(index);
		tabs->removeTab(index);
		delete page;
		if (tabs->count() == 0)
			showWelcome();
	});
	connect(sidebar, &QtFileSidebar::fileActivated, this, [this](const QString &path) {
		openPath(path, true);
	});

	// Global shortcuts (keybinds.json parity comes with the NedKey host layer).
	// Cmd/Ctrl+1..9 — switch tab N; Cmd/Ctrl+W — close active tab (ImGui parity).
	for (int i = 1; i <= 9; ++i)
	{
		auto *tabShortcut = new QShortcut(QKeySequence(QString("Ctrl+%1").arg(i)), this);
		connect(tabShortcut, &QShortcut::activated, this, [this, i] {
			if (i - 1 < tabs->count())
				tabs->setCurrentIndex(i - 1);
		});
	}
	auto *closeShortcut = new QShortcut(QKeySequence("Ctrl+W"), this);
	connect(closeShortcut, &QShortcut::activated, tabs, [this] {
		if (tabs->count() > 0)
			Q_EMIT tabs->tabCloseRequested(tabs->currentIndex());
	});

	auto *finderShortcut = new QShortcut(QKeySequence("Ctrl+P"), this);
	connect(finderShortcut, &QShortcut::activated, this, [this] {
		if (workspaceRoot.isEmpty())
			return;
		auto *finder = new QtFileFinder(workspaceRoot, this);
		connect(finder, &QtFileFinder::fileSelected, this, [this](const QString &path) {
			openPath(path, true);
		});
		finder->show();
	});
	auto *settingsShortcut = new QShortcut(QKeySequence("Ctrl+,"), this);
	connect(settingsShortcut, &QShortcut::activated, this, [this] {
		QtSettingsDialog dialog(settings, this);
		dialog.exec();
		applyFontToEditors();
	});
	auto *openShortcut = new QShortcut(QKeySequence("Ctrl+O"), this);
	connect(openShortcut, &QShortcut::activated, this, [this] {
		const QString path = QFileDialog::getOpenFileName(this, "Open File");
		if (!path.isEmpty())
			openPath(path, true);
	});

	// Force native window creation now so the titlebar chrome is in place
	// before the first paint (no layout flash at open).
	(void)winId();
	applyNativeChrome();
	chromeApplied = true;

	// Files from the command line; otherwise welcome screen.
	QCommandLineParser args;
	args.process(*QApplication::instance());
	const QStringList positional = args.positionalArguments();
	if (positional.isEmpty())
	{
		showWelcome();
	} else
	{
		for (const QString &path : positional)
			openPath(path, false);
		openWorkspace(QFileInfo(positional.first()).absolutePath());
	}

	gQtHost = this;
#ifdef __APPLE__
	setMacOSTitlebarActions(
		[] {
			if (gQtHost)
				Q_EMIT gQtHost->sidebarToggleRequested();
		},
		nullptr,
		[] {
			if (gQtHost)
				Q_EMIT gQtHost->settingsRequested();
		});
#endif
	chromeApplied = false;
	untitledCounter = 1;
}

NedQtHost::~NedQtHost() = default;

void NedQtHost::showEvent(QShowEvent *event)
{
	QMainWindow::showEvent(event);
	// Qt re-asserts window flags on show and can clobber the custom
	// titlebar style mask — re-apply chrome (idempotent) after every show.
	if (chromeApplied)
		applyNativeChrome();
}

void NedQtHost::openPath(const QString &path, bool focus)
{
	// One tab per file (matches the ImGui workbench behavior).
	for (int i = 0; i < tabs->count(); ++i)
	{
		QtEditorView *existing = qobject_cast<QtEditorView *>(tabs->widget(i));
		if (existing && !path.isEmpty() && existing->filePath() == path)
		{
			if (focus)
				tabs->setCurrentIndex(i);
			return;
		}
	}

	auto *editor = new QtEditorView(settings, this);
	// Git needs the workspace root before the document opens.
	if (!workspaceRoot.isEmpty())
		editor->openWorkspaceRoot(workspaceRoot.toStdString());
	editor->openFile(path);
	editor->setFocusPolicy(Qt::StrongFocus);
	// Drop the welcome tab once a real document opens.
	for (int i = 0; i < tabs->count(); ++i)
		if (tabs->tabText(i) == "Welcome")
		{
			QWidget *welcome = tabs->widget(i);
			tabs->removeTab(i);
			delete welcome;
			break;
		}
	const QString tabName = path.isEmpty() ? QString("Untitled %1").arg(untitledCounter++)
										   : QFileInfo(path).fileName();
	const int index = tabs->addTab(editor, tabName);
	tabs->tabBar()->setVisible(true);
	connect(editor, &QtEditorView::documentEdited, this, [this, editor] {
		refreshTabTitle(tabs->indexOf(editor));
	});
	if (focus)
	{
		tabs->setCurrentIndex(index);
		tabs->currentWidget()->setFocus(Qt::OtherFocusReason);
	}
}

void NedQtHost::applyFontToEditors()
{
	QApplication::setPalette(NedQtTheme::palette(settings));
	{
		const QColor bg = NedQtTheme::background(settings);
		applyNedQtWindowColor(
			reinterpret_cast<void *>(winId()), bg.redF(), bg.greenF(), bg.blueF());
	}
	for (int i = 0; i < tabs->count(); ++i)
		if (QtEditorView *editor = qobject_cast<QtEditorView *>(tabs->widget(i)))
			editor->applyProfileFont();
}

void NedQtHost::openWorkspace(const QString &root)
{
	workspaceRoot = root;
	sidebar->openWorkspace(root);
	for (int i = 0; i < tabs->count(); ++i)
		if (QtEditorView *editor = qobject_cast<QtEditorView *>(tabs->widget(i)))
			editor->openWorkspaceRoot(root.toStdString());
	for (QDockWidget *dock : findChildren<QDockWidget *>())
		dock->show();
}

void NedQtHost::refreshTabTitle(int index)
{
	QWidget *page = tabs->widget(index);
	if (QtEditorView *editor = qobject_cast<QtEditorView *>(page))
	{
		const QString name = QFileInfo(editor->filePath()).fileName();
		tabs->setTabText(index, editor->isDirty() ? "● " + name : name);
	}
}

void NedQtHost::showWelcome()
{
	auto *welcome = new QWidget(this);
	auto *layout = new QVBoxLayout(welcome);
	layout->setAlignment(Qt::AlignCenter);

	auto *title = new QLabel("Ned", welcome);
	title->setAlignment(Qt::AlignCenter);
	QFont titleFont = title->font();
	titleFont.setPointSize(48);
	titleFont.setBold(true);
	title->setFont(titleFont);
	layout->addWidget(title);

	auto *subtitle = new QLabel("retro text editor — Qt backend", welcome);
	subtitle->setAlignment(Qt::AlignCenter);
	layout->addWidget(subtitle);
	layout->addSpacing(24);

	auto *buttons = new QWidget(welcome);
	auto *row = new QHBoxLayout(buttons);
	row->setAlignment(Qt::AlignCenter);

	auto *newFile = new QPushButton("New File", buttons);
	auto *openFileBtn = new QPushButton("Open File…", buttons);
	auto *openFolder = new QPushButton("Open Folder…", buttons);
	row->addWidget(newFile);
	row->addWidget(openFileBtn);
	row->addWidget(openFolder);

	connect(newFile, &QPushButton::clicked, this, [this] { openPath("", true); });
	connect(openFileBtn, &QPushButton::clicked, this, [this] {
		const QString path = QFileDialog::getOpenFileName(this, "Open File");
		if (!path.isEmpty())
			openPath(path, true);
	});
	connect(openFolder, &QPushButton::clicked, this, [this] {
		const QString root = QFileDialog::getExistingDirectory(this, "Open Folder");
		if (!root.isEmpty())
			openWorkspace(root);
	});

	layout->addWidget(buttons);
	tabs->addTab(welcome, "Welcome");
	tabs->tabBar()->hide(); // welcome is a page, not a tab
}

void NedQtHost::applyNativeChrome()
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
}
