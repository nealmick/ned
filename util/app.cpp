/*
File: app.cpp
Description: Combined application and graphics management implementation for NED text
editor. This class combines the functionality of ApplicationManager and GraphicsManager.
*/

#include "app.h"
#include "../files/files.h"
#include "ai/ai_agent.h"
#include "editor/editor_scroll.h"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "settings.h"
#include "shaders/shader_manager.h"
#include "util/font.h"
#include "util/render.h"
#include "util/scroll.h"
#include "util/settings.h"
#include <chrono>
#include <iostream>

#ifdef __APPLE__
#include "../macos_window.h"
#endif

// Global externals
extern Font gFont;
extern FileExplorer gFileExplorer;

App::App()
	: window(nullptr), lastFocusState(false), windowFocused(true), lastOpacity(0.5f),
	  lastBlurEnabled(false), scrollCallback(nullptr), scrollXAccumulator(0.0),
	  scrollYAccumulator(0.0)
{
}

App::~App() { cleanup(); }

bool App::initialize(ShaderManager &shaderManager)
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

	// CRITICAL FIX: Configure macOS window immediately after window creation (like old
	// version)
#ifdef __APPLE__
	float opacity = 0.5f; // Default values, will be updated when settings are loaded
	bool blurEnabled = true;
	configureMacOSWindow(window, opacity, blurEnabled);
#endif

	if (!shaderManager.initializeShaders())
	{
		std::cerr << "🔴 Shader initialization failed" << std::endl;
		return false;
	}

	return true;
}

bool App::initializeApp(ShaderManager &shaderManager,
						Render &render,
						Settings &settings,
						Splitter &splitter,
						WindowResize &windowResize,
						ShaderQuad &quad,
						FramebufferState &fb,
						AccumulationBuffers &accum)
{
	// Initialize window resize handler
	windowResize.initialize(getWindow());

	// Initialize render class
	if (!render.initialize(getWindow()))
	{
		std::cerr << "Failed to initialize render class" << std::endl;
		return false;
	}

	// Initialize frame management
	render.initializeFrameManagement();

	// Apply all settings after all components are initialized
	float lastOpacity = getLastOpacity();
	bool lastBlurEnabled = getLastBlurEnabled();
	bool needFontReload = false;
	bool m_needsRedraw = false;
	int m_framesToRender = 0;
	settings.handleSettingsChanges(
		needFontReload,
		m_needsRedraw,
		m_framesToRender,
		[&shaderManager](bool enabled) { shaderManager.setShaderEnabled(enabled); },
		lastOpacity,
		lastBlurEnabled,
		true // force = true to apply settings even if they haven't "changed"
	);

	return true;
}

void App::runMainLoop(ShaderManager &shaderManager,
					  Render &render,
					  Settings &settings,
					  Splitter &splitter,
					  WindowResize &windowResize,
					  ShaderQuad &quad,
					  FramebufferState &fb,
					  AccumulationBuffers &accum,
					  bool &needFontReload,
					  float &lastOpacity,
					  bool &lastBlurEnabled)
{
	GLFWwindow *window = getWindow();

	while (!shouldWindowClose())
	{
		// Check for external file changes
		gFileExplorer.checkForExternalFileChanges();

		// Get current time for activity tracking
		double currentTime = glfwGetTime();

		// Handle event polling
		handleEventPolling(shaderManager, currentTime);

		// Handle window management
		handleWindowManagement(window);

		// Handle scroll accumulators
		handleScrollAccumulators(this->scrollXAccumulator, this->scrollYAccumulator);

		// Check for activity and decide if we should keep rendering
		render.checkForActivity();
		bool shouldKeepRendering = (currentTime - render.lastActivityTime()) < 0.5 ||
								   gEditorScroll.isScrollAnimationActive();

		// Always render a frame after polling events
		auto frame_start = std::chrono::high_resolution_clock::now();

		// Handle frame setup using Render class
		render.handleFrameSetup(
			currentTime,
			needFontReload,
			render.needsRedrawRef(),
			render.framesToRenderRef(),
			[&shaderManager](bool enabled) { shaderManager.setShaderEnabled(enabled); },
			lastOpacity,
			lastBlurEnabled,
			this->windowFocused,
			*this);

		// Setup framebuffers
		handleFramebufferSetup(shaderManager, fb, accum);

		// Handle main rendering
		handleFrameRendering(render,
							 window,
							 shaderManager,
							 fb,
							 accum,
							 quad,
							 settings,
							 splitter,
							 windowResize,
							 currentTime);

		// Handle font reloading using Font class
		handleFontReload(needFontReload);

		// Handle frame updates for font reloading
		if (needFontReload)
		{
			render.setNeedsRedraw(true);
			render.setFramesToRender(std::max(render.framesToRender(), 3));
		}

		// Handle frame timing using Render class
		render.handleFrameTiming(
			frame_start, shaderManager.isShaderEnabled(), this->windowFocused, settings);
	}
}

// Graphics manager methods
void App::setScrollCallback(GLFWscrollfun callback)
{
	if (window)
	{
		glfwSetScrollCallback(window, callback);
	}
}

void App::setAppScrollCallback(GLFWscrollfun callback)
{
	scrollCallback = callback;
	if (window)
	{
		glfwSetScrollCallback(window, callback);
	}
}

void App::enableRawMouseMotion()
{
	if (window)
	{
		glfwSetInputMode(window, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
	}
}

void App::setWindowUserPointer(void *pointer)
{
	if (window)
	{
		glfwSetWindowUserPointer(window, pointer);
	}
}

void App::makeContextCurrent()
{
	if (window)
	{
		glfwMakeContextCurrent(window);
	}
}

void App::setSwapInterval(int interval)
{
	if (window)
	{
		glfwSwapInterval(interval);
	}
}

void App::setWindowRefreshCallback(GLFWwindowrefreshfun callback)
{
	if (window)
	{
		glfwSetWindowRefreshCallback(window, callback);
	}
}

bool App::shouldWindowClose() const { return glfwWindowShouldClose(window); }

void App::getFramebufferSize(int *width, int *height)
{
	if (window)
	{
		glfwGetFramebufferSize(window, width, height);
	}
}

bool App::isWindowFocused() const
{
	return window ? glfwGetWindowAttrib(window, GLFW_FOCUSED) != 0 : false;
}

void App::pollEvents(double currentTime, bool shaderEnabled, double lastActivityTime)
{
#ifdef PLATFORM_WINDOWS
	// On Windows, always use non-blocking polling for consistent frame timing
	glfwPollEvents();
#else
	// On macOS/Linux, use timeout-based polling for power efficiency
	double timeout = calculateEventTimeout(currentTime, shaderEnabled, lastActivityTime);
	glfwWaitEventsTimeout(timeout);
#endif
}

double App::calculateEventTimeout(double currentTime,
								  bool shaderEnabled,
								  double lastActivityTime)
{
#ifdef PLATFORM_WINDOWS
	// On Windows, always use consistent FPS-based timeout
	float fpsTarget = 60.0f;
	extern Settings gSettings;
	if (gSettings.getSettings().contains("fps_target") &&
		gSettings.getSettings()["fps_target"].is_number())
	{
		fpsTarget = gSettings.getSettings()["fps_target"].get<float>();
	}
	return 1.0 / fpsTarget; // Convert FPS to timeout
#else
	// On macOS/Linux, keep the variable timeout logic for smooth user interaction
	// Check if scroll animation is active - if so, use shorter timeout for responsiveness
	extern EditorScroll gEditorScroll;
	bool scrollAnimationActive = gEditorScroll.isScrollAnimationActive();

	// When shaders are enabled, use settings FPS target for timeout
	// When shaders are disabled, use normal timeout logic
	if (shaderEnabled)
	{
		// Use settings FPS target when shaders are enabled
		float fpsTarget = 60.0f;
		extern Settings gSettings;
		if (gSettings.getSettings().contains("fps_target") &&
			gSettings.getSettings()["fps_target"].is_number())
		{
			fpsTarget = gSettings.getSettings()["fps_target"].get<float>();
		}
		return 1.0 / fpsTarget; // Convert FPS to timeout
	} else
	{
		// Use shorter timeout if we recently had activity (for smoother
		// interaction) Also respect minimum FPS: 25 FPS normally
		double minFPS = 25.0;
		double maxTimeout = 1.0 / minFPS; // Convert FPS to timeout

		// When scroll animation is active, use much shorter timeout for responsiveness
		if (scrollAnimationActive)
		{
			return 0.001; // 1ms timeout for very responsive scrolling
		}

		return (currentTime - lastActivityTime) < 0.5 ? 0.016 : maxTimeout;
	}
#endif
}

// Window management methods
void App::initializeWindowManagement(GLFWwindow *window)
{
	this->window = window;
	lastFocusState = isWindowFocused(window);
}

void App::handleWindowFocus(bool &windowFocused, FileExplorer &fileExplorer)
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

bool App::shouldTerminateApplication() const
{
#ifdef __APPLE__
	return ::shouldTerminateApplication();
#else
	return false;
#endif
}

void App::configureMacOSWindow(GLFWwindow *window, float opacity, bool blurEnabled)
{
#ifdef __APPLE__
	::configureMacOSWindow(window, opacity, blurEnabled);
#endif
}

void App::updateMacOSWindowProperties(float opacity, bool blurEnabled)
{
#ifdef __APPLE__
	::updateMacOSWindowProperties(opacity, blurEnabled);
#endif
}

bool App::isWindowFocused(GLFWwindow *window) const
{
	return window ? glfwGetWindowAttrib(window, GLFW_FOCUSED) != 0 : false;
}

void App::handleSettingsChanges(Settings &settings,
								float &lastOpacity,
								bool &lastBlurEnabled)
{
#ifdef __APPLE__
	float opacity = settings.getSettings().value("mac_background_opacity", 0.5f);
	bool blurEnabled = settings.getSettings().value("mac_blur_enabled", true);

	if (opacity != lastOpacity || blurEnabled != lastBlurEnabled)
	{
		// Use updateMacOSWindowProperties for settings changes (like old version)
		updateMacOSWindowProperties(opacity, blurEnabled);
		lastOpacity = opacity;
		lastBlurEnabled = blurEnabled;
	}
#endif
}

void App::cleanup()
{
	if (window)
	{
		glfwDestroyWindow(window);
		window = nullptr;
	}
	glfwTerminate();
}

// Graphics initialization methods
bool App::initializeGLFW()
{
	std::cout << "Initializing GLFW..." << std::endl;

	if (!glfwInit())
	{
		std::cerr << "❌ Failed to initialize GLFW" << std::endl;

		// Get detailed GLFW error
		const char *error_description;
		glfwGetError(&error_description);
		if (error_description)
		{
			std::cerr << "GLFW Init Error: " << error_description << std::endl;
		}

		return false;
	}

	std::cout << "✅ GLFW initialized successfully" << std::endl;
	return true;
}

void App::setupWindowHints()
{
	std::cout << "Setting up GLFW window hints..." << std::endl;

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	std::cout << "  - OpenGL 3.3 Core Profile" << std::endl;

#ifdef __APPLE__
	// For app bundles, use more conservative window hints
	glfwWindowHint(GLFW_DECORATED, GLFW_TRUE); // Use OS decorations for app bundles
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);	  // macOS specific
	glfwWindowHint(GLFW_TRANSPARENT_FRAMEBUFFER, GLFW_FALSE); // Disable for compatibility
	glfwWindowHint(GLFW_COCOA_RETINA_FRAMEBUFFER, GLFW_TRUE); // Enable retina support
	std::cout << "  - macOS specific hints set" << std::endl;
#else // For Linux/Ubuntu and other non-Apple platforms
	glfwWindowHint(GLFW_DECORATED, GLFW_TRUE); // Use OS decorations
	std::cout << "  - Linux/Ubuntu hints set" << std::endl;
#endif
	// GLFW_RESIZABLE should be TRUE for both.
	// On Linux, the OS window manager handles resizing if DECORATED is TRUE.
	// On macOS, your custom logic handles resizing.
	glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
	std::cout << "  - Resizable: TRUE" << std::endl;

	std::cout << "Window hints configured successfully" << std::endl;
}

bool App::createWindow()
{
	std::cout << "=== GLFW Window Creation Debug ===" << std::endl;

	setupWindowHints();
	std::cout << "Window hints set, attempting to create window with OpenGL 3.3..."
			  << std::endl;

	window = glfwCreateWindow(1200, 750, "NED", NULL, NULL);

	if (!window)
	{
		std::cerr << "❌ Failed with OpenGL 3.3, trying OpenGL 2.1..." << std::endl;

		// Get detailed error for OpenGL 3.3 failure
		const char *error_description;
		glfwGetError(&error_description);
		if (error_description)
		{
			std::cerr << "OpenGL 3.3 Error: " << error_description << std::endl;
		}

		// Try with older OpenGL for VM compatibility
		std::cout << "Setting OpenGL 2.1 hints..." << std::endl;
		glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
		glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
		// Remove core profile requirement for older OpenGL
		glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_ANY_PROFILE);

		std::cout << "Attempting to create window with OpenGL 2.1..." << std::endl;
		window = glfwCreateWindow(1200, 750, "NED", NULL, NULL);

		if (!window)
		{
			std::cerr << "❌ Failed to create GLFW window with any OpenGL version"
					  << std::endl;

			// Get detailed error for OpenGL 2.1 failure
			glfwGetError(&error_description);
			if (error_description)
			{
				std::cerr << "OpenGL 2.1 Error: " << error_description << std::endl;
			}

			glfwTerminate();
			return false;
		} else
		{
			std::cout << "✅ OpenGL 2.1 window created successfully!" << std::endl;
		}
	} else
	{
		std::cout << "✅ OpenGL 3.3 window created successfully!" << std::endl;
	}

	std::cout << "=== Window Creation Complete ===" << std::endl;
	return true;
}

bool App::initializeGLEW()
{
	glfwMakeContextCurrent(window);
#ifdef PLATFORM_WINDOWS
	// On Windows, try different VSync disable approaches for better compatibility
	glfwSwapInterval(-1); // Try adaptive VSync first
	glfwSwapInterval(0);  // Then disable completely
#else
	glfwSwapInterval(0);
#endif
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

void App::initializeOpenGLState()
{
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glGetError(); // Clear any GLEW startup errors
}

// Application loop helper methods
void App::handleEventPolling(ShaderManager &shaderManager, double currentTime)
{
	// Handle event polling using this class
	pollEvents(currentTime,
			   shaderManager.isShaderEnabled(),
			   0.0); // We'll get activity time from render object
}

void App::handleWindowManagement(GLFWwindow *window)
{
	// Handle window management (macOS termination, etc.)
	if (shouldTerminateApplication())
	{
		glfwSetWindowShouldClose(window, 1);
	}
}

void App::handleScrollAccumulators(double &scrollXAccumulator, double &scrollYAccumulator)
{
	// Handle scroll accumulators
	Scroll::handleScrollAccumulators(scrollXAccumulator, scrollYAccumulator);
}

void App::handleFramebufferSetup(ShaderManager &shaderManager,
								 FramebufferState &fb,
								 AccumulationBuffers &accum)
{
	// Setup framebuffers
	int display_w, display_h;
	getFramebufferSize(&display_w, &display_h);
	shaderManager.initializeFramebuffers(display_w, display_h, fb, accum);
}

void App::handleFrameRendering(Render &render,
							   GLFWwindow *window,
							   ShaderManager &shaderManager,
							   FramebufferState &fb,
							   AccumulationBuffers &accum,
							   ShaderQuad &quad,
							   Settings &settings,
							   Splitter &splitter,
							   WindowResize &windowResize,
							   double currentTime)
{
	// Handle main rendering
	render.renderFrame(window,
					   shaderManager,
					   fb,
					   accum,
					   quad,
					   settings,
					   splitter,
					   windowResize,
					   currentTime);
}

void App::handleFontReload(bool &needFontReload)
{
	// Handle font reloading using Font class
	gFont.handleFontReloadWithFrameUpdates();
	needFontReload = gFont.getNeedFontReload();
}

// Cleanup methods
void App::cleanupAll(ShaderQuad &quad,
					 ShaderManager &shaderManager,
					 FramebufferState &fb,
					 AccumulationBuffers &accum)
{
	// Cleanup quad
	cleanupQuad(quad);

	// Cleanup framebuffers
	cleanupFramebuffers(shaderManager, fb, accum);

	// Save settings
	extern Settings gSettings;
	gSettings.saveSettings();

	// Save current file
	extern FileExplorer gFileExplorer;
	gFileExplorer.saveCurrentFile();

	// Save AI agent history
	extern AIAgent gAIAgent;
	gAIAgent.getHistoryManager().saveConversationHistory();

	// Cleanup graphics
	cleanup();

	// Cleanup ImGui
	cleanupImGui();
}

void App::cleanupQuad(ShaderQuad &quad) { quad.cleanup(); }

void App::cleanupFramebuffers(ShaderManager &shaderManager,
							  FramebufferState &fb,
							  AccumulationBuffers &accum)
{
	shaderManager.cleanupFramebuffers(fb, accum);
}

void App::cleanupImGui()
{
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();
}