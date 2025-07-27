/*
File: graphics_manager.h
Description: Graphics initialization and management for NED text editor.
Handles GLFW, GLEW, and OpenGL setup.
*/

#pragma once

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <iostream>

// Forward declarations
class ShaderManager;

class GraphicsManager
{
  public:
	GraphicsManager();
	~GraphicsManager();

	// Initialize graphics system
	bool initialize(ShaderManager &shaderManager);

	// Get the created window
	GLFWwindow *getWindow() const { return window; }

	// Set scroll callback
	void setScrollCallback(GLFWscrollfun callback);

	// Enable raw mouse motion
	void enableRawMouseMotion();

	// Set window user pointer
	void setWindowUserPointer(void *pointer);

	// Make context current
	void makeContextCurrent();

	// Set swap interval
	void setSwapInterval(int interval);

	// Set window refresh callback
	void setWindowRefreshCallback(GLFWwindowrefreshfun callback);

	// Check if window should close
	bool shouldWindowClose() const;

	// Get framebuffer size
	void getFramebufferSize(int *width, int *height);

	// Get window focus state
	bool isWindowFocused() const;

	// Event polling methods
	void pollEvents(double currentTime, bool shaderEnabled, double lastActivityTime);
	double calculateEventTimeout(double currentTime,
								 bool shaderEnabled,
								 double lastActivityTime);

	// Cleanup graphics resources
	void cleanup();

  private:
	GLFWwindow *window;

	// Initialize GLFW
	bool initializeGLFW();

	// Create window
	bool createWindow();

	// Initialize GLEW
	bool initializeGLEW();

	// Initialize OpenGL state
	void initializeOpenGLState();

	// Initialize shader manager
	bool initializeShaderManager();

	// Set up window hints
	void setupWindowHints();
};