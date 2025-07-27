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
#include "util/keybinds.h"
#include "util/settings.h"
#include "util/splitter.h"
#include <imgui.h>

#ifdef __APPLE__
#include "macos_window.h"
#endif

// Global externals
extern DebugConsole &gDebugConsole;
extern Settings gSettings;
extern Font gFont;
extern EditorHighlight gEditorHighlight;
extern FileExplorer gFileExplorer;

void Init::setupSignalHandlers()
{
	// Set up signal handlers to catch crashes and prevent crash reports
	signal(SIGABRT, [](int signal) {
		std::cerr << "Crash detected! Signal: " << signal << std::endl;
		exit(1);
	});
	signal(SIGSEGV, [](int signal) {
		std::cerr << "Crash detected! Signal: " << signal << std::endl;
		exit(1);
	});
	signal(SIGILL, [](int signal) {
		std::cerr << "Crash detected! Signal: " << signal << std::endl;
		exit(1);
	});
	signal(SIGFPE, [](int signal) {
		std::cerr << "Crash detected! Signal: " << signal << std::endl;
		exit(1);
	});
}

void Init::initializeSettings()
{
	// Load settings and keybinds
	gSettings.loadSettings();
	if (gKeybinds.loadKeybinds())
	{
		std::cout << "Initial keybinds loaded successfully." << std::endl;
	} else
	{
		std::cout << "Failed to load initial keybinds." << std::endl;
	}
}

void Init::initializeUISettings()
{
	// Load UI settings
	Splitter::loadSidebarSettings();
	Splitter::loadAgentPaneSettings();
	Splitter::adjustAgentSplitPosition();
}

void Init::initializeMacOS(GLFWwindow *window)
{
#ifdef __APPLE__
	float opacity = gSettings.getSettings().value("mac_background_opacity", 0.5f);
	bool blurEnabled = gSettings.getSettings().value("mac_blur_enabled", true);

	// Set up application delegate for proper Cmd+Q handling
	setupMacOSApplicationDelegate();

	// Initial configuration
	configureMacOSWindow(window, opacity, blurEnabled);
#endif
}

bool Init::initializeGraphics(GLFWwindow *window)
{
	// Initialize ImGui
	initializeImGui(window);

	// Initialize resources
	initializeResources();

	return true;
}

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

void Init::initializeAll(GLFWwindow *window)
{
	// Set up signal handlers for crash detection
	setupSignalHandlers();

	// Initialize settings and configuration
	initializeSettings();

	// Load UI settings
	initializeUISettings();

	// Initialize macOS-specific settings
	initializeMacOS(window);

	// Initialize graphics and rendering components
	initializeGraphics(window);
}