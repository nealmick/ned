#pragma once
#include "settings.h"
#include "../editor/editor.h"
#include "../editor/editor_highlight.h" 
#include "../files/files.h"				
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

std::string Settings::getAppResourcesPath()
{
#ifdef __APPLE__
	char exePath[MAXPATHLEN];
	uint32_t size = sizeof(exePath);
	if (_NSGetExecutablePath(exePath, &size) == 0)
	{
		char resolvedPath[MAXPATHLEN];
		if (realpath(exePath, resolvedPath) != nullptr)
		{
			std::string p = resolvedPath;
			p = dirname((char *)p.c_str());
			p = dirname((char *)p.c_str());
			std::string resourcesPath = p + "/Resources";
			if (fs::exists(resourcesPath) && fs::is_directory(resourcesPath))
			{
				return resourcesPath;
			} else
			{
				return p;
			}
		} else
		{
			std::string p = exePath;
			p = dirname((char *)p.c_str());
			p = dirname((char *)p.c_str());
			p += "/Resources";
			return p;
		}
	}
#elif defined(__linux__)
	// Linux implementation
	char exePath[PATH_MAX];
	ssize_t count = readlink("/proc/self/exe", exePath, PATH_MAX);
	if (count != -1)
	{
		exePath[count] = '\0';
		std::string p = dirname(exePath);

		// Look for resources in common Linux locations
		std::vector<std::string> pathsToCheck = {
			p + "/../share/ned", // Typical for system-wide install
			p + "/resources",	 // Local build
			p					 // Fallback
		};

		for (const auto &path : pathsToCheck)
		{
			if (fs::exists(path) && fs::is_directory(path))
			{
				return path;
			}
		}
	}
	return "."; // Fallback
#else
	return ".";
#endif
}

std::string Settings::getUserSettingsPath()
{
	const char *home = getenv("HOME");
	if (!home)
	{
		std::cerr << "[Settings] WARNING: HOME environment variable not found. Using "
					 "'./ned/settings/ned.json' for primary settings."
				  << std::endl;
		return "ned/settings/ned.json";
	}
	return std::string(home) + "/ned/settings/ned.json";
}

Settings::Settings() : splitPos(0.3f) { currentFontSize = 20.0f; }

void Settings::loadSettings()
{
	std::string primarySettingsFilePath = getUserSettingsPath();
	fs::path primarySettingsDir = fs::path(primarySettingsFilePath).parent_path();

	if (!fs::exists(primarySettingsDir))
	{
		try
		{
			fs::create_directories(primarySettingsDir);
			std::cout << "[Settings] Created settings directory: " << primarySettingsDir.string()
					  << std::endl;
		} catch (const fs::filesystem_error &e)
		{
			std::cerr << "[Settings] Error creating settings directory "
					  << primarySettingsDir.string() << ": " << e.what() << std::endl;
		}
	}
	json primaryJsonConfig;
	bool primaryJsonModified = false;
	if (!fs::exists(primarySettingsFilePath))
	{
		std::cout << "[Settings] Primary settings file " << primarySettingsFilePath
				  << " not found.\n";

		std::string bundleSettingsDir = getAppResourcesPath() + "/settings";
		std::vector<std::string> filesToCopy = {"ned.json","amber.json", "test.json", "solarized.json", "solarized-light.json"};

		for (const auto &filename : filesToCopy)
		{
			std::string sourcePath = bundleSettingsDir + "/" + filename;
			std::string destPath = fs::path(primarySettingsDir) / filename;

			if (fs::exists(sourcePath))
			{
				try
				{
					fs::copy_file(sourcePath, destPath, fs::copy_options::overwrite_existing);
					std::cout << "[Settings] Copied " << filename << " from bundle to " << destPath
							  << "\n";
				} catch (const fs::filesystem_error &e)
				{
					std::cerr << "[Settings] Error copying " << filename << ": " << e.what()
							  << "\n";
					// Only critical error for ned.json
					if (filename == "ned.json")
					{
						std::cerr << "CRITICAL: Failed to copy primary settings file\n";
					}
				}
			} else
			{
				std::cerr << (filename == "ned.json" ? "CRITICAL" : "WARNING") << ": Bundle file "
						  << sourcePath << " not found\n";
				if (filename == "ned.json")
				{
					return; // Can't proceed without primary
				}
			}
		}
	}
	if (fs::exists(primarySettingsFilePath))
	{
		try
		{
			std::ifstream primaryFile(primarySettingsFilePath);
			if (primaryFile.is_open())
			{
				primaryFile >> primaryJsonConfig;
			} else
			{
				std::cerr << "[Settings] Failed to open primary settings file "
						  << primarySettingsFilePath << " for reading." << std::endl;
			}
		} catch (const json::parse_error &e)
		{
			std::cerr << "[Settings] Error parsing primary " << primarySettingsFilePath << ": "
					  << e.what() << ". Using empty config for primary." << std::endl;
			primaryJsonConfig = json::object();
		} catch (const std::exception &e)
		{
			std::cerr << "[Settings] Generic error loading primary " << primarySettingsFilePath
					  << ": " << e.what() << ". Using empty config for primary." << std::endl;
			primaryJsonConfig = json::object();
		}
	} else
	{
		std::cerr << "[Settings] Primary settings file " << primarySettingsFilePath
				  << " could not be accessed after setup attempt. Using empty primary config."
				  << std::endl;
		primaryJsonConfig = json::object();
	}
	std::string activeSettingsFilename;
	if (primaryJsonConfig.contains("settings_file") &&
		primaryJsonConfig["settings_file"].is_string())
	{
		activeSettingsFilename = primaryJsonConfig["settings_file"].get<std::string>();
	} else
	{
		std::cout << "[Settings] 'settings_file' key missing or invalid in "
				  << primarySettingsFilePath
				  << ". Defaulting to 'ned.json' and updating primary file." << std::endl;
		activeSettingsFilename = "ned.json";
		primaryJsonConfig["settings_file"] = activeSettingsFilename;
		primaryJsonModified = true;
	}
	if (primaryJsonModified)
	{
		try
		{
			std::ofstream primaryFileOut(primarySettingsFilePath);
			if (primaryFileOut.is_open())
			{
				primaryFileOut << std::setw(4) << primaryJsonConfig << std::endl;
				std::cout << "[Settings] Saved " << primarySettingsFilePath
						  << " after ensuring 'settings_file' key." << std::endl;
			} else
			{
				std::cerr << "[Settings] Failed to open " << primarySettingsFilePath
						  << " for writing to update 'settings_file' key." << std::endl;
			}
		} catch (const std::exception &e)
		{
			std::cerr << "[Settings] Error saving " << primarySettingsFilePath
					  << " after ensuring 'settings_file' key: " << e.what() << std::endl;
		}
	}
	this->settingsPath = (primarySettingsDir / activeSettingsFilename).string();
	std::cout << "[Settings] Active settings file target: " << this->settingsPath << std::endl;
	bool activeSettingsFileNewlyCreatedOrModified = false;
	if (activeSettingsFilename != "ned.json" && !fs::exists(this->settingsPath))
	{
		std::cout << "[Settings] Active settings file " << this->settingsPath << " not found."
				  << std::endl;
		if (fs::exists(primarySettingsFilePath))
		{
			try
			{
				fs::copy_file(primarySettingsFilePath,
							  this->settingsPath,
							  fs::copy_options::overwrite_existing);
				std::cout << "[Settings] Copied " << primarySettingsFilePath << " to "
						  << this->settingsPath << " as a base." << std::endl;
				activeSettingsFileNewlyCreatedOrModified = true;
			} catch (const fs::filesystem_error &e)
			{
				std::cerr << "[Settings] Error copying " << primarySettingsFilePath << " to "
						  << this->settingsPath << ": " << e.what() << std::endl;
				std::cout << "[Settings] Fallback: Using " << primarySettingsFilePath
						  << " as active settings file due to copy error." << std::endl;
				this->settingsPath = primarySettingsFilePath;
				activeSettingsFilename = fs::path(primarySettingsFilePath).filename().string();
			}
		} else
		{
			std::cerr << "[Settings] Primary settings file " << primarySettingsFilePath
					  << " not found. Cannot use it to create " << this->settingsPath << std::endl;
			std::cout << "[Settings] Fallback: Attempting to use " << primarySettingsFilePath
					  << " as active settings path." << std::endl;
			this->settingsPath = primarySettingsFilePath;
			activeSettingsFilename = fs::path(primarySettingsFilePath).filename().string();
		}
	}
	settings = json::object();
	if (fs::exists(this->settingsPath))
	{
		try
		{
			std::ifstream settingsFileStream(this->settingsPath);
			if (settingsFileStream.is_open())
			{
				settingsFileStream >> settings;
				std::cout << "[Settings] Successfully loaded settings from: " << this->settingsPath
						  << std::endl;
			} else
			{
				std::cerr << "[Settings] Failed to open active settings file " << this->settingsPath
						  << " for reading. Using empty settings." << std::endl;
			}
		} catch (const json::parse_error &e)
		{
			std::cerr << "[Settings] Error parsing active settings file " << this->settingsPath
					  << ": " << e.what() << ". Using empty settings." << std::endl;
			settings = json::object();
		} catch (const std::exception &e)
		{
			std::cerr << "[Settings] Generic error loading " << this->settingsPath << ": "
					  << e.what() << ". Using empty settings." << std::endl;
			settings = json::object();
		}
	} else
	{
		std::cout << "[Settings] Active settings file " << this->settingsPath
				  << " does not exist (even after creation attempt). Starting with empty settings."
				  << std::endl;
	}
	if (activeSettingsFilename != "ned.json" && settings.contains("settings_file"))
	{
		std::cout
			<< "[Settings] Removing 'settings_file' key from non-primary active settings file: "
			<< this->settingsPath << std::endl;
		settings.erase("settings_file");
		activeSettingsFileNewlyCreatedOrModified = true;
	}
	const std::vector<std::pair<std::string, json>> defaults = {
		{"backgroundColor",
		 json::array(
			 {0.05816289037466049, 0.19437342882156372, 0.1578674018383026, 1.0000001192092896})},
		{"bloom_intensity", 0.75},
		{"burnin_intensity", 0.9525200128555298},
		{"colorshift_intensity", 0.8999999761581421},
		{"curvature_intensity", 0.0},
		{"font", "SourceCodePro-Regular"},
		{"fontSize", 20.0f},
		{"jitter_intensity", 2.809999942779541},
		{"pixel_width", 5000.0},
		{"pixelation_intensity", -0.10999999940395355},
		{"rainbow", true},
		{"scanline_intensity", 0.20000000298023224},
		{"shader_toggle", true},
		{"splitPos", 0.2142857164144516},
		{"static_intensity", 0.20800000429153442},
		{"theme", "default"},
		{"treesitter", true},
		{"vignet_intensity", 0.25},
	 	{"mac_background_opacity", 0.5},
    	{"mac_blur_enabled", true},};
	for (const auto &[key, value] : defaults)
	{
		if (!settings.contains(key))
		{
			settings[key] = value;
			activeSettingsFileNewlyCreatedOrModified = true;
			std::cout << "[Settings] Applied default for key '" << key << "' to "
					  << this->settingsPath << std::endl;
		}
	}
	if (!settings.contains("themes") || !settings["themes"].is_object())
	{
		settings["themes"] = {
			{"default",
			 {{"background", json::array({0.2, 0.2, 0.2, 1.0})},
			  {"comment",
			   json::array({0.4565933346748352,
							0.4565933346748352,
							0.4565933346748352,
							0.8999999761581421})},
			  {"function",
			   json::array({0.6752017140388489, 0.3185149133205414, 0.7726563811302185, 1.0})},
			  {"keyword", json::array({0.0, 0.5786033272743225, 0.9643363952636719, 1.0})},
			  {"number",
			   json::array({0.6439791321754456, 0.3536583185195923, 0.7698838710784912, 1.0})},
			  {"string",
			   json::array({0.08516374975442886, 0.6660587787628174, 0.6660587787628174, 1.0})},
			  {"text", json::array({0.680115282535553, 0.680115282535553, 0.680115282535553, 1.0})},
			  {"type",
			   json::array({0.6007077097892761, 0.7665653228759766, 0.44595927000045776, 1.0})},
			  {"variable",
			   json::array({0.3506303131580353, 0.7735447883605957, 0.8863282203674316, 1.0})}}}};
		activeSettingsFileNewlyCreatedOrModified = true;
		std::cout << "[Settings] Applied default themes structure to " << this->settingsPath
				  << std::endl;
	}
	currentFontName = settings.value("font", "SourceCodePro-Regular");
	currentFontSize = settings.value("fontSize", 20.0f);
	splitPos = settings.value("splitPos", 0.2142857164144516f);
	if (activeSettingsFileNewlyCreatedOrModified)
	{
		std::cout << "[Settings] Saving active settings file " << this->settingsPath
				  << " due to initial setup or modifications." << std::endl;
		saveSettings();
	}
	if (fs::exists(this->settingsPath))
	{
		try
		{
			lastSettingsModification = fs::last_write_time(this->settingsPath);
		} catch (const fs::filesystem_error &e)
		{
			std::cerr << "[Settings] Error getting last write time for " << this->settingsPath
					  << ": " << e.what() << std::endl;
			lastSettingsModification = fs::file_time_type::min();
		}
	} else
	{
		lastSettingsModification = fs::file_time_type::min();
	}
	settingsChanged = false;
	themeChanged = false;
	fontChanged = false;
	fontSizeChanged = false;
	// profileJustSwitched is handled by renderSettingsWindow
}

void Settings::saveSettings()
{
	if (this->settingsPath.empty())
	{
		std::cerr << "[Settings] Error: settingsPath is empty, cannot save settings." << std::endl;
		return;
	}

	std::ofstream settingsFile(this->settingsPath);
	if (settingsFile.is_open())
	{
		settingsFile << std::setw(4) << settings << std::endl;
		settingsFile.close();
	} else
	{
		std::cerr << "[Settings] Failed to open " << this->settingsPath << " for saving."
				  << std::endl;
		return;
	}

	if (fs::exists(this->settingsPath))
	{
		try
		{
			lastSettingsModification = fs::last_write_time(this->settingsPath);
		} catch (const fs::filesystem_error &e)
		{
			std::cerr << "[Settings] Error getting last write time after saving "
					  << this->settingsPath << ": " << e.what() << std::endl;
		}
	}
}

void Settings::checkSettingsFile()
{
	if (this->settingsPath.empty() || !fs::exists(this->settingsPath))
	{
		return;
	}

	try
	{
		auto currentModification = fs::last_write_time(this->settingsPath);
		if (currentModification <= lastSettingsModification)
			return;

		std::cout << "[Settings] Active settings file " << this->settingsPath
				  << " was modified externally, reloading." << std::endl;

		json oldSettings = settings;
		loadSettings();

		if (oldSettings.contains("fontSize") && settings.contains("fontSize") &&
			oldSettings["fontSize"] != settings["fontSize"])
		{
			if (settings["fontSize"].get<float>() != currentFontSize)
			{ 
				fontSizeChanged = true; // Simpler, as currentFontSize is already correct.
			}
		}
		if (oldSettings.contains("font") && settings.contains("font") &&
			oldSettings["font"] != settings["font"])
		{
			fontChanged = true;
		}
		if (oldSettings.contains("theme") && settings.contains("theme") &&
			oldSettings["theme"] != settings["theme"])
		{
			themeChanged = true;
		}
		if (oldSettings.contains("themes") && settings.contains("themes") &&
			oldSettings["themes"] != settings["themes"])
		{
			themeChanged = true; 
		}
		if (oldSettings.contains("mac_background_opacity") && 
		    settings.contains("mac_background_opacity") &&
		    oldSettings["mac_background_opacity"] != settings["mac_background_opacity"]) 
		{
		    settingsChanged = true;
		}

		if (oldSettings.contains("mac_blur_enabled") && 
		    settings.contains("mac_blur_enabled") &&
		    oldSettings["mac_blur_enabled"] != settings["mac_blur_enabled"]) 
		{
		    settingsChanged = true;
		}
		const std::vector<std::string> checkKeys = {
			"backgroundColor",
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
			"mac_background_opacity",
			"mac_blur_enabled",};
		for (const auto &key : checkKeys)
		{
			if (oldSettings.contains(key) && settings.contains(key))
			{
				if (oldSettings[key] != settings[key])
				{
					settingsChanged = true;
					break;
				}
			} else if (oldSettings.contains(key) != settings.contains(key))
			{
				settingsChanged = true;
				break;
			}
		}
		if (settingsChanged || fontChanged || fontSizeChanged || themeChanged)
		{
			gSettings.profileJustSwitched =
				true; 
		}

	} catch (const fs::filesystem_error &e)
	{
		std::cerr << "[Settings] Filesystem error checking settings file " << this->settingsPath
				  << ": " << e.what() << std::endl;
	} catch (const std::exception &e)
	{
		std::cerr << "[Settings] Error during settings file check for " << this->settingsPath
				  << ": " << e.what() << std::endl;
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

	std::vector<std::string> availableProfileFiles;
	std::string currentActiveFilename = "ned.json";
	fs::path userSettingsDir = fs::path(getUserSettingsPath()).parent_path();

	if (fs::exists(userSettingsDir) && fs::is_directory(userSettingsDir))
	{
		for (const auto &entry : fs::directory_iterator(userSettingsDir))
		{
			if (entry.is_regular_file() && entry.path().extension() == ".json")
			{
				availableProfileFiles.push_back(entry.path().filename().string());
			}
		}
		std::sort(availableProfileFiles.begin(),
				  availableProfileFiles.end());
	}

	if (!this->settingsPath.empty())
	{
		currentActiveFilename = fs::path(this->settingsPath).filename().string();
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
		availableProfileFiles.insert(availableProfileFiles.begin(),
									 currentActiveFilename);
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

					std::string primarySettingsFilePath = getUserSettingsPath();
					json primaryJson;
					std::ifstream primaryFileIn(primarySettingsFilePath);
					if (primaryFileIn.is_open())
					{
						try
						{
							primaryFileIn >> primaryJson;
						} catch (const json::parse_error &e)
						{
							std::cerr << "[Settings] Error parsing primary "
									  << primarySettingsFilePath
									  << " for profile switch: " << e.what() << std::endl;
							primaryJson = json::object();
						}
						primaryFileIn.close();
					} else
					{
						std::cerr << "[Settings] Could not open primary " << primarySettingsFilePath
								  << " for profile switch." << std::endl;
						primaryJson = json::object();
					}

					primaryJson["settings_file"] = filename;

					std::ofstream primaryFileOut(primarySettingsFilePath);
					if (primaryFileOut.is_open())
					{
						primaryFileOut << std::setw(4) << primaryJson << std::endl;
						primaryFileOut.close();
						std::cout << "[Settings] Updated " << primarySettingsFilePath
								  << " to use profile: " << filename << std::endl;
					} else
					{
						std::cerr << "[Settings] Failed to save " << primarySettingsFilePath
								  << " for profile switch." << std::endl;
					}

					loadSettings();

					profileJustSwitched = true;
					settingsChanged = true;
					fontChanged = true;
					settingsChanged = true;
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
		tempFontSize = getFontSize();
	}
	if (ImGui::SliderFloat("Font Size", &tempFontSize, 4.0f, 32.0f, "%.0f"))
	{
		if (settings.value("fontSize", 0.0f) != tempFontSize)
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
		settings["backgroundColor"] = {bgColor.x, bgColor.y, bgColor.z, bgColor.w};
		settingsChanged = true;
		saveSettings();
	}



	#ifdef __APPLE__
	ImGui::Spacing();
	ImGui::TextUnformatted("macOS Settings");
	ImGui::Separator();
	ImGui::Spacing();

	// Get current values
	float currentOpacity = settings.value("mac_background_opacity", 0.5f);
	bool currentBlurEnabled = settings.value("mac_blur_enabled", true);

	// Opacity slider
	if (ImGui::SliderFloat("Background Opacity", &currentOpacity, 0.0f, 1.0f, "%.2f")) {
	    settings["mac_background_opacity"] = currentOpacity;
	    settingsChanged = true;
	    saveSettings();
	}

	// Blur checkbox
	if (ImGui::Checkbox("Enable Background Blur", &currentBlurEnabled)) {
	    settings["mac_blur_enabled"] = currentBlurEnabled;
	    settingsChanged = true;
	    saveSettings();
	}
	#endif


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
				themeChanged = true;
				settingsChanged = true;
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

	auto shaderFloatSlider = [&](const char *label,
								 const char *key,
								 float min_val,
								 float max_val,
								 const char *format,
								 float default_val) {
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
	};

	shaderFloatSlider("Scanline", "scanline_intensity", 0.00f, 1.00f, "%.02f", 0.20f);
	shaderFloatSlider("Vignette", "vignet_intensity", 0.00f, 1.00f, "%.02f", 0.25f);
	shaderFloatSlider("Bloom", "bloom_intensity", 0.00f, 1.00f, "%.02f", 0.75f);
	shaderFloatSlider("Static", "static_intensity", 0.00f, 0.5f, "%.03f", 0.208f);
	shaderFloatSlider("RGB Shift", "colorshift_intensity", 0.0f, 10.0f, "%.02f", 0.90f);
	shaderFloatSlider("Curvature(bugged)", "curvature_intensity", 0.0f, 0.5f, "%.02f", 0.0f);
	shaderFloatSlider("Burn-in", "burnin_intensity", 0.9f, 0.999f, "%.03f", 0.9525f);
	shaderFloatSlider("Jitter", "jitter_intensity", 0.0f, 10.0f, "%.02f", 2.81f);
	shaderFloatSlider("Pixel lines", "pixelation_intensity", -1.00f, 1.00f, "%.03f", -0.11f);

	ImGui::Spacing();
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

	ImGui::End();
	ImGui::PopStyleColor(7);
	ImGui::PopStyleVar(5);

	if (profileJustSwitched)
	{
		profileJustSwitched = false;
	}
}