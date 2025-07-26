/*
File: render.h
Description: Render class for handling UI rendering logic in NED text editor.
*/

#pragma once

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "shaders/shader_manager.h"
#include "shaders/shader_types.h"
#include "util/font.h"
#include "util/frame.h"
#include "util/splitter.h"
#include "util/window_resize.h"

// Forward declarations
struct GLFWwindow;
class ImFont;

// Render class for handling UI rendering logic
class Render
{
  public:
	Render();
	~Render();

	// Initialize render components
	bool initialize(GLFWwindow *window);

	// Main render functions
	void renderFrame(GLFWwindow *window,
					 ShaderManager &shaderManager,
					 FramebufferState &fb,
					 AccumulationBuffers &accum,
					 ShaderQuad &quad,
					 Settings &gSettings,
					 Splitter &splitter,
					 WindowResize &windowResize,
					 double currentTime);

	void
	renderMainWindow(GLFWwindow *window, Splitter &splitter, WindowResize &windowResize);

  private:
	// Helper functions for render logic
	void setupFrameRendering(Settings &gSettings,
							 bool shaderEnabled,
							 FramebufferState &fb,
							 int display_w,
							 int display_h,
							 double currentTime);

	void finishFrameRendering(FramebufferState &fb);
};