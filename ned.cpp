/*
File: ned.cpp
Description: Main application class implementation for NED text editor.
*/
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "imgui_internal.h"
// ned.cpp
#ifdef __APPLE__
#include "macos_window.h"
#endif

#include "../ai/ai_tab.h"
#include "ned.h"

#include "editor/editor_bookmarks.h"
#include "editor/editor_highlight.h"
#include "util/debug_console.h"
#include "util/settings.h"
#include "util/terminal.h"
#include "util/welcome.h"

#include <cstdio>
#include <filesystem>
#include <iostream>
#include <thread>

// global scope
Bookmarks gBookmarks;
bool shader_toggle = false;
bool showSidebar = true;

Ned::Ned()
	: window(nullptr), currentFont(nullptr), needFontReload(false), windowFocused(true),
	  explorerWidth(0.0f), editorWidth(0.0f), initialized(false)
{
}

Ned::~Ned()
{
	if (initialized)
	{
		cleanup();
	}
}

void ApplySettings(ImGuiStyle &style);

void Ned::ShaderQuad::initialize()
{
	float quadVertices[] = {-1.0f, -1.0f, 0.0f, 0.0f, 1.0f,	 -1.0f, 1.0f, 0.0f,
							1.0f,  1.0f,  1.0f, 1.0f, -1.0f, -1.0f, 0.0f, 0.0f,
							1.0f,  1.0f,  1.0f, 1.0f, -1.0f, 1.0f,	0.0f, 1.0f};

	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);
	glBindVertexArray(VAO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void *)0);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void *)(2 * sizeof(float)));
	glEnableVertexAttribArray(1);
}

void Ned::ShaderQuad::cleanup()
{
	glDeleteVertexArrays(1, &VAO);
	glDeleteBuffers(1, &VBO);
}
bool Ned::initialize()
{
	if (!initializeGraphics())
	{
		return false;
	}
	gSettings.loadSettings();


	
#ifdef __APPLE__
	float opacity = gSettings.getSettings().value("mac_background_opacity", 0.5f);
	bool blurEnabled = gSettings.getSettings().value("mac_blur_enabled", true);
	
	// Initial configuration
	configureMacOSWindow(window, opacity, blurEnabled);
	
	// Initialize tracking variables
	lastOpacity = opacity;
	lastBlurEnabled = blurEnabled;
#endif

	// Rest of initialization...
	glfwSetWindowUserPointer(window, this);
	initializeImGui();
	initializeResources();
	quad.initialize();
	initialized = true;
	return true;
}
void Ned::renderResizeHandles()
{
	const float resizeBorder = 10.0f;
	ImVec2 windowPos = ImGui::GetMainViewport()->Pos;
	ImVec2 windowSize = ImGui::GetMainViewport()->Size;

	// Right edge

	ImGui::PushID("resize_right");
	ImGui::SetCursorScreenPos(ImVec2(windowPos.x + windowSize.x - resizeBorder, windowPos.y));
	ImGui::InvisibleButton("##resize_right", ImVec2(resizeBorder, windowSize.y));
	bool hoverRight = ImGui::IsItemHovered();
	ImGui::PopID();

	// Bottom edge
	ImGui::PushID("resize_bottom");
	ImGui::SetCursorScreenPos(ImVec2(windowPos.x, windowPos.y + windowSize.y - resizeBorder));
	ImGui::InvisibleButton("##resize_bottom", ImVec2(windowSize.x, resizeBorder));
	bool hoverBottom = ImGui::IsItemHovered();
	ImGui::PopID();

	// Corner
	ImGui::PushID("resize_corner");
	ImGui::SetCursorScreenPos(ImVec2(windowPos.x + windowSize.x - resizeBorder,
									 windowPos.y + windowSize.y - resizeBorder));
	ImGui::InvisibleButton("##resize_corner", ImVec2(resizeBorder, resizeBorder));
	bool hoverCorner = ImGui::IsItemHovered();
	ImGui::PopID();

	// Update cursors based on hover state first
	if (hoverCorner || resizingCorner)
	{
		ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeNWSE);
	} else if (hoverRight || resizingRight)
	{
		ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);
	} else if (hoverBottom || resizingBottom)
	{
		ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeNS);
	}
}

void Ned::handleManualResizing()
{
	ImGuiIO &io = ImGui::GetIO();
	int screenWidth, screenHeight;
	glfwGetWindowSize(window, &screenWidth, &screenHeight);

	double mouseX, mouseY;
	glfwGetCursorPos(window, &mouseX, &mouseY);

	const float resizeBorder = 10.0f;
	bool hoverRight = mouseX >= (screenWidth - resizeBorder);
	bool hoverBottom = mouseY >= (screenHeight - resizeBorder);
	bool hoverCorner = hoverRight && hoverBottom;

	// Handle drag start
	if (ImGui::IsMouseClicked(ImGuiMouseButton_Left))
	{
		resizingRight = hoverRight || hoverCorner;
		resizingBottom = hoverBottom || hoverCorner;
		resizingCorner = hoverCorner;

		if (resizingRight || resizingBottom || resizingCorner)
		{
			dragStart = ImVec2(mouseX, mouseY);
			initialWindowSize = ImVec2(screenWidth, screenHeight);
		}
	}

	// Handle dragging
	if (ImGui::IsMouseDragging(ImGuiMouseButton_Left))
	{
		if (resizingRight || resizingBottom || resizingCorner)
		{
			int newWidth = screenWidth;
			int newHeight = screenHeight;

			if (resizingRight || resizingCorner)
				newWidth = initialWindowSize.x + (mouseX - dragStart.x);
			if (resizingBottom || resizingCorner)
				newHeight = initialWindowSize.y + (mouseY - dragStart.y);

			newWidth = std::max(newWidth, 100);
			newHeight = std::max(newHeight, 100);

			glfwSetWindowSize(window, newWidth, newHeight);
		}
	}

	// Handle drag end
	if (ImGui::IsMouseReleased(ImGuiMouseButton_Left))
	{
		resizingRight = false;
		resizingBottom = false;
		resizingCorner = false;
	}
}

bool Ned::initializeGraphics()
{
	if (!glfwInit())
	{
		std::cerr << "Failed to initialize GLFW" << std::endl;
		return false;
	}
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
	glfwWindowHint(GLFW_DECORATED, GLFW_FALSE); // No OS decorations for macOS (custom handling)
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // macOS specific
	glfwWindowHint(GLFW_TRANSPARENT_FRAMEBUFFER, GLFW_TRUE);
#else // For Linux/Ubuntu and other non-Apple platforms
	glfwWindowHint(GLFW_DECORATED, GLFW_TRUE); // Use OS decorations
#endif
	// GLFW_RESIZABLE should be TRUE for both.
	// On Linux, the OS window manager handles resizing if DECORATED is TRUE.
	// On macOS, your custom logic handles resizing.
	glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

	window = glfwCreateWindow(1200, 750, "NED", NULL, NULL);

	if (!window)
	{
		std::cerr << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		return false;
	}

	glfwMakeContextCurrent(window);
	glfwSwapInterval(1);
	glfwSetWindowRefreshCallback(window, [](GLFWwindow *window) { glfwPostEmptyEvent(); });

	glewExperimental = GL_TRUE;
	if (GLenum err = glewInit(); GLEW_OK != err)
	{
		std::cerr << "🔴 GLEW initialization failed: " << glewGetErrorString(err) << std::endl;
		glfwTerminate();
		return false;
	}
	glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); 
	glGetError(); // Clear any GLEW startup errors

	// Load both shaders
	if (!crtShader.loadShader("shaders/vertex.glsl", "shaders/fragment.glsl") ||
		!accum.burnInShader.loadShader("shaders/vertex.glsl", "shaders/burn_in.frag"))
	{
		std::cerr << "🔴 Shader load failed" << std::endl;
		glfwTerminate();
		return false;
	}

	return true;
}

void Ned::initializeImGui()
{
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO &io = ImGui::GetIO();
	(void)io;
	ImGui::StyleColorsDark();
	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init("#version 330");
	glfwSetScrollCallback(window, Ned::scrollCallback);
}

void Ned::scrollCallback(GLFWwindow *window, double xoffset, double yoffset)
{
	Ned *app = static_cast<Ned *>(glfwGetWindowUserPointer(window));
	app->scrollXAccumulator += xoffset * 0.3; // Same multiplier as vertical
	app->scrollYAccumulator += yoffset * 0.3;
}
// In Ned::initializeResources()
void Ned::initializeResources()
{
	gDebugConsole.toggleVisibility();
	gEditorHighlight.setTheme(gSettings.getCurrentTheme());

	// Save original font size
	float originalFontSize = gSettings.getSettings()["fontSize"].get<float>();

	// Temporarily force font size to 19.0 for proper terminal initialization
	gSettings.getSettings()["fontSize"] = 19.0f;

	// Apply settings with temporary font size
	ApplySettings(ImGui::GetStyle());
	currentFont = loadFont(gSettings.getCurrentFont(), 19.0f);

	// Restore original font size
	gSettings.getSettings()["fontSize"] = originalFontSize;
	needFontReload = true;
	handleFontReload(); // This will reload fonts with original size

	// Continue with remaining initialization
	shader_toggle = gSettings.getSettings()["shader_toggle"].get<bool>();
	gFileExplorer.loadIcons();

	if (!currentFont)
	{
		std::cerr << "🔴 Failed to load font, using default font" << std::endl;
		currentFont = ImGui::GetIO().Fonts->AddFontDefault();
	}
}

float Ned::clamp(float value, float min, float max)
{
	if (value < min)
		return min;
	if (value > max)
		return max;
	return value;
}

ImFont *Ned::loadFont(const std::string &fontName, float fontSize)
{
	ImGuiIO &io = ImGui::GetIO();

	// Build the path from .app/Contents/Resources/fonts/
	std::string resourcePath = Settings::getAppResourcesPath();
	std::string fontPath = resourcePath + "/fonts/" + fontName + ".ttf";
	// Always print the path, before existence check
	// std::cout << "[Ned::loadFont] Attempting to load font from: " << fontPath << " at size "
	//		  << fontSize << std::endl;

	if (!std::filesystem::exists(fontPath))
	{
		std::cerr << "[Ned::loadFont] Font does not exist: " << fontPath << std::endl;
		return io.Fonts->AddFontDefault();
	}

	static const ImWchar ranges[] = {
		0x0020, 0x00FF, // Basic Latin + Latin Supplement
		0x2500, 0x257F, // Box Drawing Characters
		0x2580, 0x259F, // Block Elements
		0x25A0, 0x25FF, // Geometric Shapes
		0x2600, 0x26FF, // Miscellaneous Symbols
		0x2700, 0x27BF, // Dingbats
		0x2900, 0x297F, // Supplemental Arrows-B
		0x2B00, 0x2BFF, // Miscellaneous Symbols and Arrows
		0x3000, 0x303F, // CJK Symbols and Punctuation
		0xE000, 0xE0FF, // Private Use Area
		0,
	};
	ImFontConfig config_main;
	config_main.MergeMode = false;
	config_main.GlyphRanges = ranges;

	// Clear existing fonts if you want a single font each time, or
	// if you want to stack multiple fonts, you might skip clearing.
	io.Fonts->Clear();

	ImFont *font = io.Fonts->AddFontFromFileTTF(fontPath.c_str(), fontSize, &config_main, ranges);

	// Merge DejaVu Sans for Braille, etc. if you want
	ImFontConfig config_braille;
	config_braille.MergeMode = true;
	static const ImWchar braille_ranges[] = {0x2800, 0x28FF, 0};
	std::string dejaVuPath = resourcePath + "/fonts/DejaVuSans.ttf";
	if (std::filesystem::exists(dejaVuPath))
	{
		io.Fonts->AddFontFromFileTTF(dejaVuPath.c_str(), fontSize, &config_braille, braille_ranges);
	}

	if (!font)
	{
		std::cerr << "[Ned::loadFont] Failed to load font: " << fontPath << std::endl;
		return io.Fonts->AddFontDefault();
	}

	// After adding new fonts, re-create the OpenGL font texture
	ImGui_ImplOpenGL3_DestroyFontsTexture();
	ImGui_ImplOpenGL3_CreateFontsTexture();

	// std::cout << "[Ned::loadFont] Successfully loaded font: " << fontName << " from " << fontPath
	//		  << " at size " << fontSize << std::endl;

	return font;
}
void Ned::handleWindowDragging()
{
	if (isDraggingWindow)
	{
		if (ImGui::IsMouseDown(ImGuiMouseButton_Left))
		{
			// Get current mouse position in screen coordinates
			double currentMouseX, currentMouseY;
			glfwGetCursorPos(window, &currentMouseX, &currentMouseY);
			int windowX, windowY;
			glfwGetWindowPos(window, &windowX, &windowY);
			ImVec2 currentScreenPos(windowX + currentMouseX, windowY + currentMouseY);

			// Calculate delta from initial position
			ImVec2 delta(currentScreenPos.x - dragStartMousePos.x,
						 currentScreenPos.y - dragStartMousePos.y);

			// Update window position
			glfwSetWindowPos(window,
							 static_cast<int>(dragStartWindowPos.x + delta.x),
							 static_cast<int>(dragStartWindowPos.y + delta.y));
		} else
		{
			isDraggingWindow = false;
		}
	}
}

void Ned::renderTopLeftMenu()
{
	const float padding = 1.0f;
	const float iconSize = 14.0f;
	const float spacing = 4.0f;
	static bool wasMenuOpen = false;
	static bool closeHovered = false, minimizeHovered = false, maximizeHovered = false;
	controllsDisplayFrame += 1;
	// Get absolute screen coordinates
	ImGuiViewport *viewport = ImGui::GetMainViewport();
	ImVec2 screenPos = viewport->Pos;
	ImVec2 mousePos = ImGui::GetIO().MousePos;

	// Calculate button position
	ImVec2 buttonPos = ImVec2(screenPos.x + padding, screenPos.y + padding);
	ImRect buttonRect(buttonPos, ImVec2(buttonPos.x + 120, buttonPos.y + 40));

	// Draw visible button (transparent hitbox)
	ImDrawList *draw_list = ImGui::GetForegroundDrawList();
	draw_list->AddRectFilled(buttonRect.Min, buttonRect.Max, IM_COL32(0, 0, 0, 0));

	// Check hover state manually
	bool isHovered = buttonRect.Contains(mousePos);
	if (controllsDisplayFrame < 120)
	{
		isHovered = true;
	}
	// isHovered = true;
	//  Update hover state
	static bool menuOpen = false;
	if (isHovered)
	{
		menuOpen = true;
		ImGui::SetNextWindowPos(buttonPos);
	}

	// Show popup when hovered
	if (menuOpen)
	{
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8, 6));
		ImGui::PushStyleColor(ImGuiCol_PopupBg, IM_COL32(80, 80, 80, 255));

		// Custom rounding for bottom-right corner only
		const float rounding = 8.0f;
		ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, rounding);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);

		if (ImGui::Begin("TopMenu",
						 nullptr,
						 ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
							 ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings))
		{
			// Set window size and force bottom-right rounding
			ImGui::SetWindowSize(ImVec2(120, 40));
			if (controllsDisplayFrame < 120)
			{
				isHovered = true;
				ImGui::BringWindowToDisplayFront(ImGui::GetCurrentWindow());
			}

			//  Manual background draw
			ImDrawList *bg_draw_list = ImGui::GetWindowDrawList();
			ImVec2 p_min = ImGui::GetWindowPos();
			ImVec2 p_max = ImVec2(p_min.x + 120, p_min.y + 40);
			bg_draw_list->AddRectFilled(p_min,
										p_max,
										IM_COL32(80, 80, 80, 255), // Dark grey fill
										rounding,
										ImDrawFlags_RoundCornersBottomRight);

			// Add grey border
			bg_draw_list->AddRect(p_min,
								  p_max,
								  IM_COL32(120, 120, 120, 255), // Border color (light grey)
								  rounding,
								  ImDrawFlags_RoundCornersBottomRight,
								  1.0f // Border thickness (1 pixel)
			);

			// Centered icons group
			ImGui::SetCursorPosY((ImGui::GetWindowHeight() - iconSize) * 0.5f);
			ImGui::SetCursorPosX((ImGui::GetWindowWidth() - (iconSize * 3 + spacing * 2)) * 0.35f);

			ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(0, 0, 0, 0));
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, IM_COL32(0, 0, 0, 0));
			ImGui::PushStyleColor(ImGuiCol_ButtonActive, IM_COL32(0, 0, 0, 0));

			// Close button
			ImTextureID closeIcon =
				gFileExplorer.getIcon(closeHovered ? "close-mac-hover" : "close-mac");
			if (ImGui::ImageButton("##Close", closeIcon, ImVec2(iconSize, iconSize)))
			{
				glfwSetWindowShouldClose(window, GLFW_TRUE); // Close the window
				std::cout << "Clicked close icon" << std::endl;
			}
			closeHovered = ImGui::IsItemHovered();

			ImGui::SameLine(0, spacing);

			// Minimize button
			ImTextureID minimizeIcon =
				gFileExplorer.getIcon(minimizeHovered ? "minimize-mac-hover" : "minimize-mac");
			if (ImGui::ImageButton("##Min", minimizeIcon, ImVec2(iconSize, iconSize)))
			{
				glfwIconifyWindow(window);
				std::cout << "Clicked minimize icon" << std::endl;
			}
			minimizeHovered = ImGui::IsItemHovered();

			ImGui::SameLine(0, spacing);

			// Maximize button
			ImTextureID maximizeIcon =
				gFileExplorer.getIcon(maximizeHovered ? "maximize-mac-hover" : "maximize-mac");
			if (ImGui::ImageButton("##Max", maximizeIcon, ImVec2(iconSize, iconSize)))
			{
				static bool isFullscreen = false;
				static int prevWidth, prevHeight, prevX, prevY;

				if (!isFullscreen)
				{
					// Store current window position and size
					glfwGetWindowPos(window, &prevX, &prevY);
					glfwGetWindowSize(window, &prevWidth, &prevHeight);

					// Get primary monitor resolution
					GLFWmonitor *monitor = glfwGetPrimaryMonitor();
					const GLFWvidmode *mode = glfwGetVideoMode(monitor);

					// Switch to fullscreen
					glfwSetWindowMonitor(
						window, monitor, 0, 0, mode->width, mode->height, mode->refreshRate);
					isFullscreen = true;
				} else
				{
					// Restore window
					glfwSetWindowMonitor(
						window, nullptr, prevX, prevY, prevWidth, prevHeight, GLFW_DONT_CARE);
					isFullscreen = false;
				}
			}
			maximizeHovered = ImGui::IsItemHovered();

			ImGui::PopStyleColor(3);

			// Close logic
			if (!ImGui::IsWindowHovered() && !isHovered)
			{
				menuOpen = false;
			}

			// Window dragging logic
			bool isClicked =
				ImGui::IsWindowHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Left);
			if (isClicked)
			{
				std::cout << "Dragging window...." << std::endl;
				isDraggingWindow = true;

				// Store initial positions
				double mouseX, mouseY;
				glfwGetCursorPos(window, &mouseX, &mouseY);
				int winX, winY;
				glfwGetWindowPos(window, &winX, &winY);
				dragStartMousePos = ImVec2(winX + mouseX, winY + mouseY);
				dragStartWindowPos = ImVec2(winX, winY);

				ImGui::ResetMouseDragDelta(ImGuiMouseButton_Left);
			}

			ImGui::End();
			wasMenuOpen = true;
		} else
		{
			wasMenuOpen = false;
		}

		// Cleanup styles
		ImGui::PopStyleVar(2);
		ImGui::PopStyleColor();
		ImGui::PopStyleVar();
	} else
	{
		// Reset hover states when menu closes
		if (wasMenuOpen)
		{
			closeHovered = minimizeHovered = maximizeHovered = false;
			wasMenuOpen = false;
		}
	}
}

void Ned::run()
{
	if (!initialized)
	{
		std::cerr << "Cannot run: Not initialized" << std::endl;
		return;
	}

	while (!glfwWindowShouldClose(window))
	{
		auto frame_start = std::chrono::high_resolution_clock::now();

		// Handle events and updates
		handleEvents();
		handleWindowDragging();
		double currentTime = glfwGetTime();
		handleBackgroundUpdates(currentTime);

		// Handle framebuffer updates
		int display_w, display_h;
		glfwGetFramebufferSize(window, &display_w, &display_h);
		handleFramebuffer(display_w, display_h);

		// Setup ImGui frame and state
		setupImGuiFrame();
		handleWindowFocus();
		handleSettingsChanges();
		handleFileDialog();

		// Render frame
		renderFrame();
		handleFontReload();
		// Frame timing
		handleFrameTiming(frame_start);
	}
}
void Ned::handleEvents()
{
	// Handle scroll accumulators at the start
	if (scrollXAccumulator != 0.0 || scrollYAccumulator != 0.0)
	{
		ImGui::GetIO().MouseWheel += scrollYAccumulator;  // Vertical
		ImGui::GetIO().MouseWheelH += scrollXAccumulator; // Horizontal
		scrollXAccumulator = 0.0;
		scrollYAccumulator = 0.0;
	}

	// Always poll events if resizing
	if (resizingRight || resizingBottom || resizingCorner)
	{
	} else if (glfwGetWindowAttrib(window, GLFW_FOCUSED))
	{
		glfwPollEvents();
	} else
	{
		glfwWaitEventsTimeout(0.016);
	}
}
void Ned::handleBackgroundUpdates(double currentTime)
{
	if (currentTime - timing.lastSettingsCheck >= SETTINGS_CHECK_INTERVAL)
	{
		gSettings.checkSettingsFile();
		timing.lastSettingsCheck = currentTime;
	}

	if (currentTime - timing.lastFileTreeRefresh >= FILE_TREE_REFRESH_INTERVAL)
	{
		gFileTree.refreshFileTree(); // Changed from gFileExplorer to gFileTree
		timing.lastFileTreeRefresh = currentTime;
	}
}

void Ned::handleFramebuffer(int width, int height)
{
	auto initFB = [](FramebufferState &fb, int w, int h) {
		if (fb.initialized && w == fb.last_display_w && h == fb.last_display_h)
			return;

		// Delete old resources if they exist
		if (fb.initialized)
		{
			glDeleteFramebuffers(1, &fb.framebuffer);
			glDeleteTextures(1, &fb.renderTexture);
			glDeleteRenderbuffers(1, &fb.rbo);
		}

		// Create new framebuffer
		glGenFramebuffers(1, &fb.framebuffer);
		glBindFramebuffer(GL_FRAMEBUFFER, fb.framebuffer);

		// Create texture
		glGenTextures(1, &fb.renderTexture);
		glBindTexture(GL_TEXTURE_2D, fb.renderTexture);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, w, h, 0, GL_RGBA, GL_FLOAT, NULL);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glFramebufferTexture2D(
			GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, fb.renderTexture, 0);

		// Create renderbuffer
		glGenRenderbuffers(1, &fb.rbo);
		glBindRenderbuffer(GL_RENDERBUFFER, fb.rbo);
		glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, w, h);
		glFramebufferRenderbuffer(GL_FRAMEBUFFER,
								  GL_DEPTH_STENCIL_ATTACHMENT,
								  GL_RENDERBUFFER,
								  fb.rbo);

		// Check completeness
		if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		{
			std::cerr << "🔴 Framebuffer not complete!" << std::endl;
		}

		fb.last_display_w = w;
		fb.last_display_h = h;
		fb.initialized = true;

		glBindFramebuffer(GL_FRAMEBUFFER, fb.framebuffer);
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);
		if (fb.renderTexture)
		{
			glBindTexture(GL_TEXTURE_2D, fb.renderTexture);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
		}
	};

	initFB(fb, width, height);
	initFB(accum.accum[0], width, height);
	initFB(accum.accum[1], width, height);

	// Add debug checks after initialization
	glBindFramebuffer(GL_FRAMEBUFFER, accum.accum[0].framebuffer);
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
	{
		std::cerr << "🔴 Accumulation buffer 0 incomplete!" << std::endl;
	}
};

void Ned::setupImGuiFrame()
{
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplGlfw_NewFrame();

	ImGui::NewFrame();
}

void Ned::handleWindowFocus()
{
	bool currentFocus = glfwGetWindowAttrib(window, GLFW_FOCUSED) != 0;
	if (windowFocused && !currentFocus)
	{
		gFileExplorer.saveCurrentFile();
	}
	windowFocused = currentFocus;
}
void Ned::handleKeyboardShortcuts()
{
	ImGuiIO &io = ImGui::GetIO();
	// Accept either Ctrl or Super (Command on macOS)
	bool modPressed = io.KeyCtrl || io.KeySuper;

	if (modPressed && ImGui::IsKeyPressed(ImGuiKey_S, false))
	{
		showSidebar = !showSidebar; // Toggle sidebar visibility
		std::cout << "Toggled sidebar visibility" << std::endl;
	}
	if (modPressed && ImGui::IsKeyPressed(ImGuiKey_T, false))
	{
		gTerminal.toggleVisibility();
		gFileExplorer.saveCurrentFile();
		if (gTerminal.isTerminalVisible())
		{
			ClosePopper::closeAll();
		}
	}
	if (modPressed && ImGui::IsKeyPressed(ImGuiKey_Comma, false))
	{
		gFileExplorer.showWelcomeScreen = false;
		gSettings.toggleSettingsWindow();
	}

	if (modPressed)
	{
		if (ImGui::IsKeyPressed(ImGuiKey_Equal))
		{ // '+' key
			float currentSize = gSettings.getFontSize();
			gSettings.setFontSize(currentSize + 2.0f);
			editor_state.ensure_cursor_visible.vertical = true;
			editor_state.ensure_cursor_visible.horizontal = true;
			std::cout << "Cmd++: Font size increased to " << gSettings.getFontSize() << std::endl;
		} else if (ImGui::IsKeyPressed(ImGuiKey_Minus))
		{ // '-' key
			float currentSize = gSettings.getFontSize();
			gSettings.setFontSize(std::max(currentSize - 2.0f, 8.0f));
			editor_state.ensure_cursor_visible.vertical = true;
			editor_state.ensure_cursor_visible.horizontal = true;
			std::cout << "Cmd+-: Font size decreased to " << gSettings.getFontSize() << std::endl;
		}
	}

	if (modPressed && ImGui::IsKeyPressed(ImGuiKey_Slash, false))
	{
		ClosePopper::closeAll();
		gFileExplorer.showWelcomeScreen = !gFileExplorer.showWelcomeScreen;
		if (gTerminal.isTerminalVisible())
		{
			gTerminal.toggleVisibility();
		}
		gFileExplorer.saveCurrentFile();
	}
	if (modPressed && ImGui::IsKeyPressed(ImGuiKey_O, false))
	{
		std::cout << "triggering file dialog" << std::endl;
		ClosePopper::closeAll();
		gFileExplorer.showWelcomeScreen = false;
		gFileExplorer.saveCurrentFile();
		gFileExplorer._showFileDialog = true;
	}
}

void Ned::renderFileExplorer(float explorerWidth)
{
	ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 0.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_ChildBorderSize, 1.0f);
	ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.5f, 0.5f, 0.5f, 0.0f));
	ImGui::BeginChild("File Explorer",
					  ImVec2(explorerWidth, -1),
					  true,
					  ImGuiWindowFlags_NoScrollbar);

	if (!gFileExplorer.selectedFolder.empty())
	{
		gFileTree.displayFileTree(gFileTree.rootNode); // Changed to use gFileTree
	}
	ImGui::EndChild();
	ImGui::PopStyleColor();
	ImGui::PopStyleVar(2);
}

std::string Ned::truncateFilePath(const std::string &path, float maxWidth, ImFont *font)
{
	if (path.empty())
	{
		return "";
	}

	fs::path p(path);
	std::string root_part;
	std::vector<std::string> components;

	// Split into root and components
	if (p.has_root_path())
	{
		root_part = p.root_path().string();
		// Iterate over components after the root
		for (auto it = ++p.begin(); it != p.end(); ++it)
		{
			if (!it->empty())
			{
				components.push_back(it->string());
			}
		}
	} else
	{
		for (const auto &part : p)
		{
			if (!part.empty())
			{
				components.push_back(part.string());
			}
		}
	}

	if (components.empty())
	{
		return root_part.empty() ? path : root_part;
	}

	// Check full path first
	std::string fullPath =
		root_part + std::accumulate(components.begin(),
									components.end(),
									std::string(),
									[](const std::string &a, const std::string &b) {
										return a.empty() ? b : a + "/" + b;
									});
	if (ImGui::CalcTextSize(fullPath.c_str()).x <= maxWidth)
	{
		return fullPath;
	}

	// Try removing directories from the front
	for (size_t start = 0; start < components.size(); ++start)
	{
		std::string candidate;

		if (start == 0)
		{
			candidate = fullPath;
		} else
		{
			std::string middle =
				".../" + std::accumulate(components.begin() + start,
										 components.end(),
										 std::string(),
										 [](const std::string &a, const std::string &b) {
											 return a.empty() ? b : a + "/" + b;
										 });
			candidate = root_part + middle;
		}

		float width = ImGui::CalcTextSize(candidate.c_str()).x;
		if (width <= maxWidth)
		{
			return candidate;
		}
	}

	// Only filename left (with root if applicable)
	std::string filename = root_part + components.back();
	if (ImGui::CalcTextSize(filename.c_str()).x <= maxWidth)
	{
		return filename;
	}

	// Truncate filename
	std::string truncated = components.back();
	int maxLength = truncated.length();
	while (maxLength > 0)
	{
		std::string temp = truncated.substr(0, maxLength) + "...";
		std::string candidate = root_part + temp;
		if (ImGui::CalcTextSize(candidate.c_str()).x <= maxWidth)
		{
			return candidate;
		}
		maxLength--;
	}

	// Minimum case
	return root_part + "...";
}
void Ned::renderEditorHeader(ImFont *currentFont)
{
	ImGui::BeginGroup();
	ImGui::PushFont(currentFont);

	// Determine the base icon size (equal to font size)
	float iconSize = ImGui::GetFontSize();
	std::string currentFile = gFileExplorer.currentFile;

	// Render left side (file icon and name)
	if (currentFile.empty())
	{
		ImGui::Text("Editor - No file selected");
	} else
	{
		ImTextureID fileIcon = gFileExplorer.getIconForFile(currentFile);
		if (fileIcon)
		{
			// Vertical centering for file icon
			float textHeight = ImGui::GetTextLineHeight();
			float iconTopY = ImGui::GetCursorPosY() + (textHeight - iconSize) * 0.5f;
			ImGui::SetCursorPosY(iconTopY);
			ImGui::Image(fileIcon, ImVec2(iconSize, iconSize));
			ImGui::SameLine();
		}

		// Calculate available width for the text
		float x_cursor = ImGui::GetCursorPosX();
		float x_right_group = ImGui::GetWindowWidth();
		float available_width = x_right_group - x_cursor - ImGui::GetStyle().ItemSpacing.x - 60.0f;

		// Truncate the file path to fit available space
		std::string truncatedText =
			truncateFilePath(currentFile, available_width, ImGui::GetFont());

		ImGui::Text("%s", truncatedText.c_str());
	}

	// Right-aligned status area
	const float rightPadding = 25.0f;							// Space from window edge
	const float totalStatusWidth = iconSize * 2 + rightPadding; // Brain + Gear icons

	// Position at far right edge
	ImGui::SameLine(ImGui::GetWindowWidth() - totalStatusWidth);

	// Status group
	ImGui::BeginGroup();
	{
		// Vertical centering
		float textHeight = ImGui::GetTextLineHeight();
		ImGui::SetCursorPosY(ImGui::GetCursorPosY() + (textHeight - iconSize) * 0.5f);

		// Brain icon (only visible when active)
		if (gAITab.request_active)
		{
			ImGui::Image(gFileExplorer.getIcon("brain"), ImVec2(iconSize, iconSize));
		} else
		{
			// Invisible placeholder to maintain layout
			ImGui::InvisibleButton("##brain-placeholder", ImVec2(iconSize, iconSize));
		}
		ImGui::SameLine();

		// Settings icon (always in same position)
		renderSettingsIcon(iconSize * 0.8f);
	}
	ImGui::EndGroup();

	ImGui::PopFont();
	ImGui::EndGroup();
	ImGui::Separator();
}

void Ned::renderSettingsIcon(float iconSize)
{
	bool settingsOpen = gSettings.showSettingsWindow;
	ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));
	ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0, 0, 0, 0));
	ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0, 0, 0, 0));

	// Vertical centering
	float textHeight = ImGui::GetTextLineHeight();
	float iconTopY = ImGui::GetCursorPosY() + (textHeight - iconSize) * 0.5f;
	ImGui::SetCursorPosY(iconTopY);

	if (!settingsOpen)
	{
		ImVec2 cursor_pos = ImGui::GetCursorPos();
		if (ImGui::InvisibleButton("##gear-hitbox", ImVec2(iconSize, iconSize)))
		{
			gSettings.toggleSettingsWindow();
		}
		bool isHovered = ImGui::IsItemHovered();
		ImGui::SetCursorPos(cursor_pos);
		ImTextureID icon =
			isHovered ? gFileExplorer.getIcon("gear-hover") : gFileExplorer.getIcon("gear");
		ImGui::Image(icon, ImVec2(iconSize, iconSize));
	} else
	{
		ImGui::Image(gFileExplorer.getIcon("gear"), ImVec2(iconSize, iconSize));
	}

	ImGui::PopStyleColor(3);
	ImGui::PopStyleVar();
}

void Ned::renderSplitter(float padding, float availableWidth)
{
	ImGui::SameLine(0, 0);

	// Interaction settings
	const float visible_width = 1.0f;	// Rendered width at rest
	const float hover_width = 6.0f;		// Invisible hitbox size
	const float hover_expansion = 3.0f; // Expanded visual width
	const float hover_delay = 0.15f;	// Seconds before hover effect

	ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(0, 0, 0, 0));
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, IM_COL32(0, 0, 0, 0));
	ImGui::PushStyleColor(ImGuiCol_ButtonActive, IM_COL32(0, 0, 0, 0));

	// Create invisible button with larger hitbox
	ImGui::Button("##vsplitter", ImVec2(hover_width, -1));

	// Hover delay logic
	static float hover_start_time = -1.0f;
	bool visual_hover = false;
	const bool is_hovered = ImGui::IsItemHovered();
	const bool is_active = ImGui::IsItemActive();

	if (is_hovered && !is_active)
	{
		if (hover_start_time < 0)
		{
			hover_start_time = ImGui::GetTime();
		}
		float elapsed = ImGui::GetTime() - hover_start_time;
		visual_hover = elapsed >= hover_delay;
	} else
	{
		hover_start_time = -1.0f;
	}

	// Calculate dimensions
	ImVec2 min = ImGui::GetItemRectMin();
	ImVec2 max = ImGui::GetItemRectMax();
	const float width = (visual_hover || is_active) ? hover_expansion : visible_width;

	// Center the visible splitter in the hover zone
	min.x += (hover_width - width) * 0.5f;
	max.x = min.x + width;

	// Color setup
	const ImU32 color_base = IM_COL32(134, 134, 134, 140);
	const ImU32 color_hover = IM_COL32(13, 110, 253, 255);
	const ImU32 color_active = IM_COL32(11, 94, 215, 255);

	// Determine color
	ImU32 current_color = color_base;
	if (is_active)
	{
		current_color = color_active;
	} else if (visual_hover)
	{
		current_color = color_hover;
	}

	// Draw the splitter
	ImGui::GetWindowDrawList()->AddRectFilled(min, max, current_color);

	// Handle dragging
	if (is_active)
	{
		const float mouse_x = ImGui::GetMousePos().x - ImGui::GetWindowPos().x;
		const float new_split = (mouse_x - padding * 2) / (availableWidth - padding * 4 - 6);
		gSettings.setSplitPos(clamp(new_split, 0.1f, 0.9f));
	}

	ImGui::PopStyleColor(3);
}

void Ned::renderEditor(ImFont *currentFont, float editorWidth)
{
	ImGui::SameLine(0, 0);
	ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 0.0f);
	ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.2f, 0.2f, 0.2f, 0.0f));

	ImGui::BeginChild("Editor", ImVec2(editorWidth, -1), true);
	renderEditorHeader(currentFont);
	gFileExplorer.renderFileContent();
	ImGui::EndChild();

	ImGui::PopStyleColor();
	ImGui::PopStyleVar();
}
void Ned::renderMainWindow()
{

#ifdef __APPLE__
	renderTopLeftMenu();
#endif

	handleKeyboardShortcuts();

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

	ImGui::PushFont(currentFont);
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
	float availableWidth = windowWidth - padding * 3 - 4.0f; // Account for splitter width

	if (showSidebar)
	{
		explorerWidth = availableWidth * gSettings.getSplitPos();
		editorWidth = availableWidth - explorerWidth - 4.0f;

		// Render elements in correct z-order
		renderFileExplorer(explorerWidth);
		ImGui::SameLine(0, 0);
		renderSplitter(padding, availableWidth);
		ImGui::SameLine(0, 0);
		renderEditor(currentFont, editorWidth);
	} else
	{
		editorWidth = availableWidth;
		renderEditor(currentFont, editorWidth);
	}
	renderResizeHandles();
	handleManualResizing();
	ImGui::End();
	ImGui::PopFont();
}
void Ned::renderFrame()
{
	int display_w, display_h;

	glfwGetFramebufferSize(window, &display_w, &display_h);

	// [STEP 1] Render UI to framebuffer
	glBindFramebuffer(GL_FRAMEBUFFER, fb.framebuffer);
	glViewport(0, 0, display_w, display_h);

	// Get background color from settings
	auto &bg = gSettings.getSettings()["backgroundColor"];
	glClearColor(bg[0], bg[1], bg[2], 0.0f);
	glClear(GL_COLOR_BUFFER_BIT);

	renderMainWindow();
	gBookmarks.renderBookmarksWindow();
	gSettings.renderSettingsWindow();
	handleUltraSimpleResizeOverlay();

	ImGui::Render();
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glClear(GL_COLOR_BUFFER_BIT);
	renderWithShader(display_w, display_h, glfwGetTime());
	glfwSwapBuffers(window);
}
void Ned::handleUltraSimpleResizeOverlay()
{
	if (!window)
		return;

	int currentWidth, currentHeight;
	glfwGetWindowSize(window, &currentWidth, &currentHeight);

	bool currentSizeIsValid = (currentWidth > 0 && currentHeight > 0);

	if (m_sroLastWidth == 0 && m_sroLastHeight == 0 && currentSizeIsValid)
	{
		m_sroLastWidth = currentWidth;
		m_sroLastHeight = currentHeight;
		return;
	}
	if (currentWidth == 1200 && currentHeight == 750)
	{
		return;
	}
	if (currentSizeIsValid && (currentWidth != m_sroLastWidth || currentHeight != m_sroLastHeight))
	{
		m_sroFramesToShow = 35;
		m_sroLastWidth = currentWidth;
		m_sroLastHeight = currentHeight;
	}
	if (m_sroFramesToShow > 0)
	{
		ImDrawList *drawList = ImGui::GetForegroundDrawList();
		ImGuiViewport *viewport = ImGui::GetMainViewport();
		ImVec2 viewportPos = viewport->Pos;
		ImVec2 viewportSize = viewport->Size;

		drawList->AddRectFilled(viewportPos,
								ImVec2(viewportPos.x + viewportSize.x,
									   viewportPos.y + viewportSize.y),
								IM_COL32(0, 0, 0, 128));

		char buffer[64];
		snprintf(buffer, sizeof(buffer), "%d x %d", m_sroLastWidth, m_sroLastHeight);

		ImFont *font = ImGui::GetFont();
		float targetFontSize = 52.0f;

		ImVec2 textSize = font->CalcTextSizeA(targetFontSize, FLT_MAX, 0.0f, buffer);

		ImVec2 textPos = ImVec2(viewportPos.x + (viewportSize.x - textSize.x) * 0.5f,
								viewportPos.y + (viewportSize.y - textSize.y) * 0.5f);

		drawList->AddText(font, targetFontSize, textPos, IM_COL32(255, 255, 255, 255), buffer);

		m_sroFramesToShow--;
	}
}
void Ned::handleFileDialog()
{
	if (gFileExplorer.showFileDialog())
	{
		gFileExplorer.openFolderDialog();
		if (!gFileExplorer.selectedFolder.empty())
		{
			auto &rootNode = gFileTree.rootNode; // Changed to use gFileTree
			rootNode.name = fs::path(gFileExplorer.selectedFolder).filename().string();
			rootNode.fullPath = gFileExplorer.selectedFolder;
			rootNode.isDirectory = true;
			rootNode.children.clear();
			gFileTree.buildFileTree(gFileExplorer.selectedFolder,
									rootNode); // Changed to use gFileTree
			gFileExplorer.showWelcomeScreen = false;
		}
	}
}
void Ned::renderWithShader(int display_w, int display_h, double currentTime)
{
	// Get current accumulation buffers
	int prev = accum.swap ? 1 : 0;
	int curr = accum.swap ? 0 : 1;

	// First pass: Burn-in accumulation
	glBindFramebuffer(GL_FRAMEBUFFER, accum.accum[curr].framebuffer);
	accum.burnInShader.useShader();

	// Set texture units and uniforms
	accum.burnInShader.setInt("currentFrame", 0);
	accum.burnInShader.setInt("previousFrame", 1);
	accum.burnInShader.setFloat("decay",
								gSettings.getSettings()["burnin_intensity"]); // Optimal decay value

	// Bind textures
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, fb.renderTexture); // Current frame
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, accum.accum[prev].renderTexture); // Previous accumulation

	// Clear accumulation buffer before drawing
	glClear(GL_COLOR_BUFFER_BIT);
	glBindVertexArray(quad.VAO);
	glDrawArrays(GL_TRIANGLES, 0, 6);

	accum.swap = !accum.swap;

	// Second pass: CRT effects
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glClear(GL_COLOR_BUFFER_BIT);
	crtShader.useShader();

	// Set CRT shader uniforms
	crtShader.setInt("screenTexture", 0);
	if (shader_toggle)
	{
		crtShader.setFloat("u_effects_enabled", 1.0f);
	} else
	{
		crtShader.setFloat("u_effects_enabled", 0.0f);
	}
	crtShader.setFloat("u_scanline_intensity", gSettings.getSettings()["scanline_intensity"]);
	crtShader.setFloat("u_vignet_intensity", gSettings.getSettings()["vignet_intensity"]);
	crtShader.setFloat("u_bloom_intensity", gSettings.getSettings()["bloom_intensity"]);
	crtShader.setFloat("u_static_intensity", gSettings.getSettings()["static_intensity"]);
	crtShader.setFloat("u_colorshift_intensity", gSettings.getSettings()["colorshift_intensity"]);
	crtShader.setFloat("u_jitter_intensity",
					   gSettings.getSettings()["jitter_intensity"].get<float>());
	crtShader.setFloat("u_curvature_intensity",
					   gSettings.getSettings()["curvature_intensity"].get<float>());
	crtShader.setFloat("u_pixelation_intensity",
					   gSettings.getSettings()["pixelation_intensity"].get<float>());
	crtShader.setFloat("u_pixel_width", gSettings.getSettings()["pixel_width"].get<float>());
	// Set time and resolution uniforms
	GLint timeLocation = glGetUniformLocation(crtShader.shaderProgram, "time");
	GLint resolutionLocation = glGetUniformLocation(crtShader.shaderProgram, "resolution");
	if (timeLocation != -1)
		glUniform1f(timeLocation, currentTime);
	if (resolutionLocation != -1)
		glUniform2f(resolutionLocation, display_w, display_h);

	// Bind accumulated texture
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, accum.accum[curr].renderTexture); // Use current accum buffer

	// Draw final quad
	glBindVertexArray(quad.VAO);
	glDrawArrays(GL_TRIANGLES, 0, 6);
}
void Ned::handleFrameTiming(std::chrono::high_resolution_clock::time_point frame_start)
{
	auto frame_end = std::chrono::high_resolution_clock::now();
	auto frame_duration = frame_end - frame_start;
	std::this_thread::sleep_for(std::chrono::duration<double>(1.0 / TARGET_FPS) - frame_duration);
}
void Ned::handleSettingsChanges()
{
	if (gSettings.hasSettingsChanged())
	{
		ImGuiStyle &style = ImGui::GetStyle();

		
		ApplySettings(style); 

		style.Colors[ImGuiCol_WindowBg] =
			ImVec4(gSettings.getSettings()["backgroundColor"][0].get<float>(),
				   gSettings.getSettings()["backgroundColor"][1].get<float>(),
				   gSettings.getSettings()["backgroundColor"][2].get<float>(),
				   0.0f);

		
		shader_toggle = gSettings.getSettings()["shader_toggle"].get<bool>();

		if (gSettings.hasThemeChanged())
		{
			gEditorHighlight.setTheme(gSettings.getCurrentTheme());
			if (!gFileExplorer.currentFile.empty())
			{
				gEditorHighlight.highlightContent();
			}
			gSettings.resetThemeChanged();
		}
		if (gSettings.hasFontChanged() || gSettings.hasFontSizeChanged())
		{
			needFontReload = true;
		}
		 #ifdef __APPLE__
            // Always update with current values
            float currentOpacity = gSettings.getSettings().value("mac_background_opacity", 0.5f);
            bool currentBlurEnabled = gSettings.getSettings().value("mac_blur_enabled", true);
            
            updateMacOSWindowProperties(currentOpacity, currentBlurEnabled);
            
            // Update tracking variables
            lastOpacity = currentOpacity;
            lastBlurEnabled = currentBlurEnabled;
        #endif
		
		gSettings.resetSettingsChanged();
	}
}
void Ned::handleFontReload()
{
	if (needFontReload)
	{
		ImGui_ImplOpenGL3_DestroyFontsTexture();
		ImGui::GetIO().Fonts->Clear();
		currentFont =
			loadFont(gSettings.getCurrentFont(), gSettings.getSettings()["fontSize"].get<float>());
		ImGui::GetIO().Fonts->Build();
		ImGui_ImplOpenGL3_CreateFontsTexture();
		gSettings.resetFontChanged();
		gSettings.resetFontSizeChanged();
		needFontReload = false;
	}
}

void Ned::cleanup()
{
	quad.cleanup();
	if (fb.initialized)
	{
		glDeleteFramebuffers(1, &fb.framebuffer);
		glDeleteTextures(1, &fb.renderTexture);
		glDeleteRenderbuffers(1, &fb.rbo);
	}

	gSettings.saveSettings();
	gFileExplorer.saveCurrentFile();

	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();
	if (accum.accum[0].initialized)
	{
		glDeleteFramebuffers(1, &accum.accum[0].framebuffer);
		glDeleteTextures(1, &accum.accum[0].renderTexture);
	}
	if (accum.accum[1].initialized)
	{
		glDeleteFramebuffers(1, &accum.accum[1].framebuffer);
		glDeleteTextures(1, &accum.accum[1].renderTexture);
	}
	glfwDestroyWindow(window);
	glfwTerminate();
}
void ApplySettings(ImGuiStyle &style)
{
	// Set the window background color from settings.
	style.Colors[ImGuiCol_WindowBg] =
		ImVec4(gSettings.getSettings()["backgroundColor"][0].get<float>(),
			   gSettings.getSettings()["backgroundColor"][1].get<float>(),
			   gSettings.getSettings()["backgroundColor"][2].get<float>(),
			   gSettings.getSettings()["backgroundColor"][3].get<float>());
	shader_toggle = gSettings.getSettings()["shader_toggle"].get<bool>();

	// Set text colors from the current theme.
	std::string currentTheme = gSettings.getCurrentTheme();
	auto &textColor = gSettings.getSettings()["themes"][currentTheme]["text"];
	ImVec4 textCol(textColor[0].get<float>(),
				   textColor[1].get<float>(),
				   textColor[2].get<float>(),
				   textColor[3].get<float>());
	style.Colors[ImGuiCol_Text] = textCol;
	style.Colors[ImGuiCol_TextDisabled] =
		ImVec4(textCol.x * 0.6f, textCol.y * 0.6f, textCol.z * 0.6f, textCol.w);
	style.Colors[ImGuiCol_TextSelectedBg] = ImVec4(1.0f, 0.1f, 0.7f, 0.3f);

	// Hide scrollbars by setting their alpha to 0.
	style.ScrollbarSize = 30.0f;
	style.ScaleAllSizes(1.0f); // Keep this if you scale other UI elements
	style.Colors[ImGuiCol_ScrollbarBg] = ImVec4(0, 0, 0, 0);
	style.Colors[ImGuiCol_ScrollbarGrab] = ImVec4(0, 0, 0, 0);
	style.Colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0, 0, 0, 0);
	style.Colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0, 0, 0, 0);

	// Set the global font scale.
	// ImGui::GetIO().FontGlobalScale = gSettings.getSettings()["fontSize"].get<float>() / 16.0f;
}
