/*
File: window_resize.h
Description: Window resize handling class for NED text editor.
This class handles manual window resizing functionality including resize handles,
drag detection, and cursor management.
*/

#pragma once

#include <GLFW/glfw3.h>
#include <imgui.h>

// Forward declarations
struct GLFWwindow;

class WindowResize
{
  public:
	WindowResize();
	~WindowResize();

	// Initialize the resize handler with the window
	void initialize(GLFWwindow *window);

	// Main functions called from the main application
	void renderResizeHandles();
	void handleManualResizing();
	void resize(); // Combines both resize functions
	void renderResizeOverlay(ImFont *font = nullptr);

	// Get current resize state
	bool isResizing() const { return resizingRight || resizingBottom || resizingCorner; }

  private:
	// Window reference
	GLFWwindow *window;

	// Resize state
	bool resizingRight = false;
	bool resizingBottom = false;
	bool resizingCorner = false;
	ImVec2 dragStart;
	ImVec2 initialWindowSize;

	// Resize border size
	static constexpr float RESIZE_BORDER = 10.0f;

	// Resize overlay state
	int lastWidth = 0;
	int lastHeight = 0;
	double startTime = 0.0;

	// Utility functions
	static float clamp(float value, float min, float max);
};