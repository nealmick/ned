/*
File: render.cpp
Description: Render and frame management class implementation for NED text editor.
*/

#include "render.h"

#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "imgui_internal.h"

#include "ai/ai_agent.h"
#include "editor/editor.h"
#include "editor/editor_bookmarks.h"
#include "editor/editor_scroll.h"
#include "files/file_tree.h"
#include "files/files.h"
#include "lsp/lsp_dashboard.h"
#include "util/app.h"
#include "util/keybinds.h"
#include "util/settings.h"
#include "util/splitter.h"
#include "util/terminal.h"
#include "util/welcome.h"
#include <GLFW/glfw3.h>
#include <iostream>
#include <thread>

// Global scope variables (these are defined in ned.cpp)
extern Bookmarks gBookmarks;
extern AIAgent gAIAgent;
extern Font gFont;
extern Settings gSettings;

// Constants
constexpr float kAgentSplitterWidth = 6.0f;

Render::Render()
	: lastTime(0.0), frames(0), currentFPS(0.0), m_needsRedraw(true), m_framesToRender(0),
	  m_lastActivityTime(0.0)
{
}

Render::~Render() = default;

bool Render::initialize(GLFWwindow *window)
{
	// Initialize any render-specific components here if needed in the future
	(void)window; // Suppress unused parameter warning
	return true;
}

void Render::initializeFrameManagement()
{
	lastTime = 0.0;
	frames = 0;
	currentFPS = 0.0;
}

// Frame management methods (merged from Frame)
void Render::updateFPS(double currentTime)
{
	frames++;
	if (currentTime - lastTime >= FPS_UPDATE_INTERVAL)
	{
		currentFPS = frames / (currentTime - lastTime);
		frames = 0;
		lastTime = currentTime;
	}
}

void Render::handleFrameTiming(std::chrono::high_resolution_clock::time_point frame_start,
							   bool shaderEnabled,
							   bool windowFocused,
							   Settings &settings)
{
	auto frame_end = std::chrono::high_resolution_clock::now();
	auto frame_duration = frame_end - frame_start;

	float fpsTarget = DEFAULT_FPS_TARGET;

#ifdef PLATFORM_WINDOWS
	// On Windows, use simple fixed frame timing - just sleep for the target frame time
	if (settings.getSettings().contains("fps_target") &&
		settings.getSettings()["fps_target"].is_number())
	{
		fpsTarget = settings.getSettings()["fps_target"].get<float>();
	}

	// Simple busy waiting - the only thing that works reliably on Windows
	if (fpsTarget > MIN_FPS_TARGET && fpsTarget < MAX_FPS_TARGET)
	{
		auto targetFrameTime = std::chrono::duration<double>(1.0 / fpsTarget);
		auto frame_duration_seconds = std::chrono::duration<double>(frame_duration);

		if (frame_duration_seconds < targetFrameTime)
		{
			auto endTime =
				frame_start +
				std::chrono::duration_cast<std::chrono::high_resolution_clock::duration>(
					targetFrameTime);
			while (std::chrono::high_resolution_clock::now() < endTime)
			{
				// Busy wait for precise timing
			}
		}
	}
#else
	// On macOS/Linux, keep the variable FPS logic for smooth user interaction
	// Check if scroll animation is active - if so, bypass FPS restrictions like mouse wheel
	extern EditorScroll gEditorScroll;
	bool scrollAnimationActive = gEditorScroll.isScrollAnimationActive();

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

		// When scroll animation is active, set FPS target to settings value
		if (scrollAnimationActive)
		{
			if (settings.getSettings().contains("fps_target") &&
				settings.getSettings()["fps_target"].is_number())
			{
				fpsTarget = settings.getSettings()["fps_target"].get<float>();
			}
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
#endif
}

void Render::checkForActivity()
{
	// This function will be called every loop to check for immediate input.
	ImGuiIO &io = ImGui::GetIO();

	// Check for any input activity
	bool hasInput = false;

	// Mouse activity
	if (io.MousePos.x != io.MousePosPrev.x || io.MousePos.y != io.MousePosPrev.y ||
		io.MouseWheel != 0.0f || io.MouseWheelH != 0.0f || io.MouseDown[0] ||
		io.MouseDown[1] || io.MouseDown[2])
	{
		hasInput = true;
	}

	// Keyboard activity - check if any key is pressed
	bool hasKeyActivity = false;
	for (int key = ImGuiKey_NamedKey_BEGIN; key < ImGuiKey_NamedKey_END; key++)
	{
		if (ImGui::IsKeyDown((ImGuiKey)key))
		{
			hasKeyActivity = true;
			break;
		}
	}
	if (io.InputQueueCharacters.size() > 0 || hasKeyActivity)
	{
		hasInput = true;
	}

	// ImGui widget activity
	if (ImGui::IsAnyItemActive())
	{
		hasInput = true;
	}

	// Check for active scroll animation (treat as input activity for smooth rendering)
	extern EditorScroll gEditorScroll;
	if (gEditorScroll.isScrollAnimationActive())
	{
		hasInput = true;
	}

	// If we have any input, update activity time
	if (hasInput)
	{
		setLastActivityTime(glfwGetTime());
	}
}

void Render::handleBackgroundUpdates(double currentTime)
{
	if (currentTime - timing.lastSettingsCheck >= SETTINGS_CHECK_INTERVAL)
	{
		// Store previous state to detect changes
		extern Settings gSettings;
		bool hadSettingsChanged = gSettings.hasSettingsChanged();
		bool hadFontChanged = gSettings.hasFontChanged();
		bool hadFontSizeChanged = gSettings.hasFontSizeChanged();
		bool hadThemeChanged = gSettings.hasThemeChanged();

		gSettings.checkSettingsFile();

		// Check if any settings changed
		if (gSettings.hasSettingsChanged() != hadSettingsChanged ||
			gSettings.hasFontChanged() != hadFontChanged ||
			gSettings.hasFontSizeChanged() != hadFontSizeChanged ||
			gSettings.hasThemeChanged() != hadThemeChanged)
		{
			setNeedsRedraw(true);
			setFramesToRender(std::max(framesToRender(), 2)); // Reduced frame count
		}
		timing.lastSettingsCheck = currentTime;
	}

	if (currentTime - timing.lastFileTreeRefresh >= FILE_TREE_REFRESH_INTERVAL)
	{
		// File tree refresh doesn't return a value, but we can trigger redraw
		// when it's called since it updates the UI
		extern FileTree gFileTree;
		gFileTree.refreshFileTree();
		setNeedsRedraw(true); // Always redraw after file tree refresh
		setFramesToRender(std::max(framesToRender(), 2)); // Reduced frame count
		timing.lastFileTreeRefresh = currentTime;
	}
}

void Render::handleFrameSetup(double currentTime,
							  bool &needFontReload,
							  bool &m_needsRedraw,
							  int &m_framesToRender,
							  std::function<void(bool)> setShaderEnabled,
							  float &lastOpacity,
							  bool &lastBlurEnabled,
							  bool &windowFocused,
							  App &app)
{
	// Handle background updates
	handleBackgroundUpdates(currentTime);

	// Handle settings changes
	extern Settings gSettings;
	gSettings.handleSettingsChanges(needFontReload,
									m_needsRedraw,
									m_framesToRender,
									setShaderEnabled,
									lastOpacity,
									lastBlurEnabled);

	// Setup ImGui frame
	setupImGuiFrame();

	// Handle window focus
	app.handleWindowFocus(windowFocused, gFileExplorer);

	// Handle file dialog
	if (gFileExplorer.handleFileDialog())
	{
		setNeedsRedraw(true);
		setFramesToRender(std::max(framesToRender(), 3));
	}
}

void Render::renderFPSCounter(Settings &settings)
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

void Render::setupFrame(Settings &settings,
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

void Render::finishFrame(FramebufferState &fb)
{
	ImGui::Render();
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glClear(GL_COLOR_BUFFER_BIT);
}

void Render::setupImGuiFrame()
{
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplGlfw_NewFrame();

	ImGui::NewFrame();
}

void Render::renderFrame(GLFWwindow *window,
						 ShaderManager &shaderManager,
						 FramebufferState &fb,
						 AccumulationBuffers &accum,
						 ShaderQuad &quad,
						 Settings &gSettings,
						 Splitter &splitter,
						 WindowResize &windowResize,
						 double currentTime)
{
	int display_w, display_h;
	glfwGetFramebufferSize(window, &display_w, &display_h);

	// Setup frame rendering (framebuffer, background, FPS)
	setupFrameRendering(
		gSettings, shaderManager.isShaderEnabled(), fb, display_w, display_h, currentTime);

	// Render UI components
	renderMainWindow(window, splitter, windowResize);
	gBookmarks.renderBookmarksWindow();
	gSettings.renderSettingsWindow();
	gLSPDashboard.render();
	gSettings.renderNotification("");
	gKeybinds.checkKeybindsFile();
	windowResize.renderResizeOverlay(gFont.largeFont);

	// Finish frame rendering
	finishFrameRendering(fb);

	shaderManager.renderWithEffects(
		display_w, display_h, glfwGetTime(), fb, accum, quad, gSettings);
	glfwSwapBuffers(window);
}

void Render::renderMainWindow(GLFWwindow *window,
							  Splitter &splitter,
							  WindowResize &windowResize)
{
#ifdef __APPLE__
	// renderTopLeftMenu();
#endif

	if (gKeybinds.handleKeyboardShortcuts())
	{
		setNeedsRedraw(true);
		setFramesToRender(std::max(framesToRender(), 12));
	}

	if (gTerminal.isTerminalVisible())
	{
		gTerminal.render();
		return;
	}

	if (gFileExplorer.showWelcomeScreen)
	{
		gWelcome.render();
		return;
	}

	ImGui::PushFont(gFont.currentFont);
	ImGui::SetNextWindowPos(ImVec2(0, 0));
	ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);
#ifdef __APPLE__
	ImGui::Begin("Main Window",
				 nullptr,
				 ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
					 ImGuiWindowFlags_NoResize |
					 ImGuiWindowFlags_NoScrollWithMouse | // Prevent window from scrolling
					 ImGuiWindowFlags_NoScrollbar);
#else
	ImGui::Begin("Main Window",
				 nullptr,
				 ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
					 ImGuiWindowFlags_NoResize |
					 ImGuiWindowFlags_NoScrollWithMouse | // Prevent window from scrolling
					 ImGuiWindowFlags_NoScrollbar);

#endif

	float windowWidth = ImGui::GetWindowWidth();
	float padding = ImGui::GetStyle().WindowPadding.x;
	float availableWidth =
		windowWidth - padding * 3 -
		(Splitter::showAgentPane ? kAgentSplitterWidth
								 : 0.0f); // Only account for splitter width
										  // when agent pane is visible

	if (Splitter::showSidebar)
	{
		// First split: File Explorer vs. Editor+Agent
		float leftSplit = gSettings.getSplitPos();
		float rightSplit = gSettings.getAgentSplitPos();

		float explorerWidth = availableWidth * leftSplit;
		float agentPaneWidth = availableWidth * rightSplit;
		float editorWidth = availableWidth - explorerWidth -
							(Splitter::showAgentPane ? agentPaneWidth : 0.0f) -
							(padding * 2) + 16.0f;

		// Render File Explorer
		gFileExplorer.renderFileExplorer(explorerWidth);
		ImGui::SameLine(0, 0);

		// Render left splitter
		splitter.renderSplitter(padding, availableWidth);
		ImGui::SameLine(0, 0);

		// Render Editor
		gEditor.renderEditor(gFont.currentFont, editorWidth);
		if (Splitter::showAgentPane)
		{
			ImGui::SameLine(0, 0);
			// Render right splitter (new)
			splitter.renderAgentSplitter(padding, availableWidth, Splitter::showSidebar);
			ImGui::SameLine(0, 0);
			// Render Agent Pane (new)
			gAIAgent.render(agentPaneWidth, gFont.largeFont);
		}
	} else
	{
		// No sidebar: just editor and agent pane
		float agentSplit = gSettings.getAgentSplitPos();

		// Ensure agent pane has minimum width of 300px
		float minAgentWidth = 300.0f;
		float maxEditorWidth = availableWidth - minAgentWidth - kAgentSplitterWidth;

		float editorWidth =
			Splitter::showAgentPane
				? std::min(availableWidth * agentSplit, maxEditorWidth)
				: availableWidth + 5.0f; // Add extra width when agent pane is hidden

		float agentPaneWidth =
			Splitter::showAgentPane
				? std::max(availableWidth - editorWidth - kAgentSplitterWidth,
						   minAgentWidth)
				: 0.0f;

		gEditor.renderEditor(gFont.currentFont, editorWidth);
		if (Splitter::showAgentPane)
		{
			ImGui::SameLine(0, 0);
			splitter.renderAgentSplitter(padding, availableWidth, Splitter::showSidebar);
			ImGui::SameLine(0, 0);
			gAIAgent.render(agentPaneWidth, gFont.largeFont);
		}
	}
	windowResize.resize();
	ImGui::End();
	ImGui::PopFont();
}

void Render::setupFrameRendering(Settings &gSettings,
								 bool shaderEnabled,
								 FramebufferState &fb,
								 int display_w,
								 int display_h,
								 double currentTime)
{
	setupFrame(gSettings, shaderEnabled, fb, display_w, display_h, currentTime);
}

void Render::finishFrameRendering(FramebufferState &fb) { finishFrame(fb); }
