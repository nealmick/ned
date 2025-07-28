/*
File: application_manager.cpp
Description: Application manager class implementation for handling main application loop
logic. Moved from ned.cpp to better organize the code.
*/

#include "application_manager.h"
#include "editor/editor_scroll.h"
#include "shaders/shader_manager.h"
#include "util/font.h"
#include "util/frame.h"
#include "util/graphics_manager.h"
#include "util/render.h"
#include "util/scroll.h"
#include "util/settings.h"
#include "util/window_manager.h"
#include <chrono>
#include <iostream>

// Global externals
extern Frame gFrame;
extern Font gFont;

ApplicationManager::ApplicationManager() {}

ApplicationManager::~ApplicationManager() {}

bool ApplicationManager::initialize(GraphicsManager &graphicsManager,
									WindowManager &windowManager,
									ShaderManager &shaderManager,
									Render &render,
									Settings &settings,
									Splitter &splitter,
									WindowResize &windowResize,
									ShaderQuad &quad,
									FramebufferState &fb,
									AccumulationBuffers &accum)
{
	// Set up scroll callback
	if (scrollCallback)
	{
		graphicsManager.setScrollCallback(scrollCallback);
	}

	// Initialize window resize handler
	windowResize.initialize(graphicsManager.getWindow());

	// Initialize render class
	if (!render.initialize(graphicsManager.getWindow()))
	{
		std::cerr << "Failed to initialize render class" << std::endl;
		return false;
	}

	// Initialize frame management
	gFrame.initialize();

	return true;
}

void ApplicationManager::runMainLoop(GraphicsManager &graphicsManager,
									 WindowManager &windowManager,
									 ShaderManager &shaderManager,
									 Render &render,
									 Settings &settings,
									 Splitter &splitter,
									 WindowResize &windowResize,
									 ShaderQuad &quad,
									 FramebufferState &fb,
									 AccumulationBuffers &accum,
									 bool &needFontReload,
									 bool &windowFocused,
									 double &scrollXAccumulator,
									 double &scrollYAccumulator,
									 float &lastOpacity,
									 bool &lastBlurEnabled)
{
	GLFWwindow *window = graphicsManager.getWindow();

	while (!graphicsManager.shouldWindowClose())
	{
		// Get current time for activity tracking
		double currentTime = glfwGetTime();

		// Handle event polling
		handleEventPolling(graphicsManager, windowManager, shaderManager, currentTime);

		// Handle window management
		handleWindowManagement(windowManager, window);

		// Handle scroll accumulators
		handleScrollAccumulators(scrollXAccumulator, scrollYAccumulator);

		// Check for activity and decide if we should keep rendering
		gFrame.checkForActivity();
		bool shouldKeepRendering = (currentTime - gFrame.lastActivityTime()) < 0.5 ||
								   gEditorScroll.isScrollAnimationActive();

		// Always render a frame after polling events
		auto frame_start = std::chrono::high_resolution_clock::now();

		// Handle frame setup using Frame class
		gFrame.handleFrameSetup(
			currentTime,
			needFontReload,
			gFrame.needsRedrawRef(),
			gFrame.framesToRenderRef(),
			[&shaderManager](bool enabled) { shaderManager.setShaderEnabled(enabled); },
			lastOpacity,
			lastBlurEnabled,
			windowFocused,
			windowManager);

		// Setup framebuffers
		handleFramebufferSetup(graphicsManager, shaderManager, fb, accum);

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

		// Handle frame timing using Frame class
		gFrame.handleFrameTiming(
			frame_start, shaderManager.isShaderEnabled(), windowFocused, settings);
	}
}

void ApplicationManager::setScrollCallback(GLFWscrollfun callback)
{
	scrollCallback = callback;
}

void ApplicationManager::cleanup()
{
	// Cleanup is handled by individual components
}

void ApplicationManager::handleEventPolling(GraphicsManager &graphicsManager,
											WindowManager &windowManager,
											ShaderManager &shaderManager,
											double currentTime)
{
	// Handle event polling using GraphicsManager
	graphicsManager.pollEvents(currentTime,
							   shaderManager.isShaderEnabled(),
							   gFrame.lastActivityTime());
}

void ApplicationManager::handleWindowManagement(WindowManager &windowManager,
												GLFWwindow *window)
{
	// Handle window management (macOS termination, etc.)
	if (windowManager.shouldTerminateApplication())
	{
		glfwSetWindowShouldClose(window, 1);
	}
}

void ApplicationManager::handleScrollAccumulators(double &scrollXAccumulator,
												  double &scrollYAccumulator)
{
	// Handle scroll accumulators
	Scroll::handleScrollAccumulators(scrollXAccumulator, scrollYAccumulator);
}

void ApplicationManager::handleFramebufferSetup(GraphicsManager &graphicsManager,
												ShaderManager &shaderManager,
												FramebufferState &fb,
												AccumulationBuffers &accum)
{
	// Setup framebuffers
	int display_w, display_h;
	graphicsManager.getFramebufferSize(&display_w, &display_h);
	shaderManager.initializeFramebuffers(display_w, display_h, fb, accum);
}

void ApplicationManager::handleFrameRendering(Render &render,
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

void ApplicationManager::handleFontReload(bool &needFontReload)
{
	// Handle font reloading using Font class
	gFont.handleFontReloadWithFrameUpdates(needFontReload);
}