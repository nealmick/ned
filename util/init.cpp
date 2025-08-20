/*
File: init.cpp
Description: Initialization class implementation for handling all initialization logic.
Consolidated from ned.cpp and initialization_manager.cpp
*/

#include "init.h"
#include "editor/editor_highlight.h"
#include "files/files.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "shaders/shader_types.h"
#include "util/app.h"
#include "util/debug_console.h"
#include "util/font.h"
#include "util/keybinds.h"
#include "util/settings.h"
#include "util/splitter.h"
// Window manager functionality merged into GraphicsManager
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

	// Customize ImGui style for rounded borders
	ImGuiStyle &style = ImGui::GetStyle();
	style.FrameRounding = 8.0f; // Rounded corners for input fields, sliders, etc.
	style.GrabRounding = 8.0f;	// Rounded corners for grab handles
#ifdef PLATFORM_WINDOWS
	style.WindowRounding =
		0.0f; // Square corners for windows on Windows to match title bar
#else
	style.WindowRounding = 12.0f; // Rounded corners for windows on macOS/Linux
#endif
	style.ChildRounding = 8.0f;		// Rounded corners for child windows
	style.PopupRounding = 8.0f;		// Rounded corners for popups
	style.ScrollbarRounding = 8.0f; // Rounded corners for scrollbars
	style.TabRounding = 8.0f;		// Rounded corners for tabs

	// Global padding for all buttons and framed elements
	style.FramePadding = ImVec2(6.0f, 4.0f); // Horizontal and vertical padding

	// Bootstrap-like color scheme
	ImVec4 *colors = style.Colors;

	// Primary colors (Darker Bootstrap blue)
	colors[ImGuiCol_Button] = ImVec4(0.08f, 0.45f, 0.75f, 1.00f);		 // Primary button
	colors[ImGuiCol_ButtonHovered] = ImVec4(0.06f, 0.35f, 0.60f, 1.00f); // Hovered button
	colors[ImGuiCol_ButtonActive] = ImVec4(0.04f, 0.25f, 0.45f, 1.00f);	 // Active button

	// Checkbox and radio button colors
	colors[ImGuiCol_CheckMark] = ImVec4(0.08f, 0.45f, 0.75f, 1.00f); // Checkmark color
	colors[ImGuiCol_FrameBg] =
		ImVec4(0.95f, 0.95f, 0.95f, 0.30f); // Input field background
	colors[ImGuiCol_FrameBgHovered] =
		ImVec4(0.90f, 0.90f, 0.90f, 0.40f); // Input field hover
	colors[ImGuiCol_FrameBgActive] =
		ImVec4(0.85f, 0.85f, 0.85f, 0.50f); // Input field active

	// Slider colors
	colors[ImGuiCol_SliderGrab] = ImVec4(0.08f, 0.45f, 0.75f, 1.00f); // Slider grab
	colors[ImGuiCol_SliderGrabActive] =
		ImVec4(0.06f, 0.35f, 0.60f, 1.00f); // Slider grab active

	// Header colors
	colors[ImGuiCol_Header] = ImVec4(0.08f, 0.45f, 0.75f, 0.31f); // Header background
	colors[ImGuiCol_HeaderHovered] = ImVec4(0.08f, 0.45f, 0.75f, 0.60f); // Header hover
	colors[ImGuiCol_HeaderActive] = ImVec4(0.08f, 0.45f, 0.75f, 0.80f);	 // Header active

	// Tab colors
	colors[ImGuiCol_Tab] = ImVec4(0.08f, 0.45f, 0.75f, 0.31f);		  // Tab background
	colors[ImGuiCol_TabHovered] = ImVec4(0.08f, 0.45f, 0.75f, 0.60f); // Tab hover
	colors[ImGuiCol_TabActive] = ImVec4(0.08f, 0.45f, 0.75f, 0.80f);  // Tab active

	// Scrollbar colors
	colors[ImGuiCol_ScrollbarBg] =
		ImVec4(0.95f, 0.95f, 0.95f, 0.30f); // Scrollbar background
	colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.08f, 0.45f, 0.75f, 0.60f); // Scrollbar grab
	colors[ImGuiCol_ScrollbarGrabHovered] =
		ImVec4(0.08f, 0.45f, 0.75f, 0.80f); // Scrollbar grab hover
	colors[ImGuiCol_ScrollbarGrabActive] =
		ImVec4(0.06f, 0.35f, 0.60f, 1.00f); // Scrollbar grab active

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

bool Init::initializeAllComponents(App &app,
								   ShaderManager &shaderManager,
								   Render &render,
								   Settings &settings,
								   Splitter &splitter,
								   WindowResize &windowResize,
								   ShaderQuad &quad,
								   FramebufferState &fb,
								   AccumulationBuffers &accum)
{
	// Initialize graphics system
	if (!initializeGraphicsSystem(app, shaderManager))
	{
		return false;
	}

	// Initialize window management in app
	app.initializeWindowManagement(app.getWindow());

	// Initialize components
	if (!initializeComponents(app))
	{
		return false;
	}

	// Initialize quad
	if (!initializeQuad(quad))
	{
		return false;
	}

	return true;
}

bool Init::initializeGraphicsSystem(App &app, ShaderManager &shaderManager)
{
	// Initialize graphics system
	if (!app.initialize(shaderManager))
	{
		return false;
	}
	return true;
}

// Window manager functionality is now part of App

bool Init::initializeComponents(App &app)
{
	// Get window from app
	GLFWwindow *window = app.getWindow();

	// Initialize all settings, UI, macOS, and graphics components
	initializeAll(window);

	return true;
}

bool Init::initializeQuad(ShaderQuad &quad)
{
	// Initialize quad
	quad.initialize();
	return true;
}