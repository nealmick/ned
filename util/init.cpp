/*
File: init.cpp
Description: Initialization class implementation moved from ned.cpp
*/

#include "init.h"
#include "editor/editor_highlight.h"
#include "files/files.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "util/debug_console.h"
#include "util/font.h"
#include "util/settings.h"
#include <imgui.h>

// Global externals
extern DebugConsole &gDebugConsole;
extern Settings gSettings;
extern Font gFont;
extern EditorHighlight gEditorHighlight;
extern FileExplorer gFileExplorer;

void Init::initializeResources()
{
	gDebugConsole.toggleVisibility();
	gEditorHighlight.setTheme(gSettings.getCurrentTheme());

	// Apply settings with the actual loaded font size
	gSettings.ApplySettings(ImGui::GetStyle());

	// Initialize fonts using the Font class
	gFont.initialize();

	// Continue with remaining initialization
	gFileExplorer.loadIcons();
}

void Init::initializeImGui(GLFWwindow *window)
{
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO &io = ImGui::GetIO();
	io.IniFilename = NULL; // Disable ImGui .ini file writing
	(void)io;
	ImGui::StyleColorsDark();
	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init("#version 330");
}