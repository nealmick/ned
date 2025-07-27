/*
File: graphics_manager.cpp
Description: Graphics initialization and management implementation for NED text editor.
*/

#include "graphics_manager.h"
#include "shaders/shader_manager.h"
#include <iostream>

GraphicsManager::GraphicsManager() : window(nullptr) {}

GraphicsManager::~GraphicsManager() { cleanup(); }

bool GraphicsManager::initialize(ShaderManager &shaderManager)
{
	if (!initializeGLFW())
	{
		return false;
	}

	if (!createWindow())
	{
		return false;
	}

	if (!initializeGLEW())
	{
		return false;
	}

	initializeOpenGLState();

	if (!shaderManager.initializeShaders())
	{
		std::cerr << "🔴 Shader initialization failed" << std::endl;
		glfwTerminate();
		return false;
	}

	return true;
}

bool GraphicsManager::initializeGLFW()
{
	if (!glfwInit())
	{
		std::cerr << "Failed to initialize GLFW" << std::endl;
		return false;
	}
	return true;
}

void GraphicsManager::setupWindowHints()
{
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
	glfwWindowHint(GLFW_DECORATED,
				   GLFW_FALSE); // No OS decorations for macOS (custom handling)
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // macOS specific
	glfwWindowHint(GLFW_TRANSPARENT_FRAMEBUFFER, GLFW_TRUE);
#else // For Linux/Ubuntu and other non-Apple platforms
	glfwWindowHint(GLFW_DECORATED, GLFW_TRUE); // Use OS decorations
#endif
	// GLFW_RESIZABLE should be TRUE for both.
	// On Linux, the OS window manager handles resizing if DECORATED is TRUE.
	// On macOS, your custom logic handles resizing.
	glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
}

bool GraphicsManager::createWindow()
{
	setupWindowHints();
	window = glfwCreateWindow(1200, 750, "NED", NULL, NULL);

	if (!window)
	{
		std::cerr << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		return false;
	}

	return true;
}

bool GraphicsManager::initializeGLEW()
{
	glfwMakeContextCurrent(window);
	glfwSwapInterval(0);
	glfwSetWindowRefreshCallback(window, [](GLFWwindow *window) { glfwPostEmptyEvent(); });

	// Enable raw mouse motion for more accurate tracking
	glfwSetInputMode(window, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);

	glewExperimental = GL_TRUE;
	if (GLenum err = glewInit(); GLEW_OK != err)
	{
		std::cerr << "🔴 GLEW initialization failed: " << glewGetErrorString(err)
				  << std::endl;
		glfwTerminate();
		return false;
	}

	return true;
}

void GraphicsManager::initializeOpenGLState()
{
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glGetError(); // Clear any GLEW startup errors
}

void GraphicsManager::setScrollCallback(GLFWscrollfun callback)
{
	if (window)
	{
		glfwSetScrollCallback(window, callback);
	}
}

void GraphicsManager::enableRawMouseMotion()
{
	if (window)
	{
		glfwSetInputMode(window, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
	}
}

void GraphicsManager::setWindowUserPointer(void *pointer)
{
	if (window)
	{
		glfwSetWindowUserPointer(window, pointer);
	}
}

void GraphicsManager::makeContextCurrent()
{
	if (window)
	{
		glfwMakeContextCurrent(window);
	}
}

void GraphicsManager::setSwapInterval(int interval)
{
	if (window)
	{
		glfwSwapInterval(interval);
	}
}

void GraphicsManager::setWindowRefreshCallback(GLFWwindowrefreshfun callback)
{
	if (window)
	{
		glfwSetWindowRefreshCallback(window, callback);
	}
}

bool GraphicsManager::shouldWindowClose() const
{
	return window ? glfwWindowShouldClose(window) : true;
}

void GraphicsManager::getFramebufferSize(int *width, int *height)
{
	if (window)
	{
		glfwGetFramebufferSize(window, width, height);
	}
}

bool GraphicsManager::isWindowFocused() const
{
	return window ? glfwGetWindowAttrib(window, GLFW_FOCUSED) != 0 : false;
}

void GraphicsManager::cleanup()
{
	if (window)
	{
		glfwDestroyWindow(window);
		window = nullptr;
	}
	glfwTerminate();
}