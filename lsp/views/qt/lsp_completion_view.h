/*
	File: views/qt/lsp_completion_view.h
	Description: Completion session controller for the Qt backend — the
	sibling of LSPSymbolInfo. Owns the popup, the typing trigger (debounced
	DidEdit), prefix re-filtering and accept/replace. Keys are routed here
	from LSPView's event filter while a session is live (the editor keeps
	focus); results flow in through the shared LSPCompletion core on
	LSPClient, folded in by poll().
*/

#pragma once

#include "../../../lsp/lsp_completion_model.h"

#include <QObject>
#include <QPointer>
#include <string>
#include <vector>

class EditorFrame;
class LSPClient;
class QKeyEvent;
class QTimer;
class Settings;
class LSPCompletionPopup;

class LSPCompletionView : public QObject
{
	Q_OBJECT

  public:
	LSPCompletionView(LSPClient &client, Settings &settings, QObject *parent);
	~LSPCompletionView() override;

	// A new editor was opened: subscribe to its DidEdit stream (the typing
	// trigger). The subscription is guarded by QPointer — closing the tab
	// just stops the feed.
	void editorOpened(EditorFrame &view);

	// Focused editor changed (or null): retarget + drop the session.
	void setEditor(EditorFrame *view);

	// Keybind (Ctrl/Cmd+L, "lsp_completion"): explicit completion at the
	// caret, regardless of prefix length.
	void triggerAtCaret();

	// Keys while a session is live (popup open or request in flight).
	// Returns true when consumed — runs BEFORE the editor's key handler;
	// typing keys fall through and re-trigger via DidEdit.
	bool handleKey(QKeyEvent *event);

	// Any dismissal signal (key/click/retarget/close).
	void dismiss();

	// Fold async results into the popup (call from a short timer).
	void poll();

	bool isLive() const { return session; }

  private:
	void scheduleTrigger(); // restart the post-edit debounce
	void evaluate();		// decide: request / re-filter / dismiss
	void refreshPopup();	// re-filter delivered items against the prefix
	void accept(int filteredIndex);
	int caretColumn(int &row) const; // caret row/col of the active editor

	LSPClient &client;
	Settings &settings;
	QPointer<EditorFrame> view;
	LSPCompletionPopup *popup = nullptr;
	QTimer *debounce = nullptr; // post-edit trigger delay (single-shot)

	// Session state. The word span anchors everything: the request was
	// made at (caretRow, wordStartCol..caretCol) and acceptance replaces
	// exactly that span.
	bool session = false;
	int caretRow = 0;
	int wordStartCol = 0;
	std::string prefix;

	std::vector<LSPCompletionEntry> items; // last delivered (normalized)
	std::vector<int> filtered;			   // item indices shown in the popup
	bool itemsIncomplete = false;
	bool refilteredOnce = false; // local filtering already shown for prefix
};
