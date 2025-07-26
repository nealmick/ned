/*
File: render.cpp
Description: Render class implementation for NED text editor.
*/

#include "render.h"

#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "imgui_internal.h"

#include "ai/ai_agent.h"
#include "editor/editor.h"
#include "editor/editor_bookmarks.h"
#include "files/files.h"
#include "util/keybinds.h"
#include "util/settings.h"
#include "util/terminal.h"
#include "util/welcome.h"

// Global scope variables (these are defined in ned.cpp)
extern Bookmarks gBookmarks;
extern bool showSidebar;
extern bool showAgentPane;
extern AIAgent gAIAgent;
extern Font gFont;
extern Frame gFrame;
extern Settings gSettings;

// Constants
constexpr float kAgentSplitterWidth = 6.0f;

Render::Render() = default;

Render::~Render() = default;

bool Render::initialize(GLFWwindow *window)
{
	// Initialize any render-specific components here if needed in the future
	(void)window; // Suppress unused parameter warning
	return true;
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
		gFrame.setNeedsRedraw(true);
		gFrame.setFramesToRender(std::max(gFrame.framesToRender(), 12));
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
		(showAgentPane ? kAgentSplitterWidth : 0.0f); // Only account for splitter width
													  // when agent pane is visible

	if (showSidebar)
	{
		// First split: File Explorer vs. Editor+Agent
		float leftSplit = gSettings.getSplitPos();
		float rightSplit = gSettings.getAgentSplitPos();

		float explorerWidth = availableWidth * leftSplit;
		float agentPaneWidth = availableWidth * rightSplit;
		float editorWidth = availableWidth - explorerWidth -
							(showAgentPane ? agentPaneWidth : 0.0f) - (padding * 2) +
							16.0f;

		// Render File Explorer
		gFileExplorer.renderFileExplorer(explorerWidth);
		ImGui::SameLine(0, 0);

		// Render left splitter
		splitter.renderSplitter(padding, availableWidth);
		ImGui::SameLine(0, 0);

		// Render Editor
		gEditor.renderEditor(gFont.currentFont, editorWidth);
		if (showAgentPane)
		{
			ImGui::SameLine(0, 0);
			// Render right splitter (new)
			splitter.renderAgentSplitter(padding, availableWidth, showSidebar);
			ImGui::SameLine(0, 0);
			// Render Agent Pane (new)
			gAIAgent.render(agentPaneWidth, gFont.largeFont);
		}
	} else
	{
		// No sidebar: just editor and agent pane
		float agentSplit = gSettings.getAgentSplitPos();
		float editorWidth =
			showAgentPane
				? (availableWidth * agentSplit)
				: availableWidth + 5.0f; // Add extra width when agent pane is hidden
		float agentPaneWidth =
			showAgentPane ? (availableWidth - editorWidth - kAgentSplitterWidth) : 0.0f;

		gEditor.renderEditor(gFont.currentFont, editorWidth);
		if (showAgentPane)
		{
			ImGui::SameLine(0, 0);
			splitter.renderAgentSplitter(padding, availableWidth, showSidebar);
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
	gFrame.setupFrame(gSettings, shaderEnabled, fb, display_w, display_h, currentTime);
}

void Render::finishFrameRendering(FramebufferState &fb) { gFrame.finishFrame(fb); }
