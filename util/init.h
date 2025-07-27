/*
File: init.h
Description: Initialization class moved from ned.cpp
*/

#pragma once

#include <GLFW/glfw3.h>
#include <csignal>
#include <iostream>

class Init
{
  public:
	// Core ImGui initialization
	static void initializeImGui(GLFWwindow *window);

	// Resource initialization
	static void initializeResources();

	// Signal handler setup for crash detection
	static void setupSignalHandlers();

	// Settings and configuration initialization
	static void initializeSettings();

	// macOS-specific initialization
	static void initializeMacOS(GLFWwindow *window);

	// Graphics and rendering initialization
	static bool initializeGraphics(GLFWwindow *window);

	// UI settings initialization
	static void initializeUISettings();

	// Consolidated initialization - calls all the above methods
	static void initializeAll(GLFWwindow *window);
};