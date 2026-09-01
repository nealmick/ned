#include "qt_host.h"

#include "editor/services/highlight/highlight_service.h"
#include "editor/views/qt/qt_editor_view.h"
#include "editor/views/qt/qt_finder.h"
#include "editor/views/qt/qt_settings_dialog.h"
#include "editor/views/qt/qt_sidebar.h"
#include "util/macos_window.h"

#include <QDockWidget>

#include <QApplication>
#include <QCommandLineParser>
#include <QDockWidget>
#include <QFileDialog>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QShortcut>
#include <QTabWidget>
#include <QVBoxLayout>

#ifdef __APPLE__
// Defined in qt_mac_chrome.mm (ObjC++).
extern void configureNedQtChrome(void *nsWindow, float opacity, bool blurEnabled);
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

	// Warm the async tree-sitter parser pool (ImGui host does this in
	// Workbench::initialize) — without it highlighting never starts.
	EditorHighlight::startBackgroundPrewarm();

	// Dark chrome under the transparent title bar.
	QPalette dark = palette();
	dark.setColor(QPalette::Window, QColor(0x1e, 0x1e, 0x1e));
	dark.setColor(QPalette::Base, QColor(0x1e, 0x1e, 0x1e));
	dark.setColor(QPalette::Text, QColor(0xd0, 0xd0, 0xd0));
	setPalette(dark);

	// Sidebar (hidden until a workspace opens).
	sidebar = new QtFileSidebar(this);
	auto *dock = new QDockWidget("Files", this);
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
	connect(sidebar, &QtFileSidebar::fileActivated, this,
			[this](const QString &path) { openPath(path, true); });

	// Global shortcuts (keybinds.json parity comes with the NedKey host layer).
	auto *finderShortcut = new QShortcut(QKeySequence("Ctrl+P"), this);
	connect(finderShortcut, &QShortcut::activated, this, [this] {
		if (workspaceRoot.isEmpty())
			return;
		auto *finder = new QtFileFinder(workspaceRoot, this);
		connect(finder, &QtFileFinder::fileSelected, this,
				[this](const QString &path) { openPath(path, true); });
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
		[] { if (gQtHost) Q_EMIT gQtHost->sidebarToggleRequested(); },
		nullptr,
		[] { if (gQtHost) Q_EMIT gQtHost->settingsRequested(); });
#endif
	chromeApplied = false;
	untitledCounter = 1;
}

NedQtHost::~NedQtHost() = default;

void NedQtHost::showEvent(QShowEvent *event)
{
	QMainWindow::showEvent(event);
	// winId()/NSWindow are only valid once the native window exists.
	if (!chromeApplied)
	{
		chromeApplied = true;
		applyNativeChrome();
	}
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
	editor->openFile(path);
	// Drop the welcome tab once a real document opens.
	for (int i = 0; i < tabs->count(); ++i)
		if (tabs->tabText(i) == "Welcome")
		{
			QWidget *welcome = tabs->widget(i);
			tabs->removeTab(i);
			delete welcome;
			break;
		}
	const QString tabName =
		path.isEmpty() ? QString("Untitled %1").arg(untitledCounter++) : QFileInfo(path).fileName();
	const int index = tabs->addTab(editor, tabName);
	connect(editor, &QtEditorView::documentEdited, this,
			[this, editor] { refreshTabTitle(tabs->indexOf(editor)); });
	if (focus)
		tabs->setCurrentIndex(index);
}

void NedQtHost::applyFontToEditors()
{
	// Font size comes from the shared profile; editors repaint with the
	// service timer's next tick.
	for (int i = 0; i < tabs->count(); ++i)
		if (QtEditorView *editor = qobject_cast<QtEditorView *>(tabs->widget(i)))
			editor->update();
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
		const QString name =
			QFileInfo(editor->filePath()).fileName();
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
}

void NedQtHost::applyNativeChrome()
{
#ifdef __APPLE__
	// Same treatment as the GLFW host: transparent title bar, vibrancy
	// blur behind, and the content extending under the title bar.
	configureNedQtChrome(reinterpret_cast<void *>(winId()),
						 settings.settings.value("mac_background_opacity", 0.5f),
						 settings.settings.value("mac_blur_enabled", true));
#endif
}
