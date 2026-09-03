#pragma once

/*
	Hover requests, shared by every backend view (LSPGoto's sibling): keybind
	get() or mouse requestAt() -> TextDocument_Hover -> delivered markdown in
	LSPRequestState (thread-safe, stale-response guarded). Contents are
	normalized here (plaintext fenced as code, MarkedString joined) so each
	backend renders the same text.
*/

#include "lsp_request.h"
#include <optional>
#include <string>

class LSPClient;
class LspEditor;

class LSPHover
{
  public:
	LSPHover(LSPClient &client, LspEditor &api);
	~LSPHover();

	// Check keybind-style trigger: hover for the symbol at the caret.
	void get();

	// Mouse-hover path: explicit cell (row + UTF-8 byte column). `target`
	// overrides the bound editor for split layouts, where the hovered
	// editor differs from the focused one. Returns false when no request
	// was sent (no handler) — callers that dedup per cell should not
	// consume their request slot in that case.
	bool requestAt(int row, int utf8Column, LspEditor *target = nullptr);

	// Dismiss: drops the result and invalidates any in-flight delivery.
	void cancel() { state.cancel(); }

	bool isPending() const { return state.isPending(); }
	// Delivered markdown; nullopt until (and absent on empty/failed) delivery.
	std::optional<std::string> snapshot() const { return state.snapshot(); }

	// Point at a different editor (multi-tab focus / split hover).
	void setApi(LspEditor *editorApi) { api = editorApi; }

  private:
	LSPClient *client = nullptr;
	LspEditor *api = nullptr;
	LSPRequestState<std::string> state;
};
