/*
File: cleanup_manager.h
Description: Cleanup manager class for handling all cleanup logic.
Moved from ned.cpp to better organize the code.
*/

#pragma once

#include <GL/glew.h>
#include <GLFW/glfw3.h>

// Forward declarations
class GraphicsManager;
class ShaderManager;
class ShaderQuad;
struct FramebufferState;
struct AccumulationBuffers;

class CleanupManager
{
  public:
	CleanupManager();
	~CleanupManager();

	// Main cleanup method
	void cleanupAll(ShaderQuad &quad,
					ShaderManager &shaderManager,
					FramebufferState &fb,
					AccumulationBuffers &accum,
					GraphicsManager &graphicsManager);

  private:
	// Helper cleanup methods
	void cleanupQuad(ShaderQuad &quad);
	void cleanupFramebuffers(ShaderManager &shaderManager,
							 FramebufferState &fb,
							 AccumulationBuffers &accum);
	void cleanupGraphics(GraphicsManager &graphicsManager);
	void cleanupImGui();
};