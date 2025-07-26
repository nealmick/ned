/*
	File: ned.h
	Description: Main application class for NED text editor.
	This encapsulates the core application logic, initialization, and render
   loop.
*/

#pragma once

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <chrono>
#include <string>

#include "ai/ai_agent.h"
#include "editor/editor.h"
#include "editor/editor_bookmarks.h"
#include "files/file_tree.h"
#include "files/files.h"
#include "shaders/shader_manager.h"
#include "shaders/shader_types.h"
#include "util/font.h"
#include "util/frame.h"
#include "util/splitter.h"
#include "util/window_resize.h"

// Forward declarations
struct GLFWwindow;
class ImFont;

// main application class
class Ned
{
  public:
	Ned();
	~Ned();

	bool initialize();
	void run();
	void cleanup();

  private:
	// Member variables
	GLFWwindow *window;
	ShaderManager shaderManager;
	FramebufferState fb;
	ShaderQuad quad;
	Splitter splitter;
	bool needFontReload;
	bool windowFocused;
	float explorerWidth;
	float editorWidth;
	bool initialized;

	// Core functions
	bool initializeGraphics();
	void handleBackgroundUpdates(double currentTime);
	void handleWindowFocus();

	void checkForActivity(); // Check for immediate user input

	// Render functions
	void renderFrame();
	void renderMainWindow();

	// slow scrolling momentum accumulator
	static void scrollCallback(GLFWwindow *window, double xoffset, double yoffset);
	double scrollXAccumulator = 0.0;
	double scrollYAccumulator = 0.0;

	// burn in accumulation buffer
	AccumulationBuffers accum;

	// Window resize handling
	WindowResize windowResize;

	float lastOpacity = 0.5f;
	bool lastBlurEnabled = false;

	// Removed gAIAgent member variable - using global instance instead
};

// Global scope
extern Bookmarks gBookmarks;
extern bool showSidebar;
extern bool showAgentPane;
