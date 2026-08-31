#pragma once

#include "imgui.h"
#include "lsp_request.h"
#include <string>

class EditorApi;
class LSPClient;

class LSPSymbolInfo
{
  public:
	LSPSymbolInfo(LSPClient &client, EditorApi &api);
	~LSPSymbolInfo();

	// Ctrl/Cmd+I — show hover for the symbol at the caret (until dismissed).
	void get();

	void render();

	void setApi(EditorApi &editorApi) { api = &editorApi; }
	// Mouse-hover features target the editor under the mouse, which in split
	// layouts differs from the focused editor keybinds act on.
	void setHoverApi(EditorApi &editorApi) { hoverApi = &editorApi; }

  private:
	void updateMouseHover();
	void requestAt(int row, int utf8Column);
	void hideMouseHover();
	// Anchor != null: tooltip pinned at that screen position (keybind hover),
	// mouse-stickiness bookkeeping skipped.
	void renderMouseTooltip(const std::string &markdown, const ImVec2 *anchor = nullptr);

	bool atCaret = false; // keybind-triggered: anchored at the caret
	bool requestedForCell = false;
	LSPRequestState<std::string> hoverState; // delivered text; nullopt = failure
	LSPClient *client = nullptr;
	EditorApi *api = nullptr;	   // focused editor (keybinds, caret hover)
	EditorApi *hoverApi = nullptr; // hovered editor (mouse tooltip)

	int hoverRow = -1;
	int hoverCol = -1;
	// Rendered tooltip rect last frame — moving onto it keeps the popup.
	bool popupRectValid = false;
	ImVec2 popupMin{0.0f, 0.0f};
	ImVec2 popupMax{0.0f, 0.0f};
};
