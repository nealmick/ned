#include "lsp_view.h"
#include "../../editor/views/imgui/ned_key_imgui.h"

#if NED_ENABLE_LSP

#include "../../lsp_client.h"
#include "../../lsp_goto.h"
#include "imgui.h"

LspImGuiView::LspImGuiView(LSPClient &lspClient,
						   EditorApi &api,
						   FileExplorer &fileExplorer,
						   Settings &settings)
	: dashboard(lspClient, fileExplorer, settings),
	  client(lspClient),
	  symbolInfo(lspClient, api),
	  uriOptions(api, fileExplorer, settings)
{
	dashboard.refreshServerInfo();

	auto renderThroughPicker = [this](const std::string &title,
									  const std::vector<LSPLocation> &locations,
									  bool &show) {
		uriOptions.render(title, locations, show);
	};
	client.gotoDef.resultRenderer = renderThroughPicker;
	client.gotoRef.resultRenderer = renderThroughPicker;
}

bool LspImGuiView::keybinds()
{
	if (!client.isInitialized())
		return false;

	bool modPressed = ImGui::GetIO().KeyCtrl;
	if (!modPressed)
		return false;

	bool shortcutPressed = false;

	// LSP Symbol Info keybind
	const ImGuiKey symbolInfoKey = imguiKeyFromNed(client.settingsKeybinds().getActionKey("lsp_symbol_info"));
	if (symbolInfoKey != ImGuiKey_None && ImGui::IsKeyPressed(symbolInfoKey, false))
	{
		symbolInfo.get();
		shortcutPressed = true;
	}

	// LSP Goto Definition keybind
	const ImGuiKey gotoDefKey = imguiKeyFromNed(client.settingsKeybinds().getActionKey("lsp_find_def"));
	if (gotoDefKey != ImGuiKey_None && ImGui::IsKeyPressed(gotoDefKey, false))
	{
		client.gotoDef.get();
		shortcutPressed = true;
	}

	// LSP Goto References keybind
	const ImGuiKey gotoRefKey = imguiKeyFromNed(client.settingsKeybinds().getActionKey("lsp_find_ref"));
	if (gotoRefKey != ImGuiKey_None && ImGui::IsKeyPressed(gotoRefKey, false))
	{
		client.gotoRef.get();
		shortcutPressed = true;
	}

	return shortcutPressed;
}

void LspImGuiView::bindEditorApi(EditorApi &api)
{
	symbolInfo.setApi(api);
	uriOptions.setApi(api);
	setHoverApi(api);
}

void LspImGuiView::setHoverApi(EditorApi &api) { symbolInfo.setHoverApi(api); }

void LspImGuiView::render()
{
	symbolInfo.render();
	client.gotoDef.render();
	client.gotoRef.render();
}

#endif // NED_ENABLE_LSP
