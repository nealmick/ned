/*
File: app.h
Description: Combined application and graphics management class for NED text editor.
This class combines the functionality of ApplicationManager and GraphicsManager.
*/

#pragma once

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <functional>

// Forward declarations
class ShaderManager;
class Render;
class Settings;
class Splitter;
class WindowResize;
class ShaderQuad;
class FileExplorer;
struct FramebufferState;
struct AccumulationBuffers;

class App
{
  public:
	App();
	~App();

	// Initialize graphics system
	bool initialize(ShaderManager &shaderManager);

	// Initialize application manager
	bool initializeApp(ShaderManager &shaderManager,
					   Render &render,
					   Settings &settings,
					   Splitter &splitter,
					   WindowResize &windowResize,
					   ShaderQuad &quad,
					   FramebufferState &fb,
					   AccumulationBuffers &accum);

	// Main application loop
	void runMainLoop(ShaderManager &shaderManager,
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

	// Graphics manager methods
	GLFWwindow *getWindow() const { return window; }
	void setScrollCallback(GLFWscrollfun callback);
	void setAppScrollCallback(GLFWscrollfun callback);
	void enableRawMouseMotion();
	void setWindowUserPointer(void *pointer);
	void makeContextCurrent();
	void setSwapInterval(int interval);
	void setWindowRefreshCallback(GLFWwindowrefreshfun callback);
	bool shouldWindowClose() const;
	void getFramebufferSize(int *width, int *height);
	bool isWindowFocused() const;
	void pollEvents(double currentTime, bool shaderEnabled, double lastActivityTime);
	double calculateEventTimeout(double currentTime,
								 bool shaderEnabled,
								 double lastActivityTime);

	// Window management methods
	void initializeWindowManagement(GLFWwindow *window);
	void handleWindowFocus(bool &windowFocused, FileExplorer &fileExplorer);
	bool shouldTerminateApplication() const;
	void setupMacOSApplicationDelegate();
	void configureMacOSWindow(GLFWwindow *window, float opacity, bool blurEnabled);
	void cleanupMacOSApplicationDelegate();
	bool isWindowFocused(GLFWwindow *window) const;
	void
	handleSettingsChanges(Settings &settings, float &lastOpacity, bool &lastBlurEnabled);

	// Cleanup resources
	void cleanup();

  private:
	// Graphics manager members
	GLFWwindow *window;
	bool lastFocusState;

#ifdef __APPLE__
	float lastOpacity;
	bool lastBlurEnabled;
#endif

	// Application manager members
	GLFWscrollfun scrollCallback;

	// Graphics initialization methods
	bool initializeGLFW();
	bool createWindow();
	bool initializeGLEW();
	void initializeOpenGLState();
	void setupWindowHints();

	// Application loop helper methods
	void handleEventPolling(ShaderManager &shaderManager, double currentTime);
	void handleWindowManagement(GLFWwindow *window);
	void handleScrollAccumulators(double &scrollXAccumulator, double &scrollYAccumulator);
	void handleFramebufferSetup(ShaderManager &shaderManager,
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
};