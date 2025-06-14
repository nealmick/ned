#pragma once
#include "../lib/json.hpp"
#include "close_popper.h"
#include "settings_file_manager.h"
#include <filesystem>
#include <string>
#include <vector>

namespace fs = std::filesystem;
using json = nlohmann::json;

class Settings
{
  public:
	// --- STATIC HELPERS: publicly accessible ---
	static std::string getAppResourcesPath() { return SettingsFileManager::getAppResourcesPath(); }
	static std::string getUserSettingsPath() { return SettingsFileManager::getUserSettingsPath(); }

	// --- Normal members & methods ---
	Settings();
	void loadSettings();
	void saveSettings();
	void checkSettingsFile();

	json &getSettings() { return settings; }

	float getSplitPos() const { return splitPos; }
	void setSplitPos(float pos)
	{
		splitPos = pos;
		settings["splitPos"] = pos;
		settingsChanged = true;
	}

	bool hasSettingsChanged() const { return settingsChanged; }
	void resetSettingsChanged() { settingsChanged = false; }

	std::string getCurrentTheme() const
	{
		if (settings.contains("theme") && settings["theme"].is_string())
		{
			return settings["theme"].get<std::string>();
		}
		return "default"; // Fallback
	}
	bool hasThemeChanged() const { return themeChanged; }
	void resetThemeChanged() { themeChanged = false; }

	std::string getCurrentFont() const
	{
		if (settings.contains("font") && settings["font"].is_string())
		{
			return settings["font"].get<std::string>();
		}
		return "SourceCodePro-Regular"; // Fallback
	}
	bool hasFontChanged() const { return fontChanged; }
	void resetFontChanged() { fontChanged = false; }

	float getFontSize() const { return currentFontSize; }
	void setFontSize(float size)
	{
		if (size != currentFontSize)
		{
			settings["fontSize"] = size;
			currentFontSize = size;
			fontSizeChanged = true;
			settingsChanged = true;
		}
	}
	bool hasFontSizeChanged() const { return fontSizeChanged; }
	void resetFontSizeChanged() { fontSizeChanged = false; }

	bool showSettingsWindow = false;
	void renderSettingsWindow();
	void toggleSettingsWindow()
	{
		showSettingsWindow = !showSettingsWindow;
		if (showSettingsWindow)
		{
			ClosePopper::closeAllExcept(ClosePopper::Type::Settings);
		}
		blockInput = showSettingsWindow;
	}
	bool isBlockingInput() const { return blockInput; }

	bool getRainbowMode() const
	{
		if (settings.contains("rainbow") && settings["rainbow"].is_boolean())
		{
			return settings["rainbow"].get<bool>();
		}
		return true; // Fallback
	}
	bool getTreesitterMode() const
	{
		if (settings.contains("treesitter") && settings["treesitter"].is_boolean())
		{
			return settings["treesitter"].get<bool>();
		}
		return true; // Fallback
	}

	std::vector<std::string> fontNames = {
		"IBM_MDA",
		"SourceCodePro-Regular",
		"NotoSansMono-Regular",
		"NotoSansMono-Thin",
		"NotoSansMono-Light",
		"VT323-Regular",
		"VT100",
		"Commodore64",
		"Apple2",
		"JetBrainsMonoNL-Regular",
	};
	std::string currentFontName;
	bool profileJustSwitched = false; // Flag to indicate a settings profile was changed
	
	void renderNotification(const std::string& message, float duration = 2.0f);

  private:
	json settings;			  // Holds the settings from the *active* file
	std::string settingsPath; // Path to the *active* settings file (e.g., ned.json or test.json)
	float splitPos = 0.3f;	  // Default, will be overwritten by loaded settings

	bool settingsChanged = false;
	bool themeChanged = false;
	bool fontChanged = false;
	bool fontSizeChanged = false;
	bool blockInput = false;

	float currentFontSize = 0.0f; // Will be set by loadSettings()
	int settingsCheckFrameCounter = 0;
	const int SETTINGS_CHECK_INTERVAL = 60; // frames

	SettingsFileManager settingsFileManager; // Handles all file operations

	// Helper functions for rendering different sections of the settings window
	void renderWindowHeader();
	void renderProfileSelector();
	void renderMainSettings();
	void renderMacSettings();
	void renderSyntaxColors();
	void renderToggleSettings();
	void renderShaderSettings();
	void renderShaderSlider(const char* label, const char* key, float min_val, float max_val, 
		const char* format, float default_val);
	void renderKeybindsSettings();
	void handleWindowInput();
	void applyImGuiStyles(); // New function for handling ImGui styles
};

extern Settings gSettings;