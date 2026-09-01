#include "keybinds.h"
#include "settings.h"
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
		const NedKey key = stringToNedKey(value.get<std::string>());
		if (key != NedKey::None)
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
		settings.showNotification(
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

NedKey KeybindsManager::getActionKey(const std::string &actionName) const
{
	const auto it = keys.find(actionName);
	return it != keys.end() ? it->second : NedKey::None;
}
