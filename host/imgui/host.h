/*
	File: host.h
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

class AppHost
{
  public:
	AppHost();
	~AppHost();

	bool initialize();
	void run();
	void cleanup();

	// Shell-owned settings (Qt AppHost parity): profile state, font facility
	// and the ImGui settings window view. Workbench consumes them.
	Settings settings;
	Font font;
	SettingsView settingsView{settings, font};
	Workbench workbench{settings, font, settingsView};
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
	void applySettings(); // needsApply poll + profile re-apply (Qt applied() parity)
	void renderFrame();

	static void scrollCallback(GLFWwindow *window, double xoffset, double yoffset);
};
