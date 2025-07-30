/*
File: cleanup_manager.cpp
Description: Cleanup manager class implementation for handling all cleanup logic.
Moved from ned.cpp to better organize the code.
*/

#include "cleanup_manager.h"
#include "ai/ai_agent.h"
#include "files/files.h"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "shaders/shader_manager.h"
#include "shaders/shader_types.h"
#include "util/app.h"
#include "util/settings.h"
#include <iostream>

// Global externals
extern AIAgent gAIAgent;

CleanupManager::CleanupManager() {}

CleanupManager::~CleanupManager() {}

void CleanupManager::cleanupAll(ShaderQuad &quad,
								ShaderManager &shaderManager,
								FramebufferState &fb,
								AccumulationBuffers &accum,
								App &app)
{
	// Cleanup quad
	cleanupQuad(quad);

	// Cleanup framebuffers
	cleanupFramebuffers(shaderManager, fb, accum);

	// Save settings
	gSettings.saveSettings();

	// Save current file
	gFileExplorer.saveCurrentFile();

	// Save AI agent history
	gAIAgent.getHistoryManager().saveConversationHistory();

	// Cleanup graphics
	cleanupGraphics(app);

	// Cleanup ImGui
	cleanupImGui();
}

void CleanupManager::cleanupQuad(ShaderQuad &quad) { quad.cleanup(); }

void CleanupManager::cleanupFramebuffers(ShaderManager &shaderManager,
										 FramebufferState &fb,
										 AccumulationBuffers &accum)
{
	shaderManager.cleanupFramebuffers(fb, accum);
}

void CleanupManager::cleanupGraphics(App &app) { app.cleanup(); }

void CleanupManager::cleanupImGui()
{
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();
}