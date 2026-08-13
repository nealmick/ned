/*
	File: ned.cpp
	Description: Thin standalone host — GLFW/GL/ImGui loop, shaders, Workbench(Fullscreen).
*/

#include "ned.h"

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "imgui_internal.h"
#ifdef IMGUI_ENABLE_FREETYPE
#include "misc/freetype/imgui_freetype.h"
#endif
#ifdef __APPLE__
#include "util/macos_window.h"
#endif

#include <chrono>
#include <filesystem>
#include <iostream>
#include <vector>

// PNG decode only — STB_IMAGE_IMPLEMENTATION lives in welcome.cpp
#include "lib/stb_image.h"

#ifdef PLATFORM_WINDOWS
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>
#endif

Ned::Ned()
#if NED_ENABLE_SHADERS
	: shaderManager(workbench.settings, fb, accum, quad)
#endif
{
}

Ned::~Ned()
{
	if (initialized_)
		cleanup();
}

bool Ned::initialize()
{
	if (!glfwInit())
	{
		std::cerr << "Failed to init GLFW\n";
		return false;
	}
	if (!createWindow())
		return false;
	if (!initializeGLEW())
		return false;

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glGetError();

#if NED_ENABLE_SHADERS
	shaderManager.initializeShaders();
#endif

#ifdef __APPLE__
	{
		float opacity = workbench.settings.settings.value("mac_background_opacity", 0.5f);
		bool blurEnabled = workbench.settings.settings.value("mac_blur_enabled", true);
		setupMacOSApplicationDelegate();
		::configureMacOSWindow(window_, opacity, blurEnabled);
	}
#endif

	if (!initializeImGui())
		return false;

	// Workbench needs ImGui context (created above).
	if (!workbench.initialize(WorkbenchHostMode::Fullscreen))
		return false;

#if NED_ENABLE_SHADERS
	quad.initialize();
	shaderManager.setShaderEnabled(
		workbench.settings.settings.value("shader_toggle", true));
#endif

	glfwSetWindowUserPointer(window_, this);
	glfwSetScrollCallback(window_, Ned::scrollCallback);

	initialized_ = true;
	return true;
}

void Ned::scrollCallback(GLFWwindow *window, double xoffset, double yoffset)
{
	Ned *ned = static_cast<Ned *>(glfwGetWindowUserPointer(window));
	if (!ned)
		return;
	ned->scrollXAccumulator_ += xoffset * 0.2;
	ned->scrollYAccumulator_ += yoffset * 0.2;
}

void Ned::handleScrollAccumulators()
{
	if (scrollXAccumulator_ != 0.0 || scrollYAccumulator_ != 0.0)
	{
		ImGui::GetIO().AddMouseWheelEvent(static_cast<float>(scrollXAccumulator_),
										  static_cast<float>(scrollYAccumulator_));
		scrollXAccumulator_ = 0.0;
		scrollYAccumulator_ = 0.0;
	}
}

bool Ned::initializeImGui()
{
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO &io = ImGui::GetIO();
	io.IniFilename = NULL;
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
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
	// Close (X) uses ButtonHovered / ButtonActive for its fill — keep those grey.
	colors[ImGuiCol_Button] = ImVec4(0.40f, 0.40f, 0.40f, 0.35f);
	colors[ImGuiCol_ButtonHovered] = ImVec4(0.55f, 0.55f, 0.55f, 0.40f);
	colors[ImGuiCol_ButtonActive] = ImVec4(0.45f, 0.45f, 0.45f, 0.55f);
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

	ImGui_ImplGlfw_InitForOpenGL(window_, true);
	ImGui_ImplOpenGL3_Init("#version 330");
	return true;
}

bool Ned::createWindow()
{
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
	glfwWindowHint(GLFW_DECORATED, GLFW_TRUE);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
	glfwWindowHint(GLFW_TRANSPARENT_FRAMEBUFFER, GLFW_FALSE);
	glfwWindowHint(GLFW_COCOA_RETINA_FRAMEBUFFER, GLFW_TRUE);
#else
	glfwWindowHint(GLFW_DECORATED, GLFW_TRUE);
#endif
	glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

	window_ = glfwCreateWindow(1200, 750, "Ned Text Editor", NULL, NULL);

	if (!window_)
	{
		const char *error_description = nullptr;
		glfwGetError(&error_description);

		glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
		glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
		glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_ANY_PROFILE);

		window_ = glfwCreateWindow(1200, 750, "Ned Text Editor", NULL, NULL);
		if (!window_)
		{
			glfwGetError(&error_description);
			glfwTerminate();
			return false;
		}
	}

	setWindowIcon();
	return true;
}

void Ned::setWindowIcon()
{
	if (!window_)
		return;

	namespace fs = std::filesystem;

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
		glfwSetWindowIcon(window_, 1, &image);
		stbi_image_free(pixels);
	}

#ifdef PLATFORM_WINDOWS
	HICON hIcon = static_cast<HICON>(LoadImageW(GetModuleHandleW(nullptr),
												MAKEINTRESOURCEW(1),
												IMAGE_ICON,
												0,
												0,
												LR_DEFAULTSIZE | LR_SHARED));
	if (hIcon)
	{
		HWND hwnd = glfwGetWin32Window(window_);
		if (hwnd)
		{
			SendMessageW(hwnd, WM_SETICON, ICON_BIG, reinterpret_cast<LPARAM>(hIcon));
			SendMessageW(hwnd, WM_SETICON, ICON_SMALL, reinterpret_cast<LPARAM>(hIcon));
		}
	}
#endif
}

bool Ned::initializeGLEW()
{
	glfwMakeContextCurrent(window_);
#ifdef PLATFORM_WINDOWS
	glfwSwapInterval(-1);
	glfwSwapInterval(0);
#else
	glfwSwapInterval(0);
#endif
	glfwSetWindowRefreshCallback(window_, [](GLFWwindow *) { glfwPostEmptyEvent(); });
	glfwSetInputMode(window_, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);

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

void Ned::run()
{
	if (!initialized_)
	{
		std::cerr << "Cannot run: Not initialized" << std::endl;
		return;
	}

	// Frame pacing restored after workbench refactor: settings still expose
	// fps_target / fps_target_unfocused / fps_toggle, but the main loop had been
	// left as glfwPollEvents + swapInterval(0) with no sleep — spinning the GPU
	// as fast as possible (fans, high energy). Cap to the configured FPS.
	using clock = std::chrono::steady_clock;

	while (!glfwWindowShouldClose(window_))
	{
		const auto frameStart = clock::now();

		glfwPollEvents();

#ifdef __APPLE__
		if (::shouldTerminateApplication())
			glfwSetWindowShouldClose(window_, 1);
#endif

		handleScrollAccumulators();
		workbench.tick();
		// Font atlas rebuilds must happen *before* NewFrame (never mid-draw).
		workbench.applySettings();

#if NED_ENABLE_SHADERS
		shaderManager.setShaderEnabled(
			workbench.settings.settings.value("shader_toggle", true));
#endif

		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		const bool currentFocus =
			window_ ? glfwGetWindowAttrib(window_, GLFW_FOCUSED) != 0 : false;
		if (windowFocused_ != currentFocus)
		{
			if (windowFocused_ && !currentFocus)
			{
				if (EditorApi *api = workbench.activeApi())
					api->save();
			}
			windowFocused_ = currentFocus;
		}

#if NED_ENABLE_SHADERS
		int display_w = 0, display_h = 0;
		glfwGetFramebufferSize(window_, &display_w, &display_h);
		shaderManager.initializeFramebuffers(display_w, display_h);
#endif

		renderFrame();

		// Pace to fps_target (focused) / fps_target_unfocused. Wait for events
		// for the remainder of the frame budget so input stays snappy without
		// busy-spinning. fps_toggle false or absurd targets = uncapped.
		const auto &s = workbench.settings.settings;
		const bool fpsToggle = s.value("fps_toggle", true);
		float fpsTarget = windowFocused_ ? s.value("fps_target", 60.0f)
										 : s.value("fps_target_unfocused", 30.0f);
		// Slider allows up to 1000; treat that as "unlimited" like the old path.
		if (fpsToggle && fpsTarget > 0.0f && fpsTarget < 900.0f)
		{
			const auto target = std::chrono::duration_cast<clock::duration>(
				std::chrono::duration<double>(1.0 / static_cast<double>(fpsTarget)));
			const auto deadline = frameStart + target;
			const auto now = clock::now();
			if (now < deadline)
			{
				const double remaining =
					std::chrono::duration<double>(deadline - now).count();
				// Block until an event or the frame budget elapses (macOS-friendly).
				glfwWaitEventsTimeout(remaining);
			}
		}
	}
}

void Ned::renderFrame()
{
	int display_w = 0, display_h = 0;
	glfwGetFramebufferSize(window_, &display_w, &display_h);

	auto &bg = workbench.settings.settings["backgroundColor"];

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

	workbench.render();

	ImGui::Render();
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

#if NED_ENABLE_SHADERS
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glClear(GL_COLOR_BUFFER_BIT);
	shaderManager.renderWithEffects(window_);
#endif
	glfwSwapBuffers(window_);
}

void Ned::cleanup()
{
	if (!initialized_)
		return;

	workbench.cleanup();

#if NED_ENABLE_SHADERS
	quad.cleanup();
	shaderManager.cleanupFramebuffers();
#endif

	if (window_)
	{
		glfwDestroyWindow(window_);
		window_ = nullptr;
	}
	glfwTerminate();
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();
	initialized_ = false;
}
