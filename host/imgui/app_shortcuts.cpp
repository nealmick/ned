/*
	File: host/imgui/app_shortcuts.cpp
	Description: Global app shortcuts for the ImGui host (Cmd/Ctrl+T,
	sidebar, settings, zoom, welcome, open). Keybinds come from
	KeybindsManager (NedKey) and convert via imguiKeyFromNed.
*/

#include "app_shortcuts.h"
#include "editor/editor_api.h"
#include "editor/views/imgui/ned_key.h"
#include "files/files.h"
#include "lsp/lsp_client.h"
#include "lsp/views/imgui/lsp_view.h"
#include "util/settings.h"
#include "workbench.h"

#include "imgui.h"
#include <algorithm>

bool handleAppKeyboardShortcuts(EditorApi &api,
								FileExplorer &files,
								Settings &settings,
								LspView &lsp)
{
	bool pressed = false;
	const ImGuiIO &io = ImGui::GetIO();
	const bool mod = io.KeyCtrl || io.KeySuper;

	const auto pressedKey = [&](const char *action) {
		return ImGui::IsKeyPressed(
			imguiKeyFromNed(settings.keybinds.getActionKey(action)), false);
	};

	// Cmd/Ctrl+T — toggle bottom terminal panel (under editor dock).
	{
		ImGuiKey termKey =
			imguiKeyFromNed(settings.keybinds.getActionKey("toggle_terminal"));
		if (termKey == ImGuiKey_None)
			termKey = ImGuiKey_T; // default if keybinds.json omits it
		if (mod && ImGui::IsKeyPressed(termKey, false))
		{
			api.closeAllOverlays();
			api.save();
			files.showWelcomeScreen = false;
			settings.toggleTerminal();
			return true;
		}
	}

	if (mod && pressedKey("toggle_sidebar"))
	{
		settings.toggleSidebar();
		pressed = true;
	}

	if (mod && pressedKey("toggle_settings_window"))
	{
		files.showWelcomeScreen = false;
		settings.toggleSettingsWindow(api);
		pressed = true;
	}

	if (mod && ImGui::IsKeyPressed(ImGuiKey_Equal))
	{
		settings.settings["fontSize"] = settings.settings.value("fontSize", 20.0f) + 2.0f;
		settings.requestApply();
		settings.saveSettings();
		api.requestEnsureVisible();
		pressed = true;
	} else if (mod && ImGui::IsKeyPressed(ImGuiKey_Minus))
	{
		settings.settings["fontSize"] =
			std::max(settings.settings.value("fontSize", 20.0f) - 2.0f, 8.0f);
		settings.requestApply();
		settings.saveSettings();
		api.requestEnsureVisible();
		pressed = true;
	}

	if (mod && ImGui::IsKeyPressed(ImGuiKey_Slash, false))
	{
		api.closeAllOverlays();
		files.showWelcomeScreen = !files.showWelcomeScreen;
		api.save();
		pressed = true;
	}
	if (mod && ImGui::IsKeyPressed(ImGuiKey_O, false))
	{
		api.closeAllOverlays();
		api.save();
		files.showFileDialog = true;
		pressed = true;
	}

	if (lsp.keybinds())
		pressed = true;

	return pressed;
}
