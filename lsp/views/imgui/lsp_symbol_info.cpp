#include "lsp_symbol_info.h"
#include "../../../editor/editor_api.h"
#include "../../../editor/views/imgui/editor_surface.h"
#include "../../../lsp/lsp_client.h"
#include "../../editor/views/imgui/hover_tooltip.h"

#include <algorithm>

namespace {

// Mouse slack around the popup that still counts as "over the tooltip".
constexpr float kPopupStickyPadding = 12.0f;

} // namespace

LSPSymbolInfo::LSPSymbolInfo(LSPClient &client, EditorApi &api, EditorSurface &surface)
	: client(&client), api(&api), surface(&surface)
{
}

LSPSymbolInfo::~LSPSymbolInfo() = default;

void LSPSymbolInfo::triggerAtCaret()
{
	if (!client || !api || !client->isInitialized())
		return;

	atCaret = true;
	client->hover.get();
}

void LSPSymbolInfo::hideMouseHover()
{
	if (atCaret)
		return;
	requestedForCell = false;
	hoverRow = -1;
	hoverCol = -1;
	popupRectValid = false;
	client->hover.cancel();
}

void LSPSymbolInfo::updateMouseHover()
{
	if (!client || !api || !client->isInitialized())
		return;

	// Keybind hover: anchored at the caret, immune to mouse logic; any
	// dismissal signal (key/click/scroll) retires it.
	if (atCaret)
	{
		if (surface->hoverDismissed())
		{
			atCaret = false;
			client->hover.cancel();
		}
		return;
	}

	EditorApi *const hover = hoverApi ? hoverApi : api;
	EditorSurface *const hoverView = hoverSurface ? hoverSurface : surface;

	// Sticky popup: moving onto/inside the rendered tooltip keeps it (VSCode
	// sticky-hover) — but a dismissal signal (key/click/scroll) still wins.
	// This is purely local: the frame's trigger is never mutated, so it can
	// not get wedged into a stale "active" state.
	const ImVec2 mouse = ImGui::GetMousePos();
	const bool overPopup = popupRectValid && ImGui::IsMousePosValid(&mouse) &&
						   mouse.x >= popupMin.x - kPopupStickyPadding &&
						   mouse.x <= popupMax.x + kPopupStickyPadding &&
						   mouse.y >= popupMin.y - kPopupStickyPadding &&
						   mouse.y <= popupMax.y + kPopupStickyPadding;
	if (hoverView->hoverDismissed())
	{
		hideMouseHover();
		return;
	}
	if (overPopup && (client->hover.isPending() || client->hover.snapshot()))
		return;
	popupRectValid = false;

	// The frame's trigger owns all VSCode-style logic: armed by real mouse
	// moves only, dismissed by keys/clicks/scroll/shifted content, target
	// frozen while showing.
	const HoverTrigger::Info info = hoverView->hoverInfo();
	if (!info.active || info.zone != HoverTrigger::Zone::Text || hover->path().empty() ||
		!client->isDocumentOpen(hover->path()))
	{
		hideMouseHover();
		return;
	}

	if (info.row != hoverRow || info.column != hoverCol)
	{
		hoverRow = info.row;
		hoverCol = info.column;
		requestedForCell = false;
		client->hover.cancel();
	}

	if (!requestedForCell)
		requestedForCell = client->hover.requestAt(info.row, info.column, hover);
}

void LSPSymbolInfo::poll()
{
	updateMouseHover();

	// Visibility IS the request state: no delivered text, no tooltip (an
	// empty answer is an answer — deliver clears pending either way).
	const auto snap = client->hover.snapshot();
	if (!snap || snap->empty())
		return;

	if (atCaret)
	{
		// Anchor just below the caret cell; dismissal retires it.
		const ViewLayout &layout = surface->layout();
		int row = 0, col = 0;
		api->getCaret(row, col);
		const float fs = ImGui::GetFontSize();
		const ImVec2 anchor(surface->caretScreenX() + fs * 0.25f,
							layout.textPos.y +
								static_cast<float>(row + 1) * layout.lineHeight +
								fs * 0.25f);
		renderMouseTooltip(*snap, &anchor);
		return;
	}

	renderMouseTooltip(*snap);
}

void LSPSymbolInfo::renderMouseTooltip(const std::string &markdown, const ImVec2 *anchor)
{
	if (!api || !surface || !surface->claimTooltip())
		return;

	const float fs = ImGui::GetFontSize();
	if (anchor)
		ImGui::SetNextWindowPos(*anchor);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(fs * 0.7f, fs * 0.5f));
	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, fs * 0.3f);

	if (ImGui::BeginTooltip())
	{
		EditorApi &hover = anchor ? *api : (hoverApi ? *hoverApi : *api);
		renderHoverMarkdown(markdown, hover, hover.languageId());
		if (!anchor)
		{
			// Mouse mode: remember the rect for sticky-popup handling.
			popupMin = ImGui::GetWindowPos();
			popupMax = ImVec2(popupMin.x + ImGui::GetWindowSize().x,
							  popupMin.y + ImGui::GetWindowSize().y);
			popupRectValid = true;
		}
		ImGui::EndTooltip();
	}

	ImGui::PopStyleVar(2);
}
