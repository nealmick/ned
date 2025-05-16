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
				/*
				std::cout << "[Settings::getAppResourcesPath] Found .app "
							 "Resources at: "
						  << resourcesPath << std::endl;
				*/
				return resourcesPath;
			} else
			{
				// If no Resources folder, fallback to p
				/*
				std::cout << "[Settings::getAppResourcesPath] No /Resources "
							 "folder at "
						  << resourcesPath << "; using " << p << std::endl;
				*/
				return p;
			}
		} else
		{
			/*
			std::cerr << "[Settings::getAppResourcesPath] realpath() failed, "
						 "using exePath.\n";
			*/
			// fallback to single or double dirname calls as well
			std::string p = exePath;
			p = dirname((char *)p.c_str());
			p = dirname((char *)p.c_str());
			p += "/Resources";
			return p;
		}
	}

	// Fallback if _NSGetExecutablePath failed
	// std::cerr << "[Settings::getAppResourcesPath] _NSGetExecutablePath failed.\n";
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
	return std::string(home) + "/ned/settings/ned.json";
}

Settings::Settings() : splitPos(0.3f) {}

void Settings::loadSettings()
{
	std::string userSettingsPath = getUserSettingsPath();

	// Handle missing settings file
	if (!fs::exists(userSettingsPath))
	{
		fs::create_directories(fs::path(userSettingsPath).parent_path());

		std::string bundleDefaults = getAppResourcesPath() + "/settings/ned.json";
		if (fs::exists(bundleDefaults))
		{
			fs::copy_file(bundleDefaults, userSettingsPath, fs::copy_options::overwrite_existing);
			std::cout << "[Settings] Copied default .ned.json -> " << userSettingsPath << std::endl;
		}
	}

	// Load or initialize settings
	settingsPath = userSettingsPath;
	try
	{
		std::ifstream settingsFile(settingsPath);
		if (settingsFile.is_open())
		{
			settingsFile >> settings;
			std::cout << "[Settings] Loaded from: " << settingsPath << std::endl;
		}
	} catch (const std::exception &e)
	{
		std::cerr << "[Settings] Load error: " << e.what() << std::endl;
		settings = json::object();
	}

	const std::vector<std::pair<std::string, json>> defaults = {{"font", "default"},
																{"backgroundColor",
																 {0.45f, 0.55f, 0.60f, 1.00f}},
																{"fontSize", 16.0f},
																{"splitPos", 0.3f},
																{"theme", "default"},
																{"treesitter", true},
																{"rainbow", true},
																{"shader_toggle", true},
																{"scanline_intensity", 0.2},
																{"pixelation_intensity", 0.1},
																{"pixel_width", 750},
																{"burnin_intensity", 0.95},
																{"curvature_intensity", 0.2},
																{"vignet_intensity", 0.15},
																{"bloom_intensity", 0.15},
																{"colorshift_intensity", 2.0},
																{"static_intensity", 0.09},
																{"jitter_intensity", 1.0}};

	for (const auto &[key, value] : defaults)
	{
		if (!settings.contains(key))
		{
			settings[key] = value;
		}
	}

	// Special case for font name
	currentFontName = settings.value("font", "default");

	// Handle theme defaults
	if (!settings.contains("themes"))
	{
		settings["themes"] = {{"default",
							   {{"function", {1.0f, 1.0f, 1.0f, 1.0f}},
								{"text", {1.0f, 1.0f, 1.0f, 1.0f}},
								{"type", {0.4f, 0.8f, 0.4f, 1.0f}},
								{"variable", {0.8f, 0.6f, 0.7f, 1.0f}},
								{"background", {0.2f, 0.2f, 0.2f, 1.0f}},
								{"keyword", {0.0f, 0.4f, 1.0f, 1.0f}},
								{"string", {0.87f, 0.87f, 0.0f, 1.0f}},
								{"number", {0.0f, 0.8f, 0.8f, 1.0f}},
								{"comment", {0.5f, 0.5f, 0.5f, 1.0f}}}}};
	}

	// Update member variables
	currentFontSize = settings.value("fontSize", 16.0f);
	splitPos = settings.value("splitPos", 0.3f);

	// Track modification time
	lastSettingsModification =
		fs::exists(settingsPath) ? fs::last_write_time(settingsPath) : fs::file_time_type::min();
}

void Settings::saveSettings()
{
	std::ofstream settingsFile(settingsPath);
	if (settingsFile.is_open())
	{
		settingsFile << std::setw(4) << settings << std::endl;
	}

	if (fs::exists(settingsPath))
	{
		lastSettingsModification = fs::last_write_time(settingsPath);
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
	if (currentModification <= lastSettingsModification)
		return;

	std::cout << "[Settings] .ned.json was modified externally, reloading" << std::endl;
	json oldSettings = settings;
	loadSettings();
	lastSettingsModification = currentModification;

	// List of keys to check for changes
	const std::vector<std::string> checkKeys = {"backgroundColor",
												"fontSize",
												"splitPos",
												"rainbow",
												"treesitter",
												"shader_toggle",
												"scanline_intensity",
												"burnin_intensity",
												"curvature_intensity",
												"colorshift_intensity",
												"bloom_intensity",
												"static_intensity",
												"jitter_intensity",
												"pixelation_intensity",
												"pixel_width",
												"vignet_intensity",
												"font"};

	// Check for any changes in monitored keys
	bool hasChanges = std::any_of(checkKeys.begin(), checkKeys.end(), [&](const auto &key) {
		return oldSettings[key] != settings[key];
	});

	// Check theme-related changes
	const std::vector<std::string> themeKeys = {"theme", "themes"};
	bool themeChanges = std::any_of(themeKeys.begin(), themeKeys.end(), [&](const auto &key) {
		return oldSettings[key] != settings[key];
	});

	if (hasChanges || themeChanges)
	{
		settingsChanged = true;
		themeChanged = themeChanges;
		fontChanged = (oldSettings["font"] != settings["font"]);
	}
}
void Settings::renderSettingsWindow()
{
	if (!showSettingsWindow)
		return;
	ImVec2 main_viewport_size = ImGui::GetMainViewport()->Size;

	// Calculate proportional size
	ImVec2 window_size(main_viewport_size.x * 0.75f, // 75% of screen width
					   main_viewport_size.y * 0.85f	 // 85% of screen height
	);

	ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 1.0f);
	ImGui::SetNextWindowSize(window_size, ImGuiCond_Always);

	ImGui::SetNextWindowPos(ImVec2(ImGui::GetIO().DisplaySize.x * 0.5f,
								   ImGui::GetIO().DisplaySize.y * 0.5f),
							ImGuiCond_Always,
							ImVec2(0.5f, 0.5f));

	ImGuiWindowFlags windowFlags =
		ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
		ImGuiWindowFlags_AlwaysVerticalScrollbar | ImGuiWindowFlags_Modal;

	// Push custom styles
	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 10.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.0f);
	ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.2f, 0.2f, 0.2f, 1.0f));
	ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.3f, 0.3f, 0.3f, 1.0f));
	ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.15f, 0.15f, 0.15f, 1.0f));

	ImGui::PushStyleColor(ImGuiCol_ScrollbarBg, ImVec4(0.15f, 0.15f, 0.15f, 0.0f));
	ImGui::PushStyleColor(ImGuiCol_ScrollbarGrab, ImVec4(0.3f, 0.3f, 0.3f, 1.0f));
	ImGui::PushStyleColor(ImGuiCol_ScrollbarGrabHovered, ImVec4(0.4f, 0.4f, 0.4f, 1.0f));
	ImGui::PushStyleColor(ImGuiCol_ScrollbarGrabActive, ImVec4(0.5f, 0.5f, 0.5f, 1.0f));
	ImGui::PushStyleVar(ImGuiStyleVar_ScrollbarSize, 14.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_ScrollbarRounding, 10.0f);
	ImGui::Begin("Settings", nullptr, windowFlags);

	// Handle focus detection
	static bool wasFocused = false;
	bool isFocused = ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);

	if (wasFocused && !isFocused)
	{
		showSettingsWindow = false;
		saveSettings();
	} else if (!wasFocused && isFocused)
	{
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
	ImGui::Spacing();
	ImGui::Spacing();
	ImGui::Spacing();
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
	std::string previewName = currentFontName;

	// Clean up display name (remove extension and hyphens)
	if (previewName != "System Default")
	{
		previewName = previewName.substr(0, previewName.find_last_of("."));
		std::replace(previewName.begin(), previewName.end(), '-', ' ');
	}

	if (ImGui::BeginCombo("Editor Font", previewName.c_str()))
	{
		for (const auto &fontFile : fontNames)
		{
			bool isSelected = (fontFile == settings["font"]);
			std::string displayName = fontFile;

			// Format display name
			if (fontFile != "System Default")
			{
				displayName = fontFile.substr(0, fontFile.find_last_of("."));
				std::replace(displayName.begin(), displayName.end(), '-', ' ');
			}

			if (ImGui::Selectable(displayName.c_str(), isSelected))
			{
				settings["font"] = fontFile;
				currentFontName = fontFile;
				fontChanged = true;
				settingsChanged = true;
				saveSettings();
			}
			if (isSelected)
			{
				ImGui::SetItemDefaultFocus();
			}
		}
		ImGui::EndCombo();
	}
	ImGui::Spacing();
	ImGui::Spacing();
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
	ImGui::Spacing();
	ImGui::Spacing();
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

	ImGui::Spacing();
	ImGui::Spacing();
	ImGui::Spacing();
	ImGui::Spacing();
	ImGui::TextUnformatted("Toggle Settings");
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

	ImGui::Spacing();
	ImGui::Spacing();
	ImGui::Spacing();
	ImGui::Spacing();
	ImGui::TextUnformatted("GL Shaders");
	ImGui::Separator();
	ImGui::Spacing();
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

	// Scanline Intensity Slider
	ImGui::Spacing();
	static float tempScanlineIntensity = settings["scanline_intensity"].get<float>();
	if (ImGui::SliderFloat("Scanline Intensity",
						   &tempScanlineIntensity,
						   0.00f,
						   1.00f,
						   "%.02f", // Changed from %.01f to show 2 decimal places
						   ImGuiSliderFlags_AlwaysClamp))
	{
		settings["scanline_intensity"] = tempScanlineIntensity;
		settingsChanged = true;
	}
	if (ImGui::IsItemDeactivatedAfterEdit())
	{
		saveSettings();
	}

	// Vignette Intensity Slider
	ImGui::Spacing();
	static float tempVignetIntensity = settings["vignet_intensity"].get<float>();
	if (ImGui::SliderFloat("Vignette Intensity",
						   &tempVignetIntensity,
						   0.00f,
						   1.00f,
						   "%.02f", // Changed precision
						   ImGuiSliderFlags_AlwaysClamp))
	{
		settings["vignet_intensity"] = tempVignetIntensity;
		settingsChanged = true;
	}
	if (ImGui::IsItemDeactivatedAfterEdit())
	{
		saveSettings();
	}
	// bloom Intensity Slider
	ImGui::Spacing();
	static float tempBloomIntensity = settings["bloom_intensity"].get<float>();
	if (ImGui::SliderFloat("Bloom Intensity",
						   &tempBloomIntensity,
						   0.00f,
						   1.00f,
						   "%.02f", // Changed precision
						   ImGuiSliderFlags_AlwaysClamp))
	{
		settings["bloom_intensity"] = tempBloomIntensity;
		settingsChanged = true;
	}
	if (ImGui::IsItemDeactivatedAfterEdit())
	{
		saveSettings();
	}
	// Add after vignette slider
	ImGui::Spacing();
	static float tempStaticIntensity = settings["static_intensity"].get<float>();
	if (ImGui::SliderFloat("Static Intensity",
						   &tempStaticIntensity,
						   0.00f,
						   0.5f, // Max 0.5 to prevent overwhelming effect
						   "%.03f",
						   ImGuiSliderFlags_AlwaysClamp))
	{
		settings["static_intensity"] = tempStaticIntensity;
		settingsChanged = true;
	}
	if (ImGui::IsItemDeactivatedAfterEdit())
	{
		saveSettings();
	}
	ImGui::Spacing();

	static float tempColorShift = settings["colorshift_intensity"].get<float>();
	if (ImGui::SliderFloat("RGB Shift Intensity",
						   &tempColorShift,
						   0.0f,  // Min
						   10.0f, // Max (200% of original)
						   "%.02f",
						   ImGuiSliderFlags_AlwaysClamp))
	{
		settings["colorshift_intensity"] = tempColorShift;
		settingsChanged = true;
	}
	if (ImGui::IsItemDeactivatedAfterEdit())
	{
		saveSettings();
	}
	ImGui::Spacing();

	static float tempcurvature = settings["curvature_intensity"].get<float>();
	if (ImGui::SliderFloat("Curvature (bugged)",
						   &tempcurvature,
						   0.0f, // Min
						   0.5f, // Max (200% of original)
						   "%.02f",
						   ImGuiSliderFlags_AlwaysClamp))
	{
		settings["curvature_intensity"] = tempcurvature;
		settingsChanged = true;
	}
	if (ImGui::IsItemDeactivatedAfterEdit())
	{
		saveSettings();
	}
	ImGui::Spacing();

	static float tempburnin = settings["burnin_intensity"].get<float>();
	if (ImGui::SliderFloat("Burn-in Intensity",
						   &tempburnin,
						   0.9f,  // Min
						   0.99f, // Max (200% of original)
						   "%.005f",
						   ImGuiSliderFlags_AlwaysClamp))
	{
		settings["burnin_intensity"] = tempburnin;
		settingsChanged = true;
	}
	if (ImGui::IsItemDeactivatedAfterEdit())
	{
		saveSettings();
	}
	// In renderSettingsWindow() after other sliders:
	ImGui::Spacing();
	static float tempJitter = settings["jitter_intensity"].get<float>();
	if (ImGui::SliderFloat("Jitter Intensity",
						   &tempJitter,
						   0.0f,  // Min (no jitter)
						   10.0f, // Max (2x original)
						   "%.02f",
						   ImGuiSliderFlags_AlwaysClamp))
	{
		settings["jitter_intensity"] = tempJitter;
		settingsChanged = true;
	}
	if (ImGui::IsItemDeactivatedAfterEdit())
	{
		saveSettings();
	}

	ImGui::Spacing();
	/*
	static float tempPixelWidth = settings["pixel_width"].get<float>();
	if (ImGui::SliderFloat(
			"Pixel Width", &tempPixelWidth, 250.0f, 5000.0f, "%1f", ImGuiSliderFlags_AlwaysClamp))
	{
		settings["pixel_width"] = tempPixelWidth;
		settingsChanged = true;
	}
	if (ImGui::IsItemDeactivatedAfterEdit())
	{
		saveSettings();
	}

	ImGui::Spacing();
	*/
	static float tempPixelelation = settings["pixelation_intensity"].get<float>();
	if (ImGui::SliderFloat("Pixelation Lines",
						   &tempPixelelation,
						   -1.00f,
						   1.00f,
						   "%.002f",
						   ImGuiSliderFlags_AlwaysClamp))
	{
		settings["pixelation_intensity"] = tempPixelelation;
		settingsChanged = true;
	}
	if (ImGui::IsItemDeactivatedAfterEdit())
	{
		saveSettings();
	}

	ImGui::Spacing();
	ImGui::Spacing();
	ImGui::Spacing();
	ImGui::Spacing();

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
	ImGui::PopStyleColor(7);
	ImGui::PopStyleVar(5);
}
