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
#include "util/splitter.h"
#include "util/terminal.h"
#include "util/welcome.h"

#include <cstdio>
#include <filesystem>
#include <iostream>
#include <thread>

#include "ai/ai_agent.h"

// global scope
Bookmarks gBookmarks;

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

	// Initialize graphics system
	if (!graphicsManager.initialize(shaderManager))
	{
		return false;
	}

	// Get window from graphics manager
	GLFWwindow *window = graphicsManager.getWindow();

	// Initialize window manager
	windowManager.initialize(window);

	// Initialize all settings, UI, macOS, and graphics components
	Init::initializeAll(window);

	// Set up window user pointer
	graphicsManager.setWindowUserPointer(this);

	// Set up scroll callback AFTER ImGui is initialized
	graphicsManager.setScrollCallback(Ned::scrollCallback);

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
	app->scrollXAccumulator += xoffset * 0.2; // Same multiplier as vertical
	app->scrollYAccumulator += yoffset * 0.2;
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
}
void Ned::cleanup()
{
	quad.cleanup();
	shaderManager.cleanupFramebuffers(fb, accum);
	gSettings.saveSettings();
	gFileExplorer.saveCurrentFile();
	gAIAgent.getHistoryManager().saveConversationHistory();
	graphicsManager.cleanup();
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();
}
