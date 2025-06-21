/*
File: ned.cpp
Description: Main application class implementation for NED text editor.
*/
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "imgui_internal.h"
#ifdef __APPLE__
#include "macos_window.h"
#endif

#include "../ai/ai_tab.h"
#include "ned.h"

#include "editor/editor_bookmarks.h"
#include "editor/editor_highlight.h"
#include "util/debug_console.h"
#include "util/settings.h"
#include "util/keybinds.h"
#include "util/terminal.h"
#include "util/welcome.h"

#include <cstdio>
#include <filesystem>
#include <iostream>
#include <thread>

#include "ai/ai_agent.h"

// global scope
Bookmarks gBookmarks;
bool shader_toggle = false;
bool showSidebar = true;
bool showAgentPane = true;

// FPS counter variables
static double fps_lastTime = 0.0;
static int fps_frames = 0;
static double fps_currentFPS = 0.0;

static double lastMouseX = 0.0;
static double lastMouseY = 0.0;
static double dragStartMouseX = 0.0;
static double dragStartMouseY = 0.0;
static int dragStartWindowX = 0;
static int dragStartWindowY = 0;
static int dragStartWindowWidth = 0;
static int dragStartWindowHeight = 0;

float agentSplitPos = 0.75f; // 75% editor, 25% agent pane by default

constexpr float kAgentSplitterWidth = 6.0f;

AIAgent gAIAgent;

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
 	if (gKeybinds.loadKeybinds()) {
        std::cout << "Initial keybinds loaded successfully." << std::endl;
    } else {
        std::cout << "Failed to load initial keybinds." << std::endl;
    }

	// Load sidebar visibility settings
	showSidebar = gSettings.getSettings().value("sidebar_visible", true);
	showAgentPane = gSettings.getSettings().value("agent_pane_visible", true);

	// Adjust agent split position if sidebar is hidden (first load only)
	if (!gSettings.isAgentSplitPosProcessed() && !showSidebar) {
		float currentAgentSplitPos = gSettings.getAgentSplitPos();
		float originalValue = currentAgentSplitPos;
		// When sidebar is hidden, the agent split position represents editor width, not agent pane width
		// So we need to invert it: 1 - saved_agent_position
		float newAgentSplitPos = 1.0f - currentAgentSplitPos;
		gSettings.setAgentSplitPos(newAgentSplitPos);
		std::cout << "[Ned] Adjusted agent_split_pos from " << originalValue << " to " << newAgentSplitPos << " (sidebar hidden) on first load" << std::endl;
	}

	
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
	const float resizeBorder = 10.0f;   // <--- CHANGE THIS VALUE
	ImVec2 windowPos = ImGui::GetMainViewport()->Pos;
	ImVec2 windowSize = ImGui::GetMainViewport()->Size;

	// Right edge
	ImGui::PushID("resize_right");
	ImGui::SetCursorScreenPos(ImVec2(windowPos.x + windowSize.x - resizeBorder, windowPos.y));
	ImGui::InvisibleButton("##resize_right", ImVec2(resizeBorder, windowSize.y)); // Will now be 100px wide
	bool hoverRight = ImGui::IsItemHovered();
	ImGui::PopID();

	// Bottom edge
	ImGui::PushID("resize_bottom");
	ImGui::SetCursorScreenPos(ImVec2(windowPos.x, windowPos.y + windowSize.y - resizeBorder));
	ImGui::InvisibleButton("##resize_bottom", ImVec2(windowSize.x, resizeBorder)); // Will now be 100px high
	bool hoverBottom = ImGui::IsItemHovered();
	ImGui::PopID();

	// Corner
	ImGui::PushID("resize_corner");
	ImGui::SetCursorScreenPos(ImVec2(windowPos.x + windowSize.x - resizeBorder,
									 windowPos.y + windowSize.y - resizeBorder));
	ImGui::InvisibleButton("##resize_corner", ImVec2(resizeBorder, resizeBorder)); // Will now be 100x100px
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
	// else {
	// Optionally set a default cursor if not hovering any resize handle and not resizing
	// ImGui::SetMouseCursor(ImGuiMouseCursor_Arrow); 
	// }
}

void Ned::handleManualResizing()
{

    ImGuiIO &io = ImGui::GetIO();
    int currentScreenWidth, currentScreenHeight; // Renamed to avoid confusion with initial size
    glfwGetWindowSize(window, &currentScreenWidth, &currentScreenHeight);

    double mouseX, mouseY;
    glfwGetCursorPos(window, &mouseX, &mouseY);

    const float resizeBorder = 10.0f;
    // Hover detection based on current mouse position and current window size
    // This is primarily for initiating the resize operation.
    bool hoverRightEdge = mouseX >= (currentScreenWidth - resizeBorder) && mouseX < currentScreenWidth;
    bool hoverBottomEdge = mouseY >= (currentScreenHeight - resizeBorder) && mouseY < currentScreenHeight;
    bool hoverCorner = hoverRightEdge && hoverBottomEdge; // More specific corner detection

    if (ImGui::IsMouseClicked(ImGuiMouseButton_Left))
    {
        if (hoverCorner) {
            resizingCorner = true;
            resizingRight = true; 
            resizingBottom = true;
        } else if (hoverRightEdge) {
            resizingCorner = false;
            resizingRight = true;
            resizingBottom = false;
        } else if (hoverBottomEdge) {
            resizingCorner = false;
            resizingRight = false;
            resizingBottom = true;
        } else {
            resizingCorner = false;
            resizingRight = false;
            resizingBottom = false;
        }

        if (resizingRight || resizingBottom) 
        {
            dragStart = ImVec2(static_cast<float>(mouseX), static_cast<float>(mouseY));
            initialWindowSize = ImVec2(static_cast<float>(currentScreenWidth), static_cast<float>(currentScreenHeight));
        }
    }

    if (ImGui::IsMouseDragging(ImGuiMouseButton_Left))

		{

		    if (resizingRight || resizingBottom) 
		    {
		        int newWidth = static_cast<int>(initialWindowSize.x);
		        int newHeight = static_cast<int>(initialWindowSize.y);

		        if (resizingRight) 
		        {
		            newWidth = static_cast<int>(initialWindowSize.x + (mouseX - dragStart.x));   
		        }

		        if (resizingBottom)
		        {
		            float deltaY = static_cast<float>(mouseY - dragStart.y);
		            newHeight = static_cast<int>(initialWindowSize.y + deltaY);

		        }
		        newWidth = std::max(newWidth, 100);  
		        newHeight = std::max(newHeight, 100); 


		        // Conditional break or further check if values are extreme
		        if (newHeight > 10000 || newHeight < 0 || newWidth > 10000 || newWidth < 0) {
		            std::cerr << "CRITICAL: Extreme resize values detected before glfwSetWindowSize!" << std::endl;
		        }

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
	glfwSwapInterval(0);
	glfwSetWindowRefreshCallback(window, [](GLFWwindow *window) { glfwPostEmptyEvent(); });

	// Enable raw mouse motion for more accurate tracking
	glfwSetInputMode(window, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
	
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
	
	// Load large font for resolution overlay
	largeFont = loadLargeFont(gSettings.getCurrentFont(), 52.0f);
	if (!largeFont)
	{
		std::cerr << "🔴 Failed to load large font, using default font" << std::endl;
		largeFont = ImGui::GetIO().Fonts->AddFontDefault();
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

ImFont *Ned::loadLargeFont(const std::string &fontName, float fontSize)
{
	ImGuiIO &io = ImGui::GetIO();

	// Build the path from .app/Contents/Resources/fonts/
	std::string resourcePath = Settings::getAppResourcesPath();
	std::string fontPath = resourcePath + "/fonts/" + fontName + ".ttf";

	if (!std::filesystem::exists(fontPath))
	{
		std::cerr << "[Ned::loadLargeFont] Font does not exist: " << fontPath << std::endl;
		return io.Fonts->AddFontDefault();
	}

	static const ImWchar ranges[] = {
		0x0020, 0x00FF, // Basic Latin + Latin Supplement
		0,
	};
	
	ImFontConfig config;
	config.MergeMode = false;
	config.GlyphRanges = ranges;

	// Don't clear existing fonts - just add the new one
	ImFont *font = io.Fonts->AddFontFromFileTTF(fontPath.c_str(), fontSize, &config, ranges);

	if (!font)
	{
		std::cerr << "[Ned::loadLargeFont] Failed to load font: " << fontPath << std::endl;
		return io.Fonts->AddFontDefault();
	}

	// After adding new fonts, re-create the OpenGL font texture
	ImGui_ImplOpenGL3_DestroyFontsTexture();
	ImGui_ImplOpenGL3_CreateFontsTexture();

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
			
			// Get current window position and size
			int windowX, windowY, windowWidth, windowHeight;
			glfwGetWindowPos(window, &windowX, &windowY);
			glfwGetWindowSize(window, &windowWidth, &windowHeight);
			
			// Calculate delta from initial position
			double deltaX = currentMouseX - dragStartMouseX;
			double deltaY = currentMouseY - dragStartMouseY;
			
			// Calculate new window position
			int newX = dragStartWindowX + static_cast<int>(deltaX);
			int newY = dragStartWindowY + static_cast<int>(deltaY);
			
			// Update window position
			glfwSetWindowPos(window, newX, newY);
			
			// If window size changed during drag (e.g., from snapping), update our reference points
			if (windowWidth != dragStartWindowWidth || windowHeight != dragStartWindowHeight)
			{
				// Update drag start position to current position
				dragStartWindowX = newX;
				dragStartWindowY = newY;
				dragStartMouseX = currentMouseX;
				dragStartMouseY = currentMouseY;
				dragStartWindowWidth = windowWidth;
				dragStartWindowHeight = windowHeight;
			}
		}
		else
		{
			isDraggingWindow = false;
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
		 glfwPollEvents();
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
	ImGuiKey toggleSidebar = gKeybinds.getActionKey("toggle_sidebar");
	 
	if (modPressed && ImGui::IsKeyPressed(toggleSidebar, false)){
		float windowWidth = ImGui::GetWindowWidth();
		float padding = ImGui::GetStyle().WindowPadding.x;
		float availableWidth = windowWidth - padding * 3 - kAgentSplitterWidth;

		float agentPaneWidthPx;
		if (showSidebar) {
			agentPaneWidthPx = availableWidth * gSettings.getAgentSplitPos();
		} else {
			float agentSplit = gSettings.getAgentSplitPos();
			float editorWidth = availableWidth * agentSplit;
			agentPaneWidthPx = availableWidth - editorWidth - kAgentSplitterWidth;
		}

		// Toggle sidebar
		showSidebar = !showSidebar;

		// Save sidebar visibility setting
		gSettings.getSettings()["sidebar_visible"] = showSidebar;
		gSettings.saveSettings();

		// Recompute availableWidth after toggling
		windowWidth = ImGui::GetWindowWidth();
		availableWidth = windowWidth - padding * 3 - kAgentSplitterWidth;

		float newRightSplit;
		if (showSidebar) {
			newRightSplit = agentPaneWidthPx / availableWidth;
		} else {
			float editorWidth = availableWidth - agentPaneWidthPx - kAgentSplitterWidth;
			newRightSplit = editorWidth / availableWidth;
		}
		gSettings.setAgentSplitPos(clamp(newRightSplit, 0.1f, 0.9f));

		std::cout << "Toggled sidebar visibility" << std::endl;
	}

	ImGuiKey toggleAgent = gKeybinds.getActionKey("toggle_agent");
	if (modPressed && ImGui::IsKeyPressed(toggleAgent, false)) {
		// Only toggle visibility, do not recalculate or set agentSplitPos
		showAgentPane = !showAgentPane;
		
		// Save agent pane visibility setting
		gSettings.getSettings()["agent_pane_visible"] = showAgentPane;
		gSettings.saveSettings();
		
		std::cout << "Toggled agent pane visibility" << std::endl;
	}

	ImGuiKey toggleTerminal = gKeybinds.getActionKey("toggle_terminal");
	
	if (modPressed && ImGui::IsKeyPressed(toggleTerminal, false)){
		gTerminal.toggleVisibility();
		gFileExplorer.saveCurrentFile();
		if (gTerminal.isTerminalVisible())
		{
			ClosePopper::closeAll();
		}
	}
	ImGuiKey togglesetings = gKeybinds.getActionKey("toggle_settings_window");
	 
	if (modPressed && ImGui::IsKeyPressed(togglesetings, false))
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
	float windowWidth = ImGui::GetWindowWidth();
	printf("Header Window Width: %.1f\n", windowWidth);
	
	// Disable git changes if window width is less than 250
	bool showGitChanges = windowWidth >= 250.0f;
	
	ImGui::BeginGroup();
	ImGui::PushFont(currentFont);

	// Determine the base icon size (equal to font size)
	float iconSize = ImGui::GetFontSize() * 1.15f;
	std::string currentFile = gFileExplorer.currentFile;

	// Calculate space needed for right-aligned status area
	const float rightPadding = 25.0f;							// Space from window edge
	const float totalStatusWidth = iconSize * 2 + rightPadding; // Brain + Gear icons

	// Calculate space needed for git changes if enabled and available
	float gitChangesWidth = 0.0f;
	if(showGitChanges && gSettings.getSettings()["git_changed_lines"] && !gEditorGit.currentGitChanges.empty()) {
		gitChangesWidth = ImGui::CalcTextSize(gEditorGit.currentGitChanges.c_str()).x + ImGui::GetStyle().ItemSpacing.x;
	}

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

		// Calculate available width for the text, accounting for all elements
		float x_cursor = ImGui::GetCursorPosX();
		float x_right_group = ImGui::GetWindowWidth();
		float available_width = x_right_group - x_cursor - ImGui::GetStyle().ItemSpacing.x - totalStatusWidth - gitChangesWidth;

		// Truncate the file path to fit available space
		std::string truncatedText =
			truncateFilePath(currentFile, available_width, ImGui::GetFont());

		ImGui::Text("%s", truncatedText.c_str());

		// Add git changes info if available and window is wide enough
		if(showGitChanges && gSettings.getSettings()["git_changed_lines"]){
			if (!gEditorGit.currentGitChanges.empty()) {
				ImGui::SameLine();
				ImGui::Text("%s", gEditorGit.currentGitChanges.c_str());
			}
		}
		
	}

	// Right-aligned status area
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
			ImGui::Image(gFileExplorer.getIcon("green-dot"), ImVec2(iconSize, iconSize));
		} else
		{
			// Invisible placeholder to maintain layout
			ImGui::InvisibleButton("##brain-placeholder", ImVec2(iconSize, iconSize));
		}
		ImGui::SameLine();

		// Settings icon (always in same position)
		renderSettingsIcon(iconSize * 0.6f);
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
	ImGui::Button("##vsplitter_left", ImVec2(hover_width, -1));

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
		float new_split = (mouse_x - padding * 2) / (availableWidth - padding * 4 - 6);
		// Clamp so that editor is always at least 10% of availableWidth
		float rightSplit = gSettings.getAgentSplitPos();
		bool agentPaneVisible = showAgentPane;
		float maxLeftSplit = agentPaneVisible ? (0.9f - rightSplit) : 0.9f;
		new_split = clamp(new_split, 0.1f, maxLeftSplit);
		gSettings.setSplitPos(new_split);
	}

	ImGui::PopStyleColor(3);
}

void Ned::renderAgentSplitter(float padding, float availableWidth, bool sidebarVisible)
{
    ImGui::SameLine(0, 0);

    // Interaction settings
    const float visible_width = 1.0f;    // Rendered width at rest
    const float hover_width = 6.0f;        // Invisible hitbox size
    const float hover_expansion = 3.0f;    // Expanded visual width
    const float hover_delay = 0.15f;        // Seconds before hover effect

    ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(0, 0, 0, 0));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, IM_COL32(0, 0, 0, 0));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, IM_COL32(0, 0, 0, 0));

    // Create invisible button with larger hitbox
    ImGui::Button("##vsplitter_right", ImVec2(hover_width, -1));

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

    // Drag logic
    static bool dragging = false;
    static float dragOffset = 0.0f;

    float rightSplit = gSettings.getAgentSplitPos();
    float splitterX;
    if (sidebarVisible) {
        splitterX = availableWidth - (availableWidth * rightSplit) - kAgentSplitterWidth;
    } else {
        splitterX = availableWidth * rightSplit;
    }

    if (ImGui::IsItemActive() && !dragging) {
        dragging = true;
        dragOffset = ImGui::GetMousePos().x - (ImGui::GetWindowPos().x + splitterX);
    }
    if (!ImGui::IsItemActive() && dragging) {
        dragging = false;
        gSettings.saveSettings();
    }
    if (dragging) {
        float mouseX = ImGui::GetMousePos().x - ImGui::GetWindowPos().x - dragOffset;
        float new_split;
        float leftSplit = gSettings.getSplitPos();
        float maxRightSplit = sidebarVisible ? (0.85f - leftSplit) : 0.85f;
        if (sidebarVisible) {
            new_split = clamp((availableWidth - mouseX - kAgentSplitterWidth) / availableWidth, 0.15f, maxRightSplit);
        } else {
            new_split = clamp(mouseX / availableWidth, 0.15f, maxRightSplit);
        }
        gSettings.setAgentSplitPos(new_split);
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

void Ned::renderAgentPane(float agentPaneWidth)
{
    gAIAgent.render(agentPaneWidth);
}

void Ned::renderMainWindow()
{

#ifdef __APPLE__
	//renderTopLeftMenu();
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
	float availableWidth = windowWidth - padding * 3 - kAgentSplitterWidth; // Account for splitter width

	if (showSidebar)
	{
		// First split: File Explorer vs. Editor+Agent
		float leftSplit = gSettings.getSplitPos();
		float rightSplit = gSettings.getAgentSplitPos();

		float explorerWidth = availableWidth * leftSplit;
		float agentPaneWidth = availableWidth * rightSplit;
		float editorWidth = availableWidth - explorerWidth - (showAgentPane ? agentPaneWidth : 0.0f) - (padding * 2);

		// Render File Explorer
		renderFileExplorer(explorerWidth);
		ImGui::SameLine(0, 0);

		// Render left splitter
		renderSplitter(padding, availableWidth);
		ImGui::SameLine(0, 0);

		// Render Editor
		renderEditor(currentFont, editorWidth);
		if (showAgentPane) {
			ImGui::SameLine(0, 0);
			// Render right splitter (new)
			renderAgentSplitter(padding, availableWidth, showSidebar);
			ImGui::SameLine(0, 0);
			// Render Agent Pane (new)
			renderAgentPane(agentPaneWidth);
		}
	} else {
		// No sidebar: just editor and agent pane
		float agentSplit = gSettings.getAgentSplitPos();
		float editorWidth = showAgentPane ? (availableWidth * agentSplit) : availableWidth;
		float agentPaneWidth = showAgentPane ? (availableWidth - editorWidth - kAgentSplitterWidth) : 0.0f;

		renderEditor(currentFont, editorWidth);
		if (showAgentPane) {
			ImGui::SameLine(0, 0);
			renderAgentSplitter(padding, availableWidth, showSidebar);
			ImGui::SameLine(0, 0);
			renderAgentPane(agentPaneWidth);
		}
	}
	renderResizeHandles();
	handleManualResizing();
	ImGui::End();
	ImGui::PopFont();
}

void Ned::renderFrame()
{
    // Calculate FPS
    double currentTime = glfwGetTime();
    fps_frames++;
    if (currentTime - fps_lastTime >= 1.0) {
        fps_currentFPS = fps_frames / (currentTime - fps_lastTime);
        fps_frames = 0;
        fps_lastTime = currentTime;
    }

    int display_w, display_h;
    glfwGetFramebufferSize(window, &display_w, &display_h);

    // [STEP 1] Render UI to framebuffer
    glBindFramebuffer(GL_FRAMEBUFFER, fb.framebuffer);
    glViewport(0, 0, display_w, display_h);

    // Get background color from settings
    auto &bg = gSettings.getSettings()["backgroundColor"];
    
    // Use different alpha based on shader state
    float alpha = shader_toggle ? bg[3].get<float>() : 1.0f;
    glClearColor(bg[0], bg[1], bg[2], alpha);
    glClear(GL_COLOR_BUFFER_BIT);

    renderMainWindow();
    gBookmarks.renderBookmarksWindow();
    gSettings.renderSettingsWindow();
    gSettings.renderNotification("");
    gKeybinds.checkKeybindsFile(); 

    handleUltraSimpleResizeOverlay();

    // Render FPS counter overlay
    renderFPSCounter();

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
		m_sroStartTime = glfwGetTime();
		m_sroLastWidth = currentWidth;
		m_sroLastHeight = currentHeight;
	}
	
	double currentTime = glfwGetTime();
	double elapsedTime = currentTime - m_sroStartTime;
	const double displayDuration = 0.5; // Display for 0.5 seconds
	
	if (m_sroStartTime > 0.0 && elapsedTime < displayDuration)
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

		ImFont *font = largeFont; // Use the large font instead of scaling
		float targetFontSize = 52.0f;

		ImVec2 textSize = font->CalcTextSizeA(targetFontSize, FLT_MAX, 0.0f, buffer);

		ImVec2 textPos = ImVec2(viewportPos.x + (viewportSize.x - textSize.x) * 0.5f,
								viewportPos.y + (viewportSize.y - textSize.y) * 0.5f);

		drawList->AddText(font, targetFontSize, textPos, IM_COL32(255, 255, 255, 255), buffer);
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
    // Only run burn-in shader if shaders are enabled
    if (shader_toggle) 
    {
        // Get current accumulation buffers
        int prev = accum.swap ? 1 : 0;
        int curr = accum.swap ? 0 : 1;

        // Burn-in accumulation pass
        glBindFramebuffer(GL_FRAMEBUFFER, accum.accum[curr].framebuffer);
        accum.burnInShader.useShader();
        accum.burnInShader.setInt("currentFrame", 0);
        accum.burnInShader.setInt("previousFrame", 1);
        accum.burnInShader.setFloat("decay", gSettings.getSettings()["burnin_intensity"]);
        
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, fb.renderTexture);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, accum.accum[prev].renderTexture);
        
        glClear(GL_COLOR_BUFFER_BIT);
        glBindVertexArray(quad.VAO);
        glDrawArrays(GL_TRIANGLES, 0, 6);

        accum.swap = !accum.swap;
    }

    // CRT effects pass
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glClear(GL_COLOR_BUFFER_BIT);
    crtShader.useShader();

    // Set CRT shader uniforms
    crtShader.setInt("screenTexture", 0);
    if (shader_toggle) {
        crtShader.setFloat("u_effects_enabled", 1.0f);
    } else {
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

    // Bind appropriate texture based on shader toggle
    glActiveTexture(GL_TEXTURE0);
    if (shader_toggle) {
        int curr = accum.swap ? 0 : 1; // Get current accumulation buffer
        glBindTexture(GL_TEXTURE_2D, accum.accum[curr].renderTexture);
    } else {
        glBindTexture(GL_TEXTURE_2D, fb.renderTexture);
    }

    // Draw final quad
    glBindVertexArray(quad.VAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
}

void Ned::renderFPSCounter()
{
    // Check if FPS counter is enabled in settings
    if (!gSettings.getSettings()["fps_toggle"].get<bool>()) {
        return;
    }

    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImVec2 viewportPos = viewport->Pos;
    ImVec2 viewportSize = viewport->Size;
    
    // Position in bottom right corner with some padding
    const float padding = 10.0f;
    const float fontSize = gSettings.getFontSize();
    
    // Format FPS text
    char fpsText[32];
    snprintf(fpsText, sizeof(fpsText), "%.1f", fps_currentFPS);
    
    // Calculate text size
    ImVec2 textSize = ImGui::CalcTextSize(fpsText);
    
    // Position text in bottom right
    ImVec2 textPos = ImVec2(
        viewportPos.x + viewportSize.x - textSize.x - padding,
        viewportPos.y + viewportSize.y - textSize.y - padding
    );
    
    // Draw background rectangle for better visibility
    ImDrawList* drawList = ImGui::GetForegroundDrawList();
    ImVec2 bgMin = ImVec2(textPos.x - 5.0f, textPos.y - 2.0f);
    ImVec2 bgMax = ImVec2(textPos.x + textSize.x + 5.0f, textPos.y + textSize.y + 2.0f);
    drawList->AddRectFilled(bgMin, bgMax, IM_COL32(0, 0, 0, 180));
    
    // Draw FPS text
    drawList->AddText(textPos, IM_COL32(255, 255, 255, 255), fpsText);
}

void Ned::handleFrameTiming(std::chrono::high_resolution_clock::time_point frame_start)
{
	auto frame_end = std::chrono::high_resolution_clock::now();
	auto frame_duration = frame_end - frame_start;
	
	// Get FPS target from settings directly
	float fpsTarget = 120.0f;
	if (gSettings.getSettings().contains("fps_target") && gSettings.getSettings()["fps_target"].is_number()) {
		fpsTarget = gSettings.getSettings()["fps_target"].get<float>();
	}
	// Only apply frame timing if FPS target is reasonable (not unlimited)
	if (fpsTarget > 0.0f && fpsTarget < 10000.0f) {
		auto targetFrameTime = std::chrono::duration<double>(1.0 / fpsTarget);
		if (frame_duration < targetFrameTime) {
			std::this_thread::sleep_for(targetFrameTime - frame_duration);
		}
	}
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

		// Update sidebar visibility from settings
		showSidebar = gSettings.getSettings().value("sidebar_visible", true);
		showAgentPane = gSettings.getSettings().value("agent_pane_visible", true);

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
		
		// Also reload largeFont since it was cleared above
		largeFont = loadLargeFont(gSettings.getCurrentFont(), 52.0f);
		
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
