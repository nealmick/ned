/*
File: window_manager.h
Description: Window management for NED text editor.
Handles window focus, macOS-specific window management, and window state.
*/

#pragma once

#include <GLFW/glfw3.h>
#include <iostream>

// Forward declarations
class Settings;
class FileExplorer;

class WindowManager
{
  public:
	WindowManager();
	~WindowManager();

	// Initialize window management
	void initialize(GLFWwindow *window);

	// Handle window focus changes
	void handleWindowFocus(bool &windowFocused, FileExplorer &fileExplorer);

	// Check for macOS termination
	bool shouldTerminateApplication() const;

	// Setup macOS application delegate
	void setupMacOSApplicationDelegate();

	// Configure macOS window
	void configureMacOSWindow(GLFWwindow *window, float opacity, bool blurEnabled);

	// Cleanup macOS application delegate
	void cleanupMacOSApplicationDelegate();

	// Get current window focus state
	bool isWindowFocused(GLFWwindow *window) const;

	// Handle settings changes for window
	void
	handleSettingsChanges(Settings &settings, float &lastOpacity, bool &lastBlurEnabled);

  private:
	GLFWwindow *window;
	bool lastFocusState;

#ifdef __APPLE__
	float lastOpacity;
	bool lastBlurEnabled;
#endif
};