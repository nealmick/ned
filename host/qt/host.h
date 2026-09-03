/*
	File: host/qt/host.h
	Description: Qt application shell — native QMainWindow host for the ned
	core. Owns window lifecycle, the workbench (splits + tab groups),
	sidebar, finder and the NSWindow chrome on macOS; editor surface lives
	in editor/views/qt/.
*/

#pragma once

#include "lsp/lsp_client.h"
#include "lsp/views/qt/lsp_view.h"
#include "util/settings.h"

#include <QMainWindow>

#include <memory>

class QDialog;
class QEvent;
class QResizeEvent;
class QSplitter;
class TerminalPanel;
class Workbench;
class EditorFrame;
class FileSidebarView;
class QTimer;
class QWidget;

class AppHost : public QMainWindow
{
	// app_shortcuts.cpp installs every host-level QShortcut at startup.
	friend void installAppShortcuts(AppHost &);

	Q_OBJECT

  public:
	explicit AppHost(QWidget *parent = nullptr);
	~AppHost() override;

	// Host-facing surface.
	void openPath(const QString &path, bool focus);
	void openWorkspace(const QString &root);
	void showWelcome();
	// Check modes read/restore persisted settings (e.g. the font-zoom
	// test must put the original size back).

  Q_SIGNALS:
	void sidebarToggleRequested();
	void settingsRequested();

  protected:
	void showEvent(QShowEvent *event) override;
	void resizeEvent(QResizeEvent *event) override;
	void paintEvent(QPaintEvent *event) override;
	bool eventFilter(QObject *watched, QEvent *event) override;

  private:
	void applyNativeChrome();

	// LSP session + UI, created with the first editor (workspace known by
	// then). Document-sync subscriptions live in each editor's events.
	void ensureLsp(EditorFrame &editor);

	void refreshTabTitle(EditorFrame *editor);
	void applyProfileAppWide();

  private:
	// In-window settings popup (no second OS window — ImGui parity).
	void showSettingsPopup();
	void closeSettingsPopup();
	void repositionSettingsPopup();
	// App-wide font/palette/stylesheet from the profile (initial + re-apply).
	void applyAppFontAndPalette();
	// One path for every toggle surface (Ctrl+T, title-bar button).
	void toggleTerminalPanel();

	Settings settings;
	Workbench *workbench = nullptr;
	QSplitter *mainSplit = nullptr; // workbench (top) + terminal (bottom)
	TerminalPanel *terminalPanel = nullptr;
	FileSidebarView *sidebar = nullptr;
	QDialog *settingsPopup = nullptr;
	QWidget *settingsScrim = nullptr;
	std::unique_ptr<LSPClient> lspClient;
	std::unique_ptr<LspView> lspView;
	QTimer *keybindsWatch = nullptr; // keybinds.json live reload (ImGui tick parity)
	QString workspaceRoot;
	bool chromeApplied = false;
	int untitledCounter = 1;
};
