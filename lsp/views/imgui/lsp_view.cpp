#include "lsp_view.h"
#include "../../editor/views/imgui/ned_key.h"

#if NED_ENABLE_LSP

#include "../../lsp_client.h"
#include "../../lsp_goto.h"
#include "imgui.h"

LSPView::LSPView(LSPClient &lspClient,
				 EditorApi &api,
				 EditorSurface &surface,
				 FileExplorer &fileExplorer,
				 Settings &settings)
	: dashboard_(lspClient, fileExplorer, settings),
	  client(lspClient),
	  symbolInfo(lspClient, api, surface),
	  uriOptions(api, surface, fileExplorer, settings)
{
	dashboard_.refresh();

	auto renderThroughPicker = [this](const std::string &title,
									  const std::vector<LSPLocation> &locations,
									  bool &show) {
		uriOptions.present(title, locations, show);
	};
	client.gotoDef.resultRenderer = renderThroughPicker;
	client.gotoRef.resultRenderer = renderThroughPicker;
}

bool LSPView::handleKeybinds()
{
	if (!client.isInitialized())
		return false;

	bool modPressed = ImGui::GetIO().KeyCtrl;
	if (!modPressed)
		return false;

	bool shortcutPressed = false;

	// LSP Symbol Info keybind
	const ImGuiKey symbolInfoKey =
		imguiKeyFromNed(client.settingsKeybinds().getActionKey("lsp_symbol_info"));
	if (symbolInfoKey != ImGuiKey_None && ImGui::IsKeyPressed(symbolInfoKey, false))
	{
		symbolInfo.triggerAtCaret();
		shortcutPressed = true;
	}

	// LSP Goto Definition keybind
	const ImGuiKey gotoDefKey =
		imguiKeyFromNed(client.settingsKeybinds().getActionKey("lsp_find_def"));
	if (gotoDefKey != ImGuiKey_None && ImGui::IsKeyPressed(gotoDefKey, false))
	{
		client.gotoDef.get();
		shortcutPressed = true;
	}

	// LSP Goto References keybind
	const ImGuiKey gotoRefKey =
		imguiKeyFromNed(client.settingsKeybinds().getActionKey("lsp_find_ref"));
	if (gotoRefKey != ImGuiKey_None && ImGui::IsKeyPressed(gotoRefKey, false))
	{
		client.gotoRef.get();
		shortcutPressed = true;
	}

	return shortcutPressed;
}

void LSPView::rebind(EditorApi &api, EditorSurface &surface)
{
	symbolInfo.setEditor(api, surface);
	uriOptions.setApi(api, surface);
	setHoverApi(api, surface);
}

void LSPView::setHoverApi(EditorApi &api, EditorSurface &surface)
{
	symbolInfo.setHoverEditor(api, surface);
}

LSPDashboard &LSPView::dashboard() { return dashboard_; }

void LSPView::poll()
{
	symbolInfo.poll();
	client.gotoDef.render();
	client.gotoRef.render();
}

#endif // NED_ENABLE_LSP
