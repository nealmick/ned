#pragma once
#include "settings.h"
#include "../editor/editor.h"
#include "../editor/editor_highlight.h" 
#include "../files/files.h"				
#include "../util/terminal.h"				
#include "config.h"
#include "imgui.h"
#include <GLFW/glfw3.h>

#include <algorithm> //
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iomanip> 
#include <iostream>
#include <libgen.h> 

#ifdef __APPLE__
#include <mach-o/dyld.h>
#include <sys/param.h>	 
#endif
#ifdef __linux__
#include <linux/limits.h>
#endif

#include <map>
#include <unistd.h>

namespace fs = std::filesystem;
Settings gSettings; 

Settings::Settings() : splitPos(0.3f) {
    // Initialize with default values that will be overwritten by loadSettings
    currentFontSize = 0.0f;
    loadSettings(); // Load settings immediately to set proper values
}

void Settings::loadSettings()
{
	settingsFileManager.loadSettings(settings, settingsPath);
	currentFontName = getCurrentFont();
	currentFontSize = settings.value("fontSize", 20.0f); // Use 20.0f as fallback if not found
	splitPos = settings.value("splitPos", 0.2142857164144516f);
	settingsChanged = false;
	themeChanged = false;
	fontChanged = false;
	fontSizeChanged = false;
	gTerminal.UpdateTerminalColors();
}

void Settings::saveSettings()
{
	settingsFileManager.saveSettings(settings, settingsPath);
}

void Settings::checkSettingsFile()
{
	settingsFileManager.checkSettingsFile(settingsPath, settings, settingsChanged, fontChanged, fontSizeChanged, themeChanged);
	if (settingsChanged || fontChanged || fontSizeChanged || themeChanged)
	{
		profileJustSwitched = true;
	}
}

void Settings::renderSettingsWindow()
{
	if (!showSettingsWindow)
		return;

	ImVec2 main_viewport_size = ImGui::GetMainViewport()->Size;
	float settings_window_width;
	if (main_viewport_size.x < 1000.0f)
	{
		settings_window_width = main_viewport_size.x * 0.95f;
	} else
	{
		settings_window_width = main_viewport_size.x * 0.75f;
	}
	ImVec2 window_size(settings_window_width, main_viewport_size.y * 0.85f);

	ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 1.0f);
	ImGui::SetNextWindowSize(window_size, ImGuiCond_Always);
	ImGui::SetNextWindowPos(ImVec2(ImGui::GetIO().DisplaySize.x * 0.5f,
								   ImGui::GetIO().DisplaySize.y * 0.5f),
							ImGuiCond_Always,
							ImVec2(0.5f, 0.5f));
	ImGuiWindowFlags windowFlags =
		ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
		ImGuiWindowFlags_AlwaysVerticalScrollbar | ImGuiWindowFlags_Modal;

	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 10.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.0f);

	ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(gSettings.getSettings()["backgroundColor"][0].get<float>()* .8,
				   gSettings.getSettings()["backgroundColor"][1].get<float>()* .8,
				   gSettings.getSettings()["backgroundColor"][2].get<float>()* .8,
				   1.0f));
	ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(gSettings.getSettings()["backgroundColor"][0].get<float>()* .5,
			   gSettings.getSettings()["backgroundColor"][1].get<float>()* .5,
			   gSettings.getSettings()["backgroundColor"][2].get<float>()* .5,
			   1.0f));

	ImGui::PushStyleColor(ImGuiCol_ScrollbarBg, ImVec4(gSettings.getSettings()["backgroundColor"][0].get<float>()* .5,
			   gSettings.getSettings()["backgroundColor"][1].get<float>()* .5,
			   gSettings.getSettings()["backgroundColor"][2].get<float>()* .5,
			   1.0f));

	ImGui::PushStyleColor(ImGuiCol_PopupBg, ImVec4(gSettings.getSettings()["backgroundColor"][0].get<float>()* .5,
	   gSettings.getSettings()["backgroundColor"][1].get<float>()* .5,
	   gSettings.getSettings()["backgroundColor"][2].get<float>()* .5,
	   1.0f));
	ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.3f, 0.3f, 0.3f, 1.0f));
	ImGui::PushStyleColor(ImGuiCol_ScrollbarGrab, ImVec4(0.3f, 0.3f, 0.3f, 1.0f));
	ImGui::PushStyleColor(ImGuiCol_ScrollbarGrabHovered, ImVec4(0.4f, 0.4f, 0.4f, 1.0f));
	ImGui::PushStyleColor(ImGuiCol_ScrollbarGrabActive, ImVec4(0.5f, 0.5f, 0.5f, 1.0f));
	ImGui::PushStyleVar(ImGuiStyleVar_ScrollbarSize, 14.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_ScrollbarRounding, 10.0f);
	ImGui::Begin("Settings", nullptr, windowFlags);

	renderWindowHeader();
	renderProfileSelector();
	renderMainSettings();
	renderMacSettings();
	renderSyntaxColors();
	renderToggleSettings();
	renderShaderSettings();
	handleWindowInput();

	ImGui::End();
	ImGui::PopStyleColor(8);
	ImGui::PopStyleVar(5);

	if (profileJustSwitched)
	{
		profileJustSwitched = false;
	}
}

void Settings::renderWindowHeader()
{
	static bool wasFocused = false;
	bool isFocused = ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);
	if (wasFocused && !isFocused && showSettingsWindow)
	{
		showSettingsWindow = false;
		if (settingsChanged)
			saveSettings();
	}
	wasFocused = isFocused;

	ImGui::BeginGroup();
	ImGui::TextUnformatted("Settings");
	float closeIconSize = ImGui::GetFrameHeight() - 10;
	ImVec2 contentAvail = ImGui::GetContentRegionAvail();
	float buttonPosX = contentAvail.x - closeIconSize - ImGui::GetStyle().FramePadding.x * 2;
	ImGui::SameLine(buttonPosX > 0 ? buttonPosX : ImGui::GetCursorPosX() + 100);

	ImVec2 cursor_pos = ImGui::GetCursorPos();
	if (ImGui::InvisibleButton("##close-settings", ImVec2(closeIconSize, closeIconSize)))
	{
		showSettingsWindow = false;
		if (settingsChanged)
			saveSettings();
	}
	bool isHovered = ImGui::IsItemHovered();
	ImGui::SetCursorPos(cursor_pos);
	ImTextureID closeIcon = gFileExplorer.getIcon("close");
	ImGui::Image(closeIcon,
				 ImVec2(closeIconSize, closeIconSize),
				 ImVec2(0, 0),
				 ImVec2(1, 1),
				 isHovered ? ImVec4(1, 1, 1, 0.6f) : ImVec4(1, 1, 1, 1));
	ImGui::EndGroup();
	ImGui::Separator();
	ImGui::Spacing();
}

void Settings::renderProfileSelector()
{
	std::vector<std::string> availableProfileFiles = settingsFileManager.getAvailableProfileFiles();
	std::string currentActiveFilename = "ned.json";

	if (!settingsPath.empty())
	{
		currentActiveFilename = fs::path(settingsPath).filename().string();
	}

	bool foundCurrentInList = false;
	for (const auto &fname : availableProfileFiles)
	{
		if (fname == currentActiveFilename)
		{
			foundCurrentInList = true;
			break;
		}
	}
	if (!foundCurrentInList && !currentActiveFilename.empty())
	{
		availableProfileFiles.insert(availableProfileFiles.begin(), currentActiveFilename);
	}

	if (ImGui::BeginCombo("##ActiveSettingsFileCombo", currentActiveFilename.c_str()))
	{
		for (const std::string &filename : availableProfileFiles)
		{
			bool isSelected = (currentActiveFilename == filename);
			if (ImGui::Selectable(filename.c_str(), isSelected))
			{
				if (currentActiveFilename != filename)
				{
					std::cout << "[Settings] User selected new profile: " << filename << std::endl;
					settingsFileManager.switchProfile(filename, settings, settingsPath, settingsChanged, fontChanged, themeChanged);
					profileJustSwitched = true;
					settingsChanged = true;
					fontChanged = true;
					settingsChanged = true;
					gEditorHighlight.forceColorUpdate();
				}
			}
			if (isSelected)
			{
				ImGui::SetItemDefaultFocus();
			}
		}
		ImGui::EndCombo();
	}
	ImGui::SameLine();
	ImGui::TextUnformatted("Profile");
	ImGui::Spacing();
}

void Settings::renderMainSettings()
{
	std::string displayPreviewName = getCurrentFont();
	if (ImGui::IsWindowAppearing() || fontChanged || profileJustSwitched)
	{
		currentFontName = getCurrentFont();
	}

	if (displayPreviewName != "System Default" && displayPreviewName.find('.') != std::string::npos)
	{
		displayPreviewName = displayPreviewName.substr(0, displayPreviewName.find_last_of("."));
		std::replace(displayPreviewName.begin(), displayPreviewName.end(), '-', ' ');
	}

	if (ImGui::BeginCombo("Font", displayPreviewName.c_str()))
	{
		for (const auto &fontFile : fontNames)
		{
			bool isSelected = (fontFile == currentFontName);
			std::string displayName = fontFile;
			if (fontFile != "System Default" && fontFile.find('.') != std::string::npos)
			{
				displayName = fontFile.substr(0, fontFile.find_last_of("."));
				std::replace(displayName.begin(), displayName.end(), '-', ' ');
			}
			if (ImGui::Selectable(displayName.c_str(), isSelected))
			{
				if (currentFontName != fontFile)
				{
					settings["font"] = fontFile;
					currentFontName = fontFile;
					fontChanged = true;
					settingsChanged = true;
					saveSettings();
				}
			}
			if (isSelected)
				ImGui::SetItemDefaultFocus();
		}
		ImGui::EndCombo();
	}
	ImGui::Spacing();

	static float tempFontSize;
	if (ImGui::IsWindowAppearing() || fontSizeChanged || profileJustSwitched)
	{
		tempFontSize = settings.value("fontSize", 20.0f); // Get the value directly from settings
	}
	if (ImGui::SliderFloat("Font Size", &tempFontSize, 4.0f, 32.0f, "%.0f"))
	{
		if (settings.value("fontSize", 20.0f) != tempFontSize)
		{
			settingsChanged = true;
		}
	}
	if (ImGui::IsItemDeactivatedAfterEdit())
	{
		if (getFontSize() != tempFontSize)
		{
			setFontSize(tempFontSize);
			saveSettings();
		} else if (settingsChanged)
		{
			saveSettings();
		}
	}

	ImGui::Spacing();

	ImVec4 bgColor;
	if (settings.contains("backgroundColor") && settings["backgroundColor"].is_array() &&
		settings["backgroundColor"].size() == 4)
	{
		auto &bgArray = settings["backgroundColor"];
		bgColor = ImVec4(bgArray[0].get<float>(),
						 bgArray[1].get<float>(),
						 bgArray[2].get<float>(),
						 bgArray[3].get<float>());
	} else
	{
		bgColor = ImVec4(0.058f, 0.194f, 0.158f, 1.0f);
	}

	if (ImGui::ColorEdit4("Background Color", (float *)&bgColor))
	{
		if (settings["backgroundColor"][0].get<float>() != bgColor.x ||
			settings["backgroundColor"][1].get<float>() != bgColor.y ||
			settings["backgroundColor"][2].get<float>() != bgColor.z ||
			settings["backgroundColor"][3].get<float>() != bgColor.w)
		{
			settings["backgroundColor"] = {bgColor.x, bgColor.y, bgColor.z, bgColor.w};
			if (!settingsChanged) settingsChanged = true;
		}
	}

	if (ImGui::IsItemDeactivatedAfterEdit())
	{
		if (settings["backgroundColor"][0].get<float>() != bgColor.x ||
			settings["backgroundColor"][1].get<float>() != bgColor.y ||
			settings["backgroundColor"][2].get<float>() != bgColor.z ||
			settings["backgroundColor"][3].get<float>() != bgColor.w)
		{
			settings["backgroundColor"] = {bgColor.x, bgColor.y, bgColor.z, bgColor.w};
			if (!settingsChanged) settingsChanged = true;
		}
		
		if (settingsChanged)
		{
			std::cout << "[Settings] Background Color edit finished. Saving." << std::endl;
			saveSettings();
		}
	}
}

void Settings::renderMacSettings()
{
#ifdef __APPLE__
	ImGui::Spacing();
	ImGui::TextUnformatted("macOS Settings");
	ImGui::Separator();
	ImGui::Spacing();

	float currentOpacity = settings.value("mac_background_opacity", 0.5f);
	bool currentBlurEnabled = settings.value("mac_blur_enabled", true);

	if (ImGui::SliderFloat("Background Opacity", &currentOpacity, 0.0f, 1.0f, "%.2f")) {
		settings["mac_background_opacity"] = currentOpacity;
		settingsChanged = true;
		saveSettings();
	}

	if (ImGui::Checkbox("Enable Background Blur", &currentBlurEnabled)) {
		settings["mac_blur_enabled"] = currentBlurEnabled;
		settingsChanged = true;
		saveSettings();
	}
#endif
}

void Settings::renderSyntaxColors()
{
	std::string currentThemeName = getCurrentTheme();
	if (ImGui::IsWindowAppearing() || themeChanged || profileJustSwitched)
	{
	}

	if (settings.contains("themes") && settings["themes"].is_object() &&
		settings["themes"].contains(currentThemeName))
	{
		auto &themeColorsJson = settings["themes"][currentThemeName];
				
		auto editThemeColor = [&](const char *label, const char *colorKey) {
			if (!themeColorsJson.contains(colorKey) || !themeColorsJson[colorKey].is_array() ||
				themeColorsJson[colorKey].size() != 4)
			{
				return;
			}
			auto &colorArray = themeColorsJson[colorKey];
			ImVec4 color(colorArray[0].get<float>(),
						 colorArray[1].get<float>(),
						 colorArray[2].get<float>(),
						 colorArray[3].get<float>());

			ImGui::TextUnformatted(label);
			ImGui::SameLine(200);

			if (ImGui::ColorEdit4(("##" + std::string(colorKey)).c_str(), (float *)&color))
			{
				themeColorsJson[colorKey] = {color.x, color.y, color.z, color.w};
				if (!settingsChanged) settingsChanged = true;
				if (!themeChanged) themeChanged = true;
			}

			if (ImGui::IsItemDeactivatedAfterEdit())
			{
				if (themeColorsJson[colorKey][0].get<float>() != color.x ||
					themeColorsJson[colorKey][1].get<float>() != color.y ||
					themeColorsJson[colorKey][2].get<float>() != color.z ||
					themeColorsJson[colorKey][3].get<float>() != color.w)
				{
					themeColorsJson[colorKey] = {color.x, color.y, color.z, color.w};
					if (!settingsChanged) settingsChanged = true;
					if (!themeChanged) themeChanged = true;
				}
				
				std::cout << "[Settings] Color " << colorKey << " edit finished. Forcing update and saving." << std::endl;
				gEditorHighlight.forceColorUpdate();
				saveSettings();
			}
		};
		ImGui::Spacing();
		ImGui::TextUnformatted("Syntax Colors");
		ImGui::Separator();
		ImGui::Spacing();
		editThemeColor("Text Color", "text");
		editThemeColor("Keywords", "keyword");
		editThemeColor("Strings", "string");
		editThemeColor("Numbers", "number");
		editThemeColor("Comments", "comment");
		editThemeColor("Functions", "function");
		editThemeColor("Types", "type");
		editThemeColor("Identifier", "variable");
	} else
	{
		ImGui::Text("Current theme '%s' not found or themes object missing.",
					currentThemeName.c_str());
	}
}

void Settings::renderToggleSettings()
{
	ImGui::Spacing();
	ImGui::TextUnformatted("Toggle Settings");
	ImGui::Separator();
	ImGui::Spacing();

	bool rainbowMode = getRainbowMode();
	if (ImGui::Checkbox("Rainbow Mode", &rainbowMode))
	{
		settings["rainbow"] = rainbowMode;
		settingsChanged = true;
		saveSettings();
	}
	ImGui::SameLine();
	ImGui::TextDisabled("(Rainbow cursor & line numbers)");

	bool treesitterMode = getTreesitterMode();
	if (ImGui::Checkbox("TreeSitter Mode", &treesitterMode))
	{
		settings["treesitter"] = treesitterMode;
		settingsChanged = true;
		editor_state.text_changed = true;
		saveSettings();
	}
	ImGui::SameLine();
	ImGui::TextDisabled("(Syntax Highlighting)");
}

void Settings::renderShaderSettings()
{
	ImGui::Spacing();
	ImGui::TextUnformatted("GL Shaders");
	ImGui::Separator();
	ImGui::Spacing();

	bool shaderEnabled = settings.value("shader_toggle", true);
	if (ImGui::Checkbox("Enable Shader Effects", &shaderEnabled))
	{
		settings["shader_toggle"] = shaderEnabled;
		settingsChanged = true;
		saveSettings();
	}
	ImGui::SameLine();
	ImGui::TextDisabled("(CRT & visual effects)");

	renderShaderSlider("Scanline", "scanline_intensity", 0.00f, 1.00f, "%.02f", 0.20f);
	renderShaderSlider("Vignette", "vignet_intensity", 0.00f, 1.00f, "%.02f", 0.25f);
	renderShaderSlider("Bloom", "bloom_intensity", 0.00f, 1.00f, "%.02f", 0.75f);
	renderShaderSlider("Static", "static_intensity", 0.00f, 0.5f, "%.03f", 0.208f);
	renderShaderSlider("RGB Shift", "colorshift_intensity", 0.0f, 10.0f, "%.02f", 0.90f);
	renderShaderSlider("Curvature(bugged)", "curvature_intensity", 0.0f, 0.5f, "%.02f", 0.0f);
	renderShaderSlider("Burn-in", "burnin_intensity", 0.9f, 0.999f, "%.03f", 0.9525f);
	renderShaderSlider("Jitter", "jitter_intensity", 0.0f, 10.0f, "%.02f", 2.81f);
	renderShaderSlider("Pixel lines", "pixelation_intensity", -1.00f, 1.00f, "%.03f", -0.11f);
}

void Settings::renderShaderSlider(const char* label, const char* key, float min_val, float max_val, 
	const char* format, float default_val)
{
	static std::map<std::string, float> temp_shader_values;

	if (ImGui::IsWindowAppearing() || profileJustSwitched)
	{
		if (profileJustSwitched)
			temp_shader_values.clear();
	}
	if (temp_shader_values.find(key) == temp_shader_values.end() || profileJustSwitched)
	{
		temp_shader_values[key] = settings.value(key, default_val);
	}

	float &temp_val = temp_shader_values[key];

	if (ImGui::SliderFloat(
			label, &temp_val, min_val, max_val, format, ImGuiSliderFlags_AlwaysClamp))
	{
		if (settings.value(key, default_val) != temp_val)
		{
			settingsChanged = true;
		}
	}
	if (ImGui::IsItemDeactivatedAfterEdit())
	{
		if (settings.value(key, default_val) != temp_val)
		{
			settings[key] = temp_val;
			settingsChanged = true;
			saveSettings();
		} else if (settingsChanged)
		{
		}
	}
	ImGui::Spacing();
}

void Settings::handleWindowInput()
{
	if (ImGui::IsKeyPressed(ImGuiKey_Escape))
	{
		showSettingsWindow = false;
		if (settingsChanged)
			saveSettings();
	}
	if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) && !ImGui::IsAnyItemHovered() &&
		!ImGui::IsWindowHovered(ImGuiHoveredFlags_AnyWindow |
								ImGuiHoveredFlags_AllowWhenBlockedByPopup))
	{
		ImVec2 mousePos = ImGui::GetMousePos();
		ImVec2 windowPos = ImGui::GetWindowPos();
		ImVec2 windowSize = ImGui::GetWindowSize();
		if (mousePos.x < windowPos.x || mousePos.x > windowPos.x + windowSize.x ||
			mousePos.y < windowPos.y || mousePos.y > windowPos.y + windowSize.y)
		{
			if (!ImGui::IsPopupOpen(nullptr, ImGuiPopupFlags_AnyPopupId))
			{
				showSettingsWindow = false;
				if (settingsChanged)
					saveSettings();
			}
		}
	}
}