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
#include "lsp/lsp_goto_ref.h"
#include "lsp/lsp_goto_def.h"

// Include global variable declarations
#include "globals.h"

#include <algorithm> // for std::max

// Global variables are now defined in globals.cpp

// Constants
constexpr float kAgentSplitterWidth = 6.0f;

NedEmbed::NedEmbed()
	: settings(nullptr), splitter(nullptr), windowResize(nullptr), showSidebar(true),
	  showAgentPane(false), showLineNumbers(true), showWelcome(true), isEmbedded(true),
	  initialized(false)
{
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

	return true;
}

void NedEmbed::render(float width, float height)
{
	std::cout << "NedEmbed::render() called with width=" << width << ", height=" << height
			  << std::endl;

	if (!initialized || !settings || !splitter || !windowResize)
	{
		std::cout << "NedEmbed::render() - early return due to uninitialized components"
				  << std::endl;
		return;
	}

	std::cout << "NedEmbed::render() - components initialized, handling input..."
			  << std::endl;
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

	std::cout << "NedEmbed::render() - setting up editor area..." << std::endl;
	// Set up the editor area - this is the key part where we extract from
	// Render::renderMainWindow()
	std::cout << "NedEmbed::render() - pushing font..." << std::endl;
	ImGui::PushFont(gFont.currentFont);

	float padding = ImGui::GetStyle().WindowPadding.x;
	float availableWidth =
		width - padding * 3 - (showAgentPane ? kAgentSplitterWidth : 0.0f);

	std::cout << "NedEmbed::render() - calculating layout..." << std::endl;
	// Force agent pane to be hidden in embedded mode
	Splitter::showAgentPane = false;

	// Check if welcome screen should be shown (like in standalone app)
	if (showWelcome)
	{
		// Welcome screen takes over the entire window when visible
		// Set the welcome screen's embedded flag based on our embedded state
		gWelcome.setEmbedded(isEmbedded);
		gWelcome.render();
		// Make sure we pop the font before returning to avoid font stack issues
		ImGui::PopFont();
		return; // Don't render editor when welcome screen is visible
	}

	if (Splitter::showSidebar)
	{
		std::cout << "NedEmbed::render() - rendering with sidebar..." << std::endl;
		// Render with sidebar: File Explorer + Editor (no agent pane)
		float leftSplit = settings->getSplitPos();

		// Ensure minimum widths and prevent negative values
		float minWidth = 50.0f; // Minimum width for any component
		float explorerWidth = std::max(availableWidth * leftSplit, minWidth);
		float editorWidth =
			std::max(availableWidth - explorerWidth - (padding * 2), minWidth);

		std::cout << "NedEmbed::render() - layout: availableWidth=" << availableWidth
				  << ", leftSplit=" << leftSplit << ", explorerWidth=" << explorerWidth
				  << ", editorWidth=" << editorWidth << std::endl;

		std::cout << "NedEmbed::render() - rendering file explorer..." << std::endl;
		// Render File Explorer
		renderFileExplorer(explorerWidth);
		std::cout << "NedEmbed::render() - file explorer rendered successfully"
				  << std::endl;
		ImGui::SameLine(0, 0);

		std::cout << "NedEmbed::render() - about to render splitter..." << std::endl;
		// Render left splitter (only when sidebar is visible)
		renderSplitter(padding, availableWidth);
		std::cout << "NedEmbed::render() - splitter rendered successfully" << std::endl;
		ImGui::SameLine(0, 0);

		std::cout << "NedEmbed::render() - about to render editor..." << std::endl;
		// Render Editor
		renderEditor(editorWidth);
		std::cout << "NedEmbed::render() - editor rendered successfully" << std::endl;
	} else
	{
		// No sidebar: just editor (no agent pane)
		float editorWidth = availableWidth + 5.0f; // Full width for editor

		renderEditor(editorWidth);
	}

	std::cout << "NedEmbed::render() - about to call windowResize->resize()..."
			  << std::endl;
	// windowResize->resize(); // Temporarily disabled to test
	std::cout << "NedEmbed::render() - windowResize disabled for testing, about to "
				 "render additional components..."
			  << std::endl;

	// Render additional components that are called in the standalone app's renderFrame
	// These are crucial for settings popup, notifications, and keybinds

	// Check if terminal should be rendered (like in standalone app)
	if (gTerminal.isTerminalVisible())
	{
		// Terminal takes over the editor area when visible
		// Set the terminal's embedded flag based on our embedded state
		gTerminal.setEmbedded(isEmbedded);
		// Make sure we pop the font before rendering terminal to avoid font stack issues
		ImGui::PopFont();
		gTerminal.render();
		return; // Don't render other UI when terminal is visible
	}

	// Set embedded flag for settings to constrain popup to editor pane
	gSettings.setEmbedded(true);
	gSettings.renderSettingsWindow();
	gSettings.setEmbedded(false); // Reset for standalone app
	
	// Set embedded flag for LSP popups to constrain them to editor pane
	gLSPGotoRef.setEmbedded(true);
	gLSPGotoDef.setEmbedded(true);

	gSettings.renderNotification("");
	gKeybinds.checkKeybindsFile();
	// windowResize.renderResizeOverlay(gFont.largeFont); // Temporarily disabled

	ImGui::PopFont();
	std::cout << "NedEmbed::render() - font popped successfully, render complete!"
			  << std::endl;
}

void NedEmbed::renderEditor(float editorWidth)
{
	// Use the existing editor render function
	gEditor.renderEditor(gFont.currentFont, editorWidth);
}

void NedEmbed::renderFileExplorer(float explorerWidth)
{
	std::cout << "NedEmbed::renderFileExplorer() called with width=" << explorerWidth
			  << std::endl;
	// Use the existing file explorer render function
	gFileExplorer.renderFileExplorer(explorerWidth);
	std::cout << "NedEmbed::renderFileExplorer() completed successfully" << std::endl;
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
	bool isEditorPaneFocused = ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);
	
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