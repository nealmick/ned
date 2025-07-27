/*
File: frame.h
Description: Frame management class for NED text editor.
Handles FPS tracking, frame timing, and FPS counter rendering.
*/

#pragma once

#include <GL/glew.h>
#include <chrono>
#include <string>

// Forward declarations
struct ImDrawList;
struct ImVec2;
class Settings;
struct FramebufferState;
class WindowManager;

// Timing state structure
struct TimingState
{
	int frameCount = 0;
	double lastFPSTime = 0.0;
	double lastSettingsCheck = 0.0;
	double lastFileTreeRefresh = 0.0;
};

class Frame
{
  public:
	Frame();
	~Frame();

	// Initialize frame management
	void initialize();

	// Update FPS calculation
	void updateFPS(double currentTime);

	// Handle frame timing and sleep
	void handleFrameTiming(std::chrono::high_resolution_clock::time_point frame_start,
						   bool shaderEnabled,
						   bool windowFocused,
						   Settings &settings);

	// Setup ImGui frame
	void setupImGuiFrame();

	// Setup frame rendering (framebuffer, background, FPS)
	void setupFrame(Settings &settings,
					bool shaderEnabled,
					FramebufferState &fb,
					int display_w,
					int display_h,
					double currentTime);

	// Finish frame rendering
	void finishFrame(FramebufferState &fb);

	// Render FPS counter overlay
	void renderFPSCounter(Settings &settings);

	// Check for activity and update activity time
	void checkForActivity();

	// Handle background updates (settings, file tree, etc.)
	void handleBackgroundUpdates(double currentTime);

	// Handle complete frame setup (background updates, settings changes, framebuffer setup)
	void handleFrameSetup(double currentTime,
						  bool &needFontReload,
						  bool &m_needsRedraw,
						  int &m_framesToRender,
						  std::function<void(bool)> setShaderEnabled,
						  float &lastOpacity,
						  bool &lastBlurEnabled,
						  bool &windowFocused,
						  WindowManager &windowManager);

	// Get current FPS
	double getCurrentFPS() const { return currentFPS; }

	// Get frame count
	int getFrameCount() const { return frames; }

	// Get redraw flags
	bool needsRedraw() const { return m_needsRedraw; }
	int framesToRender() const { return m_framesToRender; }
	double lastActivityTime() const { return m_lastActivityTime; }

	// Get redraw flags as references (for compatibility with existing interfaces)
	bool &needsRedrawRef() { return m_needsRedraw; }
	int &framesToRenderRef() { return m_framesToRender; }

	// Set redraw flags
	void setNeedsRedraw(bool needs) { m_needsRedraw = needs; }
	void setFramesToRender(int frames) { m_framesToRender = frames; }
	void setLastActivityTime(double time) { m_lastActivityTime = time; }

	// Timing state accessors
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
	// FPS tracking variables
	double lastTime;
	int frames;
	double currentFPS;

	// Redraw management variables
	bool m_needsRedraw = true;		 // Start true to draw the first frame
	int m_framesToRender = 0;		 // Number of frames to render for smooth interactions
	double m_lastActivityTime = 0.0; // Track when we last had activity

	// Timing state
	TimingState timing;
};

// Global frame instance
extern Frame gFrame;