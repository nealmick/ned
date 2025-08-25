#include "settings_file_manager.h"
#include <algorithm>
#include <fstream>
#include <iomanip>
#include <iostream>
#ifndef PLATFORM_WINDOWS
#include <libgen.h>
#include <unistd.h>
#endif

#ifdef __APPLE__
#include <mach-o/dyld.h>
#include <sys/param.h>
#endif
#ifdef __linux__
#include <linux/limits.h>
#include <sys/types.h> // For ssize_t
#endif

#include "terminal.h"
#include <map>

namespace fs = std::filesystem;
using json = nlohmann::json;

SettingsFileManager gSettingsFileManager;

SettingsFileManager::SettingsFileManager() {}

std::string SettingsFileManager::getAppResourcesPath()
{
	// First, try to find the ned project directory relative to current working directory
	// This is needed for embedded mode where the library is used from a different project
	std::string currentDir = fs::current_path().string();
	std::vector<std::string> nedPathsToCheck = {
		currentDir + "/ned",					// If running from ImGui_Ned_Embed/
		currentDir + "/../ned",					// If running from ImGui_Ned_Embed/build/
		currentDir + "/../../ned",				// If running from deeper build dir
		currentDir + "/../ImGui_Ned_Embed/ned", // Alternative path
		currentDir + "/ned/ned",				// Nested ned directory
		currentDir + "/../ned/ned",				// Nested ned directory from build
	};

	for (const auto &path : nedPathsToCheck)
	{
		if (fs::exists(path) && fs::is_directory(path))
		{
			// Check if this path contains fonts and settings
			if (fs::exists(path + "/fonts") && fs::exists(path + "/settings"))
			{
				std::cout << "[Settings] Using ned resource path: " << path << std::endl;
				return path;
			}
		}
	}

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
		std::string p = dirname(exePath); // If exe is /usr/lib/Ned/ned, p is /usr/lib/Ned

		// Look for resources in common Linux locations
		std::vector<std::string> pathsToCheck = {
			p + "/../share/Ned", // <<-- THIS IS THE KEY CHANGE (Ned with capital N)
			p + "/resources",	 // For local build (e.g., ./build/resources)
			p					 // Fallback (e.g. binary dir itself)
		};

		for (const auto &path : pathsToCheck)
		{
			// Ensure the path actually exists before returning it
			// The actual check for "settings/ned.json" happens later in loadSettings
			if (fs::exists(path) && fs::is_directory(path))
			{
				std::cout << "[Settings] Using app resource path: " << path << std::endl;
				return path;
			}
		}
		std::cerr << "[Settings] Warning: Could not find a valid app resource "
					 "path from checks. "
					 "Defaulting to executable directory."
				  << std::endl;
		// If none of the preferred paths exist, return the executable's
		// directory as a last resort. This might happen if `p +
		// "/../share/Ned"` or `p + "/resources"` don't exist.
		if (fs::exists(p) && fs::is_directory(p))
			return p;
	}
	std::cerr << "[Settings] CRITICAL: Could not determine app resources path "
				 "for Linux (readlink "
				 "failed or no valid path found)."
			  << std::endl;
#endif
	return "."; // Absolute fallback
}

std::string SettingsFileManager::getUserSettingsPath()
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

void SettingsFileManager::loadSettings(json &settings, std::string &settingsPath)
{
	std::string primarySettingsFilePath = getUserSettingsPath();
	fs::path primarySettingsDir = fs::path(primarySettingsFilePath).parent_path();

	if (!fs::exists(primarySettingsDir))
	{
		try
		{
			fs::create_directories(primarySettingsDir);
			std::cout << "[Settings] Created settings directory: "
					  << primarySettingsDir.string() << std::endl;
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
		std::vector<std::string> filesToCopy = {"lsp.json",
												"ned.json",
												"amber.json",
												"test.json",
												"solarized.json",
												"solarized-light.json",
												"custom1.json",
												"custom2.json",
												"custom3.json"};

		for (const auto &filename : filesToCopy)
		{
			std::string sourcePath = bundleSettingsDir + "/" + filename;
			std::string destPath = (fs::path(primarySettingsDir) / filename).string();

			if (fs::exists(sourcePath))
			{
				try
				{
					fs::copy_file(sourcePath,
								  destPath,
								  fs::copy_options::overwrite_existing);
					std::cout << "[Settings] Copied " << filename << " from bundle to "
							  << destPath << "\n";
				} catch (const fs::filesystem_error &e)
				{
					std::cerr << "[Settings] Error copying " << filename << ": "
							  << e.what() << "\n";
					// Only critical error for ned.json
					if (filename == "ned.json")
					{
						std::cerr << "CRITICAL: Failed to copy primary "
									 "settings file\n";
					}
				}
			} else
			{
				std::cerr << (filename == "ned.json" ? "CRITICAL" : "WARNING")
						  << ": Bundle file " << sourcePath << " not found\n";
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
			std::cerr << "[Settings] Error parsing primary " << primarySettingsFilePath
					  << ": " << e.what() << ". Using empty config for primary."
					  << std::endl;
			primaryJsonConfig = json::object();
		} catch (const std::exception &e)
		{
			std::cerr << "[Settings] Generic error loading primary "
					  << primarySettingsFilePath << ": " << e.what()
					  << ". Using empty config for primary." << std::endl;
			primaryJsonConfig = json::object();
		}
	} else
	{
		std::cerr << "[Settings] Primary settings file " << primarySettingsFilePath
				  << " could not be accessed after setup attempt. Using empty "
					 "primary config."
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
	settingsPath = (primarySettingsDir / activeSettingsFilename).string();
	bool activeSettingsFileNewlyCreatedOrModified = false;
	if (activeSettingsFilename != "ned.json" && !fs::exists(settingsPath))
	{
		std::cout << "[Settings] Active settings file " << settingsPath << " not found."
				  << std::endl;
		if (fs::exists(primarySettingsFilePath))
		{
			try
			{
				fs::copy_file(primarySettingsFilePath,
							  settingsPath,
							  fs::copy_options::overwrite_existing);
				std::cout << "[Settings] Copied " << primarySettingsFilePath << " to "
						  << settingsPath << " as a base." << std::endl;
				activeSettingsFileNewlyCreatedOrModified = true;
			} catch (const fs::filesystem_error &e)
			{
				std::cerr << "[Settings] Error copying " << primarySettingsFilePath
						  << " to " << settingsPath << ": " << e.what() << std::endl;
				std::cout << "[Settings] Fallback: Using " << primarySettingsFilePath
						  << " as active settings file due to copy error." << std::endl;
				settingsPath = primarySettingsFilePath;
				activeSettingsFilename =
					fs::path(primarySettingsFilePath).filename().string();
			}
		} else
		{
			std::cerr << "[Settings] Primary settings file " << primarySettingsFilePath
					  << " not found. Cannot use it to create " << settingsPath
					  << std::endl;
			std::cout << "[Settings] Fallback: Attempting to use "
					  << primarySettingsFilePath << " as active settings path."
					  << std::endl;
			settingsPath = primarySettingsFilePath;
			activeSettingsFilename =
				fs::path(primarySettingsFilePath).filename().string();
		}
	}
	settings = json::object();
	if (fs::exists(settingsPath))
	{
		try
		{
			std::ifstream settingsFileStream(settingsPath);
			if (settingsFileStream.is_open())
			{
				settingsFileStream >> settings;
			} else
			{
				std::cerr << "[Settings] Failed to open active settings file "
						  << settingsPath << " for reading. Using empty settings."
						  << std::endl;
			}
		} catch (const json::parse_error &e)
		{
			std::cerr << "[Settings] Error parsing active settings file " << settingsPath
					  << ": " << e.what() << ". Using empty settings." << std::endl;
			settings = json::object();
		} catch (const std::exception &e)
		{
			std::cerr << "[Settings] Generic error loading " << settingsPath << ": "
					  << e.what() << ". Using empty settings." << std::endl;
			settings = json::object();
		}
	} else
	{
		std::cout << "[Settings] Active settings file " << settingsPath
				  << " does not exist (even after creation attempt). Starting "
					 "with empty settings."
				  << std::endl;
	}
	if (activeSettingsFilename != "ned.json" && settings.contains("settings_file"))
	{
		std::cout << "[Settings] Removing 'settings_file' key from non-primary "
					 "active settings file: "
				  << settingsPath << std::endl;
		settings.erase("settings_file");
		activeSettingsFileNewlyCreatedOrModified = true;
	}
	const std::vector<std::pair<std::string, json>> defaults = {
		{"backgroundColor",
		 json::array({0.05816289037466049,
					  0.19437342882156372,
					  0.1578674018383026,
					  1.0000001192092896})},
		{"bloom_intensity", 0.75},
		{"burnin_intensity", 0.9525200128555298},
		{"colorshift_intensity", 0.8999999761581421},
		{"curvature_intensity", 0.0},
		{"font", "SourceCodePro-Regular"},
		{"fontSize", 20.0f},
		{"git_changed_lines", true},
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
		{"mac_blur_enabled", true},
		{"fps_target", 120.0},
		{"fps_target_unfocused", 30.0},
		{"sidebar_visible", true},
		{"agent_pane_visible", true},
		{"agent_model", "deepseek/deepseek-chat-v3-0324"},
		{"completion_model", "meta-llama/llama-4-scout"}};
	for (const auto &[key, value] : defaults)
	{
		if (!settings.contains(key))
		{
			settings[key] = value;
			activeSettingsFileNewlyCreatedOrModified = true;
			std::cout << "[Settings] Applied default for key '" << key << "' to "
					  << settingsPath << std::endl;
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
			   json::array(
				   {0.6752017140388489, 0.3185149133205414, 0.7726563811302185, 1.0})},
			  {"keyword", json::array({0.0, 0.5786033272743225, 0.9643363952636719, 1.0})},
			  {"number",
			   json::array(
				   {0.6439791321754456, 0.3536583185195923, 0.7698838710784912, 1.0})},
			  {"string",
			   json::array(
				   {0.08516374975442886, 0.6660587787628174, 0.6660587787628174, 1.0})},
			  {"text",
			   json::array({0.680115282535553, 0.680115282535553, 0.680115282535553, 1.0})},
			  {"type",
			   json::array(
				   {0.6007077097892761, 0.7665653228759766, 0.44595927000045776, 1.0})},
			  {"variable",
			   json::array(
				   {0.3506303131580353, 0.7735447883605957, 0.8863282203674316, 1.0})}}}};
		activeSettingsFileNewlyCreatedOrModified = true;
		std::cout << "[Settings] Applied default themes structure to " << settingsPath
				  << std::endl;
	}
	if (activeSettingsFileNewlyCreatedOrModified)
	{
		std::cout << "[Settings] Saving active settings file " << settingsPath
				  << " due to initial setup or modifications." << std::endl;
		saveSettings(settings, settingsPath);
	}
}

void SettingsFileManager::createSettingsDirectory(const fs::path &dir)
{
	if (!fs::exists(dir))
	{
		try
		{
			fs::create_directories(dir);
			std::cout << "[Settings] Created settings directory: " << dir.string()
					  << std::endl;
		} catch (const fs::filesystem_error &e)
		{
			std::cerr << "[Settings] Error creating settings directory " << dir.string()
					  << ": " << e.what() << std::endl;
		}
	}
}

void SettingsFileManager::copyDefaultSettingsFiles(const fs::path &destDir)
{
	std::string bundleSettingsDir = getAppResourcesPath() + "/settings";
	std::vector<std::string> filesToCopy = {"lsp.json",
											"ned.json",
											"amber.json",
											"test.json",
											"solarized.json",
											"solarized-light.json",
											"custom1.json",
											"custom2.json",
											"custom3.json"};

	for (const auto &filename : filesToCopy)
	{
		std::string sourcePath = bundleSettingsDir + "/" + filename;
		std::string destPath = (destDir / filename).string();

		if (fs::exists(sourcePath))
		{
			try
			{
				fs::copy_file(sourcePath, destPath, fs::copy_options::overwrite_existing);
				std::cout << "[Settings] Copied " << filename << " from bundle to "
						  << destPath << std::endl;
			} catch (const fs::filesystem_error &e)
			{
				std::cerr << "[Settings] Error copying " << filename << ": " << e.what()
						  << std::endl;
				if (filename == "ned.json")
				{
					std::cerr << "CRITICAL: Failed to copy primary settings file"
							  << std::endl;
				}
			}
		} else
		{
			std::cerr << (filename == "ned.json" ? "CRITICAL" : "WARNING")
					  << ": Bundle file " << sourcePath << " not found" << std::endl;
		}
	}
}

bool SettingsFileManager::loadJsonFile(const std::string &filePath, json &jsonData)
{
	try
	{
		std::ifstream file(filePath);
		if (file.is_open())
		{
			file >> jsonData;
			return true;
		}
		std::cerr << "[Settings] Failed to open file " << filePath << " for reading."
				  << std::endl;
	} catch (const json::parse_error &e)
	{
		std::cerr << "[Settings] Error parsing " << filePath << ": " << e.what()
				  << std::endl;
	} catch (const std::exception &e)
	{
		std::cerr << "[Settings] Generic error loading " << filePath << ": " << e.what()
				  << std::endl;
	}
	return false;
}

void SettingsFileManager::saveJsonFile(const std::string &filePath, const json &jsonData)
{
	try
	{
		std::ofstream file(filePath);
		if (file.is_open())
		{
			file << std::setw(4) << jsonData << std::endl;
			file.close();
			updateLastModificationTime(filePath);
		} else
		{
			std::cerr << "[Settings] Failed to open " << filePath << " for saving."
					  << std::endl;
		}
	} catch (const std::exception &e)
	{
		std::cerr << "[Settings] Error saving " << filePath << ": " << e.what()
				  << std::endl;
	}
}

void SettingsFileManager::updateLastModificationTime(const std::string &filePath)
{
	try
	{
		lastSettingsModification = fs::last_write_time(filePath);
	} catch (const fs::filesystem_error &e)
	{
		std::cerr << "[Settings] Error getting last write time for " << filePath << ": "
				  << e.what() << std::endl;
		lastSettingsModification = fs::file_time_type::min();
	}
}

void SettingsFileManager::saveSettings(const json &settings,
									   const std::string &settingsPath)
{
	if (settingsPath.empty())
	{
		std::cerr << "[Settings] Error: settingsPath is empty, cannot save settings."
				  << std::endl;
		return;
	}
	saveJsonFile(settingsPath, settings);
}

void SettingsFileManager::checkSettingsFile(const std::string &settingsPath,
											json &settings,
											bool &settingsChanged,
											bool &fontChanged,
											bool &fontSizeChanged,
											bool &themeChanged)
{
	if (settingsPath.empty() || !fs::exists(settingsPath))
	{
		return;
	}

	try
	{
		auto currentModification = fs::last_write_time(settingsPath);
		if (currentModification <= lastSettingsModification)
		{
			return;
		}

		json oldSettings = settings;
		loadSettings(settings, const_cast<std::string &>(settingsPath));

		if (oldSettings.contains("fontSize") && settings.contains("fontSize") &&
			oldSettings["fontSize"] != settings["fontSize"])
		{
			fontSizeChanged = true;
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

		const std::vector<std::string> checkKeys = {"backgroundColor",
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
													"mac_blur_enabled",
													"fps_target",
													"sidebar_visible",
													"agent_pane_visible",
													"agent_model",
													"completion_model"};

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

	} catch (const fs::filesystem_error &e)
	{
		std::cerr << "[Settings] Filesystem error checking settings file " << settingsPath
				  << ": " << e.what() << std::endl;
	} catch (const std::exception &e)
	{
		std::cerr << "[Settings] Error during settings file check for " << settingsPath
				  << ": " << e.what() << std::endl;
	}
}

std::vector<std::string> SettingsFileManager::getAvailableProfileFiles()
{
	std::vector<std::string> availableProfileFiles;
	fs::path userSettingsDir = fs::path(getUserSettingsPath()).parent_path();

	if (fs::exists(userSettingsDir) && fs::is_directory(userSettingsDir))
	{
		for (const auto &entry : fs::directory_iterator(userSettingsDir))
		{
			if (entry.is_regular_file() && entry.path().extension() == ".json")
			{
				std::string filename = entry.path().filename().string();
				// Exclude "keybinds.json"
				if (filename == "keybinds.json" || filename == "open_router_key.json" ||
					filename == "default-keybinds.json" || filename == "lsp.json" ||
					filename == ".undo-redo-ned.json")
				{
				} else
				{
					availableProfileFiles.push_back(filename);
				}
			}
		}
		std::sort(availableProfileFiles.begin(), availableProfileFiles.end());
	} else
	{
		return availableProfileFiles;
	}
	return availableProfileFiles;
}

void SettingsFileManager::switchProfile(const std::string &newProfile,
										json &settings,
										std::string &settingsPath,
										bool &settingsChanged,
										bool &fontChanged,
										bool &themeChanged)
{
	std::string primarySettingsFilePath = getUserSettingsPath();
	json primaryJson;

	if (loadJsonFile(primarySettingsFilePath, primaryJson))
	{
		primaryJson["settings_file"] = newProfile;
		saveJsonFile(primarySettingsFilePath, primaryJson);
		loadSettings(settings, settingsPath);
		settingsChanged = true;
		fontChanged = true;
		themeChanged = true;
		gTerminal.UpdateTerminalColors();
	}
}

void SettingsFileManager::applyDefaultSettings(json &settings)
{
	const std::vector<std::pair<std::string, json>> defaults = {
		{"backgroundColor",
		 json::array({0.05816289037466049,
					  0.19437342882156372,
					  0.1578674018383026,
					  1.0000001192092896})},
		{"bloom_intensity", 0.75},
		{"burnin_intensity", 0.9525200128555298},
		{"colorshift_intensity", 0.8999999761581421},
		{"curvature_intensity", 0.0},
		{"font", "SourceCodePro-Regular"},
		{"fontSize", 20.0f},
		{"git_changed_lines", true},
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
		{"mac_blur_enabled", true},
		{"fps_target", 120.0},
		{"fps_target_unfocused", 30.0},
		{"sidebar_visible", true},
		{"agent_pane_visible", true},
		{"agent_model", "deepseek/deepseek-chat-v3-0324"},
		{"completion_model", "meta-llama/llama-4-scout"}};

	for (const auto &[key, value] : defaults)
	{
		if (!settings.contains(key))
		{
			settings[key] = value;
		}
	}
}

void SettingsFileManager::applyDefaultThemes(json &settings)
{
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
			   json::array(
				   {0.6752017140388489, 0.3185149133205414, 0.7726563811302185, 1.0})},
			  {"keyword", json::array({0.0, 0.5786033272743225, 0.9643363952636719, 1.0})},
			  {"number",
			   json::array(
				   {0.6439791321754456, 0.3536583185195923, 0.7698838710784912, 1.0})},
			  {"string",
			   json::array(
				   {0.08516374975442886, 0.6660587787628174, 0.6660587787628174, 1.0})},
			  {"text",
			   json::array({0.680115282535553, 0.680115282535553, 0.680115282535553, 1.0})},
			  {"type",
			   json::array(
				   {0.6007077097892761, 0.7665653228759766, 0.44595927000045776, 1.0})},
			  {"variable",
			   json::array(
				   {0.3506303131580353, 0.7735447883605957, 0.8863282203674316, 1.0})}}}};
	}
}

// OpenRouter key management
std::string SettingsFileManager::getOpenRouterKeyFilePath()
{
	// Place in the same settings directory as other profiles
	std::string settingsDir = getAppResourcesPath() + "/settings";
	return settingsDir + "/open_router_key.json";
}

std::string SettingsFileManager::getOpenRouterKey()
{
	std::string keyFilePath = getOpenRouterKeyFilePath();
	json keyJson;
	if (loadJsonFile(keyFilePath, keyJson))
	{
		if (keyJson.contains("key") && keyJson["key"].is_string())
		{
			return keyJson["key"].get<std::string>();
		}
	}
	return "";
}

void SettingsFileManager::setOpenRouterKey(const std::string &key)
{
	std::string keyFilePath = getOpenRouterKeyFilePath();
	fs::path settingsDir = fs::path(keyFilePath).parent_path();
	if (!fs::exists(settingsDir))
	{
		try
		{
			fs::create_directories(settingsDir);
			std::cout << "[Settings] Created settings directory for OpenRouter key: "
					  << settingsDir.string() << std::endl;
		} catch (const fs::filesystem_error &e)
		{
			std::cerr << "[Settings] Error creating settings directory for "
						 "OpenRouter key: "
					  << e.what() << std::endl;
			return;
		}
	}
	json keyJson = {{"key", key}};
	saveJsonFile(keyFilePath, keyJson);
}