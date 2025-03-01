/*
    File: ned.h
    Description: Main application class for NED text editor.
    This encapsulates the core application logic, initialization, and render loop.
*/

#pragma once

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <chrono>
#include <string>

#include "editor/editor.h"
#include "files/file_tree.h"
#include "files/files.h"
#include "shaders/shader.h"

// Forward declarations
struct GLFWwindow;
class ImFont;

class Ned
{
  public:
    Ned();
    ~Ned();

    bool initialize();
    void run();
    void cleanup();

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
    static constexpr double TARGET_FPS = 60.0;

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
    void renderSettingsIcon(float iconSize);
    void renderSplitter(float padding, float availableWidth);
    void renderEditor(ImFont *currentFont, float editorWidth);
    void renderWithShader(int display_w, int display_h, double currentTime);

    // Utility functions
    ImFont *loadFont(const std::string &fontName, float fontSize);
    static float clamp(float value, float min, float max);

    // Smooth scrolling variables
    ImVec2 targetScrollVelocity = ImVec2(0.0f, 0.0f);  // Where we want to go
    ImVec2 currentScrollVelocity = ImVec2(0.0f, 0.0f); // Where we are now

    // Add a new function to handle scroll input
    void handleScrollEvent(double xoffset, double yoffset);
    void handleScrollInput();
};