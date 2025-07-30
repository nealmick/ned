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
#include "util/app.h"
#include "util/font.h"
#include "util/render.h"
#include "util/splitter.h"
#include "util/window_resize.h"

struct GLFWwindow;
class ImFont;

class Ned
{
  public:
	Ned();
	~Ned();

	bool initialize();
	void run();
	void cleanup();

  private:
	App app;
	ShaderManager shaderManager;
	FramebufferState fb;
	ShaderQuad quad;
	Splitter splitter;
	Render render;
	bool initialized;

	static void scrollCallback(GLFWwindow *window, double xoffset, double yoffset);

	AccumulationBuffers accum;

	WindowResize windowResize;
};

extern Bookmarks gBookmarks;
