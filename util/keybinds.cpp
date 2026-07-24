#include "keybinds.h"
#include "../editor/editor_api.h"
#include "../files/files.h"
#include "../lsp/lsp_client.h"
#include "../util/ned_terminal.h"
#include "../util/settings.h"
#include "../util/splitter.h"
#include <algorithm>
#include <cctype>
#include <iostream>

KeybindsManager::KeybindsManager(Settings &settings) : settings(settings) {}

void KeybindsManager::touchDiskTime()
{
	std::error_code ec;
	diskTime = path.empty() ? fs::file_time_type::min() : fs::last_write_time(path, ec);
	if (ec)
		diskTime = fs::file_time_type::min();
}

void KeybindsManager::ensureFileExists()
{
	path = (fs::path(Settings::getUserConfigDir()) / "keybinds.json").string();
	const fs::path file = path;
	const fs::path dir = file.parent_path();

	if (!fs::exists(dir))
	{
		std::error_code ec;
		fs::create_directories(dir, ec);
		if (ec)
			return;
	}
	if (fs::exists(file))
		return;

	const fs::path bundled =
		fs::path(Settings::getAppResourcesPath()) / "resources/config/keybinds.json";
	if (fs::exists(bundled))
	{
		std::error_code ec;
		fs::copy_file(bundled, file, fs::copy_options::overwrite_existing, ec);
		if (!ec)
			return;
	}
	Settings::writeJson(path, {{"toggle_file_finder", "p"}});
}

void KeybindsManager::rebuildMap()
{
	keys.clear();
	if (!keybinds.is_object())
		return;

	for (auto &[action, value] : keybinds.items())
	{
		if (!value.is_string())
			continue;
		const ImGuiKey key = stringToImGuiKey(value.get<std::string>());
		if (key != ImGuiKey_None)
			keys[action] = key;
	}
}

bool KeybindsManager::loadKeybinds()
{
	ensureFileExists();
	if (path.empty() || !fs::exists(path))
	{
		keybinds = json::object();
		rebuildMap();
		return false;
	}

	if (!Settings::readJson(path, keybinds))
	{
		// Fall back to default-keybinds.json next to it.
		const std::string fallback =
			(fs::path(path).parent_path() / "default-keybinds.json").string();
		if (!Settings::readJson(fallback, keybinds))
		{
			keybinds = json::object();
			rebuildMap();
			return false;
		}
		settings.renderNotification(
			"Error in keybinds.json\nLoaded default backup keybinds");
	}

	rebuildMap();
	touchDiskTime();
	return true;
}

void KeybindsManager::checkKeybindsFile()
{
	if (path.empty() || !fs::exists(path))
	{
		loadKeybinds();
		return;
	}

	std::error_code ec;
	const auto modified = fs::last_write_time(path, ec);
	if (ec || modified <= diskTime)
		return;

	if (Settings::readJson(path, keybinds))
	{
		rebuildMap();
		diskTime = modified;
	}
}

ImGuiKey KeybindsManager::getActionKey(const std::string &actionName) const
{
	const auto it = keys.find(actionName);
	return it != keys.end() ? it->second : ImGuiKey_None;
}

ImGuiKey KeybindsManager::stringToImGuiKey(const std::string &keyString)
{
	std::string key = keyString;
	std::transform(key.begin(), key.end(), key.begin(), ::tolower);

	if (key.length() == 1)
	{
		const char c = key[0];
		if (c >= 'a' && c <= 'z')
			return static_cast<ImGuiKey>(ImGuiKey_A + (c - 'a'));
		if (c >= '0' && c <= '9')
			return static_cast<ImGuiKey>(ImGuiKey_0 + (c - '0'));
	}

	if (key == "space" || key == "spacebar")
		return ImGuiKey_Space;
	if (key == "enter" || key == "return")
		return ImGuiKey_Enter;
	if (key == "escape" || key == "esc")
		return ImGuiKey_Escape;
	if (key == "tab")
		return ImGuiKey_Tab;
	if (key == "backspace")
		return ImGuiKey_Backspace;
	if (key == "delete" || key == "del")
		return ImGuiKey_Delete;
	if (key == "insert" || key == "ins")
		return ImGuiKey_Insert;
	if (key == "up")
		return ImGuiKey_UpArrow;
	if (key == "down")
		return ImGuiKey_DownArrow;
	if (key == "left")
		return ImGuiKey_LeftArrow;
	if (key == "right")
		return ImGuiKey_RightArrow;
	if (key == "home")
		return ImGuiKey_Home;
	if (key == "end")
		return ImGuiKey_End;
	if (key == "pageup" || key == "pgup")
		return ImGuiKey_PageUp;
	if (key == "pagedown" || key == "pgdn")
		return ImGuiKey_PageDown;
	if (key == "leftctrl" || key == "lctrl")
		return ImGuiKey_LeftCtrl;
	if (key == "rightctrl" || key == "rctrl")
		return ImGuiKey_RightCtrl;
	if (key == "leftshift" || key == "lshift")
		return ImGuiKey_LeftShift;
	if (key == "rightshift" || key == "rshift")
		return ImGuiKey_RightShift;
	if (key == "leftalt" || key == "lalt")
		return ImGuiKey_LeftAlt;
	if (key == "rightalt" || key == "ralt")
		return ImGuiKey_RightAlt;
	if (key == "leftsuper" || key == "lsuper" || key == "cmd" || key == "command" ||
		key == "win" || key == "windows")
		return ImGuiKey_LeftSuper;
	if (key == "rightsuper" || key == "rsuper")
		return ImGuiKey_RightSuper;
	if (key == "f1")
		return ImGuiKey_F1;
	if (key == "f2")
		return ImGuiKey_F2;
	if (key == "f3")
		return ImGuiKey_F3;
	if (key == "f4")
		return ImGuiKey_F4;
	if (key == "f5")
		return ImGuiKey_F5;
	if (key == "f6")
		return ImGuiKey_F6;
	if (key == "f7")
		return ImGuiKey_F7;
	if (key == "f8")
		return ImGuiKey_F8;
	if (key == "f9")
		return ImGuiKey_F9;
	if (key == "f10")
		return ImGuiKey_F10;
	if (key == "f11")
		return ImGuiKey_F11;
	if (key == "f12")
		return ImGuiKey_F12;
	if (key == "apostrophe" || key == "'")
		return ImGuiKey_Apostrophe;
	if (key == "comma" || key == ",")
		return ImGuiKey_Comma;
	if (key == "minus" || key == "-")
		return ImGuiKey_Minus;
	if (key == "period" || key == ".")
		return ImGuiKey_Period;
	if (key == "slash" || key == "/")
		return ImGuiKey_Slash;
	if (key == "semicolon" || key == ";")
		return ImGuiKey_Semicolon;
	if (key == "equal" || key == "=")
		return ImGuiKey_Equal;
	if (key == "leftbracket" || key == "[")
		return ImGuiKey_LeftBracket;
	if (key == "backslash" || key == "\\")
		return ImGuiKey_Backslash;
	if (key == "rightbracket" || key == "]")
		return ImGuiKey_RightBracket;
	if (key == "graveaccent" || key == "`")
		return ImGuiKey_GraveAccent;

	std::cerr << "[Keybinds] Unrecognized key '" << keyString << "'" << std::endl;
	return ImGuiKey_None;
}

bool KeybindsManager::handleKeyboardShortcuts(EditorApi &api,
											  FileExplorer &files,
											  LSPClient &lsp,
											  NedTerminal &terminal)
{
	bool pressed = false;
	const ImGuiIO &io = ImGui::GetIO();
	const bool mod = io.KeyCtrl || io.KeySuper;

	// Cmd/Ctrl+T — fullscreen terminal (replaces explorer + editor).
	// Always handled so the terminal can be dismissed; when open, this is the
	// only app shortcut that may fire (keys go to the PTY otherwise).
	{
		ImGuiKey termKey = getActionKey("toggle_terminal");
		if (termKey == ImGuiKey_None)
			termKey = ImGuiKey_T; // default if keybinds.json omits it
		if (mod && ImGui::IsKeyPressed(termKey, false))
		{
			api.closeAllOverlays();
			api.save();
			files.showWelcomeScreen = false;
			terminal.toggle();
			return true;
		}
	}

	// Terminal owns the keyboard — skip sidebar/settings/LSP/etc.
	if (terminal.visible())
		return false;

	if (mod && ImGui::IsKeyPressed(getActionKey("toggle_sidebar"), false))
	{
		Splitter::showSidebar = !Splitter::showSidebar;
		settings.settings["sidebar_visible"] = Splitter::showSidebar;
		settings.saveSettings();
		pressed = true;
	}

	if (mod && ImGui::IsKeyPressed(getActionKey("toggle_settings_window"), false))
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
