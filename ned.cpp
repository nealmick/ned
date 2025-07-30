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
	if (!app.initialize(shaderManager))
	{
		return false;
	}

	// Initialize all components using Init class
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
		Ned *app = static_cast<Ned *>(glfwGetWindowUserPointer(window));
		app->scrollXAccumulator += x;
		app->scrollYAccumulator += y;
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
	app.runMainLoop(shaderManager,
					render,
					gSettings,
					splitter,
					windowResize,
					quad,
					fb,
					accum,
					needFontReload,
					windowFocused,
					scrollXAccumulator,
					scrollYAccumulator,
					lastOpacity,
					lastBlurEnabled);
}

void Ned::cleanup() { app.cleanupAll(quad, shaderManager, fb, accum); }
