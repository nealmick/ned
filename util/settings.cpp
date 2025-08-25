#include "settings.h"
#include "../ai/ai_tab.h"
#include "../editor/editor.h"
#include "../editor/editor_highlight.h"
#include "../files/files.h"
#include "../lsp/lsp_dashboard.h"
#include "../util/font.h"
#include "../util/keybinds.h"
#include "../util/splitter.h"
#include "../util/terminal.h"
#include "config.h"
#include "imgui.h"
#include "imgui_internal.h"
#include <GLFW/glfw3.h>

// Add includes for macOS window functions
#ifdef __APPLE__
#include "../macos_window.h"
#endif

#include <algorithm> //
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#ifndef PLATFORM_WINDOWS
#include <libgen.h>
#endif

#ifdef __APPLE__
#include <mach-o/dyld.h>
#include <sys/param.h>
#endif
#ifdef __linux__
#include <linux/limits.h>
#endif

#include <map>
#ifndef PLATFORM_WINDOWS
#include <unistd.h>
#endif

namespace fs = std::filesystem;
extern Settings gSettings;

Settings::Settings() : splitPos(0.3f)
{
	// Initialize with default values that will be overwritten by loadSettings
	currentFontSize = 0.0f;
	loadSettings(); // Load settings immediately to set proper values
}

void Settings::loadSettings()
{
	// Store the previous settings path to detect if we're loading a different file
	std::string previousSettingsPath = settingsPath;

	settingsFileManager.loadSettings(settings, settingsPath);
	currentFontName = getCurrentFont();
	currentFontSize =
		settings.value("fontSize", 20.0f); // Use 20.0f as fallback if not found
	splitPos = settings.value("splitPos", 0.2142857164144516f);
	agentSplitPos = settings.value("agent_split_pos", 0.75f);

	// On first load of this specific settings file, always reset agent split
	// pos to 0.3
	if (previousSettingsPath != settingsPath)
	{
		agentSplitPosProcessed = false;
	}
	if (!agentSplitPosProcessed && !settings.value("sidebar_visible", true))
	{
		float originalValue = agentSplitPos;
		agentSplitPos = 0.7f;
		settings["agent_split_pos"] = agentSplitPos;
		settingsChanged = true;
		std::cout << "[Settings] Reset agent_split_pos from " << originalValue
				  << " to 0.3 on first load of " << settingsPath << std::endl;
		settingsFileManager.saveSettings(settings, settingsPath);
	}
	agentSplitPosProcessed = true;

	// **changed**: Add initial save to ensure proper state synchronization
	// This fixes the background color picker issue when it's the first setting changed
	if (previousSettingsPath != settingsPath)
	{
		// This is a new settings file being loaded, save it to ensure proper state
		settingsFileManager.saveSettings(settings, settingsPath);
		std::cout << "[Settings] Initial save after loading new settings file: "
				  << settingsPath << std::endl;
	}

	settingsChanged = false;
	themeChanged = false;
	fontChanged = false;
	fontSizeChanged = false;
	gTerminal.UpdateTerminalColors();
}

void Settings::saveSettings()
{
	settingsFileManager.saveSettings(settings, settingsPath);
	gTerminal.UpdateTerminalColors();
}

void Settings::checkSettingsFile()
{
	settingsFileManager.checkSettingsFile(settingsPath,
										  settings,
										  settingsChanged,
										  fontChanged,
										  fontSizeChanged,
										  themeChanged);
	if (settingsChanged || fontChanged || fontSizeChanged || themeChanged)
	{
		profileJustSwitched = true;
	}
}

void Settings::renderSettingsWindow()
{
	if (!showSettingsWindow)
		return;

	if (isEmbedded)
	{
		// In embedded mode, create a draggable, resizable window with decoration
		ImGui::SetNextWindowPos(embeddedWindowPos, ImGuiCond_FirstUseEver);
		ImGui::SetNextWindowSize(embeddedWindowSize, ImGuiCond_FirstUseEver);

		// Create a proper window with title bar, close button, and resize handles
		// but disable collapsible behavior in embedded mode
		bool windowOpen = true;
		bool windowCreated =
			ImGui::Begin("Settings",
						 &windowOpen,
						 ImGuiWindowFlags_NoCollapse); // Disable collapsible behavior

		if (windowCreated)
		{
			// Update our stored position and size for persistence
			embeddedWindowPos = ImGui::GetWindowPos();
			embeddedWindowSize = ImGui::GetWindowSize();

			// If window was closed, hide the settings
			if (!windowOpen)
			{
				showSettingsWindow = false;
			}

			// Always render settings content since window is not collapsible
			renderSettingsContent();

			// Always call End() if Begin() was called
			ImGui::End();
		}
	} else
	{
		// Standalone mode - use the original modal popup approach
		float settings_window_width;
		float settings_window_height;
		float current_font_size = settings.value("fontSize", 20.0f);

		// Standalone mode - use viewport size
		ImVec2 main_viewport_size = ImGui::GetMainViewport()->Size;

		// Dynamic sizing based on viewport width and font size
		if (main_viewport_size.x < 1100.0f || current_font_size > 40)
		{
			settings_window_width = main_viewport_size.x * 0.90f; // 90% of viewport width
			settings_window_height =
				main_viewport_size.y * 0.80f; // 80% of viewport height
		} else
		{
			settings_window_width = main_viewport_size.x * 0.75f; // 75% of viewport width
			settings_window_height =
				main_viewport_size.y * 0.85f; // 85% of viewport height
		}

		ImVec2 window_size(settings_window_width, settings_window_height);

		ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 1.0f);
		ImGui::SetNextWindowSize(window_size, ImGuiCond_Always);

		// Standalone mode - center on display
		ImGui::SetNextWindowPos(ImVec2(ImGui::GetIO().DisplaySize.x * 0.5f,
									   ImGui::GetIO().DisplaySize.y * 0.5f),
								ImGuiCond_Always,
								ImVec2(0.5f, 0.5f));
		ImGuiWindowFlags windowFlags =
			ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
			ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar |
			ImGuiWindowFlags_Modal;

		applyImGuiStyles();
		ImGui::Begin("Settings", nullptr, windowFlags);

		// Push the current font to ensure settings window uses the updated font
		extern Font gFont;
		ImGui::PushFont(gFont.currentFont);

		renderSettingsContent();

		ImGui::PopFont(); // Pop the font we pushed
		ImGui::End();
		ImGui::PopStyleColor(8);
		ImGui::PopStyleVar(6);
	}

	if (profileJustSwitched)
	{
		profileJustSwitched = false;
	}
}

void Settings::renderSettingsContent()
{
	// Fixed header section with padding - only show in standalone mode
	if (!isEmbedded)
	{
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
		renderWindowHeader();
		ImGui::PopStyleVar();
	}

	// Calculate remaining space for scrollable content
	ImVec2 contentSize = ImGui::GetContentRegionAvail();
	float contentHeight = contentSize.y; // Use full available height instead of
										 // subtracting header height

	// Create a child window for scrollable content with padding
	ImGuiWindowFlags childFlags = ImGuiWindowFlags_AlwaysVerticalScrollbar;

	// Set the child window background color to match the main window
	auto &bgColor = settings["backgroundColor"];
	float bgR = bgColor[0].get<float>();
	float bgG = bgColor[1].get<float>();
	float bgB = bgColor[2].get<float>();
	const float WINDOW_BG_MULTIPLIER = 0.8f;

	ImGui::PushStyleColor(ImGuiCol_ChildBg,
						  ImVec4(bgR * WINDOW_BG_MULTIPLIER,
								 bgG * WINDOW_BG_MULTIPLIER,
								 bgB * WINDOW_BG_MULTIPLIER,
								 1.0f));

	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,
						ImVec2(15.0f, 5.0f)); // Reduced bottom padding from 15.0f to 5.0f
	ImGui::BeginChild("SettingsContent", ImVec2(0, contentHeight), false, childFlags);

	renderProfileSelector();
	renderMainSettings();
	if (!isEmbedded)
	{
		renderMacSettings();
	}
	renderOpenRouterKeyInput();

	renderSyntaxColors();

	renderToggleSettings();

	if (!isEmbedded)
	{
		renderShaderSettings();
	}

	renderKeybindsSettings();

	ImGui::EndChild();
	ImGui::PopStyleColor(); // Pop the child background color
	ImGui::PopStyleVar();

	handleWindowInput();
}

void Settings::applyImGuiStyles()
{
	// Style variables
	const float WINDOW_BG_MULTIPLIER = 0.8f;
	const float FRAME_BG_MULTIPLIER = 0.5f;
	const float BORDER_COLOR = 0.3f;
	const float SCROLLBAR_GRAB_HOVER_COLOR = 0.4f;

	// Get background color from settings
	auto &bgColor = settings["backgroundColor"];
	float bgR = bgColor[0].get<float>();
	float bgG = bgColor[1].get<float>();
	float bgB = bgColor[2].get<float>();

	// Apply window styles
	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 10.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_ScrollbarSize, 14.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_ScrollbarRounding, 10.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,
						ImVec2(15.0f,
							   15.0f)); // Left/top/bottom padding, no right padding

	// Apply colors
	ImGui::PushStyleColor(ImGuiCol_WindowBg,
						  ImVec4(bgR * WINDOW_BG_MULTIPLIER,
								 bgG * WINDOW_BG_MULTIPLIER,
								 bgB * WINDOW_BG_MULTIPLIER,
								 1.0f));

	ImGui::PushStyleColor(ImGuiCol_FrameBg,
						  ImVec4(bgR * FRAME_BG_MULTIPLIER,
								 bgG * FRAME_BG_MULTIPLIER,
								 bgB * FRAME_BG_MULTIPLIER,
								 1.0f));

	ImGui::PushStyleColor(ImGuiCol_ScrollbarBg,
						  ImVec4(bgR * FRAME_BG_MULTIPLIER,
								 bgG * FRAME_BG_MULTIPLIER,
								 bgB * FRAME_BG_MULTIPLIER,
								 0.0f // Make scrollbar track transparent
								 ));

	ImGui::PushStyleColor(ImGuiCol_PopupBg,
						  ImVec4(bgR * FRAME_BG_MULTIPLIER,
								 bgG * FRAME_BG_MULTIPLIER,
								 bgB * FRAME_BG_MULTIPLIER,
								 1.0f));

	ImGui::PushStyleColor(ImGuiCol_Border,
						  ImVec4(BORDER_COLOR, BORDER_COLOR, BORDER_COLOR, 1.0f));
	ImGui::PushStyleColor(ImGuiCol_ScrollbarGrab,
						  ImVec4(BORDER_COLOR, BORDER_COLOR, BORDER_COLOR, 1.0f));
	ImGui::PushStyleColor(ImGuiCol_ScrollbarGrabHovered,
						  ImVec4(SCROLLBAR_GRAB_HOVER_COLOR,
								 SCROLLBAR_GRAB_HOVER_COLOR,
								 SCROLLBAR_GRAB_HOVER_COLOR,
								 1.0f));
	ImGui::PushStyleColor(
		ImGuiCol_ScrollbarGrabActive,
		ImVec4(FRAME_BG_MULTIPLIER, FRAME_BG_MULTIPLIER, FRAME_BG_MULTIPLIER, 1.0f));
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
		// Set editor_state.block_input to false when window loses focus
		extern struct EditorState editor_state;
		editor_state.block_input = false;
	}
	wasFocused = isFocused;

	ImGui::BeginGroup();
	ImGui::TextUnformatted("Settings");
	float closeIconSize = ImGui::GetFrameHeight() - 10;
	ImVec2 contentAvail = ImGui::GetContentRegionAvail();
	float buttonPosX =
		contentAvail.x - closeIconSize - ImGui::GetStyle().FramePadding.x * 2;
	ImGui::SameLine(buttonPosX > 0 ? buttonPosX : ImGui::GetCursorPosX() + 100);

	ImVec2 cursor_pos = ImGui::GetCursorPos();
	if (ImGui::InvisibleButton("##close-settings", ImVec2(closeIconSize, closeIconSize)))
	{
		showSettingsWindow = false;
		if (settingsChanged)
			saveSettings();
		// Set editor_state.block_input to false when close button is clicked
		extern struct EditorState editor_state;
		editor_state.block_input = false;
	}
	bool isHovered = ImGui::IsItemHovered();
	ImGui::SetCursorPos(cursor_pos);
	ImTextureID closeIcon = gFileExplorer.getIcon("close");
	ImGui::Image(ImTextureRef(closeIcon),
				 ImVec2(closeIconSize, closeIconSize),
				 ImVec2(0, 0),
				 ImVec2(1, 1),
				 isHovered ? ImVec4(1, 1, 1, 0.6f) : ImVec4(1, 1, 1, 1),
				 ImVec4(0, 0, 0, 0));
	ImGui::EndGroup();

	// Add horizontal line to separate header from content
	ImGui::Separator();
}

void Settings::renderOpenRouterKeyInput()
{
	static char openRouterKeyBuffer[128] = "";
	static char agentModelBuffer[128] = "";
	static char completionModelBuffer[128] = "";
	static bool showOpenRouterKey = false;
	static bool initialized = false;
	static bool keyChanged = false;
	static bool agentModelChanged = false;
	static bool completionModelChanged = false;
	static bool wasInputActive = false;

	// Reset initialization when profile changes
	if (profileJustSwitched)
	{
		initialized = false;
	}

	if (!initialized)
	{
		std::string currentKey = settingsFileManager.getOpenRouterKey();
		strncpy(openRouterKeyBuffer, currentKey.c_str(), sizeof(openRouterKeyBuffer) - 1);
		openRouterKeyBuffer[sizeof(openRouterKeyBuffer) - 1] = '\0';

		std::string currentAgentModel = getAgentModel();
		strncpy(agentModelBuffer, currentAgentModel.c_str(), sizeof(agentModelBuffer) - 1);
		agentModelBuffer[sizeof(agentModelBuffer) - 1] = '\0';

		std::string currentCompletionModel = getCompletionModel();
		strncpy(completionModelBuffer,
				currentCompletionModel.c_str(),
				sizeof(completionModelBuffer) - 1);
		completionModelBuffer[sizeof(completionModelBuffer) - 1] = '\0';

		initialized = true;
	}
	ImGui::Spacing();
	ImGui::TextUnformatted("AI Settings");
	ImGui::Separator();
	ImGui::Spacing();

	// Set a consistent width for all inputs
	const float inputWidth = 400.0f;
	const float openRouterInputWidth =
		340.0f; // Smaller width to account for extra Show/Hide button

	// OpenRouter Key Input
	ImGui::SetNextItemWidth(openRouterInputWidth);
	ImGuiInputTextFlags flags = showOpenRouterKey ? 0 : ImGuiInputTextFlags_Password;
	bool inputChanged = ImGui::InputText(
		"##openrouterkey", openRouterKeyBuffer, sizeof(openRouterKeyBuffer), flags);
	bool isInputActive = ImGui::IsItemActive();
	if (inputChanged)
	{
		keyChanged = true;
	}
	ImGui::SameLine();
	if (ImGui::Button(showOpenRouterKey ? "Hide" : "Show", ImVec2(0, 0)))
	{
		showOpenRouterKey = !showOpenRouterKey;
	}
	ImGui::SameLine();
	if (ImGui::Button("Save") && keyChanged)
	{
		settingsFileManager.setOpenRouterKey(std::string(openRouterKeyBuffer));
		gAITab.load_key();
		renderNotification("OpenRouter key saved!", 2.0f);
		keyChanged = false;
	}
	ImGui::SameLine();
	ImGui::TextUnformatted("OpenRouter Key");
	ImGui::Spacing();

	// Agent Model Input
	ImGui::SetNextItemWidth(inputWidth);
	bool agentModelInputChanged =
		ImGui::InputText("##agentmodel", agentModelBuffer, sizeof(agentModelBuffer));
	bool isAgentModelActive = ImGui::IsItemActive();
	if (agentModelInputChanged)
	{
		agentModelChanged = true;
	}
	ImGui::SameLine();
	if (ImGui::Button("Save##agent") && agentModelChanged)
	{
		settings["agent_model"] = std::string(agentModelBuffer);
		settingsChanged = true;
		saveSettings();
		renderNotification("Agent model saved!", 2.0f);
		agentModelChanged = false;
	}
	ImGui::SameLine();
	ImGui::TextUnformatted("Agent Model");
	ImGui::Spacing();

	// Completion Model Input
	ImGui::SetNextItemWidth(inputWidth);
	bool completionModelInputChanged = ImGui::InputText("##completionmodel",
														completionModelBuffer,
														sizeof(completionModelBuffer));
	bool isCompletionModelActive = ImGui::IsItemActive();
	if (completionModelInputChanged)
	{
		completionModelChanged = true;
	}
	ImGui::SameLine();
	if (ImGui::Button("Save##completion") && completionModelChanged)
	{
		settings["completion_model"] = std::string(completionModelBuffer);
		settingsChanged = true;
		saveSettings();
		renderNotification("Completion model saved!", 2.0f);
		completionModelChanged = false;
	}
	ImGui::SameLine();
	ImGui::TextUnformatted("Completion Model");
	ImGui::Spacing();

	// Handle block_input logic for all inputs
	bool anyInputActive = isInputActive || isAgentModelActive || isCompletionModelActive;

	if (anyInputActive && !wasInputActive)
	{
		editor_state.block_input = true;
	} else if (!anyInputActive && wasInputActive)
	{
		editor_state.block_input = false;
	}

	// Also ensure block_input is set if any input is currently active
	if (anyInputActive && !editor_state.block_input)
	{
		editor_state.block_input = true;
	}

	wasInputActive = anyInputActive;
}

void Settings::renderProfileSelector()
{
	ImGui::Spacing();

	std::vector<std::string> availableProfileFiles =
		settingsFileManager.getAvailableProfileFiles();
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
					std::cout << "[Settings] User selected new profile: " << filename
							  << std::endl;
					settingsFileManager.switchProfile(filename,
													  settings,
													  settingsPath,
													  settingsChanged,
													  fontChanged,
													  themeChanged);
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

	if (displayPreviewName != "System Default" &&
		displayPreviewName.find('.') != std::string::npos)
	{
		displayPreviewName =
			displayPreviewName.substr(0, displayPreviewName.find_last_of("."));
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
		tempFontSize = settings.value("fontSize",
									  20.0f); // Get the value directly from settings
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

	// **changed**: Save background color immediately when it changes, like other settings
	if (ImGui::ColorEdit4("Background Color", (float *)&bgColor))
	{
		// Update settings immediately when color changes
		settings["backgroundColor"] = {bgColor.x, bgColor.y, bgColor.z, bgColor.w};
		settingsChanged = true;
		saveSettings(); // Save immediately like other settings
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

	if (ImGui::SliderFloat("Background Opacity", &currentOpacity, 0.0f, 1.0f, "%.2f"))
	{
		settings["mac_background_opacity"] = currentOpacity;
		settingsChanged = true;
		saveSettings();
	}

	if (ImGui::Checkbox("Enable Background Blur", &currentBlurEnabled))
	{
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
			if (!themeColorsJson.contains(colorKey) ||
				!themeColorsJson[colorKey].is_array() ||
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
				if (!settingsChanged)
					settingsChanged = true;
				if (!themeChanged)
					themeChanged = true;
			}

			if (ImGui::IsItemDeactivatedAfterEdit())
			{
				if (themeColorsJson[colorKey][0].get<float>() != color.x ||
					themeColorsJson[colorKey][1].get<float>() != color.y ||
					themeColorsJson[colorKey][2].get<float>() != color.z ||
					themeColorsJson[colorKey][3].get<float>() != color.w)
				{
					themeColorsJson[colorKey] = {color.x, color.y, color.z, color.w};
					if (!settingsChanged)
						settingsChanged = true;
					if (!themeChanged)
						themeChanged = true;
				}

				std::cout << "[Settings] Color " << colorKey
						  << " edit finished. Forcing update and saving." << std::endl;
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

	bool sidebarVisible = settings.value("sidebar_visible", true);
	if (ImGui::Checkbox("File Explorer", &sidebarVisible))
	{
		toggleSidebar();
	}
	ImGui::SameLine();
	ImGui::TextDisabled("(Show/hide file explorer sidebar)");
	if (!isEmbedded)
	{
		bool agentPaneVisible = settings.value("agent_pane_visible", true);
		if (ImGui::Checkbox("Agent Pane", &agentPaneVisible))
		{
			toggleAgentPane();
		}
		ImGui::SameLine();
		ImGui::TextDisabled("(Show/hide AI agent panel)");
	}

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

	bool gitChangedLines = settings.value("git_changed_lines", true);
	if (ImGui::Checkbox("Git Changed Lines", &gitChangedLines))
	{
		settings["git_changed_lines"] = gitChangedLines;
		settingsChanged = true;
		saveSettings();
	}
	ImGui::SameLine();
	ImGui::TextDisabled("(Highlight changed lines in git)");

	bool aiAutocomplete = settings.value("ai_autocomplete", true);

	if (ImGui::Checkbox("AI Completion", &aiAutocomplete))
	{
		settings["ai_autocomplete"] = aiAutocomplete;
		settingsChanged = true;
		saveSettings();
	}
	ImGui::SameLine();
	ImGui::TextDisabled("(AI-powered code completion)");

	if (!isEmbedded)
	{
		bool fpsToggle = gSettings.getSettings()["fps_toggle"].get<bool>();
		if (ImGui::Checkbox("FPS Counter", &fpsToggle))
		{
			settings["fps_toggle"] = fpsToggle;
			settingsChanged = true;
			saveSettings();
		}
		ImGui::SameLine();
		ImGui::TextDisabled("(Show FPS counter overlay)");
	}
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
	renderShaderSlider(
		"Curvature(bugged)", "curvature_intensity", 0.0f, 0.5f, "%.02f", 0.0f);
	renderShaderSlider("Burn-in", "burnin_intensity", 0.9f, 0.999f, "%.03f", 0.9525f);
	renderShaderSlider("Jitter", "jitter_intensity", 0.0f, 10.0f, "%.02f", 2.81f);
	renderShaderSlider(
		"Pixel lines", "pixelation_intensity", -1.00f, 1.00f, "%.03f", -0.11f);

	renderShaderSlider("FPS Target", "fps_target", 20.0f, 1000.0f, "%.0f", 120.0f);
	ImGui::SameLine();
}

void Settings::renderShaderSlider(const char *label,
								  const char *key,
								  float min_val,
								  float max_val,
								  const char *format,
								  float default_val)
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

void Settings::renderKeybindsSettings()
{
	ImGui::Spacing();
	ImGui::Separator();
	ImGui::Spacing();

	if (ImGui::Button("Open Keybinds File"))
	{
		std::string keybindsPath =
			(fs::path(settingsFileManager.getUserSettingsPath()).parent_path() /
			 "keybinds.json")
				.string();
		if (fs::exists(keybindsPath))
		{
			gFileExplorer.loadFileContent(keybindsPath);
			showSettingsWindow = false; // Close settings window after opening keybinds
		} else
		{
			std::cerr << "[Settings] Keybinds file not found at: " << keybindsPath
					  << std::endl;
		}
	}
	ImGui::SameLine();
	ImGui::TextDisabled("(Edit keyboard shortcuts)");

	// Check if we're using default keybinds
	std::string keybindsPath =
		(fs::path(settingsFileManager.getUserSettingsPath()).parent_path() /
		 "keybinds.json")
			.string();
	std::string defaultKeybindsPath =
		(fs::path(settingsFileManager.getUserSettingsPath()).parent_path() /
		 "default-keybinds.json")
			.string();

	if (fs::exists(defaultKeybindsPath) && !fs::exists(keybindsPath))
	{
		ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), "Using default keybinds");
		if (ImGui::Button("Restore Default Keybinds"))
		{
			try
			{
				fs::copy_file(defaultKeybindsPath,
							  keybindsPath,
							  fs::copy_options::overwrite_existing);
				std::cout << "[Settings] Restored default keybinds" << std::endl;
				gKeybinds.loadKeybinds(); // Reload keybinds
			} catch (const fs::filesystem_error &e)
			{
				std::cerr << "[Settings] Error restoring default keybinds: " << e.what()
						  << std::endl;
			}
		}
		ImGui::SameLine();
		ImGui::TextDisabled("(Reset to default configuration)");
	}

	// LSP Dashboard section
	ImGui::Spacing();
	ImGui::Separator();
	ImGui::Spacing();
	if (ImGui::Button("LSP Dashboard"))
	{
		gLSPDashboard.setShow(true);
		showSettingsWindow = false; // Close settings window when opening LSP dashboard
	}
	ImGui::SameLine();
	ImGui::TextDisabled("(View LSP server status)");
}

void Settings::handleWindowInput()
{
	if (ImGui::IsKeyPressed(ImGuiKey_Escape))
	{
		showSettingsWindow = false;
		if (settingsChanged)
			saveSettings();
		// Set editor_state.block_input to false when window is closed via Escape
		extern struct EditorState editor_state;
		editor_state.block_input = false;
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
				// Set editor_state.block_input to false when window is closed
				// via clicking outside
				extern struct EditorState editor_state;
				editor_state.block_input = false;
			}
		}
	}
}

void Settings::renderNotification(const std::string &message, float duration)
{
	static float notificationTimer = 0.0f;
	static std::string currentMessage;
	static bool showNotification = false;

	// Update notification state
	if (!message.empty())
	{
		currentMessage = message;
		showNotification = true;
		notificationTimer = duration;
	}

	if (showNotification)
	{
		// Get viewport and calculate position
		ImGuiViewport *viewport = ImGui::GetMainViewport();
		ImVec2 screenPos = viewport->Pos;
		ImVec2 screenSize = viewport->Size;

		// Calculate base dimensions
		const float padding = 20.0f;
		const float minWidth = 200.0f;
		const float maxWidth = screenSize.x * 0.8f; // Allow up to 80% of screen width
		const float minHeight = 50.0f;
		const float maxHeight = screenSize.y * 0.4f; // Allow up to 40% of screen height
		const float textPadding = 15.0f; // Padding inside the notification box

		// Calculate available width for text (accounting for padding)
		float availableTextWidth = maxWidth - (textPadding * 2);

		// Calculate text size with proper wrapping
		ImVec2 textSize = ImGui::CalcTextSize(
			currentMessage.c_str(), nullptr, false, availableTextWidth);

		// Calculate content dimensions
		float contentWidth = textSize.x + (textPadding * 2);
		float contentHeight = textSize.y + (textPadding * 2);

		// Clamp dimensions to reasonable bounds
		float width = ImClamp(contentWidth, minWidth, maxWidth);
		float height = ImClamp(contentHeight, minHeight, maxHeight);

		// Ensure the notification doesn't go off-screen
		if (width > screenSize.x - (padding * 2))
		{
			width = screenSize.x - (padding * 2);
		}
		if (height > screenSize.y - (padding * 2))
		{
			height = screenSize.y - (padding * 2);
		}

		// Calculate notification position (bottom left)
		ImVec2 notificationPos =
			ImVec2(screenPos.x + padding, screenPos.y + screenSize.y - height - padding);

		// Use foreground draw list to ensure it's always on top
		ImDrawList *draw_list = ImGui::GetForegroundDrawList();

		// Get background color from settings and adjust opacity
		auto &bg = getSettings()["backgroundColor"];
		ImU32 bgColor = IM_COL32(static_cast<int>(bg[0].get<float>() * 255),
								 static_cast<int>(bg[1].get<float>() * 255),
								 static_cast<int>(bg[2].get<float>() * 255),
								 230 // Increased opacity for better visibility
		);

		// Draw background with rounded corners
		ImVec2 p_min = notificationPos;
		ImVec2 p_max = ImVec2(notificationPos.x + width, notificationPos.y + height);

		// Draw main background
		draw_list->AddRectFilled(p_min, p_max, bgColor, 8.0f);

		// Draw a subtle border
		ImU32 borderColor =
			IM_COL32(255, 255, 255, 255); // White border to match text color
		draw_list->AddRect(p_min, p_max, borderColor, 8.0f, 0, 1.0f);

		// Draw text with proper wrapping and positioning
		ImVec2 textPos =
			ImVec2(notificationPos.x + textPadding, notificationPos.y + textPadding);
		draw_list->AddText(textPos, IM_COL32(255, 255, 255, 255), currentMessage.c_str());

		// Update timer
		notificationTimer -= ImGui::GetIO().DeltaTime;
		if (notificationTimer <= 0.0f)
		{
			showNotification = false;
		}
	}
}

void Settings::toggleSidebar()
{
	// Duplicate the logic from handleKeyboardShortcuts

	// Get current window dimensions (we'll need to approximate this)
	float windowWidth = 1200.0f; // Default window width
	float padding = 8.0f;		 // Default padding
	float kAgentSplitterWidth = 6.0f;
	float availableWidth = windowWidth - padding * 3 - kAgentSplitterWidth;

	float agentPaneWidthPx;
	if (Splitter::showSidebar)
	{
		agentPaneWidthPx = availableWidth * getAgentSplitPos();
	} else
	{
		float agentSplit = getAgentSplitPos();
		float editorWidth = availableWidth * agentSplit;
		agentPaneWidthPx = availableWidth - editorWidth - kAgentSplitterWidth;
	}

	// Toggle sidebar
	Splitter::showSidebar = !Splitter::showSidebar;

	// Save sidebar visibility setting
	settings["sidebar_visible"] = Splitter::showSidebar;
	saveSettings();

	// Recompute availableWidth after toggling
	availableWidth = windowWidth - padding * 3 - kAgentSplitterWidth;

	float newRightSplit;
	if (Splitter::showSidebar)
	{
		newRightSplit = agentPaneWidthPx / availableWidth;
	} else
	{
		float editorWidth = availableWidth - agentPaneWidthPx - kAgentSplitterWidth;
		newRightSplit = editorWidth / availableWidth;
	}
	setAgentSplitPos(std::max(
		0.25f,
		std::min(0.9f, newRightSplit))); // Changed minimum from 0.1f to 0.25f (25%)

	std::cout << "Toggled sidebar visibility from settings" << std::endl;
}

void Settings::toggleAgentPane()
{
	// Duplicate the logic from handleKeyboardShortcuts

	// Only toggle visibility, do not recalculate or set agentSplitPos
	Splitter::showAgentPane = !Splitter::showAgentPane;

	// Save agent pane visibility setting
	settings["agent_pane_visible"] = Splitter::showAgentPane;
	saveSettings();

	std::cout << "Toggled agent pane visibility from settings" << std::endl;
}

void Settings::toggleSettingsWindow()
{
	showSettingsWindow = !showSettingsWindow;
	if (showSettingsWindow)
	{
		ClosePopper::closeAllExcept(ClosePopper::Type::Settings);
	}
	blockInput = showSettingsWindow;

	// Set editor_state.block_input to match the settings window state
	extern struct EditorState editor_state;
	editor_state.block_input = showSettingsWindow;
}

void Settings::ApplySettings(ImGuiStyle &style)
{
	// Set the window background color from settings.
	style.Colors[ImGuiCol_WindowBg] = ImVec4(settings["backgroundColor"][0].get<float>(),
											 settings["backgroundColor"][1].get<float>(),
											 settings["backgroundColor"][2].get<float>(),
											 settings["backgroundColor"][3].get<float>());
	// Note: shader state is now managed by the ShaderManager class

	// Set text colors from the current theme.
	std::string currentTheme = getCurrentTheme();
	auto &textColor = settings["themes"][currentTheme]["text"];
	ImVec4 textCol(textColor[0].get<float>(),
				   textColor[1].get<float>(),
				   textColor[2].get<float>(),
				   textColor[3].get<float>());
	style.Colors[ImGuiCol_Text] = textCol;
	style.Colors[ImGuiCol_TextDisabled] =
		ImVec4(textCol.x * 0.6f, textCol.y * 0.6f, textCol.z * 0.6f, textCol.w);
	style.Colors[ImGuiCol_TextSelectedBg] = ImVec4(1.0f, 0.1f, 0.7f, 0.3f);

	// Set scrollbar size only for standalone mode, leave default for embedded
	if (!isEmbedded)
	{
		// Use original size for standalone mode
		style.ScrollbarSize = 30.0f;
	}
	// In embedded mode, leave scrollbar size as default (don't set it)
	style.ScaleAllSizes(1.0f); // Keep this if you scale other UI elements

	// Handle scrollbar colors based on mode
	if (!isEmbedded)
	{
		// Hide all scrollbar elements in standalone mode
		style.Colors[ImGuiCol_ScrollbarBg] = ImVec4(0, 0, 0, 0);
		style.Colors[ImGuiCol_ScrollbarGrab] = ImVec4(0, 0, 0, 0);
		style.Colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0, 0, 0, 0);
		style.Colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0, 0, 0, 0);
	} else
	{
		// In embedded mode, make only the scrollbar track transparent
		// but keep the scrollbar grab (draggable part) visible
		style.Colors[ImGuiCol_ScrollbarBg] = ImVec4(0, 0, 0, 0); // Transparent track
		// Leave scrollbar grab colors as default (visible)
	}

	// Set the global font scale.
	// ImGui::GetIO().FontGlobalScale =
	// settings["fontSize"].get<float>() / 16.0f;
}

void Settings::handleSettingsChanges(bool &needFontReload,
									 bool &m_needsRedraw,
									 int &m_framesToRender,
									 std::function<void(bool)> setShaderEnabled,
									 float &lastOpacity,
									 bool &lastBlurEnabled,
									 bool force)
{
	if (hasSettingsChanged() || force)
	{
		if (!force)
		{
			m_needsRedraw = true;							  // Set the flag!
			m_framesToRender = std::max(m_framesToRender, 3); // Reduced frame count
		}

		ImGuiStyle &style = ImGui::GetStyle();

		ApplySettings(style);

		// Set the child window background color (for editor panes, file explorer, etc.)
		// Use a proper alpha value (1.0) instead of the settings alpha which might be 0
		style.Colors[ImGuiCol_ChildBg] =
			ImVec4(getSettings()["backgroundColor"][0].get<float>(),
				   getSettings()["backgroundColor"][1].get<float>(),
				   getSettings()["backgroundColor"][2].get<float>(),
				   1.0f); // Use full alpha for child windows

		// Also set the main window background color for popups and settings windows
		// but use a slightly different alpha to distinguish from child windows
		style.Colors[ImGuiCol_WindowBg] =
			ImVec4(getSettings()["backgroundColor"][0].get<float>(),
				   getSettings()["backgroundColor"][1].get<float>(),
				   getSettings()["backgroundColor"][2].get<float>(),
				   0.95f); // Slightly transparent for main windows

		// Update shader manager using function pointer
		setShaderEnabled(getSettings()["shader_toggle"].get<bool>());

		// Update sidebar visibility from settings
		Splitter::showSidebar = getSettings().value("sidebar_visible", true);
		Splitter::showAgentPane = getSettings().value("agent_pane_visible", true);

		if (hasThemeChanged() || force)
		{
			extern EditorHighlight gEditorHighlight;
			gEditorHighlight.setTheme(getCurrentTheme());
			extern FileExplorer gFileExplorer;
			if (!gFileExplorer.currentFile.empty())
			{
				gEditorHighlight.highlightContent();
			}
			resetThemeChanged();
		}
		if (hasFontChanged() || hasFontSizeChanged())
		{
			needFontReload = true;
			// Also set the global font reload flag
			extern Font gFont;
			gFont.setNeedFontReload(true);
		}
#ifdef __APPLE__
		// Always update with current values
		float currentOpacity = getSettings().value("mac_background_opacity", 0.5f);
		bool currentBlurEnabled = getSettings().value("mac_blur_enabled", true);

		updateMacOSWindowProperties(currentOpacity, currentBlurEnabled);

		// Update tracking variables
		lastOpacity = currentOpacity;
		lastBlurEnabled = currentBlurEnabled;
#endif

		if (!force)
		{
			resetSettingsChanged();
		}
	}
}

void Settings::switchToProfile(const std::string &profileName)
{
	settingsFileManager.switchProfile(
		profileName, settings, settingsPath, settingsChanged, fontChanged, themeChanged);
	profileJustSwitched = true;
	settingsChanged = true;
	fontChanged = true;

	// Force syntax color update for editor
	extern class EditorHighlight gEditorHighlight;
	gEditorHighlight.forceColorUpdate();
}

std::string Settings::getCurrentProfileName() const
{
	if (settingsPath.empty())
		return "ned.json"; // Default fallback

	// Extract filename from the path
	return fs::path(settingsPath).filename().string();
}

ImVec4 Settings::getCurrentTextColor() const
{
	std::string currentTheme = getCurrentTheme();

	// Check if theme and text color exist
	if (settings.contains("themes") && settings["themes"].contains(currentTheme) &&
		settings["themes"][currentTheme].contains("text") &&
		settings["themes"][currentTheme]["text"].is_array() &&
		settings["themes"][currentTheme]["text"].size() == 4)
	{
		auto &textColor = settings["themes"][currentTheme]["text"];
		return ImVec4(textColor[0].get<float>(),
					  textColor[1].get<float>(),
					  textColor[2].get<float>(),
					  textColor[3].get<float>());
	}

	// Fallback to white if theme data is missing
	return ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
}

ImVec4 Settings::getCurrentBackgroundColor() const
{
	// Check if background color exists
	if (settings.contains("backgroundColor") && settings["backgroundColor"].is_array() &&
		settings["backgroundColor"].size() == 4)
	{
		auto &bgColor = settings["backgroundColor"];
		return ImVec4(bgColor[0].get<float>(),
					  bgColor[1].get<float>(),
					  bgColor[2].get<float>(),
					  bgColor[3].get<float>());
	}

	// Fallback to dark background if missing
	return ImVec4(0.1f, 0.1f, 0.1f, 1.0f);
}