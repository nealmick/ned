#pragma once
#include "../lib/json.hpp"
#include "font.h"
#include "imgui.h"
#include "keybinds.h"
#include <filesystem>
#include <string>
#include <vector>

class EditorApi;
class FileExplorer;
class LSPClient;

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
	// Returns true if fonts/style were reapplied (atlas may have been rebuilt).
	bool apply(bool force, EditorApi &api);

	void ApplySettings(ImGuiStyle &style);

	KeybindsManager keybinds;
	Font font;

	bool showSettingsWindow = false;
	// True only when running as NedEmbed inside a host app. Standalone Ned leaves false.
	bool isEmbedded = false;

	ImVec2 embeddedWindowPos{200.0f, 200.0f};
	ImVec2 embeddedWindowSize{900.0f, 600.0f};
	bool embeddedWindowCollapsed{false};

	void renderSettingsWindow(EditorApi &api, FileExplorer &files, LSPClient &lsp);
	void toggleSettingsWindow(EditorApi &api);
	void toggleSidebar();
	void switchToProfile(const std::string &profileName);
	void renderNotification(const std::string &message, float duration = 2.0f);

  private:
	bool needsApply = false;
	std::string settingsPath; // e.g. ~/ned/config/amber.json
	fs::file_time_type diskTime = fs::file_time_type::min();

	void touchDiskTime();
	std::vector<std::string> listProfiles() const;
	static std::string primaryPath(); // ~/ned/config/ned.json (points at active profile)

	void renderSettingsContent(EditorApi &api, FileExplorer &files, LSPClient &lsp);
	void renderWindowHeader(EditorApi &api, FileExplorer &files);
	void renderProfileSelector();
	void renderMainSettings();
	void renderMacSettings();
	void renderSyntaxColors();
	void renderToggleSettings();
	void renderShaderSettings();
	void renderShaderSlider(const char *label,
							const char *key,
							float min_val,
							float max_val,
							const char *format,
							float default_val);
	void renderKeybindsSettings(FileExplorer &files, LSPClient &lsp);
	void handleWindowInput(EditorApi &api);
	void applyImGuiStyles();
	void closeSettingsWindow(EditorApi &api);

	static std::string displayFontName(const std::string &fontFile);
};
