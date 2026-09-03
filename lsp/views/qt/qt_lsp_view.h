/*
	File: views/qt/qt_lsp_view.h
	Description: Qt-facing LSP UI (parallel of views/imgui/lsp_view). Owns
	the goto-results picker, hover tooltip and server dashboard; an
	application event filter drives keybinds (NedKey from keybinds.json)
	and focus-following rebinding, and a short timer folds async LSP
	results into the widgets. The LSPClient stays a pure session class.
*/

#pragma once

#ifndef NED_ENABLE_LSP
#define NED_ENABLE_LSP 1
#endif

#include <QObject>
#include <functional>

class LSPClient;
class QEvent;
class QKeyEvent;
class QTimer;
class QWidget;
class QtEditorView;
class QtLspDashboard;
class QtLspUriOptions;
class QtLspSymbolInfo;
struct LSPLocation;
class Settings;

#if NED_ENABLE_LSP

class LspQtView : public QObject
{
	Q_OBJECT

  public:
	// openFile receives an absolute path and returns the editor that now
	// shows it (existing tab focused or a new one).
	using OpenFileFn = std::function<class QtEditorView *(const std::string &path)>;

	LspQtView(LSPClient &client,
			  Settings &settings,
			  QWidget *hostWindow,
			  OpenFileFn openFile);
	~LspQtView() override;

	// A new editor was opened: install hover wiring + make it the target.
	void editorOpened(QtEditorView &view);
	// Focused editor changed (or null when the last tab closed).
	void rebind(QtEditorView *view);

	QtLspDashboard *dashboard() const { return dashboard_; }

  protected:
	// Application-wide filter: LSP keybinds on editor key presses, focus
	// following, hover dismissal on clicks outside the editor.
	bool eventFilter(QObject *watched, QEvent *event) override;

  private:
	bool handleKeybinds(QtEditorView &view, QKeyEvent *event);
	void jumpTo(const LSPLocation &location);
	void poll(); // timer: goto renders (picker) + hover delivery (tooltip)

	LSPClient &client;
	Settings &settings;
	QWidget *hostWindow = nullptr;
	OpenFileFn openFile;

	QtEditorView *active = nullptr;
	QtLspUriOptions *picker = nullptr;
	QtLspDashboard *dashboard_ = nullptr;
	QtLspSymbolInfo *symbolInfo = nullptr;
	QTimer *pollTimer = nullptr;
	bool *renderedShowFlag = nullptr; // goto picker ownership arbitration
};

#else // !NED_ENABLE_LSP

// Minimal stand-in so the Qt host compiles without lsp-framework.
class LspQtView
{
  public:
	struct DashboardStub
	{
		void show() {}
	};

	// Same callable shape as the real class (a void* here would reject
	// the host's lambda at the make_unique call site).
	using OpenFileFn = std::function<class QtEditorView *(const std::string &)>;

	LspQtView(class LSPClient &, Settings &, QWidget *, OpenFileFn = nullptr) {}

	void editorOpened(QtEditorView &) {}
	void rebind(QtEditorView *) {}
	DashboardStub *dashboard() { return &stub; }

  private:
	DashboardStub stub;
};

#endif // NED_ENABLE_LSP
