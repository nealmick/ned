#pragma once
#include "../lib/json.hpp"
#include "close_popper.h"
#include <filesystem>
#include <string>
#include <vector>

namespace fs = std::filesystem;
using json = nlohmann::json;

class Settings
{
  public:
	// --- STATIC HELPERS: publicly accessible ---
	static std::string getAppResourcesPath();
	static std::string getUserSettingsPath(); // Points to the primary ned.json

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
	};
	std::string currentFontName;
	bool profileJustSwitched = false; // Flag to indicate a settings profile was changed

  private:
	json settings;			  // Holds the settings from the *active* file
	std::string settingsPath; // Path to the *active* settings file (e.g., ned.json or test.json)
	float splitPos = 0.3f;	  // Default, will be overwritten by loaded settings

	fs::file_time_type lastSettingsModification;

	bool settingsChanged = false;
	bool themeChanged = false;
	bool fontChanged = false;
	bool fontSizeChanged = false;
	bool blockInput = false;

	float currentFontSize = 16.0f; // Default, will be overwritten
	int settingsCheckFrameCounter = 0;
	const int SETTINGS_CHECK_INTERVAL = 60; // frames
};

extern Settings gSettings;