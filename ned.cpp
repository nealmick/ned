/*
File: ned.cpp
Description: Main application class implementation for NED text editor.
*/

#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "imgui_internal.h"
#ifdef IMGUI_ENABLE_FREETYPE
#include "misc/freetype/imgui_freetype.h"
#endif
#ifdef __APPLE__
#include "macos_window.h"
#endif

#include "../ai/ai_tab.h"
#include "ned.h"

#include "editor/editor_bookmarks.h"
#include "editor/editor_header.h"
#include "editor/editor_highlight.h"
#include "editor/editor_scroll.h"
#include "files/files.h"
#include "util/debug_console.h"
#include "util/keybinds.h"
#include "util/settings.h"
#include "util/terminal.h"
#include "util/welcome.h"

#include <cstdio>
#include <filesystem>
#include <iostream>
#include <thread>

#include "ai/ai_agent.h"

// global scope
Bookmarks gBookmarks;
bool showSidebar = true;
bool showAgentPane = true;

float agentSplitPos = 0.75f; // 75% editor, 25% agent pane by default

constexpr float kAgentSplitterWidth = 6.0f;
AIAgent gAIAgent;

Ned::Ned()
	: window(nullptr), needFontReload(false), windowFocused(true), explorerWidth(0.0f),
	  editorWidth(0.0f), initialized(false)
{
}

Ned::~Ned()
{
	if (initialized)
	{
		cleanup();
	}
}

bool Ned::initialize()
{
	if (!initializeGraphics())
	{
		return false;
	}
	gSettings.loadSettings();
	if (gKeybinds.loadKeybinds())
	{
		std::cout << "Initial keybinds loaded successfully." << std::endl;
	} else
	{
		std::cout << "Failed to load initial keybinds." << std::endl;
	}

	// Load sidebar visibility settings
	showSidebar = gSettings.getSettings().value("sidebar_visible", true);
	showAgentPane = gSettings.getSettings().value("agent_pane_visible", true);

	// Adjust agent split position if sidebar is hidden (first load only)
	if (!gSettings.isAgentSplitPosProcessed() && !showSidebar)
	{
		float currentAgentSplitPos = gSettings.getAgentSplitPos();
		float originalValue = currentAgentSplitPos;
		// When sidebar is hidden, the agent split position represents editor width,
		// not agent pane width So we need to invert it: 1 - saved_agent_position
		float newAgentSplitPos = 1.0f - currentAgentSplitPos;
		gSettings.setAgentSplitPos(newAgentSplitPos);
		std::cout << "[Ned] Adjusted agent_split_pos from " << originalValue << " to "
				  << newAgentSplitPos << " (sidebar hidden) on first load" << std::endl;
	}

#ifdef __APPLE__
	float opacity = gSettings.getSettings().value("mac_background_opacity", 0.5f);
	bool blurEnabled = gSettings.getSettings().value("mac_blur_enabled", true);

	// Set up application delegate for proper Cmd+Q handling
	setupMacOSApplicationDelegate();

	// Initial configuration
	configureMacOSWindow(window, opacity, blurEnabled);

	// Initialize tracking variables
	lastOpacity = opacity;
	lastBlurEnabled = blurEnabled;
#endif

	// Rest of initialization...
	glfwSetWindowUserPointer(window, this);
	initializeImGui();
	initializeResources();
	quad.initialize();

	// Initialize window resize handler
	windowResize.initialize(window);

	// Initialize frame management
	gFrame.initialize();

	initialized = true;
	return true;
}

bool Ned::initializeGraphics()
{
	if (!glfwInit())
	{
		std::cerr << "Failed to initialize GLFW" << std::endl;
		return false;
	}
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
	glfwWindowHint(GLFW_DECORATED,
				   GLFW_FALSE); // No OS decorations for macOS (custom handling)
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // macOS specific
	glfwWindowHint(GLFW_TRANSPARENT_FRAMEBUFFER, GLFW_TRUE);
#else // For Linux/Ubuntu and other non-Apple platforms
	glfwWindowHint(GLFW_DECORATED, GLFW_TRUE); // Use OS decorations
#endif
	// GLFW_RESIZABLE should be TRUE for both.
	// On Linux, the OS window manager handles resizing if DECORATED is TRUE.
	// On macOS, your custom logic handles resizing.
	glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

	window = glfwCreateWindow(1200, 750, "NED", NULL, NULL);

	if (!window)
	{
		std::cerr << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		return false;
	}

	glfwMakeContextCurrent(window);
	glfwSwapInterval(0);
	glfwSetWindowRefreshCallback(window, [](GLFWwindow *window) { glfwPostEmptyEvent(); });

	// Enable raw mouse motion for more accurate tracking
	glfwSetInputMode(window, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);

	glewExperimental = GL_TRUE;
	if (GLenum err = glewInit(); GLEW_OK != err)
	{
		std::cerr << "🔴 GLEW initialization failed: " << glewGetErrorString(err)
				  << std::endl;
		glfwTerminate();
		return false;
	}
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glGetError(); // Clear any GLEW startup errors

	// Initialize shader manager
	if (!shaderManager.initializeShaders())
	{
		std::cerr << "🔴 Shader initialization failed" << std::endl;
		glfwTerminate();
		return false;
	}

	return true;
}

void Ned::initializeImGui()
{
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO &io = ImGui::GetIO();
	io.IniFilename = NULL; // Disable ImGui .ini file writing
	(void)io;
	ImGui::StyleColorsDark();
	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init("#version 330");
	glfwSetScrollCallback(window, Ned::scrollCallback);
}

void Ned::scrollCallback(GLFWwindow *window, double xoffset, double yoffset)
{
	Ned *app = static_cast<Ned *>(glfwGetWindowUserPointer(window));
	app->scrollXAccumulator += xoffset * 0.3; // Same multiplier as vertical
	app->scrollYAccumulator += yoffset * 0.3;
}

void Ned::checkForActivity()
{
	// This function will be called every loop to check for immediate input.
	ImGuiIO &io = ImGui::GetIO();

	// Check for any input activity
	bool hasInput = false;

	// Mouse activity
	if (io.MousePos.x != io.MousePosPrev.x || io.MousePos.y != io.MousePosPrev.y ||
		io.MouseWheel != 0.0f || io.MouseWheelH != 0.0f || io.MouseDown[0] ||
		io.MouseDown[1] || io.MouseDown[2])
	{
		hasInput = true;
	}

	// Keyboard activity
	if (io.InputQueueCharacters.size() > 0 || io.KeysDown[0] || io.KeysDown[1] ||
		io.KeysDown[2] || io.KeysDown[3] || io.KeysDown[4])
	{
		hasInput = true;
	}

	// ImGui widget activity
	if (ImGui::IsAnyItemActive())
	{
		hasInput = true;
	}

	// Check for active scroll animation (treat as input activity for smooth
	// rendering)
	if (gEditorScroll.isScrollAnimationActive())
	{
		hasInput = true;
	}

	// If we have any input, update activity time
	if (hasInput)
	{
		gFrame.setLastActivityTime(glfwGetTime());
	}
}
// In Ned::initializeResources()
void Ned::initializeResources()
{
	gDebugConsole.toggleVisibility();
	gEditorHighlight.setTheme(gSettings.getCurrentTheme());

	// Apply settings with the actual loaded font size
	gSettings.ApplySettings(ImGui::GetStyle());

	// Initialize fonts using the Font class
	gFont.initialize();

	// Continue with remaining initialization
	shaderManager.setShaderEnabled(gSettings.getSettings()["shader_toggle"].get<bool>());
	gFileExplorer.loadIcons();
}


void Ned::run()
{
	if (!initialized)
	{
		std::cerr << "Cannot run: Not initialized" << std::endl;
		return;
	}

	while (!glfwWindowShouldClose(window))
	{
		// Get current time for activity tracking
		double currentTime = glfwGetTime();

		// Poll for events (this will block until something happens, or timeout)
		// When shaders are enabled, use settings FPS target for timeout
		// When shaders are disabled, use normal timeout logic
		double timeout;
		if (shaderManager.isShaderEnabled())
		{
			// Use settings FPS target when shaders are enabled
			float fpsTarget = 60.0f;
			if (gSettings.getSettings().contains("fps_target") &&
				gSettings.getSettings()["fps_target"].is_number())
			{
				fpsTarget = gSettings.getSettings()["fps_target"].get<float>();
			}
			timeout = 1.0 / fpsTarget; // Convert FPS to timeout
		} else
		{
			// Use shorter timeout if we recently had activity (for smoother
			// interaction) Also respect minimum FPS: 25 FPS normally
			double minFPS = 25.0;
			double maxTimeout = 1.0 / minFPS; // Convert FPS to timeout
			timeout =
				(currentTime - gFrame.lastActivityTime()) < 0.5 ? 0.016 : maxTimeout;
		}
		glfwWaitEventsTimeout(timeout);

		// Check for Cmd+Q termination on macOS
#ifdef __APPLE__
		if (shouldTerminateApplication())
		{
			glfwSetWindowShouldClose(window, 1);
		}
#endif

		// Handle scroll accumulators (this was removed but needed for scrolling)
		if (scrollXAccumulator != 0.0 || scrollYAccumulator != 0.0)
		{
			ImGui::GetIO().MouseWheel += scrollYAccumulator;  // Vertical
			ImGui::GetIO().MouseWheelH += scrollXAccumulator; // Horizontal
			scrollXAccumulator = 0.0;
			scrollYAccumulator = 0.0;
		}

		// Check for activity and decide if we should keep rendering
		checkForActivity();
		bool shouldKeepRendering = (currentTime - gFrame.lastActivityTime()) < 0.5 ||
								   gEditorScroll.isScrollAnimationActive();

		// Always render a frame after polling events
		auto frame_start = std::chrono::high_resolution_clock::now();

		handleBackgroundUpdates(currentTime);
		gSettings.handleSettingsChanges(
			needFontReload,
			gFrame.needsRedrawRef(),
			gFrame.framesToRenderRef(),
			[this](bool enabled) { shaderManager.setShaderEnabled(enabled); },
			lastOpacity,
			lastBlurEnabled);

		int display_w, display_h;
		glfwGetFramebufferSize(window, &display_w, &display_h);
		// Delegate framebuffer initialization to the shader manager
		shaderManager.initializeFramebuffers(display_w, display_h, fb, accum);

		gFrame.setupImGuiFrame();
		handleWindowFocus();
		if (gFileExplorer.handleFileDialog())
		{
			gFrame.setNeedsRedraw(true);
			gFrame.setFramesToRender(std::max(gFrame.framesToRender(), 3));
		}
		renderFrame();

		// Handle font reloading
		if (needFontReload)
		{
			gFrame.setNeedsRedraw(true);
			gFrame.setFramesToRender(std::max(gFrame.framesToRender(), 3));
			gFont.handleFontReload(needFontReload);
		}

		gFrame.handleFrameTiming(
			frame_start, shaderManager.isShaderEnabled(), windowFocused, gSettings);
	}
}

void Ned::handleBackgroundUpdates(double currentTime)
{
	if (currentTime - gFrame.getTiming().lastSettingsCheck >=
		gFrame.SETTINGS_CHECK_INTERVAL)
	{
		// Store previous state to detect changes
		bool hadSettingsChanged = gSettings.hasSettingsChanged();
		bool hadFontChanged = gSettings.hasFontChanged();
		bool hadFontSizeChanged = gSettings.hasFontSizeChanged();
		bool hadThemeChanged = gSettings.hasThemeChanged();

		gSettings.checkSettingsFile();

		// Check if any settings changed
		if (gSettings.hasSettingsChanged() != hadSettingsChanged ||
			gSettings.hasFontChanged() != hadFontChanged ||
			gSettings.hasFontSizeChanged() != hadFontSizeChanged ||
			gSettings.hasThemeChanged() != hadThemeChanged)
		{
			gFrame.setNeedsRedraw(true);
			gFrame.setFramesToRender(
				std::max(gFrame.framesToRender(), 2)); // Reduced frame count
		}
		gFrame.getTiming().lastSettingsCheck = currentTime;
	}

	if (currentTime - gFrame.getTiming().lastFileTreeRefresh >=
		gFrame.FILE_TREE_REFRESH_INTERVAL)
	{
		// File tree refresh doesn't return a value, but we can trigger redraw
		// when it's called since it updates the UI
		gFileTree.refreshFileTree();
		gFrame.setNeedsRedraw(true); // Always redraw after file tree refresh
		gFrame.setFramesToRender(
			std::max(gFrame.framesToRender(), 2)); // Reduced frame count
		gFrame.getTiming().lastFileTreeRefresh = currentTime;
	}
}

void Ned::handleWindowFocus()
{
	bool currentFocus = glfwGetWindowAttrib(window, GLFW_FOCUSED) != 0;
	if (windowFocused != currentFocus)
	{
		gFrame.setNeedsRedraw(true); // Redraw on focus change.
		gFrame.setFramesToRender(
			std::max(gFrame.framesToRender(), 2)); // Reduced frame count
		if (windowFocused && !currentFocus)
		{
			gFileExplorer.saveCurrentFile();
		}
		windowFocused = currentFocus;
	}
}



void Ned::renderMainWindow()
{

#ifdef __APPLE__
	// renderTopLeftMenu();
#endif

	if (gKeybinds.handleKeyboardShortcuts())
	{
		gFrame.setNeedsRedraw(true);
		gFrame.setFramesToRender(std::max(gFrame.framesToRender(), 12));
	}

	if (gTerminal.isTerminalVisible())
	{
		gTerminal.render();
		return;
	}

	if (gFileExplorer.showWelcomeScreen)
	{
		gWelcome.render();
		return;
	}

	ImGui::PushFont(gFont.currentFont);
	ImGui::SetNextWindowPos(ImVec2(0, 0));
	ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);
#ifdef __APPLE__
	ImGui::Begin("Main Window",
				 nullptr,
				 ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
					 ImGuiWindowFlags_NoResize |
					 ImGuiWindowFlags_NoScrollWithMouse | // Prevent window from scrolling
					 ImGuiWindowFlags_NoScrollbar);
#else
	ImGui::Begin("Main Window",
				 nullptr,
				 ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
					 ImGuiWindowFlags_NoResize |
					 ImGuiWindowFlags_NoScrollWithMouse | // Prevent window from scrolling
					 ImGuiWindowFlags_NoScrollbar);

#endif

	float windowWidth = ImGui::GetWindowWidth();
	float padding = ImGui::GetStyle().WindowPadding.x;
	float availableWidth =
		windowWidth - padding * 3 -
		(showAgentPane ? kAgentSplitterWidth : 0.0f); // Only account for splitter width
													  // when agent pane is visible

	if (showSidebar)
	{
		// First split: File Explorer vs. Editor+Agent
		float leftSplit = gSettings.getSplitPos();
		float rightSplit = gSettings.getAgentSplitPos();

		float explorerWidth = availableWidth * leftSplit;
		float agentPaneWidth = availableWidth * rightSplit;
		float editorWidth = availableWidth - explorerWidth -
							(showAgentPane ? agentPaneWidth : 0.0f) - (padding * 2) +
							16.0f;

		// Render File Explorer
		gFileExplorer.renderFileExplorer(explorerWidth);
		ImGui::SameLine(0, 0);

		// Render left splitter
		splitter.renderSplitter(padding, availableWidth);
		ImGui::SameLine(0, 0);

		// Render Editor
		gEditor.renderEditor(gFont.currentFont, editorWidth);
		if (showAgentPane)
		{
			ImGui::SameLine(0, 0);
			// Render right splitter (new)
			splitter.renderAgentSplitter(padding, availableWidth, showSidebar);
			ImGui::SameLine(0, 0);
					// Render Agent Pane (new)
		gAIAgent.render(agentPaneWidth, gFont.largeFont);
		}
	} else
	{
		// No sidebar: just editor and agent pane
		float agentSplit = gSettings.getAgentSplitPos();
		float editorWidth =
			showAgentPane
				? (availableWidth * agentSplit)
				: availableWidth + 5.0f; // Add extra width when agent pane is hidden
		float agentPaneWidth =
			showAgentPane ? (availableWidth - editorWidth - kAgentSplitterWidth) : 0.0f;

		gEditor.renderEditor(gFont.currentFont, editorWidth);
		if (showAgentPane)
		{
			ImGui::SameLine(0, 0);
			splitter.renderAgentSplitter(padding, availableWidth, showSidebar);
					ImGui::SameLine(0, 0);
		gAIAgent.render(agentPaneWidth, gFont.largeFont);
		}
	}
	windowResize.resize();
	ImGui::End();
	ImGui::PopFont();
}

void Ned::renderFrame()
{
	double currentTime = glfwGetTime();
	int display_w, display_h;
	glfwGetFramebufferSize(window, &display_w, &display_h);

	// Setup frame rendering (framebuffer, background, FPS)
	gFrame.setupFrame(
		gSettings, shaderManager.isShaderEnabled(), fb, display_w, display_h, currentTime);

	// Render UI components
	renderMainWindow();
	gBookmarks.renderBookmarksWindow();
	gSettings.renderSettingsWindow();
	gSettings.renderNotification("");
	gKeybinds.checkKeybindsFile();
	windowResize.renderResizeOverlay(gFont.largeFont);

	// Finish frame rendering
	gFrame.finishFrame(fb);

	shaderManager.renderWithEffects(display_w, display_h, glfwGetTime(), fb, accum, quad, gSettings);
	glfwSwapBuffers(window);
}



void Ned::cleanup()
{
	quad.cleanup();

	// Delegate framebuffer cleanup to the shader manager
	shaderManager.cleanupFramebuffers(fb, accum);

	gSettings.saveSettings();
	gFileExplorer.saveCurrentFile();

	// Save AI agent conversation history
	gAIAgent.getHistoryManager().saveConversationHistory();

#ifdef __APPLE__
	// Clean up macOS application delegate
	cleanupMacOSApplicationDelegate();
#endif

	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();
	glfwDestroyWindow(window);
	glfwTerminate();
}
