#pragma once
#include "../lib/json.hpp"
#include "imgui.h"
#include <filesystem>
#include <map>
#include <string>

namespace fs = std::filesystem;
using json = nlohmann::json;

class EditorApi;
class FileExplorer;
class LSPClient;
class Settings;

// Loads ~/ned/config/keybinds.json into a map of action → ImGuiKey.
// handleKeyboardShortcuts() runs the global app shortcuts; peers passed at call time.
class KeybindsManager
{
  public:
	explicit KeybindsManager(Settings &settings);

	bool loadKeybinds();
	void checkKeybindsFile(); // re-read if the file changed on disk
	ImGuiKey getActionKey(const std::string &actionName) const;
	bool handleKeyboardShortcuts(EditorApi &api, FileExplorer &files, LSPClient &lsp);

  private:
	void ensureFileExists();
	void rebuildMap();
	void touchDiskTime();
	static ImGuiKey stringToImGuiKey(const std::string &keyString);

	Settings &settings;
	json keybinds = json::object();
	std::map<std::string, ImGuiKey> keys;
	std::string path;
	fs::file_time_type diskTime = fs::file_time_type::min();
};
