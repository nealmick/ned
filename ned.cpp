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
#include "lsp/lsp_client.h"
#include "util/debug_console.h"
#include "util/init.h"
#include "util/keybinds.h"
#include "util/render.h"
#include "util/scroll.h"
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
// Note: gBookmarks and gAIAgent are now defined in globals.cpp

constexpr float kAgentSplitterWidth = 6.0f;

Ned::Ned() : initialized(false) {}

Ned::~Ned()
{
	if (initialized)
	{
		cleanup();
	}
}

bool Ned::initialize()
{
	// Initialize all components using Init class (this will call app.initialize internally)
	if (!Init::initializeAllComponents(
			app, shaderManager, render, gSettings, splitter, windowResize, quad, fb, accum))
	{
		return false;
	}

	// Initialize application manager
	if (!app.initializeApp(
			shaderManager, render, gSettings, splitter, windowResize, quad, fb, accum))
	{
		return false;
	}

	// Set up window user pointer
	app.setWindowUserPointer(this);

	// Set up scroll callback
	app.setAppScrollCallback(Ned::scrollCallback);

	initialized = true;
	return true;
}

void Ned::scrollCallback(GLFWwindow *window, double xoffset, double yoffset)
{
	Scroll::scrollCallback(window, xoffset, yoffset, [window](double x, double y) {
		App *app = static_cast<App *>(glfwGetWindowUserPointer(window));
		app->setScrollXAccumulator(app->getScrollXAccumulator() + x);
		app->setScrollYAccumulator(app->getScrollYAccumulator() + y);
	});
}

void Ned::run()
{
	if (!initialized)
	{
		std::cerr << "Cannot run: Not initialized" << std::endl;
		return;
	}

	// Run the main application loop using App
	bool needFontReload = gFont.getNeedFontReload();
	float lastOpacity = app.getLastOpacity();
	bool lastBlurEnabled = app.getLastBlurEnabled();
	app.runMainLoop(shaderManager,
					render,
					gSettings,
					splitter,
					windowResize,
					quad,
					fb,
					accum,
					needFontReload,
					lastOpacity,
					lastBlurEnabled);
}

void Ned::cleanup()
{
	// Shutdown LSP first to avoid hanging
	std::cout << "App: Shutting down LSP client..." << std::endl;
	gLSPClient.shutdown();

	// Then cleanup other components
	app.cleanupAll(quad, shaderManager, fb, accum);
}
