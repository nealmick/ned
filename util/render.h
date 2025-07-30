/*
File: render.h
Description: Render and frame management class for handling UI rendering logic in NED text
editor.
*/

#pragma once

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <chrono>
#include <functional>
#include <string>

#include "shaders/shader_manager.h"
#include "shaders/shader_types.h"
#include "util/font.h"
// Frame functionality merged into Render class
#include "util/splitter.h"
#include "util/window_resize.h"

// Forward declarations
struct GLFWwindow;
class ImFont;
class Settings;
struct FramebufferState;
class App;

// Timing state structure
struct TimingState
{
	int frameCount = 0;
	double lastFPSTime = 0.0;
	double lastSettingsCheck = 0.0;
	double lastFileTreeRefresh = 0.0;
};

// Render class for handling UI rendering logic and frame management
class Render
{
  public:
	Render();
	~Render();

	// Initialize render components
	bool initialize(GLFWwindow *window);

	// Initialize frame management
	void initializeFrameManagement();

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

	// Frame management methods (merged from Frame)
	void updateFPS(double currentTime);
	void handleFrameTiming(std::chrono::high_resolution_clock::time_point frame_start,
						   bool shaderEnabled,
						   bool windowFocused,
						   Settings &settings);
	void setupImGuiFrame();
	void setupFrame(Settings &settings,
					bool shaderEnabled,
					FramebufferState &fb,
					int display_w,
					int display_h,
					double currentTime);
	void finishFrame(FramebufferState &fb);
	void renderFPSCounter(Settings &settings);
	void checkForActivity();
	void handleBackgroundUpdates(double currentTime);
	void handleFrameSetup(double currentTime,
						  bool &needFontReload,
						  bool &m_needsRedraw,
						  int &m_framesToRender,
						  std::function<void(bool)> setShaderEnabled,
						  float &lastOpacity,
						  bool &lastBlurEnabled,
						  bool &windowFocused,
						  App &app);

	// Frame state accessors
	double getCurrentFPS() const { return currentFPS; }
	int getFrameCount() const { return frames; }
	bool needsRedraw() const { return m_needsRedraw; }
	int framesToRender() const { return m_framesToRender; }
	double lastActivityTime() const { return m_lastActivityTime; }
	bool &needsRedrawRef() { return m_needsRedraw; }
	int &framesToRenderRef() { return m_framesToRender; }
	void setNeedsRedraw(bool needs) { m_needsRedraw = needs; }
	void setFramesToRender(int frames) { m_framesToRender = frames; }
	void setLastActivityTime(double time) { m_lastActivityTime = time; }
	TimingState &getTiming() { return timing; }

	// Constants
	static constexpr double FPS_UPDATE_INTERVAL = 1.0;
	static constexpr float DEFAULT_FPS_TARGET = 60.0f;
	static constexpr float UNFOCUSED_FPS_TARGET = 15.0f;
	static constexpr float MIN_FPS_TARGET = 0.0f;
	static constexpr float MAX_FPS_TARGET = 10000.0f;
	static constexpr double SETTINGS_CHECK_INTERVAL = 2.0;
	static constexpr double FILE_TREE_REFRESH_INTERVAL = 2.0;

  private:
	// Helper functions for render logic
	void setupFrameRendering(Settings &gSettings,
							 bool shaderEnabled,
							 FramebufferState &fb,
							 int display_w,
							 int display_h,
							 double currentTime);

	void finishFrameRendering(FramebufferState &fb);

	// FPS tracking variables (merged from Frame)
	double lastTime;
	int frames;
	double currentFPS;

	// Redraw management variables (merged from Frame)
	bool m_needsRedraw = true;		 // Start true to draw the first frame
	int m_framesToRender = 0;		 // Number of frames to render for smooth interactions
	double m_lastActivityTime = 0.0; // Track when we last had activity

	// Timing state (merged from Frame)
	TimingState timing;
};