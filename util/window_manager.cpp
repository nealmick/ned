/*
File: window_manager.cpp
Description: Window management implementation for NED text editor.
*/

#include "window_manager.h"
#include "../files/files.h"
#include "settings.h"

#ifdef __APPLE__
#include "../macos_window.h"
#endif

WindowManager::WindowManager()
	: window(nullptr), lastFocusState(false)
#ifdef __APPLE__
	  ,
	  lastOpacity(0.5f), lastBlurEnabled(false)
#endif
{
}

WindowManager::~WindowManager()
{
#ifdef __APPLE__
	cleanupMacOSApplicationDelegate();
#endif
}

void WindowManager::initialize(GLFWwindow *window)
{
	this->window = window;
	lastFocusState = isWindowFocused(window);
}

void WindowManager::handleWindowFocus(bool &windowFocused, FileExplorer &fileExplorer)
{
	bool currentFocus = isWindowFocused(window);
	if (windowFocused != currentFocus)
	{
		// Redraw on focus change
		if (windowFocused && !currentFocus)
		{
			fileExplorer.saveCurrentFile();
		}
		windowFocused = currentFocus;
	}
}

bool WindowManager::shouldTerminateApplication() const
{
#ifdef __APPLE__
	return ::shouldTerminateApplication();
#else
	return false;
#endif
}

void WindowManager::setupMacOSApplicationDelegate()
{
#ifdef __APPLE__
	::setupMacOSApplicationDelegate();
#endif
}

void WindowManager::configureMacOSWindow(GLFWwindow *window,
										 float opacity,
										 bool blurEnabled)
{
#ifdef __APPLE__
	::configureMacOSWindow(window, opacity, blurEnabled);
#endif
}

void WindowManager::cleanupMacOSApplicationDelegate()
{
#ifdef __APPLE__
	::cleanupMacOSApplicationDelegate();
#endif
}

bool WindowManager::isWindowFocused(GLFWwindow *window) const
{
	return window ? glfwGetWindowAttrib(window, GLFW_FOCUSED) != 0 : false;
}

void WindowManager::handleSettingsChanges(Settings &settings,
										  float &lastOpacity,
										  bool &lastBlurEnabled)
{
#ifdef __APPLE__
	float opacity = settings.getSettings().value("mac_background_opacity", 0.5f);
	bool blurEnabled = settings.getSettings().value("mac_blur_enabled", true);

	if (opacity != lastOpacity || blurEnabled != lastBlurEnabled)
	{
		configureMacOSWindow(window, opacity, blurEnabled);
		lastOpacity = opacity;
		lastBlurEnabled = blurEnabled;
	}
#endif
}