/*
File: ned_embed.cpp
Description: Implementation of the embeddable NED editor wrapper.
*/

// Include GLEW first to avoid conflicts
#include <GL/glew.h>

#include "ned_embed.h"

#include "ai/ai_agent.h"
#include "ai/ai_agent.h" // Ensure AI agent is included
#include "editor/editor.h"
#include "editor/editor_bookmarks.h"
#include "files/file_finder.h"
#include "files/file_tree.h"
#include "files/files.h"

#include "shaders/shader_manager.h"
#include "shaders/shader_types.h"
#include "util/font.h"
#include "util/init.h"
#include "util/keybinds.h"
#include "util/render.h"
#include "util/settings.h"
#include "util/splitter.h"
#include "util/terminal.h"
#include "util/welcome.h"
#include "util/window_resize.h"

// Include global variable declarations
#include "globals.h"

#include <algorithm> // for std::max

// Stub for macOS-specific function that's not needed in the embedded version
extern "C" void updateMacOSWindowProperties(float opacity, bool blurEnabled)
{
	// This function is not needed for the embedded app
	(void)opacity;
	(void)blurEnabled;
}

// Global variables are now defined in globals.cpp

// Constants
constexpr float kAgentSplitterWidth = 6.0f;

NedEmbed::NedEmbed()
	: settings(nullptr), splitter(nullptr), windowResize(nullptr), showSidebar(true),
	  showAgentPane(false), showLineNumbers(true), showWelcome(true), isEmbedded(true),
	  initialized(false)
{
	// Auto-initialize when constructed
	initialize();
}

NedEmbed::~NedEmbed() { cleanup(); }

bool NedEmbed::initialize()
{
	if (initialized)
	{
		return true;
	}

	if (!initializeComponents())
	{
		return false;
	}

	initialized = true;
	return true;
}

bool NedEmbed::initializeComponents()
{
	// Initialize settings
	settings = &gSettings;

	// Initialize splitter
	splitter = new Splitter();

	// Initialize window resize (without GLFW window)
	windowResize = new WindowResize();

	// Initialize settings and configuration
	gSettings.loadSettings();
	if (gKeybinds.loadKeybinds())
	{
		std::cout << "Initial keybinds loaded successfully." << std::endl;
	}

	// Load UI settings
	Splitter::loadSidebarSettings();
	Splitter::loadAgentPaneSettings();
	Splitter::adjustAgentSplitPosition();

	// Initialize fonts
	gFont.initialize();

	// Initialize file explorer
	gFileExplorer.loadIcons();

	// Configure ImGui to only allow window movement from title bar
	// This prevents accidental window movement when clicking/dragging in content areas
	ImGuiIO &io = ImGui::GetIO();
	io.ConfigWindowsMoveFromTitleBarOnly = true;

	// Apply initial background color settings to ImGui style
	// This ensures the background color is set correctly on startup
	ImGuiStyle &style = ImGui::GetStyle();

	// Set the child window background color (for editor panes, file explorer, etc.)
	// Use a proper alpha value (1.0) instead of the settings alpha which might be 0
	style.Colors[ImGuiCol_ChildBg] =
		ImVec4(gSettings.getSettings()["backgroundColor"][0].get<float>(),
			   gSettings.getSettings()["backgroundColor"][1].get<float>(),
			   gSettings.getSettings()["backgroundColor"][2].get<float>(),
			   1.0f); // Use full alpha for child windows

	// Also set the main window background color for popups and settings windows
	// but use a slightly different alpha to distinguish from child windows
	style.Colors[ImGuiCol_WindowBg] =
		ImVec4(gSettings.getSettings()["backgroundColor"][0].get<float>(),
			   gSettings.getSettings()["backgroundColor"][1].get<float>(),
			   gSettings.getSettings()["backgroundColor"][2].get<float>(),
			   0.95f); // Slightly transparent for main windows

	// Set scrollbar background to transparent for embedded mode on startup
	style.Colors[ImGuiCol_ScrollbarBg] = ImVec4(0, 0, 0, 0); // Transparent track

	return true;
}

void NedEmbed::render()
{
	if (!initialized || !settings || !splitter || !windowResize)
	{
		return;
	}

	// Get the available content region size automatically
	ImVec2 contentSize = ImGui::GetContentRegionAvail();
	float width = contentSize.x;
	float height = contentSize.y;

	// Handle input
	handleInput();

	// Handle file dialog (this is what was missing!)
	if (gFileExplorer.handleFileDialog())
	{
		// If a folder was selected, hide the welcome screen
		if (showWelcome)
		{
			showWelcome = false;
		}
	}

	// Set up the editor area - this is the key part where we extract from
	// Render::renderMainWindow()
	ImGui::PushFont(gFont.currentFont);

	float padding = ImGui::GetStyle().WindowPadding.x;
	float availableWidth =
		width - padding * 3 - (showAgentPane ? kAgentSplitterWidth : 0.0f);

	// Force agent pane to be hidden in embedded mode
	Splitter::showAgentPane = false;

	// Check if welcome screen should be shown (like in standalone app)
	if (showWelcome)
	{
		// Welcome screen takes over the entire window when visible
		// Set the welcome screen's embedded flag based on our embedded state
		gWelcome.setEmbedded(isEmbedded);
		gWelcome.render();
		// Add a dummy item to satisfy ImGui's boundary requirements ONLY in embedded mode
		if (isEmbedded)
		{
			ImGui::Dummy(ImVec2(0, 0));
		}
		// Make sure we pop the font before returning to avoid font stack issues
		ImGui::PopFont();
		return; // Don't render editor when welcome screen is visible
	}

	if (Splitter::showSidebar)
	{
		// Render with sidebar: File Explorer + Editor (no agent pane)
		float leftSplit = settings->getSplitPos();

		// Ensure minimum widths and prevent negative values
		float minWidth = 50.0f; // Minimum width for any component
		float explorerWidth = std::max(availableWidth * leftSplit, minWidth);
		float editorWidth =
			std::max(availableWidth - explorerWidth - (padding * 2), minWidth);

		// Render File Explorer
		renderFileExplorer(explorerWidth);
		ImGui::SameLine(0, 0);

		// Render left splitter (only when sidebar is visible)
		renderSplitter(padding, availableWidth);
		ImGui::SameLine(0, 0);

		// Render Editor
		renderEditor(editorWidth);
	} else
	{
		// No sidebar: just editor (no agent pane)
		float editorWidth = availableWidth + 5.0f; // Full width for editor

		renderEditor(editorWidth);
	}

	// Render additional components that are called in the standalone app's renderFrame
	// These are crucial for settings popup, notifications, and keybinds

	// Check if terminal should be rendered (like in standalone app)
	if (gTerminal.isTerminalVisible())
	{
		// Terminal takes over the editor area when visible
		// Set the terminal's embedded flag based on our embedded state
		gTerminal.setEmbedded(isEmbedded);
		// Don't pop the font here - let the terminal handle its own font management
		gTerminal.render();
		// Don't return early - allow settings window to render even when terminal is visible
	}

	// Always render settings and other UI components, regardless of terminal visibility
	// Set embedded flag for settings to constrain popup to editor pane
	gSettings.setEmbedded(true);
	gSettings.renderSettingsWindow();
	gSettings.setEmbedded(false); // Reset for standalone app

	// Set embedded flag for FileFinder to constrain it to editor pane
	gFileFinder.setEmbedded(true);

	gSettings.renderNotification("");
	gKeybinds.checkKeybindsFile();

	ImGui::PopFont();
}

void NedEmbed::renderEditor(float editorWidth)
{
	// Use the existing editor render function
	gEditor.renderEditor(gFont.currentFont, editorWidth);
}

void NedEmbed::renderFileExplorer(float explorerWidth)
{
	// Use the existing file explorer render function
	gFileExplorer.renderFileExplorer(explorerWidth);
}

void NedEmbed::renderAgentPane(float agentWidth)
{
	// Use the existing AI agent render function
	gAIAgent.render(agentWidth, gFont.largeFont);
}

void NedEmbed::renderSplitter(float padding, float availableWidth)
{
	// Use the existing splitter render function
	splitter->renderSplitter(padding, availableWidth);
}

void NedEmbed::setShowSidebar(bool show) { showSidebar = show; }

void NedEmbed::setShowAgentPane(bool show) { showAgentPane = show; }

void NedEmbed::setShowLineNumbers(bool show) { showLineNumbers = show; }

void NedEmbed::setShowWelcome(bool show) { showWelcome = show; }

void NedEmbed::handleInput()
{
	// Check if the editor pane is focused before processing keybinds
	// This prevents keybinds from being processed when the app pane is not focused
	bool isEditorPaneFocused =
		ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);

	// Only process keybinds if the editor pane is focused
	if (isEditorPaneFocused)
	{
		// Handle keyboard shortcuts (like in standalone app)
		if (gKeybinds.handleKeyboardShortcuts())
		{
			// Mark for redraw if needed
		}
	}
}

void NedEmbed::checkForFontReload()
{
	if (!initialized || !settings)
	{
		return;
	}

	// Handle font reloading automatically (like in standalone app)
	// First check for settings changes to detect font size changes
	gSettings.setEmbedded(true);
	bool needFontReload = false;
	bool needsRedraw = false;
	int framesToRender = 0;
	float lastOpacity = 0.0f;
	bool lastBlurEnabled = false;
	gSettings.handleSettingsChanges(
		needFontReload,
		needsRedraw,
		framesToRender,
		[](bool enabled) { /* shader not used in embedded mode */ },
		lastOpacity,
		lastBlurEnabled);

	// Handle font reloading BEFORE ImGui frame starts
	if (needFontReload)
	{
		gFont.handleFontReloadWithFrameUpdates();
	}
}

void NedEmbed::cleanup() { cleanupComponents(); }

void NedEmbed::cleanupComponents()
{
	if (splitter)
	{
		delete splitter;
		splitter = nullptr;
	}

	if (windowResize)
	{
		delete windowResize;
		windowResize = nullptr;
	}

	settings = nullptr;
	initialized = false;
}