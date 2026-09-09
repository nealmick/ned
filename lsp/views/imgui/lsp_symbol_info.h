#pragma once

#include "../../../lsp/lsp_hover.h"
#include "imgui.h"
#include <string>

class EditorApi;
class EditorSurface;
class LSPClient;

class LSPSymbolInfo
{
  public:
	LSPSymbolInfo(LSPClient &client, EditorApi &api, EditorSurface &surface);
	~LSPSymbolInfo();

	// Ctrl/Cmd+I — show hover for the symbol at the caret (until dismissed).
	void triggerAtCaret();

	void poll();

	void setEditor(EditorApi &editorApi, EditorSurface &editorSurface)
	{
		api = &editorApi;
		surface = &editorSurface;
	}
	// Mouse-hover features target the editor under the mouse, which in split
	// layouts differs from the focused editor keybinds act on.
	void setHoverEditor(EditorApi &editorApi, EditorSurface &editorSurface)
	{
		hoverApi = &editorApi;
		hoverSurface = &editorSurface;
	}

  private:
	void updateMouseHover();
	void hideMouseHover();
	// Anchor != null: tooltip pinned at that screen position (keybind hover),
	// mouse-stickiness bookkeeping skipped.
	void renderMouseTooltip(const std::string &markdown, const ImVec2 *anchor = nullptr);

	bool atCaret = false; // keybind-triggered: anchored at the caret
	bool requestedForCell = false;
	LSPClient *client = nullptr;
	EditorApi *api = nullptr;	   // focused editor (keybinds, caret hover)
	EditorApi *hoverApi = nullptr; // hovered editor (mouse tooltip)
	// Presentation counterparts of api/hoverApi (layout, hover trigger,
	// tooltip arbitration) — the surface owns those in the ImGui backend.
	EditorSurface *surface = nullptr;
	EditorSurface *hoverSurface = nullptr;

	int hoverRow = -1;
	int hoverCol = -1;
	// Rendered tooltip rect last frame — moving onto it keeps the popup.
	bool popupRectValid = false;
	ImVec2 popupMin{0.0f, 0.0f};
	ImVec2 popupMax{0.0f, 0.0f};
};
