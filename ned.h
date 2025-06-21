/*
	File: ned.h
	Description: Main application class for NED text editor.
	This encapsulates the core application logic, initialization, and render
   loop.
*/

#pragma once

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <chrono>
#include <string>

#include "editor/editor.h"
#include "editor/editor_bookmarks.h"
#include "files/file_tree.h"
#include "files/files.h"
#include "shaders/shader.h"
#include "ai/ai_agent.h"

// Forward declarations
struct GLFWwindow;
class ImFont;

// main application class
class Ned
{
  public:
	Ned();
	~Ned();

	bool initialize();
	void run();
	void cleanup();
	void stopWindowDragging() { isDraggingWindow = false; }

  private:
	// Core structures
	struct FramebufferState
	{
		GLuint framebuffer = 0, renderTexture = 0, rbo = 0;
		int last_display_w = 0, last_display_h = 0;
		bool initialized = false;
	};

	struct TimingState
	{
		int frameCount = 0;
		double lastFPSTime = 0.0;
		double lastSettingsCheck = 0.0;
		double lastFileTreeRefresh = 0.0;
	};

	struct ShaderQuad
	{
		GLuint VAO = 0, VBO = 0;
		void initialize();
		void cleanup();
	};

	// Member variables
	GLFWwindow *window;
	Shader crtShader;
	ImFont *currentFont;
	ImFont *largeFont; // Font for resolution overlay
	FramebufferState fb;
	TimingState timing;
	ShaderQuad quad;
	bool needFontReload;
	bool windowFocused;
	float explorerWidth;
	float editorWidth;
	bool initialized;
	// Constants
	static constexpr double SETTINGS_CHECK_INTERVAL = 2.0;
	static constexpr double FILE_TREE_REFRESH_INTERVAL = 2.0;
	static constexpr double TARGET_FPS = 120.0; // Fallback value, actual FPS target comes from settings

	// Core functions
	bool initializeGraphics();
	void initializeImGui();
	void initializeResources();
	void handleEvents();
	void handleBackgroundUpdates(double currentTime);
	void handleFramebuffer(int width, int height);
	void handleWindowFocus();
	void handleSettingsChanges();
	void handleFontReload();
	void handleFileDialog();
	void handleKeyboardShortcuts();
	void handleFrameTiming(std::chrono::high_resolution_clock::time_point frame_start);
	void setupImGuiFrame();

	// Render functions
	void renderFrame();
	void renderMainWindow();
	void renderFileExplorer(float explorerWidth);
	void renderEditorHeader(ImFont *currentFont);
	std::string truncateFilePath(const std::string &path, float maxWidth, ImFont *font);
	void renderSettingsIcon(float iconSize);
	void renderSplitter(float padding, float availableWidth);
	void renderEditor(ImFont *currentFont, float editorWidth);
	void renderWithShader(int display_w, int display_h, double currentTime);
	void renderFPSCounter();

	// Add these declarations for the agent pane and its splitter
	void renderAgentSplitter(float padding, float availableWidth, bool sidebarVisible);
	void renderAgentPane(float agentPaneWidth);

	// Utility functions
	ImFont *loadFont(const std::string &fontName, float fontSize);
	ImFont *loadLargeFont(const std::string &fontName, float fontSize);
	static float clamp(float value, float min, float max);

	// slow scrolling momentum accumulator
	static void scrollCallback(GLFWwindow *window, double xoffset, double yoffset);
	double scrollXAccumulator = 0.0;
	double scrollYAccumulator = 0.0;

	// burn in accumlation buffer
	struct AccumulationBuffers
	{
		FramebufferState accum[2];
		bool swap = false;
		Shader burnInShader;
	};

	AccumulationBuffers accum;

	// custom resizing logic
	void renderResizeHandles();
	void handleManualResizing();
	bool resizingRight = false;
	bool resizingBottom = false;
	bool resizingCorner = false;
	ImVec2 dragStart;
	ImVec2 initialWindowSize;

	bool isDraggingWindow = false;
	void handleWindowDragging();

	int m_sroLastWidth;
	int m_sroLastHeight;
	double m_sroStartTime;

	// Declaration for the single overlay function
	void handleUltraSimpleResizeOverlay(); // Renamed for clarity
	float lastOpacity = 0.5f;
   bool lastBlurEnabled = false;
	        
	AIAgent gAIAgent;
};

// Global scope
extern Bookmarks gBookmarks;
extern bool shader_toggle;
extern bool showSidebar;
extern bool showAgentPane;


