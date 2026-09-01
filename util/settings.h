#pragma once

#ifndef NED_ENABLE_GIT
#define NED_ENABLE_GIT 1
#endif
#ifndef NED_ENABLE_LSP
#define NED_ENABLE_LSP 1
#endif
#ifndef NED_ENABLE_SHADERS
#define NED_ENABLE_SHADERS 1
#endif
#include "../lib/json.hpp"
#include "../editor/platform/ned_types.h"
#include "keybinds.h"
#include <filesystem>
#include <string>
#include <vector>

class EditorApi;
class FileExplorer;
class LspImGuiView;

namespace fs = std::filesystem;
using json = nlohmann::json;

// Owns the active profile JSON. Everyone reads/writes `settings` directly.
// Menus mutate it; call requestApply() then apply() reloads fonts/style/theme.
// Peers (editor / files / lsp) are arguments at call sites — no late attach.
class Settings
{
  public:
	// Paths / JSON helpers used by keybinds, fonts, LSP, etc.
	static std::string getAppResourcesPath();
	static std::string getUserConfigDir(); // ~/ned/config
	static bool readJson(const std::string &path, json &out);
	static bool writeJson(const std::string &path, const json &data);

	Settings();

	// The active profile — read and write this directly.
	json settings;

	void loadSettings();
	void saveSettings();
	void checkSettingsFile(); // re-read profile if someone edited it on disk
	void requestApply() { needsApply = true; }

	KeybindsManager keybinds;

	bool showSettingsWindow = false;
	// True for WorkbenchHostMode::Floating (embed). Fullscreen standalone leaves false.
	bool isEmbedded = false;
	// File-tree sidebar visibility (replaces old Splitter::showSidebar).
	bool sidebarVisible = true;
	// Bottom terminal panel visibility (Cmd/Ctrl+T).
	bool terminalVisible = true;

	NedVec2 embeddedWindowPos{200.0f, 200.0f};
	NedVec2 embeddedWindowSize{900.0f, 600.0f};
	bool embeddedWindowCollapsed{false};

	void toggleSettingsWindow(EditorApi &api);

	// Notification toast state (drawn by the UI backend's SettingsView).
	void showNotification(const std::string &message, float duration = 2.0f)
	{
		notificationText = message;
		notificationTimer = duration;
	}
	void toggleSidebar();
	void toggleTerminal();
	void switchToProfile(const std::string &profileName);

  private:
	friend class SettingsView;
	bool needsApply = false;
	std::string notificationText;
	float notificationTimer = 0.0f;
	std::string settingsPath; // e.g. ~/ned/config/amber.json
	fs::file_time_type diskTime = fs::file_time_type::min();

	void touchDiskTime();
	static bool loadBundledProfile(json &out, std::string &outPath);
	std::vector<std::string> listProfiles() const;
	static std::string primaryPath(); // ~/ned/config/ned.json (points at active profile)
};
