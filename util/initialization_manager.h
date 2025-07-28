/*
File: initialization_manager.h
Description: Initialization manager class for handling all initialization logic.
Moved from ned.cpp to better organize the code.
*/

#pragma once

#include <GL/glew.h>
#include <GLFW/glfw3.h>

// Forward declarations
class GraphicsManager;
class WindowManager;
class ShaderManager;
class Render;
class Settings;
class Splitter;
class WindowResize;
class ShaderQuad;
struct FramebufferState;
struct AccumulationBuffers;

class InitializationManager
{
  public:
	InitializationManager();
	~InitializationManager();

	// Main initialization method
	bool initializeAll(GraphicsManager &graphicsManager,
					   WindowManager &windowManager,
					   ShaderManager &shaderManager,
					   Render &render,
					   Settings &settings,
					   Splitter &splitter,
					   WindowResize &windowResize,
					   ShaderQuad &quad,
					   FramebufferState &fb,
					   AccumulationBuffers &accum);

  private:
	// Helper initialization methods
	bool initializeGraphicsSystem(GraphicsManager &graphicsManager,
								  ShaderManager &shaderManager);
	bool initializeWindowManager(WindowManager &windowManager,
								 GraphicsManager &graphicsManager);
	bool initializeComponents(GraphicsManager &graphicsManager);
	bool initializeQuad(ShaderQuad &quad);
};