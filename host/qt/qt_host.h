/*
	File: host/qt/qt_host.h
	Description: Qt application shell — native QMainWindow host for the ned
	core. Owns window lifecycle, the workbench (splits + tab groups),
	sidebar, finder and the NSWindow chrome on macOS; editor surface lives
	in editor/views/qt/.
*/

#pragma once

#include "lsp/lsp_client.h"
#include "lsp/views/qt/qt_lsp_view.h"
#include "util/settings.h"

#include <QMainWindow>

#include <memory>

class QDialog;
class QEvent;
class QResizeEvent;
class QSplitter;
class QtTerminalPanel;
class QtWorkbench;
class QtEditorView;
class QtFileSidebar;
class QTimer;
class QWidget;

class NedQtHost : public QMainWindow
{
	Q_OBJECT

  public:
	explicit NedQtHost(QWidget *parent = nullptr);
	~NedQtHost() override;

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
	void ensureLsp(QtEditorView &editor);

	void refreshTabTitle(QtEditorView *editor);
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
	QtWorkbench *workbench = nullptr;
	QSplitter *mainSplit = nullptr; // workbench (top) + terminal (bottom)
	QtTerminalPanel *terminalPanel = nullptr;
	QtFileSidebar *sidebar = nullptr;
	QDialog *settingsPopup = nullptr;
	QWidget *settingsScrim = nullptr;
	std::unique_ptr<LSPClient> lspClient;
	std::unique_ptr<LspQtView> lspView;
	QTimer *keybindsWatch = nullptr; // keybinds.json live reload (ImGui tick parity)
	QString workspaceRoot;
	bool chromeApplied = false;
	int untitledCounter = 1;
};
