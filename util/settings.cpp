#include "settings.h"
#include "../editor/editor_api.h"
#include "../editor/editor_events.h"
#include "../lsp/views/imgui/lsp_view.h"
#include "../files/files.h"
#include "../lsp/lsp_client.h"

#include "imgui.h"
#include <GLFW/glfw3.h>
#include <algorithm>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <iostream>

#ifdef __APPLE__
#include "macos_window.h"
#include <mach-o/dyld.h>
#include <sys/param.h>
#endif
#ifdef __linux__
#include <linux/limits.h>
#include <unistd.h>
#endif
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

namespace fs = std::filesystem;

// ---- paths & JSON ----

namespace {

// Dev tree, portable zip, and installed layouts all look like this.
bool looksLikeResourceRoot(const fs::path &path)
{
	std::error_code ec;
	return fs::exists(path / "resources" / "fonts", ec) &&
		   fs::exists(path / "resources" / "config", ec);
}

std::string firstResourceRoot(std::initializer_list<fs::path> candidates)
{
	for (const fs::path &path : candidates)
	{
		std::error_code ec;
		const fs::path abs = fs::weakly_canonical(path, ec);
		const fs::path &check = ec ? path : abs;
		if (looksLikeResourceRoot(check))
			return check.string();
	}
	return {};
}

} // namespace

std::string Settings::getAppResourcesPath()
{
	const fs::path cwd = fs::current_path();
	// cwd first: embed/demo and portable packages often run with resources/ nearby.
	if (std::string found = firstResourceRoot({cwd,
											   cwd / "ned",
											   cwd / ".." / "ned",
											   cwd / ".." / ".." / "ned",
											   cwd / ".." / "ImGui_Ned_Embed" / "ned",
											   cwd / "ned" / "ned",
											   cwd / ".." / "ned" / "ned"});
		!found.empty())
		return found;

#ifdef __APPLE__
	char executable[MAXPATHLEN];
	uint32_t size = sizeof(executable);
	if (_NSGetExecutablePath(executable, &size) == 0)
	{
		char resolved[MAXPATHLEN];
		const fs::path binary = realpath(executable, resolved) ? resolved : executable;
		// .app/Contents/MacOS/Ned → Contents/Resources (pack-mac layout)
		const fs::path contents = binary.parent_path().parent_path();
		const fs::path resources = contents / "Resources";
		if (looksLikeResourceRoot(resources))
			return resources.string();
		if (fs::is_directory(resources))
			return resources.string();
		if (std::string found = firstResourceRoot({binary.parent_path()}); !found.empty())
			return found;
	}
#elif defined(_WIN32)
	// Portable zip: resources/ and shaders/ sit next to ned.exe
	wchar_t modulePath[MAX_PATH];
	const DWORD n = GetModuleFileNameW(nullptr, modulePath, MAX_PATH);
	if (n > 0 && n < MAX_PATH)
	{
		const fs::path exeDir = fs::path(modulePath).parent_path();
		if (std::string found = firstResourceRoot({exeDir, exeDir / ".."}); !found.empty())
			return found;
		// Still prefer exe dir if it at least has resources/ (partial install).
		if (fs::exists(exeDir / "resources"))
			return exeDir.string();
	}
#elif defined(__linux__)
	char executable[PATH_MAX];
	const ssize_t length = readlink("/proc/self/exe", executable, sizeof(executable) - 1);
	if (length != -1)
	{
		executable[length] = '\0';
		const fs::path binaryDir = fs::path(executable).parent_path();
		// Deb: binary in /usr/lib/Ned, assets in /usr/share/Ned
		if (std::string found =
				firstResourceRoot({binaryDir / ".." / "share" / "Ned", binaryDir});
			!found.empty())
			return found;
		if (fs::is_directory(binaryDir / ".." / "share" / "Ned"))
			return fs::weakly_canonical(binaryDir / ".." / "share" / "Ned").string();
		if (fs::is_directory(binaryDir))
			return binaryDir.string();
	}
#endif
	return ".";
}

std::string Settings::getUserConfigDir()
{
	// ~/ned/config on Unix; %USERPROFILE%\ned\config on Windows.
	const char *home = nullptr;
#ifdef _WIN32
	home = std::getenv("USERPROFILE");
	if (!home || !*home)
	{
		const char *drive = std::getenv("HOMEDRIVE");
		const char *path = std::getenv("HOMEPATH");
		if (drive && path && *drive && *path)
			return (fs::path(drive) / path / "ned" / "config").string();
	}
#else
	home = std::getenv("HOME");
#endif
	if (!home || !*home)
	{
		// Git Bash / some shells set HOME on Windows too.
		home = std::getenv("HOME");
	}
	if (!home || !*home)
	{
		std::cerr << "[Settings] home directory is not set (USERPROFILE/HOME)"
				  << std::endl;
		return {};
	}
	return (fs::path(home) / "ned" / "config").string();
}

std::string Settings::primaryPath()
{
	const std::string dir = getUserConfigDir();
	return dir.empty() ? std::string{} : (fs::path(dir) / "ned.json").string();
}

bool Settings::readJson(const std::string &path, json &out)
{
	try
	{
		std::ifstream file(path);
		if (!file)
		{
			std::cerr << "[Settings] Could not open " << path << std::endl;
			return false;
		}
		file >> out;
		return true;
	} catch (const std::exception &e)
	{
		std::cerr << "[Settings] Could not read " << path << ": " << e.what()
				  << std::endl;
		return false;
	}
}

bool Settings::writeJson(const std::string &path, const json &data)
{
	std::ofstream file(path);
	if (!file)
	{
		std::cerr << "[Settings] Could not write " << path << std::endl;
		return false;
	}
	file << std::setw(4) << data << '\n';
	return true;
}

void Settings::touchDiskTime()
{
	std::error_code ec;
	diskTime = fs::last_write_time(settingsPath, ec);
	if (ec)
		diskTime = fs::file_time_type::min();
}

// ---- lifecycle ----

Settings::Settings() : keybinds(*this) { loadSettings(); }

// Copy bundled defaults into ~/ned/config when missing (first launch).
static bool seedUserConfigIfNeeded()
{
	const std::string userDir = Settings::getUserConfigDir();
	if (userDir.empty())
		return false;

	const fs::path bundled =
		fs::path(Settings::getAppResourcesPath()) / "resources" / "config";
	if (!fs::is_directory(bundled))
	{
		std::cerr << "[Settings] Bundled config not found under " << bundled.string()
				  << std::endl;
		return false;
	}

	std::error_code ec;
	fs::create_directories(userDir, ec);
	if (ec)
	{
		std::cerr << "[Settings] Could not create " << userDir << ": " << ec.message()
				  << std::endl;
		return false;
	}

	// Seed any missing profile files (ned.json pointer, themes, keybinds, lsp.json).
	for (const auto &entry : fs::directory_iterator(bundled, ec))
	{
		if (ec || !entry.is_regular_file())
			continue;
		const fs::path dest = fs::path(userDir) / entry.path().filename();
		if (fs::exists(dest))
			continue;
		fs::copy_file(entry.path(), dest, ec);
		if (ec)
		{
			std::cerr << "[Settings] Could not seed " << dest.string() << ": "
					  << ec.message() << std::endl;
			ec.clear();
		} else
		{
			std::cerr << "[Settings] Seeded " << dest.string() << std::endl;
		}
	}
	return fs::exists(fs::path(userDir) / "ned.json");
}

// Load profile JSON from bundled resources into memory (no user dir write).
bool Settings::loadBundledProfile(json &out, std::string &outPath)
{
	const fs::path bundled =
		fs::path(Settings::getAppResourcesPath()) / "resources" / "config" / "ned.json";
	if (!Settings::readJson(bundled.string(), out))
		return false;
	outPath = bundled.string();
	return out.is_object();
}

void Settings::loadSettings()
{
	// ~/ned/config/ned.json points at the active profile via "settings_file".
	const std::string primary = primaryPath();
	if (primary.empty())
	{
		if (loadBundledProfile(settings, settingsPath))
			needsApply = true;
		return;
	}

	// Always seed *missing* profile files from the bundle (new themes, etc.).
	// Does not overwrite existing user profiles.
	seedUserConfigIfNeeded();

	json pointer;
	if (!readJson(primary, pointer))
	{
		if (loadBundledProfile(settings, settingsPath))
			needsApply = true;
		return;
	}
	// Dual-purpose ned.json: pointer + default profile. If settings_file is
	// missing, treat primary itself as the active profile.
	if (!pointer.contains("settings_file") || !pointer["settings_file"].is_string())
	{
		pointer["settings_file"] = "ned.json";
		writeJson(primary, pointer);
	}

	settingsPath =
		(fs::path(getUserConfigDir()) / pointer["settings_file"].get<std::string>())
			.string();
	if (!readJson(settingsPath, settings) || !settings.is_object())
	{
		if (loadBundledProfile(settings, settingsPath))
			needsApply = true;
		return;
	}

	touchDiskTime();
	needsApply = true;
}

void Settings::saveSettings()
{
	if (settingsPath.empty())
	{
		std::cerr << "[Settings] No active profile path to save" << std::endl;
		return;
	}
	if (writeJson(settingsPath, settings))
		touchDiskTime();
}

void Settings::checkSettingsFile()
{
	if (settingsPath.empty())
		return;

	std::error_code ec;
	const auto modified = fs::last_write_time(settingsPath, ec);
	if (ec || modified <= diskTime)
		return;

	if (!readJson(settingsPath, settings))
		return;

	diskTime = modified;
	needsApply = true;
}

void Settings::switchToProfile(const std::string &profileName)
{
	const std::string dir = getUserConfigDir();
	const std::string primary = primaryPath();
	if (dir.empty() || primary.empty())
		return;

	const std::string path = (fs::path(dir) / profileName).string();
	json loaded;
	if (!readJson(path, loaded))
		return;

	// Remember which profile is active.
	json pointer;
	if (!readJson(primary, pointer))
		return;
	pointer["settings_file"] = profileName;
	if (!writeJson(primary, pointer))
		return;

	settings = std::move(loaded);
	settingsPath = path;
	touchDiskTime();
	needsApply = true;
}

std::vector<std::string> Settings::listProfiles() const
{
	std::vector<std::string> profiles;
	const std::string dir = getUserConfigDir();
	if (dir.empty())
		return profiles;

	std::error_code ec;
	for (const auto &entry : fs::directory_iterator(dir, ec))
	{
		const std::string name = entry.path().filename().string();
		if (entry.is_regular_file() && entry.path().extension() == ".json" &&
			name != "keybinds.json" && name != "default-keybinds.json" &&
			name != "lsp.json" && name != ".undo-redo-ned.json")
			profiles.push_back(name);
	}
	std::sort(profiles.begin(), profiles.end());
	return profiles;
}

void Settings::toggleSidebar()
{
	sidebarVisible = !sidebarVisible;
	settings["sidebar_visible"] = sidebarVisible;
	saveSettings();
}

void Settings::toggleTerminal()
{
	terminalVisible = !terminalVisible;
	settings["terminal_visible"] = terminalVisible;
	saveSettings();
}

void Settings::toggleSettingsWindow(EditorApi &api)
{
	showSettingsWindow = !showSettingsWindow;
	if (showSettingsWindow)
		api.requestExclusiveOverlay(
			EditorEvents::DidRequestExclusiveOverlay::Keep::Settings);
	api.setBlockInput(showSettingsWindow);
}

// ---- UI ----
