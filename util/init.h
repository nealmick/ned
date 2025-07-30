/*
File: init.h
Description: Initialization class for handling all initialization logic.
Consolidated from ned.cpp and initialization_manager.cpp
*/

#pragma once

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <csignal>
#include <iostream>

// Forward declarations
class App;
class ShaderManager;
class Render;
class Settings;
class Splitter;
class WindowResize;
class ShaderQuad;
struct FramebufferState;
struct AccumulationBuffers;

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

	// Main initialization method for all components
	static bool initializeAllComponents(App &app,
										ShaderManager &shaderManager,
										Render &render,
										Settings &settings,
										Splitter &splitter,
										WindowResize &windowResize,
										ShaderQuad &quad,
										FramebufferState &fb,
										AccumulationBuffers &accum);

  private:
	// Helper initialization methods
	static bool initializeGraphicsSystem(App &app, ShaderManager &shaderManager);
	// Window manager functionality is now part of App
	static bool initializeComponents(App &app);
	static bool initializeQuad(ShaderQuad &quad);
};