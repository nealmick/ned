#pragma once
#include "../lib/json.hpp"
#include "close_popper.h"
#include "imgui.h"
#include "settings_file_manager.h"
#include <filesystem>
#include <functional>
#include <string>
#include <vector>

// Forward declaration
class ShaderManager;

namespace fs = std::filesystem;
using json = nlohmann::json;

class Settings
{
  public:
	// --- STATIC HELPERS: publicly accessible ---
	static std::string getAppResourcesPath()
	{
		return SettingsFileManager::getAppResourcesPath();
	}
	static std::string getUserSettingsPath()
	{
		return SettingsFileManager::getUserSettingsPath();
	}

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

	float getAgentSplitPos() const { return agentSplitPos; }
	void setAgentSplitPos(float pos)
	{
		agentSplitPos = pos;
		settings["agent_split_pos"] = pos;
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

	bool isAgentSplitPosProcessed() const { return agentSplitPosProcessed; }

	// Toggle functions for sidebar and agent pane
	void toggleSidebar();
	void toggleAgentPane();

	bool showSettingsWindow = false;
	bool isEmbedded = false; // Flag to indicate if running in embedded mode
	void setEmbedded(bool embedded) { isEmbedded = embedded; }

	// Embedded settings window state (similar to terminal)
	ImVec2 embeddedWindowPos{200.0f, 200.0f};
	ImVec2 embeddedWindowSize{900.0f, 600.0f};
	bool embeddedWindowCollapsed{false};
	void renderSettingsWindow();
	void renderSettingsContent(); // New method for rendering settings content
	void toggleSettingsWindow();
	bool isBlockingInput() const { return blockInput; }

	// Public method to switch profile
	void switchToProfile(const std::string &profileName);

	// Get current profile name
	std::string getCurrentProfileName() const;

	// Get current theme text color
	ImVec4 getCurrentTextColor() const;

	// Get current background color
	ImVec4 getCurrentBackgroundColor() const;

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

	bool getAIAutocompleteMode() const
	{
		if (settings.contains("ai_autocomplete") &&
			settings["ai_autocomplete"].is_boolean())
		{
			return settings["ai_autocomplete"].get<bool>();
		}
		return true; // Fallback
	}

	std::string getAgentModel() const
	{
		if (settings.contains("agent_model") && settings["agent_model"].is_string())
		{
			return settings["agent_model"].get<std::string>();
		}
		return "deepseek/deepseek-chat-v3-0324"; // Fallback
	}

	std::string getCompletionModel() const
	{
		if (settings.contains("completion_model") &&
			settings["completion_model"].is_string())
		{
			return settings["completion_model"].get<std::string>();
		}
		return "meta-llama/llama-4-scout"; // Fallback
	}

	std::vector<std::string> fontNames = {
		"SourceCodePro-Regular",
		"JetBrainsMonoNL-Regular",
		"NotoSansMono-Regular",
		"NotoSansMono-Thin",
		"NotoSansMono-Light",
		"VT323-Regular",
		"IBM_MDA",
		"VT100",
	};
	std::string currentFontName;
	bool profileJustSwitched = false; // Flag to indicate a settings profile was changed

	void renderNotification(const std::string &message, float duration = 2.0f);

	// Method to apply settings to ImGui style
	void ApplySettings(ImGuiStyle &style);

	void handleSettingsChanges(bool &needFontReload,
							   bool &m_needsRedraw,
							   int &m_framesToRender,
							   std::function<void(bool)> setShaderEnabled,
							   float &lastOpacity,
							   bool &lastBlurEnabled,
							   bool force = false);

  private:
	json settings;				 // Holds the settings from the *active* file
	std::string settingsPath;	 // Path to the *active* settings file (e.g.,
								 // ned.json or test.json)
	float splitPos = 0.3f;		 // Default, will be overwritten by loaded settings
	float agentSplitPos = 0.75f; // Default, will be overwritten by loaded settings

	bool settingsChanged = false;
	bool themeChanged = false;
	bool fontChanged = false;
	bool fontSizeChanged = false;
	bool blockInput = false;
	bool agentSplitPosProcessed =
		false; // Track if we've processed agent split pos for current file

	float currentFontSize = 0.0f; // Will be set by loadSettings()
	int settingsCheckFrameCounter = 0;

	SettingsFileManager settingsFileManager; // Handles all file operations

	// Helper functions for rendering different sections of the settings window
	void renderWindowHeader();
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
	void renderKeybindsSettings();
	void handleWindowInput();
	void applyImGuiStyles(); // New function for handling ImGui styles
	void renderOpenRouterKeyInput();
};

extern Settings gSettings;