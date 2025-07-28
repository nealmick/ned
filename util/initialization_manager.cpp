/*
File: initialization_manager.cpp
Description: Initialization manager class implementation for handling all initialization
logic. Moved from ned.cpp to better organize the code.
*/

#include "initialization_manager.h"
#include "shaders/shader_types.h"
#include "util/graphics_manager.h"
#include "util/init.h"
#include "util/window_manager.h"
#include <iostream>

InitializationManager::InitializationManager() {}

InitializationManager::~InitializationManager() {}

bool InitializationManager::initializeAll(GraphicsManager &graphicsManager,
										  WindowManager &windowManager,
										  ShaderManager &shaderManager,
										  Render &render,
										  Settings &settings,
										  Splitter &splitter,
										  WindowResize &windowResize,
										  ShaderQuad &quad,
										  FramebufferState &fb,
										  AccumulationBuffers &accum)
{
	// Initialize graphics system
	if (!initializeGraphicsSystem(graphicsManager, shaderManager))
	{
		return false;
	}

	// Initialize window manager
	if (!initializeWindowManager(windowManager, graphicsManager))
	{
		return false;
	}

	// Initialize components
	if (!initializeComponents(graphicsManager))
	{
		return false;
	}

	// Initialize quad
	if (!initializeQuad(quad))
	{
		return false;
	}

	return true;
}

bool InitializationManager::initializeGraphicsSystem(GraphicsManager &graphicsManager,
													 ShaderManager &shaderManager)
{
	// Initialize graphics system
	if (!graphicsManager.initialize(shaderManager))
	{
		return false;
	}
	return true;
}

bool InitializationManager::initializeWindowManager(WindowManager &windowManager,
													GraphicsManager &graphicsManager)
{
	// Get window from graphics manager
	GLFWwindow *window = graphicsManager.getWindow();

	// Initialize window manager
	windowManager.initialize(window);

	return true;
}

bool InitializationManager::initializeComponents(GraphicsManager &graphicsManager)
{
	// Get window from graphics manager
	GLFWwindow *window = graphicsManager.getWindow();

	// Initialize all settings, UI, macOS, and graphics components
	Init::initializeAll(window);

	return true;
}

bool InitializationManager::initializeQuad(ShaderQuad &quad)
{
	// Initialize quad
	quad.initialize();
	return true;
}