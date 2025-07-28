/*
File: application_manager.h
Description: Application manager class for handling main application loop logic.
Moved from ned.cpp to better organize the code.
*/

#pragma once

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <functional>

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

class ApplicationManager
{
  public:
	ApplicationManager();
	~ApplicationManager();

	// Initialize application manager
	bool initialize(GraphicsManager &graphicsManager,
					WindowManager &windowManager,
					ShaderManager &shaderManager,
					Render &render,
					Settings &settings,
					Splitter &splitter,
					WindowResize &windowResize,
					ShaderQuad &quad,
					FramebufferState &fb,
					AccumulationBuffers &accum);

	// Main application loop
	void runMainLoop(GraphicsManager &graphicsManager,
					 WindowManager &windowManager,
					 ShaderManager &shaderManager,
					 Render &render,
					 Settings &settings,
					 Splitter &splitter,
					 WindowResize &windowResize,
					 ShaderQuad &quad,
					 FramebufferState &fb,
					 AccumulationBuffers &accum,
					 bool &needFontReload,
					 bool &windowFocused,
					 double &scrollXAccumulator,
					 double &scrollYAccumulator,
					 float &lastOpacity,
					 bool &lastBlurEnabled);

	// Set scroll callback
	void setScrollCallback(GLFWscrollfun callback);

	// Cleanup application resources
	void cleanup();

  private:
	// Helper methods for main loop
	void handleEventPolling(GraphicsManager &graphicsManager,
							WindowManager &windowManager,
							ShaderManager &shaderManager,
							double currentTime);

	void handleWindowManagement(WindowManager &windowManager, GLFWwindow *window);

	void handleScrollAccumulators(double &scrollXAccumulator, double &scrollYAccumulator);

	void handleFramebufferSetup(GraphicsManager &graphicsManager,
								ShaderManager &shaderManager,
								FramebufferState &fb,
								AccumulationBuffers &accum);

	void handleFrameRendering(Render &render,
							  GLFWwindow *window,
							  ShaderManager &shaderManager,
							  FramebufferState &fb,
							  AccumulationBuffers &accum,
							  ShaderQuad &quad,
							  Settings &settings,
							  Splitter &splitter,
							  WindowResize &windowResize,
							  double currentTime);

	void handleFontReload(bool &needFontReload);

	// Scroll callback function
	GLFWscrollfun scrollCallback;
};