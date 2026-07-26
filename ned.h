/*
	File: ned.h
	Description: Thin standalone host — OS window, main loop, shaders + Workbench.
*/

#pragma once

#ifndef NED_ENABLE_SHADERS
#define NED_ENABLE_SHADERS 1
#endif

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "workbench.h"
#if NED_ENABLE_SHADERS
#include "shaders/shader_manager.h"
#include "shaders/shader_types.h"
#endif

class Ned
{
  public:
	Ned();
	~Ned();

	bool initialize();
	void run();
	void cleanup();

	Workbench workbench;
#if NED_ENABLE_SHADERS
	FramebufferState fb;
	ShaderQuad quad;
	AccumulationBuffers accum;
	ShaderManager shaderManager;
#endif

  private:
	bool initialized_ = false;
	bool windowFocused_ = true;
	GLFWwindow *window_ = nullptr;
	double scrollXAccumulator_ = 0.0;
	double scrollYAccumulator_ = 0.0;

	bool createWindow();
	void setWindowIcon();
	bool initializeGLEW();
	bool initializeImGui();
	void handleScrollAccumulators();
	void renderFrame();

	static void scrollCallback(GLFWwindow *window, double xoffset, double yoffset);
};
