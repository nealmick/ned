#pragma once

/*
	Completion requests, shared by every backend view (LSPHover's sibling):
	typing/manual trigger -> TextDocument_Completion -> normalized entries in
	LSPRequestState (thread-safe, stale-response guarded). Normalization to
	LSPCompletionEntry happens once on the reader thread so views (and the
	filter in lsp_completion_model.h) never touch generated lsp types.
*/

#include "lsp_completion_model.h"
#include "lsp_request.h"

#include <optional>
#include <string>
#include <vector>

class LSPClient;
class LSPEditor;

// One delivered completion answer (a copy is cheap: popups are small).
struct LSPCompletionResult
{
	std::vector<LSPCompletionEntry> items;
	bool isIncomplete = false; // server wants re-requests, not local filtering
};

class LSPCompletion
{
  public:
	// Mirrors CompletionTriggerKind for the request context.
	enum class Trigger { Invoked, TriggerCharacter, Incomplete };

	LSPCompletion(LSPClient &client, LSPEditor &api);
	~LSPCompletion();

	// Request completions at the bound editor's caret. Returns false when
	// no request could be sent (no server/handler).
	bool requestAtCaret(Trigger trigger, char triggerCharacter = '\0');

	// Dismiss: drops the result and invalidates any in-flight delivery.
	void cancel() { state.cancel(); }

	bool isPending() const { return state.isPending(); }
	// Delivered items; nullopt until (and absent on empty/failed) delivery.
	std::optional<LSPCompletionResult> snapshot() const { return state.snapshot(); }

	// Point at a different editor (multi-tab focus / split).
	void setApi(LSPEditor *editorApi) { api = editorApi; }

	// Server trigger characters from the initialize result (empty when the
	// server declared none).
	void setTriggerCharacters(const std::vector<std::string> &chars)
	{
		triggerChars = chars;
	}
	bool hasTriggerCharacter(char c) const;

  private:
	LSPClient *client = nullptr;
	LSPEditor *api = nullptr;
	LSPRequestState<LSPCompletionResult> state;
	std::vector<std::string> triggerChars;
};
