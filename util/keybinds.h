#pragma once
#include "../lib/json.hpp"
#include "../editor/platform/ned_key.h"
#include <map>
#include <string>
#include <filesystem>
#include <map>
#include <string>

namespace fs = std::filesystem;
using json = nlohmann::json;

class Settings;

// Loads ~/ned/config/keybinds.json into a map of action → NedKey.
// Backend-neutral: hosts poll their own toolkit and convert.
// handleKeyboardShortcuts() runs the global app shortcuts; peers passed at call time.
class KeybindsManager
{
  public:
	explicit KeybindsManager(Settings &settings);

	bool loadKeybinds();
	void checkKeybindsFile(); // re-read if the file changed on disk
	NedKey getActionKey(const std::string &actionName) const;


  private:
	void ensureFileExists();
	void rebuildMap();
	void touchDiskTime();


	Settings &settings;
	json keybinds = json::object();
	std::map<std::string, NedKey> keys;
	std::string path;
	fs::file_time_type diskTime = fs::file_time_type::min();
};
