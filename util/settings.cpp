/******************************************************************************
 * settings.cpp *
 ******************************************************************************/

#include "settings.h"
#include "../editor/editor.h"
#include "../editor/editor_highlight.h"
#include "../files/files.h"
#include "config.h"
#include "imgui.h"
#include <GLFW/glfw3.h>

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <libgen.h>		 // For dirname
#include <mach-o/dyld.h> // For _NSGetExecutablePath
#include <sys/param.h>	 // For MAXPATHLEN
#include <unistd.h>

namespace fs = std::filesystem;
Settings gSettings;

std::string Settings::getAppResourcesPath()
{
	char exePath[MAXPATHLEN];
	uint32_t size = sizeof(exePath);

	// 1) Get the path to this .app’s actual executable
	if (_NSGetExecutablePath(exePath, &size) == 0)
	{
		// 2) Resolve symlinks/translocation
		char resolvedPath[MAXPATHLEN];
		if (realpath(exePath, resolvedPath) != nullptr)
		{
			// e.g. resolvedPath =
			// "/Users/you/Downloads/Ned.app/Contents/MacOS/Ned"
			std::string p = resolvedPath;

			// *** Call dirname() twice to move from:
			//     "/Users/you/Downloads/Ned.app/Contents/MacOS/Ned"
			//  -> "/Users/you/Downloads/Ned.app/Contents/MacOS"
			//  -> "/Users/you/Downloads/Ned.app/Contents"
			p = dirname((char *)p.c_str()); // remove "/Ned"
			p = dirname((char *)p.c_str()); // remove "/MacOS"

			// Now p == "/Users/you/Downloads/Ned.app/Contents"

			std::string resourcesPath = p + "/Resources";
			if (fs::exists(resourcesPath) && fs::is_directory(resourcesPath))
			{
				std::cout << "[Settings::getAppResourcesPath] Found .app "
							 "Resources at: "
						  << resourcesPath << std::endl;
				return resourcesPath;
			} else
			{
				// If no Resources folder, fallback to p
				std::cout << "[Settings::getAppResourcesPath] No /Resources "
							 "folder at "
						  << resourcesPath << "; using " << p << std::endl;
				return p;
			}
		} else
		{
			std::cerr << "[Settings::getAppResourcesPath] realpath() failed, "
						 "using exePath.\n";
			// fallback to single or double dirname calls as well
			std::string p = exePath;
			p = dirname((char *)p.c_str());
			p = dirname((char *)p.c_str());
			p += "/Resources";
			return p;
		}
	}

	// Fallback if _NSGetExecutablePath failed
	std::cerr << "[Settings::getAppResourcesPath] _NSGetExecutablePath failed.\n";
	return ".";
}

/******************************************************************************
 * getUserSettingsPath()
 * e.g. /Users/username/ned/.ned.json
 ******************************************************************************/
std::string Settings::getUserSettingsPath()
{
	const char *home = getenv("HOME");
	if (!home)
	{
		// If HOME is missing, fallback to current directory
		return ".ned.json";
	}
	return std::string(home) + "/ned/.ned.json";
}

Settings::Settings() : splitPos(0.3f) {}

void Settings::loadSettings()
{
	// 1) Where to store & load user settings
	std::string userSettingsPath = getUserSettingsPath();

	// 2) If user file doesn't exist, copy from the .app resources
	if (!fs::exists(userSettingsPath))
	{
		fs::create_directories(fs::path(userSettingsPath).parent_path());

		std::string bundleDefaults = getAppResourcesPath() + "/.ned.json";
		if (fs::exists(bundleDefaults))
		{
			fs::copy_file(bundleDefaults, userSettingsPath, fs::copy_options::overwrite_existing);
			std::cout << "[Settings] Copied default .ned.json -> " << userSettingsPath << std::endl;
		} else
		{
			std::cout << "[Settings] No default .ned.json found in app bundle. "
						 "Using empty defaults.\n";
		}
	}

	// Actually load from userSettingsPath
	settingsPath = userSettingsPath;
	std::ifstream settingsFile(settingsPath);
	if (settingsFile.is_open())
	{
		try
		{
			settingsFile >> settings;
			std::cout << "[Settings] Loaded from: " << settingsPath << std::endl;
		} catch (json::parse_error &e)
		{
			std::cerr << "[Settings] Parse error: " << e.what() << std::endl;
			settings = json::object(); // Reset to empty object
		}
	} else
	{
		std::cout << "[Settings] Could not open user settings file. "
					 "Using empty defaults.\n";
		settings = json::object();
	}

	// Fill in any missing keys with code defaults:
	if (!settings.contains("font"))
	{
		settings["font"] = "default";
	}
	if (!settings.contains("backgroundColor"))
	{
		settings["backgroundColor"] = {0.45f, 0.55f, 0.60f, 1.00f};
	}
	if (!settings.contains("fontSize"))
	{
		settings["fontSize"] = 16.0f; // safe default
	}
	if (!settings.contains("splitPos"))
	{
		settings["splitPos"] = 0.3f;
	}
	if (!settings.contains("theme"))
	{
		settings["theme"] = "default";
	}
	if (!settings.contains("treesitter"))
	{
		settings["treesitter"] = true;
	}
	if (!settings.contains("rainbow"))
	{
		settings["rainbow"] = true; // default to true
	}
	if (!settings.contains("shader_toggle"))
	{
		settings["shader_toggle"] = true; // default to shader enabled
	}
	if (!settings.contains("themes"))
	{
		settings["themes"] = {{"default",
							   {
								   {"function", {1.0f, 1.0f, 1.0f, 1.0f}},
								   {"text", {1.0f, 1.0f, 1.0f, 1.0f}},
								   {"type", {0.4f, 0.8f, 0.4f, 1.0f}},
								   {"variable", {0.8f, 0.6f, 0.7f, 1.0f}},
								   {"background", {0.2f, 0.2f, 0.2f, 1.0f}},
								   {"keyword", {0.0f, 0.4f, 1.0f, 1.0f}},
								   {"string", {0.87f, 0.87f, 0.0f, 1.0f}},
								   {"number", {0.0f, 0.8f, 0.8f, 1.0f}},
								   {"comment", {0.5f, 0.5f, 0.5f, 1.0f}},
							   }}};
	}

	// Pull out a few keys into your members
	currentFontSize = settings["fontSize"].get<float>();
	splitPos = settings["splitPos"].get<float>();

	// Track last modification time
	if (fs::exists(settingsPath))
	{
		lastSettingsModification = fs::last_write_time(settingsPath);
	} else
	{
		lastSettingsModification = fs::file_time_type::min();
	}
}

void Settings::saveSettings()
{
	std::ofstream settingsFile(settingsPath);
	if (settingsFile.is_open())
	{
		settingsFile << std::setw(4) << settings << std::endl;
	}
}

void Settings::checkSettingsFile()
{
	if (!fs::exists(settingsPath))
	{
		std::cout << "[Settings] File does not exist, skipping check" << std::endl;
		return;
	}

	auto currentModification = fs::last_write_time(settingsPath);
	if (currentModification > lastSettingsModification)
	{
		std::cout << "[Settings] .ned.json was modified externally, reloading" << std::endl;
		json oldSettings = settings;
		loadSettings();
		lastSettingsModification = currentModification;

		// Check if certain keys changed so we can trigger appropriate flags
		if (oldSettings["backgroundColor"] != settings["backgroundColor"] ||
			oldSettings["fontSize"] != settings["fontSize"] ||
			oldSettings["splitPos"] != settings["splitPos"] ||
			oldSettings["theme"] != settings["theme"] ||
			oldSettings["themes"] != settings["themes"] ||
			oldSettings["rainbow"] != settings["rainbow"] ||
			oldSettings["treesitter"] != settings["treesitter"] ||
			oldSettings["shader_toggle"] != settings["shader_toggle"] ||
			oldSettings["font"] != settings["font"])
		{
			settingsChanged = true;
			// Check if theme changed
			if (oldSettings["theme"] != settings["theme"] ||
				oldSettings["themes"] != settings["themes"])
			{
				themeChanged = true;
			}
			// Check if font changed
			if (oldSettings["font"] != settings["font"])
			{
				fontChanged = true;
			}
		}
	}
}

void Settings::renderSettingsWindow()
{
	if (!showSettingsWindow)
		return;

	ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 1.0f);
	ImGui::SetNextWindowSize(ImVec2(900, 600), ImGuiCond_Always);
	ImGui::SetNextWindowPos(ImVec2(ImGui::GetIO().DisplaySize.x * 0.5f,
								   ImGui::GetIO().DisplaySize.y * 0.5f),
							ImGuiCond_Always,
							ImVec2(0.5f, 0.5f));

	ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
								   ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar |
								   ImGuiWindowFlags_Modal;

	// Push custom styles
	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 10.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.0f);
	ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.2f, 0.2f, 0.2f, 1.0f));
	ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.3f, 0.3f, 0.3f, 1.0f));
	ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.15f, 0.15f, 0.15f, 1.0f));

	ImGui::Begin("Settings", nullptr, windowFlags);

	// Handle focus detection
	static bool wasFocused = false;
	bool isFocused = ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);

	if (wasFocused && !isFocused)
	{
		std::cout << "Settings window lost focus!" << std::endl;
		showSettingsWindow = false;
		saveSettings();
	} else if (!wasFocused && isFocused)
	{
		std::cout << "Settings window gained focus!" << std::endl;
	}
	wasFocused = isFocused;

	// Start header group
	ImGui::BeginGroup();
	float textHeight = ImGui::GetTextLineHeight();
	ImGui::TextUnformatted("Settings");

	// Calculate close button position relative to content region
	float closeIconSize = ImGui::GetFrameHeight() - 10; // Use frame height to match font scaling
	ImVec2 contentAvail = ImGui::GetContentRegionAvail();
	float buttonPosX = contentAvail.x - closeIconSize - ImGui::GetStyle().FramePadding.x * 2;
	ImGui::SameLine(buttonPosX);

	// Store cursor position for icon placement
	ImVec2 cursor_pos = ImGui::GetCursorPos();
	if (ImGui::InvisibleButton("##close-settings", ImVec2(closeIconSize, closeIconSize)))
	{
		showSettingsWindow = false;
		saveSettings();
	}
	bool isHovered = ImGui::IsItemHovered();
	ImGui::SetCursorPos(cursor_pos);

	// Get and display close icon
	ImTextureID closeIcon = gFileExplorer.getIcon("close");
	ImGui::Image(closeIcon,
				 ImVec2(closeIconSize, closeIconSize),
				 ImVec2(0, 0),
				 ImVec2(1, 1),
				 isHovered ? ImVec4(1, 1, 1, 0.6f) : ImVec4(1, 1, 1, 1));

	ImGui::EndGroup();
	ImGui::Separator();
	ImGui::Spacing();

	// Show a slider for font size
	static float tempFontSize = currentFontSize;
	if (ImGui::SliderFloat("Font Size", &tempFontSize, 4.0f, 32.0f, "%.0f"))
	{
		// Update settings but don't change display immediately
		settings["fontSize"] = tempFontSize;
	}
	if (ImGui::IsItemDeactivatedAfterEdit())
	{
		setFontSize(tempFontSize);
		saveSettings();
	}

	ImGui::Spacing();

	// Background Color
	auto &bgColorArray = settings["backgroundColor"];
	ImVec4 bgColor(bgColorArray[0].get<float>(),
				   bgColorArray[1].get<float>(),
				   bgColorArray[2].get<float>(),
				   bgColorArray[3].get<float>());

	if (ImGui::ColorEdit3("Background Color", (float *)&bgColor))
	{
		settings["backgroundColor"] = {bgColor.x, bgColor.y, bgColor.z, bgColor.w};
		settingsChanged = true;
		saveSettings();
	}

	// Theme colors
	std::string currentTheme = getCurrentTheme();
	auto &themeColors = settings["themes"][currentTheme];

	auto editThemeColor = [&](const char *label, const char *colorKey) {
		auto &colorArray = themeColors[colorKey];
		ImVec4 color(colorArray[0].get<float>(),
					 colorArray[1].get<float>(),
					 colorArray[2].get<float>(),
					 colorArray[3].get<float>());

		ImGui::TextUnformatted(label);
		ImGui::SameLine(200);
		if (ImGui::ColorEdit3(("##" + std::string(colorKey)).c_str(), (float *)&color))
		{
			themeColors[colorKey] = {color.x, color.y, color.z, color.w};
			themeChanged = true;
			settingsChanged = true;
			gEditorHighlight.forceColorUpdate();
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
	editThemeColor("Variables", "variable");

	ImGui::Spacing();
	ImGui::TextUnformatted("Toggle");
	ImGui::Separator();
	ImGui::Spacing();

	// Rainbow
	bool rainbowMode = settings["rainbow"].get<bool>();
	if (ImGui::Checkbox("Rainbow Mode", &rainbowMode))
	{
		settings["rainbow"] = rainbowMode;
		settingsChanged = true;
		saveSettings();
	}
	ImGui::SameLine();
	ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "(Rainbow cursor & line numbers)");
	// treesitter
	bool treesitterMode = settings["treesitter"].get<bool>();
	if (ImGui::Checkbox("TreeSitter Mode", &treesitterMode))
	{
		settings["treesitter"] = treesitterMode;
		settingsChanged = true;
		editor_state.text_changed = true;
		saveSettings();
	}
	ImGui::SameLine();
	ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "(Syntax Highlighting)");

	// Shader
	bool shaderEnabled = settings["shader_toggle"].get<bool>();
	if (ImGui::Checkbox("Enable Shader Effects", &shaderEnabled))
	{
		settings["shader_toggle"] = shaderEnabled;
		settingsChanged = true;
		saveSettings();
	}
	ImGui::SameLine();
	ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "(CRT & visual effects)");

	ImGui::Separator();
	ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Press Cmd+, to close this window");

	// ESC key closes the window
	if (ImGui::IsKeyPressed(ImGuiKey_Escape))
	{
		showSettingsWindow = false;
		saveSettings();
	}
	// Detect clicks outside this window (only if no popups are open)
	if (ImGui::IsMouseClicked(ImGuiMouseButton_Left))
	{
		ImVec2 mousePos = ImGui::GetMousePos();
		ImVec2 windowPos = ImGui::GetWindowPos();
		ImVec2 windowSize = ImGui::GetWindowSize();

		bool isMouseOutside =
			(mousePos.x < windowPos.x || mousePos.x > windowPos.x + windowSize.x ||
			 mousePos.y < windowPos.y || mousePos.y > windowPos.y + windowSize.y);

		if (isMouseOutside && !ImGui::IsPopupOpen(nullptr, ImGuiPopupFlags_AnyPopupId) &&
			!ImGui::IsAnyItemHovered())
		{
			showSettingsWindow = false;
			saveSettings();
		}
	}

	ImGui::End();

	// Pop style
	ImGui::PopStyleColor(3);
	ImGui::PopStyleVar(3);
}
