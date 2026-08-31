#include "settings.h"
#include "../editor/editor_api.h"
#include "../editor/editor_events.h"
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
static bool loadBundledProfile(json &out, std::string &outPath)
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

void Settings::closeSettingsWindow(EditorApi &api)
{
	showSettingsWindow = false;
	saveSettings();
	api.setBlockInput(false);
}

bool Settings::apply(bool force, EditorApi &api)
{
	if (!needsApply && !force)
		return false;
	needsApply = false;

	// Never call .value() on null / non-object (throws type_error.306).
	if (!settings.is_object())
	{
		std::cerr << "[Settings] apply: settings JSON is not an object; "
					 "loading bundled defaults"
				  << std::endl;
		std::string path;
		if (!loadBundledProfile(settings, path))
			return false;
		if (settingsPath.empty())
			settingsPath = path;
	}

	font.setFont(settings.value("font", std::string("SourceCodePro-Regular")),
				 settings.value("fontSize", 20.0f));
	font.load();

	ImGuiStyle &style = ImGui::GetStyle();
	ApplySettings(style);

	// Embedded hosts keep the host ImGui theme for window/child backgrounds.
	// Standalone: match window, child, and editor tab bar to theme background.
	if (!isEmbedded && settings.contains("backgroundColor") &&
		settings["backgroundColor"].is_array() && settings["backgroundColor"].size() >= 3)
	{
		const auto &bg = settings["backgroundColor"];
		const float a =
			settings["backgroundColor"].size() >= 4 ? bg[3].get<float>() : 1.0f;
		const ImVec4 bgCol(bg[0].get<float>(), bg[1].get<float>(), bg[2].get<float>(), a);
		const ImVec4 bgOpaque(
			bg[0].get<float>(), bg[1].get<float>(), bg[2].get<float>(), 1.0f);
		style.Colors[ImGuiCol_ChildBg] = bgOpaque;
		style.Colors[ImGuiCol_WindowBg] = bgCol;
		// Dock title-bar tabs: no fill by default; slight highlight only on hover.
		const ImVec4 tabNone(0.0f, 0.0f, 0.0f, 0.0f);
		style.Colors[ImGuiCol_Tab] = tabNone;
		style.Colors[ImGuiCol_TabSelected] = tabNone;
		style.Colors[ImGuiCol_TabDimmed] = tabNone;
		style.Colors[ImGuiCol_TabDimmedSelected] = tabNone;
		// Soft lift from text color so hover reads on light and dark themes.
		const ImVec4 &text = style.Colors[ImGuiCol_Text];
		style.Colors[ImGuiCol_TabHovered] = ImVec4(text.x, text.y, text.z, 0.12f);
		// Window/tab close (X) draws ButtonHovered / ButtonActive as its fill.
		style.Colors[ImGuiCol_Button] = ImVec4(text.x, text.y, text.z, 0.12f);
		style.Colors[ImGuiCol_ButtonHovered] = ImVec4(text.x, text.y, text.z, 0.18f);
		style.Colors[ImGuiCol_ButtonActive] = ImVec4(text.x, text.y, text.z, 0.30f);
		// Hide selected-tab overline so active doesn't look different.
		style.Colors[ImGuiCol_TabSelectedOverline] = tabNone;
		style.Colors[ImGuiCol_TabDimmedSelectedOverline] = tabNone;
		// Window / dock title bars (active, inactive, collapsed) match theme bg.
		style.Colors[ImGuiCol_TitleBg] = bgOpaque;
		style.Colors[ImGuiCol_TitleBgActive] = bgOpaque;
		style.Colors[ImGuiCol_TitleBgCollapsed] = bgOpaque;
		style.Colors[ImGuiCol_MenuBarBg] = bgOpaque;
		style.Colors[ImGuiCol_DockingEmptyBg] = bgOpaque;
		// Hide the dock title-bar / tab-bar window-menu (tab list) button.
		// See https://github.com/ocornut/imgui/issues/4880
		style.WindowMenuButtonPosition = ImGuiDir_None;
	}

	sidebarVisible = settings.value("sidebar_visible", true);
	terminalVisible = settings.value("terminal_visible", true);

	api.forceColorUpdate();

#ifdef __APPLE__
	updateMacOSWindowProperties(settings.value("mac_background_opacity", 0.5f),
								settings.value("mac_blur_enabled", true));
#endif
	return true;
}

void Settings::ApplySettings(ImGuiStyle &style)
{
	if (!settings.is_object())
		return;

	// Standalone only: embedded mode leaves WindowBg/ChildBg to the host style.
	if (!isEmbedded && settings.contains("backgroundColor") &&
		settings["backgroundColor"].is_array() && settings["backgroundColor"].size() >= 4)
	{
		const auto &bg = settings["backgroundColor"];
		style.Colors[ImGuiCol_WindowBg] = ImVec4(bg[0].get<float>(),
												 bg[1].get<float>(),
												 bg[2].get<float>(),
												 bg[3].get<float>());
	}

	const std::string theme = settings.value("theme", std::string("default"));
	if (!settings.contains("themes") || !settings["themes"].is_object() ||
		!settings["themes"].contains(theme) ||
		!settings["themes"][theme].contains("text"))
		return;

	const auto &textColor = settings["themes"][theme]["text"];
	if (!textColor.is_array() || textColor.size() < 4)
		return;
	ImVec4 textCol(textColor[0].get<float>(),
				   textColor[1].get<float>(),
				   textColor[2].get<float>(),
				   textColor[3].get<float>());
	style.Colors[ImGuiCol_Text] = textCol;
	style.Colors[ImGuiCol_TextDisabled] =
		ImVec4(textCol.x * 0.6f, textCol.y * 0.6f, textCol.z * 0.6f, textCol.w);
	style.Colors[ImGuiCol_TextSelectedBg] = ImVec4(1.0f, 0.1f, 0.7f, 0.3f);

	if (!isEmbedded)
	{
		const float dpi = std::max(1.0f, style.FontScaleDpi);
		style.ScrollbarSize = 30.0f * dpi;
		style.Colors[ImGuiCol_ScrollbarBg] = ImVec4(0, 0, 0, 0);
		style.Colors[ImGuiCol_ScrollbarGrab] = ImVec4(0, 0, 0, 0);
		style.Colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0, 0, 0, 0);
		style.Colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0, 0, 0, 0);
	} else
	{
		style.Colors[ImGuiCol_ScrollbarBg] = ImVec4(0, 0, 0, 0);
	}
	style.ScaleAllSizes(1.0f);
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

std::string Settings::displayFontName(const std::string &fontFile)
{
	if (fontFile == "System Default" || fontFile.find('.') == std::string::npos)
		return fontFile;
	std::string name = fontFile.substr(0, fontFile.find_last_of('.'));
	std::replace(name.begin(), name.end(), '-', ' ');
	return name;
}

void Settings::renderSettingsWindow(EditorApi &api, FileExplorer &files, LSPClient &lsp)
{
	if (!showSettingsWindow)
		return;

	if (isEmbedded)
	{
		ImGui::SetNextWindowPos(embeddedWindowPos, ImGuiCond_FirstUseEver);
		ImGui::SetNextWindowSize(embeddedWindowSize, ImGuiCond_FirstUseEver);

		bool windowOpen = true;
		if (ImGui::Begin("Settings", &windowOpen, ImGuiWindowFlags_NoCollapse))
		{
			embeddedWindowPos = ImGui::GetWindowPos();
			embeddedWindowSize = ImGui::GetWindowSize();
			if (!windowOpen)
				showSettingsWindow = false;
			renderSettingsContent(api, files, lsp);
			ImGui::End();
		}
		return;
	}

	const float fontSize = settings.value("fontSize", 20.0f);
	const ImVec2 viewport = ImGui::GetMainViewport()->Size;
	const float width =
		viewport.x * ((viewport.x < 1100.0f || fontSize > 40.0f) ? 0.90f : 0.75f);
	const float height =
		viewport.y * ((viewport.x < 1100.0f || fontSize > 40.0f) ? 0.80f : 0.85f);

	ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 1.0f);
	ImGui::SetNextWindowSize(ImVec2(width, height), ImGuiCond_Always);
	ImGui::SetNextWindowPos(
		ImVec2(ImGui::GetIO().DisplaySize.x * 0.5f, ImGui::GetIO().DisplaySize.y * 0.5f),
		ImGuiCond_Always,
		ImVec2(0.5f, 0.5f));

	applyImGuiStyles();
	ImGui::Begin("Settings",
				 nullptr,
				 ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
					 ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar |
					 ImGuiWindowFlags_Modal);
	ImGui::PushFont(font.getMainFont());
	renderSettingsContent(api, files, lsp);
	ImGui::PopFont();
	ImGui::End();
	ImGui::PopStyleColor(8);
	ImGui::PopStyleVar(6);
}

void Settings::renderSettingsContent(EditorApi &api, FileExplorer &files, LSPClient &lsp)
{
	if (!isEmbedded)
	{
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
		renderWindowHeader(api, files);
		ImGui::PopStyleVar();
	}

	const bool pushBg = !isEmbedded && settings.contains("backgroundColor") &&
						settings["backgroundColor"].is_array() &&
						settings["backgroundColor"].size() >= 3;
	if (pushBg)
	{
		const auto &bg = settings["backgroundColor"];
		const float m = 0.8f;
		ImGui::PushStyleColor(ImGuiCol_ChildBg,
							  ImVec4(bg[0].get<float>() * m,
									 bg[1].get<float>() * m,
									 bg[2].get<float>() * m,
									 1.0f));
	}
	ImGui::PushStyleVar(
		ImGuiStyleVar_WindowPadding,
		ImVec2(ImGui::GetFontSize() * 0.75f, ImGui::GetFontSize() * 0.25f));
	ImGui::BeginChild("SettingsContent",
					  ImVec2(0, ImGui::GetContentRegionAvail().y),
					  false,
					  ImGuiWindowFlags_AlwaysVerticalScrollbar);

	renderProfileSelector();
	renderMainSettings();
	if (!isEmbedded)
		renderMacSettings();
	renderSyntaxColors();
	renderToggleSettings();
#if NED_ENABLE_SHADERS
	if (!isEmbedded)
		renderShaderSettings();
#endif
	renderKeybindsSettings(files, lsp);

	ImGui::EndChild();
	if (pushBg)
		ImGui::PopStyleColor();
	ImGui::PopStyleVar();
	handleWindowInput(api);
}

void Settings::applyImGuiStyles()
{
	const float fs = ImGui::GetFontSize();
	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, fs * 0.5f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_ScrollbarSize, fs * 0.7f);
	ImGui::PushStyleVar(ImGuiStyleVar_ScrollbarRounding, fs * 0.5f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(fs * 0.75f, fs * 0.75f));

	// Embedded: keep host ImGui palette for window/frame backgrounds.
	if (isEmbedded)
	{
		// Still push the same color stack depth so PopStyleColor(8) stays balanced.
		const ImGuiStyle &s = ImGui::GetStyle();
		ImGui::PushStyleColor(ImGuiCol_WindowBg, s.Colors[ImGuiCol_WindowBg]);
		ImGui::PushStyleColor(ImGuiCol_FrameBg, s.Colors[ImGuiCol_FrameBg]);
		ImGui::PushStyleColor(ImGuiCol_ScrollbarBg, s.Colors[ImGuiCol_ScrollbarBg]);
		ImGui::PushStyleColor(ImGuiCol_PopupBg, s.Colors[ImGuiCol_PopupBg]);
		ImGui::PushStyleColor(ImGuiCol_Border, s.Colors[ImGuiCol_Border]);
		ImGui::PushStyleColor(ImGuiCol_ScrollbarGrab, s.Colors[ImGuiCol_ScrollbarGrab]);
		ImGui::PushStyleColor(ImGuiCol_ScrollbarGrabHovered,
							  s.Colors[ImGuiCol_ScrollbarGrabHovered]);
		ImGui::PushStyleColor(ImGuiCol_ScrollbarGrabActive,
							  s.Colors[ImGuiCol_ScrollbarGrabActive]);
		return;
	}

	const float windowMul = 0.8f;
	const float frameMul = 0.5f;
	const float border = 0.3f;
	const auto &bg = settings["backgroundColor"];
	const float r = bg[0].get<float>();
	const float g = bg[1].get<float>();
	const float b = bg[2].get<float>();

	ImGui::PushStyleColor(ImGuiCol_WindowBg,
						  ImVec4(r * windowMul, g * windowMul, b * windowMul, 1.0f));
	ImGui::PushStyleColor(ImGuiCol_FrameBg,
						  ImVec4(r * frameMul, g * frameMul, b * frameMul, 1.0f));
	ImGui::PushStyleColor(ImGuiCol_ScrollbarBg,
						  ImVec4(r * frameMul, g * frameMul, b * frameMul, 0.0f));
	ImGui::PushStyleColor(ImGuiCol_PopupBg,
						  ImVec4(r * frameMul, g * frameMul, b * frameMul, 1.0f));
	ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(border, border, border, 1.0f));
	ImGui::PushStyleColor(ImGuiCol_ScrollbarGrab, ImVec4(border, border, border, 1.0f));
	ImGui::PushStyleColor(ImGuiCol_ScrollbarGrabHovered, ImVec4(0.4f, 0.4f, 0.4f, 1.0f));
	ImGui::PushStyleColor(ImGuiCol_ScrollbarGrabActive,
						  ImVec4(frameMul, frameMul, frameMul, 1.0f));
}

void Settings::renderWindowHeader(EditorApi &api, FileExplorer &files)
{
	static bool wasFocused = false;
	const bool isFocused = ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);
	const bool windowHovered =
		ImGui::IsWindowHovered(ImGuiHoveredFlags_RootAndChildWindows);
	// Layout changes (terminal/sidebar) can steal ImGui focus for a frame.
	// Don't dismiss if the pointer is still over Settings — click-outside
	// is handled in handleWindowInput.
	if (wasFocused && !isFocused && showSettingsWindow && !windowHovered)
		closeSettingsWindow(api);
	wasFocused = isFocused;

	ImGui::BeginGroup();
	ImGui::TextUnformatted("Settings");
	const float closeSize = ImGui::GetFontSize();
	const float buttonX = ImGui::GetContentRegionAvail().x - closeSize -
						  ImGui::GetStyle().FramePadding.x * 2;
	ImGui::SameLine(buttonX > 0 ? buttonX : ImGui::GetCursorPosX() + 100);

	const ImVec2 cursor = ImGui::GetCursorPos();
	if (ImGui::InvisibleButton("##close-settings", ImVec2(closeSize, closeSize)))
		closeSettingsWindow(api);
	const bool hovered = ImGui::IsItemHovered();
	ImGui::SetCursorPos(cursor);
	ImGui::Image(ImTextureRef(files.icons.get("close")),
				 ImVec2(closeSize, closeSize),
				 ImVec2(0, 0),
				 ImVec2(1, 1),
				 hovered ? ImVec4(1, 1, 1, 0.6f) : ImVec4(1, 1, 1, 1),
				 ImVec4(0, 0, 0, 0));
	ImGui::EndGroup();
	ImGui::Separator();
}

void Settings::renderProfileSelector()
{
	ImGui::Spacing();

	std::vector<std::string> profiles = listProfiles();
	std::string current =
		settingsPath.empty() ? "ned.json" : fs::path(settingsPath).filename().string();

	if (std::find(profiles.begin(), profiles.end(), current) == profiles.end() &&
		!current.empty())
		profiles.insert(profiles.begin(), current);

	if (ImGui::BeginCombo("##ActiveSettingsFileCombo", current.c_str()))
	{
		for (const std::string &name : profiles)
		{
			const bool selected = (current == name);
			if (ImGui::Selectable(name.c_str(), selected) && !selected)
				switchToProfile(name);
			if (selected)
				ImGui::SetItemDefaultFocus();
		}
		ImGui::EndCombo();
	}
	ImGui::SameLine();
	ImGui::TextUnformatted("Profile");
	ImGui::Spacing();
}

void Settings::renderMainSettings()
{
	const std::string currentFont =
		settings.value("font", std::string("SourceCodePro-Regular"));
	if (ImGui::BeginCombo("Font", displayFontName(currentFont).c_str()))
	{
		for (const auto &fontFile : font.availableFonts())
		{
			const bool selected = (fontFile == currentFont);
			if (ImGui::Selectable(displayFontName(fontFile).c_str(), selected))
			{
				settings["font"] = fontFile;
				needsApply = true;
				saveSettings();
			}
			if (selected)
				ImGui::SetItemDefaultFocus();
		}
		ImGui::EndCombo();
	}
	ImGui::Spacing();

	float fontSize = settings.value("fontSize", 20.0f);
	if (ImGui::SliderFloat("Font Size", &fontSize, 4.0f, 64.0f, "%.0f"))
	{
		settings["fontSize"] = fontSize;
		needsApply = true;
	}
	if (ImGui::IsItemDeactivatedAfterEdit())
		saveSettings();

	ImGui::Spacing();

	ImVec4 bgColor(0.058f, 0.194f, 0.158f, 1.0f);
	if (settings.contains("backgroundColor") && settings["backgroundColor"].is_array() &&
		settings["backgroundColor"].size() == 4)
	{
		const auto &bg = settings["backgroundColor"];
		bgColor = ImVec4(bg[0].get<float>(),
						 bg[1].get<float>(),
						 bg[2].get<float>(),
						 bg[3].get<float>());
	}
	if (ImGui::ColorEdit4("Background Color", (float *)&bgColor))
	{
		settings["backgroundColor"] = {bgColor.x, bgColor.y, bgColor.z, bgColor.w};
		needsApply = true;
		saveSettings();
	}
}

void Settings::renderMacSettings()
{
#ifdef __APPLE__
	ImGui::Spacing();
	ImGui::TextUnformatted("macOS Settings");
	ImGui::Separator();
	ImGui::Spacing();

	float opacity = settings.value("mac_background_opacity", 0.5f);
	if (ImGui::SliderFloat("Background Opacity", &opacity, 0.0f, 1.0f, "%.2f"))
	{
		settings["mac_background_opacity"] = opacity;
		needsApply = true;
		saveSettings();
	}

	bool blur = settings.value("mac_blur_enabled", true);
	if (ImGui::Checkbox("Enable Background Blur", &blur))
	{
		settings["mac_blur_enabled"] = blur;
		needsApply = true;
		saveSettings();
	}
#endif
}

void Settings::renderSyntaxColors()
{
	const std::string theme = settings.value("theme", std::string("default"));
	if (!settings.contains("themes") || !settings["themes"].is_object() ||
		!settings["themes"].contains(theme))
	{
		ImGui::Text("Theme '%s' not found.", theme.c_str());
		return;
	}

	auto &colors = settings["themes"][theme];

	// Ensure key exists so older 8-slot themes can gain extras from the picker.
	auto ensureColor = [&](const char *key, const char *fallbackKey) {
		if (colors.contains(key) && colors[key].is_array() && colors[key].size() == 4)
			return;
		if (colors.contains(fallbackKey) && colors[fallbackKey].is_array() &&
			colors[fallbackKey].size() == 4)
			colors[key] = colors[fallbackKey];
		else
			colors[key] = {0.75f, 0.75f, 0.75f, 1.0f};
	};

	// Core
	ensureColor("text", "text");
	ensureColor("keyword", "text");
	ensureColor("string", "text");
	ensureColor("number", "text");
	ensureColor("comment", "text");
	ensureColor("function", "text");
	ensureColor("type", "text");
	ensureColor("variable", "text");
	// Extended (plan: ~15 slots)
	ensureColor("parameter", "variable");
	ensureColor("property", "variable");
	ensureColor("constant", "number");
	ensureColor("operator", "text");
	ensureColor("punctuation", "text");
	ensureColor("special", "keyword");

	auto editColor = [&](const char *label, const char *key) {
		if (!colors.contains(key) || !colors[key].is_array() || colors[key].size() != 4)
			return;
		auto &arr = colors[key];
		ImVec4 color(arr[0].get<float>(),
					 arr[1].get<float>(),
					 arr[2].get<float>(),
					 arr[3].get<float>());

		ImGui::TextUnformatted(label);
		ImGui::SameLine(200);
		if (ImGui::ColorEdit4(("##" + std::string(key)).c_str(), (float *)&color))
		{
			colors[key] = {color.x, color.y, color.z, color.w};
			needsApply = true;
		}
		if (ImGui::IsItemDeactivatedAfterEdit())
		{
			colors[key] = {color.x, color.y, color.z, color.w};
			needsApply = true;
			saveSettings();
		}
	};

	ImGui::Spacing();
	if (ImGui::CollapsingHeader("Syntax Colors"))
	{
		ImGui::Spacing();
		editColor("Text", "text");
		editColor("Keywords", "keyword");
		editColor("Strings", "string");
		editColor("Numbers", "number");
		editColor("Comments", "comment");
		editColor("Functions", "function");
		editColor("Types", "type");
		editColor("Identifier", "variable");
		editColor("Parameter", "parameter");
		editColor("Property / field", "property");
		editColor("Constant", "constant");
		editColor("Operator", "operator");
		editColor("Punctuation", "punctuation");
		editColor("Special / builtin", "special");
	}
}

void Settings::renderToggleSettings()
{
	ImGui::Spacing();
	ImGui::TextUnformatted("Toggle Settings");
	ImGui::Separator();
	ImGui::Spacing();

	bool sidebar = settings.value("sidebar_visible", true);
	if (ImGui::Checkbox("File Explorer", &sidebar))
		toggleSidebar();
	ImGui::SameLine();
	ImGui::TextDisabled("(Show/hide file explorer sidebar)");
	ImGui::Spacing();

	bool term = settings.value("terminal_visible", true);
	if (ImGui::Checkbox("Terminal", &term))
		toggleTerminal();
	ImGui::SameLine();
	ImGui::TextDisabled("(Show/hide bottom terminal panel)");
	ImGui::Spacing();

	bool rainbow = settings.value("rainbow", true);
	if (ImGui::Checkbox("Rainbow Mode", &rainbow))
	{
		settings["rainbow"] = rainbow;
		saveSettings();
	}
	ImGui::SameLine();
	ImGui::TextDisabled("(Rainbow cursor & line numbers)");

	bool minimap = settings.value("minimap", true);
	if (ImGui::Checkbox("Minimap", &minimap))
	{
		settings["minimap"] = minimap;
		saveSettings();
	}
	ImGui::SameLine();
	ImGui::TextDisabled("(Code overview strip on the right)");

	bool treesitter = settings.value("treesitter", true);
	if (ImGui::Checkbox("TreeSitter Mode", &treesitter))
	{
		settings["treesitter"] = treesitter;
		needsApply = true;
		saveSettings();
	}
	ImGui::SameLine();
	ImGui::TextDisabled("(Syntax Highlighting)");

#if NED_ENABLE_GIT
	bool gitLines = settings.value("git_changed_lines", true);
	if (ImGui::Checkbox("Git Changed Lines", &gitLines))
	{
		settings["git_changed_lines"] = gitLines;
		saveSettings();
	}
	ImGui::SameLine();
	ImGui::TextDisabled("(Highlight changed lines in git)");
#endif
}

void Settings::renderShaderSettings()
{
	ImGui::Spacing();
	ImGui::TextUnformatted("GL Shaders");
	ImGui::Separator();
	ImGui::Spacing();

	bool enabled = settings.value("shader_toggle", true);
	if (ImGui::Checkbox("Enable Shader Effects", &enabled))
	{
		settings["shader_toggle"] = enabled;
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
}

void Settings::renderShaderSlider(const char *label,
								  const char *key,
								  float min_val,
								  float max_val,
								  const char *format,
								  float default_val)
{
	float value = settings.value(key, default_val);
	if (ImGui::SliderFloat(
			label, &value, min_val, max_val, format, ImGuiSliderFlags_AlwaysClamp))
		settings[key] = value;
	if (ImGui::IsItemDeactivatedAfterEdit())
		saveSettings();
	ImGui::Spacing();
}

void Settings::renderKeybindsSettings(FileExplorer &files, LSPClient &lsp)
{
	ImGui::Spacing();
	ImGui::Separator();
	ImGui::Spacing();

	const fs::path configDir = getUserConfigDir();
	const std::string keybindsPath = (configDir / "keybinds.json").string();
	const std::string defaultKeybindsPath =
		(configDir / "default-keybinds.json").string();

	if (ImGui::Button("Open Keybinds File"))
	{
		if (fs::exists(keybindsPath))
		{
			files.loadFileContent(keybindsPath);
			showSettingsWindow = false;
		} else
		{
			std::cerr << "[Settings] Keybinds file not found: " << keybindsPath
					  << std::endl;
		}
	}
	ImGui::SameLine();
	ImGui::TextDisabled("(Edit keyboard shortcuts)");

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
				keybinds.loadKeybinds();
			} catch (const fs::filesystem_error &e)
			{
				std::cerr << "[Settings] Error restoring keybinds: " << e.what()
						  << std::endl;
			}
		}
		ImGui::SameLine();
		ImGui::TextDisabled("(Reset to default configuration)");
	}

#if NED_ENABLE_LSP
	ImGui::Spacing();
	ImGui::Separator();
	ImGui::Spacing();
	if (ImGui::Button("LSP Dashboard"))
	{
		lsp.dashboard.setShow(true);
		showSettingsWindow = false;
	}
	ImGui::SameLine();
	ImGui::TextDisabled("(View LSP server status)");
#else
	(void)lsp;
#endif
}

void Settings::handleWindowInput(EditorApi &api)
{
	if (ImGui::IsKeyPressed(ImGuiKey_Escape))
	{
		closeSettingsWindow(api);
		return;
	}

	if (!ImGui::IsMouseClicked(ImGuiMouseButton_Left) || ImGui::IsAnyItemHovered() ||
		ImGui::IsWindowHovered(ImGuiHoveredFlags_AnyWindow |
							   ImGuiHoveredFlags_AllowWhenBlockedByPopup) ||
		ImGui::IsPopupOpen(nullptr, ImGuiPopupFlags_AnyPopupId))
		return;

	const ImVec2 mouse = ImGui::GetMousePos();
	const ImVec2 pos = ImGui::GetWindowPos();
	const ImVec2 size = ImGui::GetWindowSize();
	if (mouse.x < pos.x || mouse.x > pos.x + size.x || mouse.y < pos.y ||
		mouse.y > pos.y + size.y)
		closeSettingsWindow(api);
}

void Settings::renderNotification(const std::string &message, float duration)
{
	static float timer = 0.0f;
	static std::string text;
	static bool show = false;

	if (!message.empty())
	{
		text = message;
		show = true;
		timer = duration;
	}
	if (!show)
		return;

	ImGuiViewport *viewport = ImGui::GetMainViewport();
	const float padding = 20.0f;
	const float textPad = 15.0f;
	const float maxWidth = viewport->Size.x * 0.8f;
	const ImVec2 textSize =
		ImGui::CalcTextSize(text.c_str(), nullptr, false, maxWidth - textPad * 2);
	const float width = std::clamp(textSize.x + textPad * 2, 200.0f, maxWidth);
	const float height =
		std::clamp(textSize.y + textPad * 2, 50.0f, viewport->Size.y * 0.4f);
	const ImVec2 origin(viewport->Pos.x + padding,
						viewport->Pos.y + viewport->Size.y - height - padding);

	const auto &bg = settings["backgroundColor"];
	ImDrawList *draw = ImGui::GetForegroundDrawList();
	draw->AddRectFilled(origin,
						ImVec2(origin.x + width, origin.y + height),
						IM_COL32(static_cast<int>(bg[0].get<float>() * 255),
								 static_cast<int>(bg[1].get<float>() * 255),
								 static_cast<int>(bg[2].get<float>() * 255),
								 230),
						8.0f);
	draw->AddRect(origin,
				  ImVec2(origin.x + width, origin.y + height),
				  IM_COL32(255, 255, 255, 255),
				  8.0f,
				  0,
				  1.0f);
	draw->AddText(ImVec2(origin.x + textPad, origin.y + textPad),
				  IM_COL32(255, 255, 255, 255),
				  text.c_str());

	timer -= ImGui::GetIO().DeltaTime;
	if (timer <= 0.0f)
		show = false;
}
