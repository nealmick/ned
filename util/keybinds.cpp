#include "keybinds.h"
#include "../editor/editor.h"
#include "../files/files.h"
#include "../lsp/lsp_client.h"
#include "../util/close_popper.h"
#include "../util/settings.h"
#include "../util/splitter.h"
#include "../util/terminal.h"
#include <algorithm>
#include <fstream>
#include <iomanip>
#include <iostream>

// Global variables needed for the function
extern Terminal gTerminal;
extern FileExplorer gFileExplorer;
extern Settings gSettings;
extern EditorState editor_state;

constexpr float kAgentSplitterWidth = 6.0f;

// Helper function for clamping values
float clamp(float value, float min, float max)
{
	if (value < min)
		return min;
	if (value > max)
		return max;
	return value;
}

extern KeybindsManager gKeybinds;

KeybindsManager::KeybindsManager()
	: keybinds_(json::object()), lastKeybindsModificationTime_(fs::file_time_type::min())
{
	// Constructor
}

std::string KeybindsManager::getUserKeybindsFilePath()
{
	fs::path userSettingsDir =
		fs::path(settingsFileManager_.getUserSettingsPath()).parent_path();
	return (userSettingsDir / KEYBINDS_FILENAME_).string();
}

void KeybindsManager::ensureUserKeybindsFileExists()
{
	keybindsFilePath_ = getUserKeybindsFilePath();
	fs::path targetUserPath = keybindsFilePath_;
	fs::path userSettingsDir = targetUserPath.parent_path();

	if (!fs::exists(userSettingsDir))
	{
		try
		{
			fs::create_directories(userSettingsDir);
			std::cout << "[Keybinds] Created user settings directory: "
					  << userSettingsDir.string() << std::endl;
		} catch (const fs::filesystem_error &e)
		{
			std::cerr << "[Keybinds] Error creating user settings directory "
					  << userSettingsDir.string() << ": " << e.what() << std::endl;
			return;
		}
	}

	if (!fs::exists(targetUserPath))
	{
		std::cout << "[Keybinds] User keybinds file " << targetUserPath.string()
				  << " not found." << std::endl;
		fs::path appResourcesDir = settingsFileManager_.getAppResourcesPath();
		fs::path bundleSourcePath = appResourcesDir / "settings" / KEYBINDS_FILENAME_;

		if (fs::exists(bundleSourcePath))
		{
			try
			{
				fs::copy_file(bundleSourcePath,
							  targetUserPath,
							  fs::copy_options::overwrite_existing);
				std::cout << "[Keybinds] Copied " << KEYBINDS_FILENAME_
						  << " from bundle (" << bundleSourcePath.string() << ") to "
						  << targetUserPath.string() << std::endl;
			} catch (const fs::filesystem_error &e)
			{
				std::cerr << "[Keybinds] Error copying " << KEYBINDS_FILENAME_
						  << " from bundle: " << e.what()
						  << ". Will create a default file." << std::endl;
				json defaultKeybinds = {{"toggle_bookmarks_menu", "b"}};
				settingsFileManager_.saveJsonFile(targetUserPath.string(),
												  defaultKeybinds);
				std::cout << "[Keybinds] Created default " << KEYBINDS_FILENAME_ << " at "
						  << targetUserPath.string() << std::endl;
			}
		} else
		{
			std::cout << "[Keybinds] Bundle keybinds file " << bundleSourcePath.string()
					  << " not found." << std::endl;
			std::cout << "[Keybinds] Creating default " << KEYBINDS_FILENAME_ << " at "
					  << targetUserPath.string() << std::endl;
			json defaultKeybinds = {{"toggle_bookmarks_menu", "b"}};
			settingsFileManager_.saveJsonFile(targetUserPath.string(), defaultKeybinds);
		}
	}
}

bool KeybindsManager::loadKeybindsFromFile(const std::string &filePath)
{
	if (!settingsFileManager_.loadJsonFile(filePath, keybinds_))
	{
		std::cerr << "[Keybinds] Failed to load or parse keybinds from " << filePath
				  << ". Attempting to load default keybinds..." << std::endl;

		// Try to load default keybinds
		fs::path defaultKeybindsPath =
			fs::path(filePath).parent_path() / "default-keybinds.json";
		if (fs::exists(defaultKeybindsPath))
		{
			if (settingsFileManager_.loadJsonFile(defaultKeybindsPath.string(), keybinds_))
			{
				std::cout << "[Keybinds] Successfully loaded default keybinds from "
						  << defaultKeybindsPath << std::endl;
				processKeybinds();
				gSettings.renderNotification(
					"Error in keybinds.json\nLoaded default backup keybinds");
				return true;
			} else
			{
				std::cerr << "[Keybinds] Failed to load default keybinds from "
						  << defaultKeybindsPath << std::endl;
			}
		} else
		{
			std::cerr << "[Keybinds] Default keybinds file not found at "
					  << defaultKeybindsPath << std::endl;
		}

		// If we get here, both regular and default keybinds failed
		keybinds_ = json::object();
		processKeybinds(); // Ensure map is cleared/reset on failure
		return false;
	}
	std::cout << "[Keybinds] Successfully loaded raw keybinds from " << filePath
			  << std::endl;
	processKeybinds(); // Process the loaded keybinds
	return true;
}

bool KeybindsManager::loadKeybinds()
{
	ensureUserKeybindsFileExists();

	if (keybindsFilePath_.empty() || !fs::exists(keybindsFilePath_))
	{
		std::cerr << "[Keybinds] Keybinds file path is invalid or file does "
					 "not exist after setup "
					 "attempt: "
				  << (keybindsFilePath_.empty() ? "(empty path)" : keybindsFilePath_)
				  << std::endl;
		keybinds_ = json::object();
		processKeybinds(); // <<< --- FIX --- Ensure map is cleared/reset
		return false;
	}

	// loadKeybindsFromFile now calls processKeybinds internally.
	if (loadKeybindsFromFile(keybindsFilePath_))
	{
		// printKeybinds(); // Prints raw JSON, processKeybinds might print its
		// own status
		updateLastModificationTime();
		return true;
	}
	// If loadKeybindsFromFile failed, it also called processKeybinds.
	return false;
}

void KeybindsManager::printKeybinds() const
{
	std::cout << "[Keybinds] Current Raw Keybinds:" << std::endl;

	if (keybinds_.is_object() && !keybinds_.empty())
	{
		std::cout << keybinds_.dump(4)
				  << std::endl; // The '4' is for 4-space indentation.
	} else
	{
		std::cout << "{}" << std::endl; // Print empty JSON if not a valid/filled object
	}
}

void KeybindsManager::updateLastModificationTime()
{
	if (keybindsFilePath_.empty() || !fs::exists(keybindsFilePath_))
	{
		lastKeybindsModificationTime_ = fs::file_time_type::min();
		return;
	}
	try
	{
		lastKeybindsModificationTime_ = fs::last_write_time(keybindsFilePath_);
	} catch (const fs::filesystem_error &e)
	{
		std::cerr << "[Keybinds] Error getting last write time for " << keybindsFilePath_
				  << ": " << e.what() << std::endl;
		lastKeybindsModificationTime_ = fs::file_time_type::min();
	}
}

void KeybindsManager::checkKeybindsFile()
{
	checkFrameCounter_++;
	if (checkFrameCounter_ < CHECK_INTERVAL_FRAMES)
	{
		return;
	}
	checkFrameCounter_ = 0;

	if (keybindsFilePath_.empty() || !fs::exists(keybindsFilePath_))
	{
		std::cout << "[Keybinds] " << KEYBINDS_FILENAME_
				  << " not found during periodic check or path is invalid. "
					 "Attempting to "
					 "reload/reinitialize."
				  << std::endl;
		// loadKeybinds will call loadKeybindsFromFile which calls processKeybinds
		bool loaded = loadKeybinds();
		if (loaded)
		{
			std::cout << "[Keybinds] Re-initialized successfully after file "
						 "was missing."
					  << std::endl;
		} else
		{
			std::cout << "[Keybinds] Failed to re-initialize after file was missing."
					  << std::endl;
		}
		return;
	}

	try
	{
		auto currentModificationTime = fs::last_write_time(keybindsFilePath_);

		if (currentModificationTime > lastKeybindsModificationTime_)
		{
			std::cout << "[Keybinds] " << KEYBINDS_FILENAME_
					  << " has changed on disk. Reloading." << std::endl;

			json oldKeybinds = keybinds_; // Save current raw state

			// loadKeybindsFromFile will call processKeybinds
			if (loadKeybindsFromFile(keybindsFilePath_))
			{
				if (oldKeybinds != keybinds_)
				{ // Compare raw JSON to see if content changed
					std::cout << "[Keybinds] Keybinds data has been updated "
								 "(raw content changed):"
							  << std::endl;
					printKeybinds();
				} else
				{
					std::cout << "[Keybinds] File modification time changed, "
								 "but raw content "
								 "remains the same."
							  << std::endl;
					// Even if raw content is same, re-process in case something
					// external expected it. Or, if processKeybinds is
					// idempotent and fast, it's fine. processKeybinds();
					// // Already called by loadKeybindsFromFile
				}
				updateLastModificationTime();
			} else
			{
				std::cerr << "[Keybinds] Failed to reload " << KEYBINDS_FILENAME_
						  << " after detecting changes. Keybinds might be "
							 "empty or stale."
						  << std::endl;
				// loadKeybindsFromFile already called processKeybinds to
				// clear/reset the map.
				updateLastModificationTime();
			}
		} else
		{
			// std::cout << "[Keybinds] No changes detected in " <<
			// KEYBINDS_FILENAME_ << "." << std::endl; // Can be spammy
		}
	} catch (const fs::filesystem_error &e)
	{
		std::cerr << "[Keybinds] Filesystem error while checking " << keybindsFilePath_
				  << ": " << e.what() << std::endl;
		lastKeybindsModificationTime_ = fs::file_time_type::min();
	} catch (const std::exception &e)
	{
		std::cerr << "[Keybinds] Unexpected error during keybinds file check for "
				  << keybindsFilePath_ << ": " << e.what() << std::endl;
	}
}

ImGuiKey KeybindsManager::stringToImGuiKey(const std::string &keyString)
{
	std::string lowerKey = keyString;
	std::transform(lowerKey.begin(), lowerKey.end(), lowerKey.begin(), ::tolower);

	if (lowerKey.length() == 1)
	{
		char c = lowerKey[0];
		if (c >= 'a' && c <= 'z')
			return static_cast<ImGuiKey>(ImGuiKey_A + (c - 'a'));
		if (c >= '0' && c <= '9')
			return static_cast<ImGuiKey>(ImGuiKey_0 + (c - '0'));
	}

	if (lowerKey == "space" || lowerKey == "spacebar")
		return ImGuiKey_Space;
	if (lowerKey == "enter" || lowerKey == "return")
		return ImGuiKey_Enter;
	if (lowerKey == "escape" || lowerKey == "esc")
		return ImGuiKey_Escape;
	if (lowerKey == "tab")
		return ImGuiKey_Tab;
	if (lowerKey == "backspace")
		return ImGuiKey_Backspace;
	if (lowerKey == "delete" || lowerKey == "del")
		return ImGuiKey_Delete;
	if (lowerKey == "insert" || lowerKey == "ins")
		return ImGuiKey_Insert;
	if (lowerKey == "up")
		return ImGuiKey_UpArrow;
	if (lowerKey == "down")
		return ImGuiKey_DownArrow;
	if (lowerKey == "left")
		return ImGuiKey_LeftArrow;
	if (lowerKey == "right")
		return ImGuiKey_RightArrow;
	if (lowerKey == "home")
		return ImGuiKey_Home;
	if (lowerKey == "end")
		return ImGuiKey_End;
	if (lowerKey == "pageup" || lowerKey == "pgup")
		return ImGuiKey_PageUp;
	if (lowerKey == "pagedown" || lowerKey == "pgdn")
		return ImGuiKey_PageDown;
	if (lowerKey == "leftctrl" || lowerKey == "lctrl")
		return ImGuiKey_LeftCtrl;
	if (lowerKey == "rightctrl" || lowerKey == "rctrl")
		return ImGuiKey_RightCtrl;
	if (lowerKey == "leftshift" || lowerKey == "lshift")
		return ImGuiKey_LeftShift;
	if (lowerKey == "rightshift" || lowerKey == "rshift")
		return ImGuiKey_RightShift;
	if (lowerKey == "leftalt" || lowerKey == "lalt")
		return ImGuiKey_LeftAlt;
	if (lowerKey == "rightalt" || lowerKey == "ralt")
		return ImGuiKey_RightAlt;
	if (lowerKey == "leftsuper" || lowerKey == "lsuper" || lowerKey == "cmd" ||
		lowerKey == "command" || lowerKey == "win" || lowerKey == "windows")
		return ImGuiKey_LeftSuper;
	if (lowerKey == "rightsuper" || lowerKey == "rsuper")
		return ImGuiKey_RightSuper;
	if (lowerKey == "f1")
		return ImGuiKey_F1;
	if (lowerKey == "f2")
		return ImGuiKey_F2;
	if (lowerKey == "f3")
		return ImGuiKey_F3;
	if (lowerKey == "f4")
		return ImGuiKey_F4;
	if (lowerKey == "f5")
		return ImGuiKey_F5;
	if (lowerKey == "f6")
		return ImGuiKey_F6;
	if (lowerKey == "f7")
		return ImGuiKey_F7;
	if (lowerKey == "f8")
		return ImGuiKey_F8;
	if (lowerKey == "f9")
		return ImGuiKey_F9;
	if (lowerKey == "f10")
		return ImGuiKey_F10;
	if (lowerKey == "f11")
		return ImGuiKey_F11;
	if (lowerKey == "f12")
		return ImGuiKey_F12;
	if (lowerKey == "apostrophe" || lowerKey == "'")
		return ImGuiKey_Apostrophe;
	if (lowerKey == "comma" || lowerKey == ",")
		return ImGuiKey_Comma;
	if (lowerKey == "minus" || lowerKey == "-")
		return ImGuiKey_Minus;
	if (lowerKey == "period" || lowerKey == ".")
		return ImGuiKey_Period;
	if (lowerKey == "slash" || lowerKey == "/")
		return ImGuiKey_Slash;
	if (lowerKey == "semicolon" || lowerKey == ";")
		return ImGuiKey_Semicolon;
	if (lowerKey == "equal" || lowerKey == "=")
		return ImGuiKey_Equal;
	if (lowerKey == "leftbracket" || lowerKey == "[")
		return ImGuiKey_LeftBracket;
	if (lowerKey == "backslash" || lowerKey == "\\")
		return ImGuiKey_Backslash;
	if (lowerKey == "rightbracket" || lowerKey == "]")
		return ImGuiKey_RightBracket;
	if (lowerKey == "graveaccent" || lowerKey == "`")
		return ImGuiKey_GraveAccent;

	std::cerr << "[Keybinds] Warning: Unrecognized key string '" << keyString << "'"
			  << std::endl;
	return ImGuiKey_None;
}

void KeybindsManager::processKeybinds()
{
	// Add a debug print to see when this is called and what keybinds_ contains
	// std::cout << "[Keybinds] processKeybinds() called. Raw keybinds_: " <<
	// keybinds_.dump(2) << std::endl;

	processedKeybinds_.clear();
	if (!keybinds_.is_object())
	{
		if (!keybinds_.is_null())
		{ // Don't print for initial null object before loading
			std::cerr << "[Keybinds] Cannot process keybinds: raw keybinds "
						 "data is not a JSON object."
					  << std::endl;
		}
		return;
	}

	for (auto &[action, keyVal] : keybinds_.items())
	{
		if (keyVal.is_string())
		{
			std::string keyString = keyVal.get<std::string>();
			ImGuiKey imKey = stringToImGuiKey(keyString);
			// Debug print for each key being processed
			// std::cout << "[Keybinds]   Processing action: '" << action << "',
			// key string: '" << keyString << "', mapped ImGuiKey: " <<
			// static_cast<int>(imKey) << std::endl;
			if (imKey != ImGuiKey_None)
			{
				processedKeybinds_[action] = imKey;
			} else
			{
				std::cout << "[Keybinds] Action '" << action
						  << "' has unmappable key string '" << keyString
						  << "'. It will be ignored." << std::endl;
			}
		} else
		{
			std::cerr << "[Keybinds] Warning: Value for action '" << action
					  << "' is not a string. Skipping." << std::endl;
		}
	}

	// Print summary only if there was something to process or if debugging.
	// This avoids spamming if keybinds_ was empty.
	if (!keybinds_.empty())
	{
		std::cout << "[Keybinds] Finished processing. " << processedKeybinds_.size()
				  << " keybinds mapped from " << keybinds_.size() << " raw entries."
				  << std::endl;
	}
}

ImGuiKey KeybindsManager::getActionKey(const std::string &actionName) const
{
	auto it = processedKeybinds_.find(actionName);
	if (it != processedKeybinds_.end())
	{
		return it->second;
	}
	// Debug print if key not found (can be noisy, enable if needed)
	// std::cout << "[Keybinds] getActionKey: Action '" << actionName << "' not
	// found in processed map (map size: " << processedKeybinds_.size() << ")"
	// << std::endl;
	return ImGuiKey_None;
}

bool KeybindsManager::handleKeyboardShortcuts()
{
	// At the very top of this function, add a local flag.
	bool shortcutPressed = false;

	ImGuiIO &io = ImGui::GetIO();
	// Accept either Ctrl or Super (Command on macOS)
	bool modPressed = io.KeyCtrl || io.KeySuper;
	ImGuiKey toggleSidebar = getActionKey("toggle_sidebar");

	if (modPressed && ImGui::IsKeyPressed(toggleSidebar, false))
	{
		float windowWidth = ImGui::GetWindowWidth();
		float padding = ImGui::GetStyle().WindowPadding.x;
		float availableWidth =
			windowWidth - padding * 3 -
			(Splitter::showAgentPane ? kAgentSplitterWidth
									 : 0.0f); // Only account for splitter width
											  // when agent pane is visible

		float agentPaneWidthPx;
		if (Splitter::showSidebar)
		{
			agentPaneWidthPx = availableWidth * gSettings.getAgentSplitPos();
		} else
		{
			float agentSplit = gSettings.getAgentSplitPos();
			float editorWidth = availableWidth * agentSplit;
			agentPaneWidthPx = availableWidth - editorWidth - kAgentSplitterWidth;
		}

		// Toggle sidebar
		Splitter::showSidebar = !Splitter::showSidebar;

		// Save sidebar visibility setting
		gSettings.getSettings()["sidebar_visible"] = Splitter::showSidebar;
		gSettings.saveSettings();

		// Recompute availableWidth after toggling
		windowWidth = ImGui::GetWindowWidth();
		availableWidth = windowWidth - padding * 3 -
						 (Splitter::showAgentPane ? kAgentSplitterWidth : 0.0f);

		float newRightSplit;
		if (Splitter::showSidebar)
		{
			newRightSplit = agentPaneWidthPx / availableWidth;
		} else
		{
			float editorWidth = availableWidth - agentPaneWidthPx - kAgentSplitterWidth;
			newRightSplit = editorWidth / availableWidth;
		}
		gSettings.setAgentSplitPos(clamp(
			newRightSplit, 0.25f, 0.9f)); // Changed minimum from 0.1f to 0.25f (25%)

		std::cout << "Toggled sidebar visibility" << std::endl;
		shortcutPressed = true;
	}

	ImGuiKey toggleAgent = getActionKey("toggle_agent");
	if (modPressed && ImGui::IsKeyPressed(toggleAgent, false))
	{
		// Only toggle visibility, do not recalculate or set agentSplitPos
		Splitter::showAgentPane = !Splitter::showAgentPane;

		// Save agent pane visibility setting
		gSettings.getSettings()["agent_pane_visible"] = Splitter::showAgentPane;
		gSettings.saveSettings();

		std::cout << "Toggled agent pane visibility" << std::endl;
		shortcutPressed = true;
	}

	ImGuiKey toggleTerminal = getActionKey("toggle_terminal");

	if (modPressed && ImGui::IsKeyPressed(toggleTerminal, false))
	{
		// If terminal is embedded and already visible, do nothing (prevent hiding)
		if (gTerminal.getEmbedded() && gTerminal.isTerminalVisible())
		{
			// Do nothing - embedded terminal should stay visible once shown
		} else
		{
			// Normal toggle behavior for non-embedded or hidden terminal
			gTerminal.toggleVisibility();
			gFileExplorer.saveCurrentFile();
			if (gTerminal.isTerminalVisible())
			{
				ClosePopper::closeAll();
			}
		}
		shortcutPressed = true;
	}
	ImGuiKey togglesetings = getActionKey("toggle_settings_window");

	if (modPressed && ImGui::IsKeyPressed(togglesetings, false))
	{
		gFileExplorer.showWelcomeScreen = false;
		gSettings.toggleSettingsWindow();
		shortcutPressed = true;
	}

	if (modPressed)
	{
		if (ImGui::IsKeyPressed(ImGuiKey_Equal))
		{ // '+' key
			float currentSize = gSettings.getFontSize();
			gSettings.setFontSize(currentSize + 2.0f);
			editor_state.ensure_cursor_visible.vertical = true;
			editor_state.ensure_cursor_visible.horizontal = true;
			std::cout << "Cmd++: Font size increased to " << gSettings.getFontSize()
					  << std::endl;
			shortcutPressed = true;
		} else if (ImGui::IsKeyPressed(ImGuiKey_Minus))
		{ // '-' key
			float currentSize = gSettings.getFontSize();
			gSettings.setFontSize(std::max(currentSize - 2.0f, 8.0f));
			editor_state.ensure_cursor_visible.vertical = true;
			editor_state.ensure_cursor_visible.horizontal = true;
			std::cout << "Cmd+-: Font size decreased to " << gSettings.getFontSize()
					  << std::endl;
			shortcutPressed = true;
		}
	}

	if (modPressed && ImGui::IsKeyPressed(ImGuiKey_Slash, false))
	{
		ClosePopper::closeAll();
		gFileExplorer.showWelcomeScreen = !gFileExplorer.showWelcomeScreen;
		if (gTerminal.isTerminalVisible())
		{
			gTerminal.toggleVisibility();
		}
		gFileExplorer.saveCurrentFile();
		shortcutPressed = true;
	}
	if (modPressed && ImGui::IsKeyPressed(ImGuiKey_O, false))
	{
		std::cout << "triggering file dialog" << std::endl;
		ClosePopper::closeAll();
		gFileExplorer.saveCurrentFile();
		gFileExplorer._showFileDialog = true;
		shortcutPressed = true;
	}

	// Handle all LSP keybinds
	if (gLSPClient.keybinds())
	{
		shortcutPressed = true;
	}

	// At the very end of the function, set the main redraw flag.
	if (shortcutPressed)
	{
		return true; // Indicate that shortcuts were pressed
	}
	return false; // Indicate that shortcuts were not pressed
}