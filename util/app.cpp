/*
File: app.cpp
Description: Combined application and graphics management implementation for NED text
editor. This class combines the functionality of ApplicationManager and GraphicsManager.
*/

#include "app.h"
#include "../files/files.h"
#include "editor/editor.h"

#include "files/file_tree.h"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "lsp/lsp_client.h"
#include "settings.h"
#if NED_ENABLE_SHADERS
#include "shaders/shader_manager.h"
#include "shaders/shader_types.h"
#endif
#include "util/keybinds.h"
#include "util/ned_terminal.h"
#include "util/settings.h"
#include "util/splitter.h"
#include "util/welcome.h"
#include <filesystem>
#include <iostream>
#include <vector>

// PNG decode only — STB_IMAGE_IMPLEMENTATION lives in welcome.cpp
#include "../lib/stb_image.h"

#ifdef __APPLE__
#include "macos_window.h"
#endif

#ifdef PLATFORM_WINDOWS
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>
#endif

App::App(Settings &settings,
		 Editor &editor,
		 FileExplorer &fileExplorer,
		 LSPClient &lspClient,
#if NED_ENABLE_SHADERS
		 ShaderManager &shaderManager,
		 FramebufferState &fb,
#endif
		 Splitter &splitter,
		 Welcome &welcome,
		 NedTerminal &terminal)
	: settings(settings),
	  editor(editor),
	  fileExplorer(fileExplorer),
	  lspClient(lspClient),
#if NED_ENABLE_SHADERS
	  shaderManager(shaderManager),
	  fb(fb),
#endif
	  splitter(splitter),
	  welcome(welcome),
	  terminal(terminal),
	  window(nullptr),
	  windowFocused(true)
{
}

App::~App()
{
	if (window)
	{
		glfwDestroyWindow(window);
		window = nullptr;
	}
	glfwTerminate();
}

bool App::initialize()
{
	glfwInit();
	createWindow();
	initializeGLEW();

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glGetError();

#if NED_ENABLE_SHADERS
	shaderManager.initializeShaders();
#endif
	settings.keybinds.loadKeybinds();

#ifdef __APPLE__
	{
		float opacity = settings.settings.value("mac_background_opacity", 0.5f);
		bool blurEnabled = settings.settings.value("mac_blur_enabled", true);
		setupMacOSApplicationDelegate();
		::configureMacOSWindow(window, opacity, blurEnabled);
	}
#endif

	initializeImGui();
	// Standalone Ned (default is already false; set explicitly for clarity).
	settings.isEmbedded = false;
	settings.apply(true, editor.api);
	fileExplorer.icons.load();
#if NED_ENABLE_SHADERS
	shaderManager.setShaderEnabled(settings.settings["shader_toggle"].get<bool>());
#endif

	return true;
}

void App::runMainLoop()
{
	while (!glfwWindowShouldClose(window))
	{
		glfwPollEvents();

#ifdef __APPLE__
		if (::shouldTerminateApplication())
			glfwSetWindowShouldClose(window, 1);
#endif

		handleScrollAccumulators();

		handleBackgroundUpdates();

		// Font atlas rebuilds must happen *before* NewFrame (never mid-draw).
		const bool settingsApplied = settings.apply(false, editor.api);
		if (terminal.isStarted() && (settingsApplied || terminal.consumeNeedsFontResync()))
		{
			terminal.reloadTerminalFonts(settings.font.getFontSize());
			settings.font.load(/*clearAtlas=*/false);
		}
#if NED_ENABLE_SHADERS
		shaderManager.setShaderEnabled(settings.settings["shader_toggle"].get<bool>());
#endif

		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		bool currentFocus =
			window ? glfwGetWindowAttrib(window, GLFW_FOCUSED) != 0 : false;
		if (this->windowFocused != currentFocus)
		{
			if (this->windowFocused && !currentFocus)
				editor.api.save();
			this->windowFocused = currentFocus;
		}
		fileExplorer.handleFileDialog();

#if NED_ENABLE_SHADERS
		int display_w, display_h;
		glfwGetFramebufferSize(window, &display_w, &display_h);
		shaderManager.initializeFramebuffers(display_w, display_h);
#endif

		renderFrame();
	}
}

void App::handleScrollAccumulators()
{
	if (scrollXAccumulator != 0.0 || scrollYAccumulator != 0.0)
	{
		ImGui::GetIO().AddMouseWheelEvent(static_cast<float>(scrollXAccumulator),
										  static_cast<float>(scrollYAccumulator));
		scrollXAccumulator = 0.0;
		scrollYAccumulator = 0.0;
	}
}

bool App::initializeImGui()
{
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO &io = ImGui::GetIO();
	io.IniFilename = NULL;
	(void)io;
	ImGui::StyleColorsDark();

	ImGuiStyle &style = ImGui::GetStyle();
	style.FrameRounding = 8.0f;
	style.GrabRounding = 8.0f;
#ifdef PLATFORM_WINDOWS
	style.WindowRounding = 0.0f;
#else
	style.WindowRounding = 12.0f;
#endif
	style.ChildRounding = 8.0f;
	style.PopupRounding = 8.0f;
	style.ScrollbarRounding = 8.0f;
	style.TabRounding = 8.0f;
	style.FramePadding = ImVec2(6.0f, 4.0f);

	ImVec4 *colors = style.Colors;
	colors[ImGuiCol_Button] = ImVec4(0.08f, 0.45f, 0.75f, 1.00f);
	colors[ImGuiCol_ButtonHovered] = ImVec4(0.06f, 0.35f, 0.60f, 1.00f);
	colors[ImGuiCol_ButtonActive] = ImVec4(0.04f, 0.25f, 0.45f, 1.00f);
	colors[ImGuiCol_CheckMark] = ImVec4(0.08f, 0.45f, 0.75f, 1.00f);
	colors[ImGuiCol_FrameBg] = ImVec4(0.95f, 0.95f, 0.95f, 0.30f);
	colors[ImGuiCol_FrameBgHovered] = ImVec4(0.90f, 0.90f, 0.90f, 0.40f);
	colors[ImGuiCol_FrameBgActive] = ImVec4(0.85f, 0.85f, 0.85f, 0.50f);
	colors[ImGuiCol_SliderGrab] = ImVec4(0.08f, 0.45f, 0.75f, 1.00f);
	colors[ImGuiCol_SliderGrabActive] = ImVec4(0.06f, 0.35f, 0.60f, 1.00f);
	colors[ImGuiCol_Header] = ImVec4(0.08f, 0.45f, 0.75f, 0.31f);
	colors[ImGuiCol_HeaderHovered] = ImVec4(0.08f, 0.45f, 0.75f, 0.60f);
	colors[ImGuiCol_HeaderActive] = ImVec4(0.08f, 0.45f, 0.75f, 0.80f);
	colors[ImGuiCol_Tab] = ImVec4(0.08f, 0.45f, 0.75f, 0.31f);
	colors[ImGuiCol_TabHovered] = ImVec4(0.08f, 0.45f, 0.75f, 0.60f);
	colors[ImGuiCol_TabActive] = ImVec4(0.08f, 0.45f, 0.75f, 0.80f);
	colors[ImGuiCol_ScrollbarBg] = ImVec4(0.95f, 0.95f, 0.95f, 0.30f);
	colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.08f, 0.45f, 0.75f, 0.60f);
	colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.08f, 0.45f, 0.75f, 0.80f);
	colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.06f, 0.35f, 0.60f, 1.00f);

	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init("#version 330");
	return true;
}

bool App::createWindow()
{

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
	glfwWindowHint(GLFW_DECORATED, GLFW_TRUE); // Use OS decorations for app bundles
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);	  // macOS specific
	glfwWindowHint(GLFW_TRANSPARENT_FRAMEBUFFER, GLFW_FALSE); // Disable for compatibility
	glfwWindowHint(GLFW_COCOA_RETINA_FRAMEBUFFER, GLFW_TRUE); // Enable retina support
#else
	glfwWindowHint(GLFW_DECORATED, GLFW_TRUE); // Use OS decorations
#endif
	glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

	window = glfwCreateWindow(1200, 750, "NED", NULL, NULL);

	if (!window)
	{

		// Get detailed error for OpenGL 3.3 failure
		const char *error_description;
		glfwGetError(&error_description);

		glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
		glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
		glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_ANY_PROFILE);

		window = glfwCreateWindow(1200, 750, "NED", NULL, NULL);

		if (!window)
		{

			glfwGetError(&error_description);

			glfwTerminate();
			return false;
		}
	}

	// Taskbar / title-bar icon. The .rc resource only covers Explorer's .exe icon;
	// GLFW creates its own window class and does not pick that up automatically.
	setWindowIcon();
	return true;
}

void App::setWindowIcon()
{
	if (!window)
		return;

	namespace fs = std::filesystem;

	// Prefer resources next to the packaged binary; fall back to cwd/dev layout.
	const std::vector<fs::path> candidates = {
		fs::path(Settings::getAppResourcesPath()) / "resources" / "icons" / "ned.png",
		fs::path("resources") / "icons" / "ned.png",
		fs::path("..") / "resources" / "icons" / "ned.png",
	};

	int width = 0;
	int height = 0;
	int channels = 0;
	unsigned char *pixels = nullptr;
	for (const fs::path &path : candidates)
	{
		pixels = stbi_load(path.string().c_str(), &width, &height, &channels, 4);
		if (pixels)
			break;
	}

	if (pixels && width > 0 && height > 0)
	{
		GLFWimage image;
		image.width = width;
		image.height = height;
		image.pixels = pixels;
		glfwSetWindowIcon(window, 1, &image);
		stbi_image_free(pixels);
	}

#ifdef PLATFORM_WINDOWS
	// Also apply the icon embedded in ned.exe (resource ID 1 from ned.rc).
	// Covers cases where the PNG path is missing and helps Win32 title-bar chrome.
	HICON hIcon = static_cast<HICON>(LoadImageW(GetModuleHandleW(nullptr),
												MAKEINTRESOURCEW(1),
												IMAGE_ICON,
												0,
												0,
												LR_DEFAULTSIZE | LR_SHARED));
	if (hIcon)
	{
		HWND hwnd = glfwGetWin32Window(window);
		if (hwnd)
		{
			SendMessageW(hwnd, WM_SETICON, ICON_BIG, reinterpret_cast<LPARAM>(hIcon));
			SendMessageW(hwnd, WM_SETICON, ICON_SMALL, reinterpret_cast<LPARAM>(hIcon));
		}
	}
#endif
}

bool App::initializeGLEW()
{
	glfwMakeContextCurrent(window);
#ifdef PLATFORM_WINDOWS
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

void App::handleBackgroundUpdates()
{
	double currentTime = glfwGetTime();
	if (currentTime - timing.lastSettingsCheck >= SETTINGS_CHECK_INTERVAL)
	{
		settings.checkSettingsFile();
		settings.keybinds.checkKeybindsFile();
		timing.lastSettingsCheck = currentTime;
	}

	if (currentTime - timing.lastFileTreeRefresh >= FILE_TREE_REFRESH_INTERVAL)
	{
		fileExplorer.fileTree.refreshFileTree();
		timing.lastFileTreeRefresh = currentTime;
	}
}

void App::renderFrame()
{
	int display_w, display_h;
	glfwGetFramebufferSize(window, &display_w, &display_h);

	auto &bg = settings.settings["backgroundColor"];

#if NED_ENABLE_SHADERS
	const bool shaderEnabled = shaderManager.isShaderEnabled();
	glBindFramebuffer(GL_FRAMEBUFFER, fb.framebuffer);
	glViewport(0, 0, display_w, display_h);
	const float alpha = shaderEnabled ? bg[3].get<float>() : 1.0f;
	glClearColor(bg[0], bg[1], bg[2], alpha);
	glClear(GL_COLOR_BUFFER_BIT);
#else
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glViewport(0, 0, display_w, display_h);
	glClearColor(bg[0], bg[1], bg[2], 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);
#endif

	renderMainWindow();
	settings.renderSettingsWindow(editor.api, fileExplorer, lspClient);
	lspClient.dashboard.render();
	settings.renderNotification("");

	ImGui::Render();
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

#if NED_ENABLE_SHADERS
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glClear(GL_COLOR_BUFFER_BIT);
	shaderManager.renderWithEffects(window);
#endif
	glfwSwapBuffers(window);
}

void App::renderMainWindow()
{
#ifdef __APPLE__
	// renderTopLeftMenu();
#endif

	settings.keybinds.handleKeyboardShortcuts(
		editor.api, fileExplorer, lspClient, terminal);

	// Keep shell cwd aligned with the open project.
	terminal.setProjectRoot(fileExplorer.projectRoot);

	if (terminal.visible())
	{
		ImGui::PushFont(settings.font.getMainFont());
		ImGui::SetNextWindowPos(ImVec2(0, 0));
		ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);
		ImGui::Begin("Main Window",
					 nullptr,
					 ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
						 ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollWithMouse |
						 ImGuiWindowFlags_NoScrollbar);
		terminal.renderFullscreen();
		ImGui::End();
		ImGui::PopFont();
		return;
	}

	if (fileExplorer.showWelcomeScreen)
	{
		welcome.render();
		return;
	}

	ImGui::PushFont(settings.font.getMainFont());
	ImGui::SetNextWindowPos(ImVec2(0, 0));
	ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);
	ImGui::Begin("Main Window",
				 nullptr,
				 ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
					 ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollWithMouse |
					 ImGuiWindowFlags_NoScrollbar);

	float windowWidth = ImGui::GetWindowWidth();
	float padding = ImGui::GetStyle().WindowPadding.x;
	float availableWidth = windowWidth - padding * 3;

	if (Splitter::showSidebar)
	{
		float leftSplit = settings.settings.value("splitPos", 0.2142857164144516f);
		float explorerWidth = availableWidth * leftSplit;
		float editorWidth = availableWidth - explorerWidth - (padding * 2) + 16.0f;

		fileExplorer.renderFileExplorer(explorerWidth);
		ImGui::SameLine(0, 0);
		splitter.renderSplitter(padding, availableWidth);
		ImGui::SameLine(0, 0);
		editor.renderEditor(settings.font.getMainFont(), editorWidth);
	} else
	{
		float editorWidth = availableWidth + 5.0f;
		editor.renderEditor(settings.font.getMainFont(), editorWidth);
	}
	lspClient.render();
	// Project UI (not editor document chrome) — always, even if sidebar is hidden.
	fileExplorer.renderFileFinder();
	ImGui::End();
	ImGui::PopFont();
}
