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
#include "util/init.h"
#include "util/keybinds.h"
#include "util/render.h"
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
	: needFontReload(false), windowFocused(true), explorerWidth(0.0f), editorWidth(0.0f),
	  initialized(false)
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
	// Set up signal handlers to catch crashes and prevent crash reports
	signal(SIGABRT, [](int signal) {
		std::cerr << "Crash detected! Signal: " << signal << std::endl;
		exit(1);
	});
	signal(SIGSEGV, [](int signal) {
		std::cerr << "Crash detected! Signal: " << signal << std::endl;
		exit(1);
	});
	signal(SIGILL, [](int signal) {
		std::cerr << "Crash detected! Signal: " << signal << std::endl;
		exit(1);
	});
	signal(SIGFPE, [](int signal) {
		std::cerr << "Crash detected! Signal: " << signal << std::endl;
		exit(1);
	});

	// Initialize graphics system
	if (!graphicsManager.initialize(shaderManager))
	{
		return false;
	}

	// Get window from graphics manager
	GLFWwindow *window = graphicsManager.getWindow();

	// Initialize window manager
	windowManager.initialize(window);

	// Load settings and keybinds
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
	windowManager.setupMacOSApplicationDelegate();

	// Initial configuration
	windowManager.configureMacOSWindow(window, opacity, blurEnabled);

	// Initialize tracking variables
	lastOpacity = opacity;
	lastBlurEnabled = blurEnabled;
#endif

	// Rest of initialization...
	graphicsManager.setWindowUserPointer(this);
	Init::initializeImGui(window);
	graphicsManager.setScrollCallback(Ned::scrollCallback);
	Init::initializeResources();
	quad.initialize();

	// Initialize window resize handler
	windowResize.initialize(window);

	// Initialize render class
	if (!render.initialize(window))
	{
		std::cerr << "Failed to initialize render class" << std::endl;
		return false;
	}

	// Initialize frame management
	gFrame.initialize();

	initialized = true;
	return true;
}

void Ned::handleScrollAccumulators()
{
	// Handle scroll accumulators (this was removed but needed for scrolling)
	if (scrollXAccumulator != 0.0 || scrollYAccumulator != 0.0)
	{
		ImGui::GetIO().MouseWheel += scrollYAccumulator;  // Vertical
		ImGui::GetIO().MouseWheelH += scrollXAccumulator; // Horizontal
		scrollXAccumulator = 0.0;
		scrollYAccumulator = 0.0;
	}
}

void Ned::scrollCallback(GLFWwindow *window, double xoffset, double yoffset)
{
	Ned *app = static_cast<Ned *>(glfwGetWindowUserPointer(window));
	app->scrollXAccumulator += xoffset * 0.3; // Same multiplier as vertical
	app->scrollYAccumulator += yoffset * 0.3;
}

void Ned::run()
{
	if (!initialized)
	{
		std::cerr << "Cannot run: Not initialized" << std::endl;
		return;
	}

	GLFWwindow *window = graphicsManager.getWindow();

	while (!graphicsManager.shouldWindowClose())
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
		if (windowManager.shouldTerminateApplication())
		{
			glfwSetWindowShouldClose(window, 1);
		}

		// Handle scroll accumulators
		handleScrollAccumulators();

		// Check for activity and decide if we should keep rendering
		gFrame.checkForActivity();
		bool shouldKeepRendering = (currentTime - gFrame.lastActivityTime()) < 0.5 ||
								   gEditorScroll.isScrollAnimationActive();

		// Always render a frame after polling events
		auto frame_start = std::chrono::high_resolution_clock::now();

		gFrame.handleBackgroundUpdates(currentTime);
		gSettings.handleSettingsChanges(
			needFontReload,
			gFrame.needsRedrawRef(),
			gFrame.framesToRenderRef(),
			[this](bool enabled) { shaderManager.setShaderEnabled(enabled); },
			lastOpacity,
			lastBlurEnabled);

		int display_w, display_h;
		graphicsManager.getFramebufferSize(&display_w, &display_h);
		// Delegate framebuffer initialization to the shader manager
		shaderManager.initializeFramebuffers(display_w, display_h, fb, accum);

		gFrame.setupImGuiFrame();
		windowManager.handleWindowFocus(windowFocused, gFileExplorer);
		if (gFileExplorer.handleFileDialog())
		{
			gFrame.setNeedsRedraw(true);
			gFrame.setFramesToRender(std::max(gFrame.framesToRender(), 3));
		}
		render.renderFrame(window,
						   shaderManager,
						   fb,
						   accum,
						   quad,
						   gSettings,
						   splitter,
						   windowResize,
						   currentTime);

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

	// Comment out cleanup to prevent crash during Cmd+Q termination
	// cleanup();
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

	// Clean up graphics and window managers
	graphicsManager.cleanup();

	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();
}
