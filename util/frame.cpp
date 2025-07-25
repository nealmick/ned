/*
File: frame.cpp
Description: Frame management class implementation for NED text editor.
*/

#include "frame.h"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "imgui_internal.h"
#include "settings.h"
#include "shaders/shader_types.h"

#include <iostream>
#include <thread>

// Global frame instance
Frame gFrame;

Frame::Frame()
	: lastTime(0.0), frames(0), currentFPS(0.0), m_needsRedraw(true), m_framesToRender(0),
	  m_lastActivityTime(0.0)
{
}

Frame::~Frame() {}

void Frame::initialize()
{
	lastTime = 0.0;
	frames = 0;
	currentFPS = 0.0;
}

void Frame::updateFPS(double currentTime)
{
	frames++;
	if (currentTime - lastTime >= FPS_UPDATE_INTERVAL)
	{
		currentFPS = frames / (currentTime - lastTime);
		frames = 0;
		lastTime = currentTime;
	}
}

void Frame::handleFrameTiming(std::chrono::high_resolution_clock::time_point frame_start,
							  bool shaderEnabled,
							  bool windowFocused,
							  Settings &settings)
{
	auto frame_end = std::chrono::high_resolution_clock::now();
	auto frame_duration = frame_end - frame_start;

	float fpsTarget = DEFAULT_FPS_TARGET;

	if (shaderEnabled)
	{
		// When shaders are enabled, use settings FPS target but don't sleep
		if (settings.getSettings().contains("fps_target") &&
			settings.getSettings()["fps_target"].is_number())
		{
			fpsTarget = settings.getSettings()["fps_target"].get<float>();
		}
		// Don't apply sleep delays when shaders are enabled - let GPU run naturally
		return;
	} else
	{
		// Only apply frame timing when shaders are disabled
		if (!windowFocused)
		{
			fpsTarget = UNFOCUSED_FPS_TARGET;
		} else if (settings.getSettings().contains("fps_target") &&
				   settings.getSettings()["fps_target"].is_number())
		{
			fpsTarget = settings.getSettings()["fps_target"].get<float>();
		}
	}

	// Only apply frame timing if FPS target is reasonable (not unlimited)
	if (fpsTarget > MIN_FPS_TARGET && fpsTarget < MAX_FPS_TARGET)
	{
		auto targetFrameTime = std::chrono::duration<double>(1.0 / fpsTarget);
		if (frame_duration < targetFrameTime)
		{
			std::this_thread::sleep_for(targetFrameTime - frame_duration);
		}
	}
}

void Frame::renderFPSCounter(Settings &settings)
{
	// Check if FPS counter is enabled in settings
	if (!settings.getSettings()["fps_toggle"].get<bool>())
	{
		return;
	}

	ImGuiViewport *viewport = ImGui::GetMainViewport();
	ImVec2 viewportPos = viewport->Pos;
	ImVec2 viewportSize = viewport->Size;

	// Position in bottom right corner with some padding
	const float padding = 10.0f;

	// Format FPS text
	char fpsText[32];
	snprintf(fpsText, sizeof(fpsText), "%.1f", currentFPS);

	// Calculate text size
	ImVec2 textSize = ImGui::CalcTextSize(fpsText);

	// Position text in bottom right
	ImVec2 textPos = ImVec2(viewportPos.x + viewportSize.x - textSize.x - padding,
							viewportPos.y + viewportSize.y - textSize.y - padding);

	// Draw background rectangle for better visibility
	ImDrawList *drawList = ImGui::GetForegroundDrawList();
	ImVec2 bgMin = ImVec2(textPos.x - 5.0f, textPos.y - 2.0f);
	ImVec2 bgMax = ImVec2(textPos.x + textSize.x + 5.0f, textPos.y + textSize.y + 2.0f);
	drawList->AddRectFilled(bgMin, bgMax, IM_COL32(0, 0, 0, 180));

	// Draw FPS text
	drawList->AddText(textPos, IM_COL32(255, 255, 255, 255), fpsText);
}

void Frame::setupFrame(Settings &settings,
					   bool shaderEnabled,
					   FramebufferState &fb,
					   int display_w,
					   int display_h,
					   double currentTime)
{
	// Update FPS calculation
	updateFPS(currentTime);

	// [STEP 1] Render UI to framebuffer
	glBindFramebuffer(GL_FRAMEBUFFER, fb.framebuffer);
	glViewport(0, 0, display_w, display_h);

	// Get background color from settings
	auto &bg = settings.getSettings()["backgroundColor"];

	// Use different alpha based on shader state
	float alpha = shaderEnabled ? bg[3].get<float>() : 1.0f;
	glClearColor(bg[0], bg[1], bg[2], alpha);
	glClear(GL_COLOR_BUFFER_BIT);

	// Render FPS counter overlay
	renderFPSCounter(settings);
}

void Frame::finishFrame(FramebufferState &fb)
{
	ImGui::Render();
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glClear(GL_COLOR_BUFFER_BIT);
}

void Frame::setupImGuiFrame()
{
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplGlfw_NewFrame();

	ImGui::NewFrame();
}
