/*
    Main.cpp
    NED is a lightweight, feature-rich text editor built with C++ and ImGui.
    It offers syntax highlighting, project file tree, and a customizable interface.
    Github:
    https://github.com/nealmick/ned
*/

#define GL_SILENCE_DEPRECATION
#define GLEW_NO_GLU

#include <GL/glew.h>

#if defined(__APPLE__)
#define GL_GLEXT_PROTOTYPES 1
#include <OpenGL/gl.h>
#else
#include <GL/gl.h>
#endif

#include <GLFW/glfw3.h>
#include <chrono>
#include <filesystem>
#include <iostream>
#include <string>
#include <thread>
#include <unistd.h>

#include "editor.h"
#include "files.h"
#include "shaders/shader.h"
#include "util/bookmarks.h"
#include "util/close_popper.h"
#include "util/debug_console.h"
#include "util/settings.h"
#include "util/terminal.h"
#include "util/welcome.h"

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

struct FramebufferState {
    GLuint framebuffer = 0, renderTexture = 0, rbo = 0;
    int last_display_w = 0, last_display_h = 0;
    bool initialized = false;
};

struct TimingState {
    int frameCount = 0;
    double lastFPSTime = glfwGetTime();
    double lastSettingsCheck = lastFPSTime;
    double lastFileTreeRefresh = lastFPSTime;
};

struct ShaderQuad {
    GLuint VAO, VBO;

    void initialize() {
        float quadVertices[] = {-1.0f, -1.0f, 0.0f, 0.0f, 1.0f, -1.0f, 1.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, -1.0f, -1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, -1.0f, 1.0f, 0.0f, 1.0f};
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

    void cleanup() {
        glDeleteVertexArrays(1, &VAO);
        glDeleteBuffers(1, &VBO);
    }
};

void InitializeGLFW();
void InitializeImGui(GLFWwindow *window);
void ApplySettings(ImGuiStyle &style);
void RenderMainWindow(ImFont *currentFont, float &explorerWidth, float &editorWidth);
void updateFileExplorer();
void handleEvents(GLFWwindow *window);
void handleBackgroundUpdates(double currentTime, double &lastSettingsTime, double &lastTreeTime);
void handleFramebuffer(int width, int height, GLuint &fb, GLuint &tex, GLuint &rbo, int &last_w, int &last_h, bool &initialized);
void setupImGuiFrame();
void handleWindowFocus(GLFWwindow *window, bool &windowFocused);
void handleSettingsChanges(ImGuiStyle &style, bool &needFontReload);
void handleFontReload(ImFont *&currentFont, bool &needFontReload);
void handleFileDialog();
void beginFrame(int display_w, int display_h);
void renderMainContent(ImFont *currentFont, float &explorerWidth, float &editorWidth);
void renderWithShader(Shader &shader, GLuint fullFramebuffer, GLuint fullRenderTexture, GLuint quadVAO, int display_w, int display_h, double currentTime);
void handleFrameTiming(std::chrono::high_resolution_clock::time_point frame_start);
void RenderMainWindow(ImFont *currentFont, float &explorerWidth, float &editorWidth);
void initializeImGuiAndResources(GLFWwindow *window, ImFont *&currentFont);
void cleanup(GLFWwindow *window, FramebufferState &fb, ShaderQuad &quad);
void handleKeyboardShortcuts();
void renderFileExplorer(float explorerWidth);
void renderEditorHeader(ImFont *currentFont);
void renderSettingsIcon(float iconSize);
void renderSplitter(float padding, float availableWidth);
void renderEditor(ImFont *currentFont, float editorWidth);

float Clamp(float value, float min, float max);
bool initializeGraphics(GLFWwindow *&window, Shader &crtShader);

Bookmarks gBookmarks;
bool shader_toggle = false;
bool showSidebar = true;  
GLFWwindow *CreateWindow();
ImFont *LoadFont(const std::string &fontName, float fontSize);

constexpr double SETTINGS_CHECK_INTERVAL = 2.0;
constexpr double FILE_TREE_REFRESH_INTERVAL = 2.0;
constexpr double TARGET_FPS = 60.0;
const std::chrono::duration<double> TARGET_FRAME_DURATION(1.0 / TARGET_FPS);

/* ---- main render pipeline ----- */
int main() {
    GLFWwindow *window = nullptr;
    Shader crtShader;

    if (!initializeGraphics(window, crtShader)) {
        return -1;
    }

    ImFont *currentFont = nullptr;
    initializeImGuiAndResources(window, currentFont);

    FramebufferState fb;
    TimingState timing;
    ShaderQuad quad;
    quad.initialize();

    bool needFontReload = false;
    bool windowFocused = true;
    float explorerWidth = 0.0f, editorWidth = 0.0f;

    // Main loop
    while (!glfwWindowShouldClose(window)) {
        auto frame_start = std::chrono::high_resolution_clock::now();

        // Handle events and updates
        handleEvents(window);
        double currentTime = glfwGetTime();
        handleBackgroundUpdates(currentTime, timing.lastSettingsCheck, timing.lastFileTreeRefresh);

        // Handle framebuffer updates
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        handleFramebuffer(display_w, display_h, fb.framebuffer, fb.renderTexture, fb.rbo, fb.last_display_w, fb.last_display_h, fb.initialized);

        // Setup ImGui frame and state
        setupImGuiFrame();
        handleWindowFocus(window, windowFocused);
        handleSettingsChanges(ImGui::GetStyle(), needFontReload);
        handleFileDialog();

        // Render frame
        beginFrame(display_w, display_h);
        renderMainContent(currentFont, explorerWidth, editorWidth);
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        // Handle shader effects
        if (shader_toggle) {
            renderWithShader(crtShader, fb.framebuffer, fb.renderTexture, quad.VAO, display_w, display_h,
                             currentTime); // Changed from quadVAO to quad.VAO
        } else {
            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        }

        handleFontReload(currentFont, needFontReload);
        glfwSwapBuffers(window);
        handleFrameTiming(frame_start);
    }

    cleanup(window, fb, quad);
    return 0;
}

float Clamp(float value, float min, float max) {
    if (value < min)
        return min;
    if (value > max)
        return max;
    return value;
}

GLFWwindow *CreateWindow() {
    GLFWwindow *window = glfwCreateWindow(1200, 750, "NED", NULL, NULL);
    if (window == NULL) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        exit(1);
    }
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);
    return window;
}

void InitializeImGui(GLFWwindow *window) {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    (void)io;
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330");
}

void ApplySettings(ImGuiStyle &style) {
    // Background color
    style.Colors[ImGuiCol_WindowBg] = ImVec4(gSettings.getSettings()["backgroundColor"][0].get<float>(), gSettings.getSettings()["backgroundColor"][1].get<float>(), gSettings.getSettings()["backgroundColor"][2].get<float>(), gSettings.getSettings()["backgroundColor"][3].get<float>());
    shader_toggle = gSettings.getSettings()["shader_toggle"].get<bool>();
    // Get text color from current theme
    std::string currentTheme = gSettings.getCurrentTheme();
    auto &textColor = gSettings.getSettings()["themes"][currentTheme]["text"];
    ImVec4 textCol(textColor[0].get<float>(), textColor[1].get<float>(), textColor[2].get<float>(), textColor[3].get<float>());

    // Apply text color to all text-related ImGui elements
    style.Colors[ImGuiCol_Text] = textCol; // Regular text
    style.Colors[ImGuiCol_TextDisabled] = ImVec4(textCol.x * 0.6f, textCol.y * 0.6f, textCol.z * 0.6f, textCol.w);
    style.Colors[ImGuiCol_TextSelectedBg] = ImVec4(1.0f, 0.1f, 0.7f, 0.3f); // Neon pink with 30% alpha

    // Rest of your existing settings
    style.ScrollbarSize = 30.0f;
    style.ScaleAllSizes(1.0f);

    style.Colors[ImGuiCol_ScrollbarBg] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
    style.Colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
    style.Colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
    style.Colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);

    ImGui::GetIO().FontGlobalScale = gSettings.getSettings()["fontSize"].get<float>() / 16.0f;
}

ImFont *LoadFont(const std::string &fontName, float fontSize) {
    ImGuiIO &io = ImGui::GetIO();
    std::string fontPath = "fonts/" + fontName + ".ttf";

    char cwd[1024];
    if (getcwd(cwd, sizeof(cwd)) != NULL) {
        std::cout << "\033[32mMain:\033[0m opening working directory : " << cwd << std::endl;
    } else {
        std::cerr << "getcwd() error" << std::endl;
    }

    std::cout << "\033[32mMain:\033[0m Attempting to load font from: " << fontPath << std::endl;
    if (!std::filesystem::exists(fontPath)) {
        std::cerr << "\033[32mMain:\033[0m Font file does not exist: " << fontPath << std::endl;
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

    // Create config for the main font (from settings)
    ImFontConfig config_main;
    config_main.MergeMode = false;
    config_main.GlyphRanges = ranges;

    // Load the main font first (from settings)
    ImFont *font = io.Fonts->AddFontFromFileTTF(fontPath.c_str(), fontSize, &config_main, ranges);

    // Then merge DejaVu Sans just for the Braille range
    ImFontConfig config_braille;
    config_braille.MergeMode = true; // Important! This will merge with previous font
    static const ImWchar braille_ranges[] = {0x2800, 0x28FF, 0};
    io.Fonts->AddFontFromFileTTF("fonts/DejaVuSans.ttf", fontSize, &config_braille, braille_ranges);

    if (font == nullptr) {
        std::cerr << "\033[32mMain:\033[0m Failed to load font: " << fontName << std::endl;
        return io.Fonts->AddFontDefault();
    }
    std::cout << "\033[32mMain:\033[0m Successfully loaded font: " << fontName << std::endl;
    return font;
}

void InitializeGLFW() {
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        exit(1);
    }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif
}

// Handle keyboard shortcuts
void handleKeyboardShortcuts() {
    bool ctrl_pressed = ImGui::GetIO().KeyCtrl;
    if (ctrl_pressed && ImGui::IsKeyPressed(ImGuiKey_S, false)) {
        showSidebar = !showSidebar;  // Toggle sidebar visibility
        std::cout << "\033[32mMain:\033[0m Toggled sidebar visibility" << std::endl;
    }
    if (ctrl_pressed && ImGui::IsKeyPressed(ImGuiKey_T, false)) {
        gTerminal.toggleVisibility();
        gFileExplorer.saveCurrentFile();
        if (gTerminal.isTerminalVisible()) {
            ClosePopper::closeAll();
        }
    }
    if (ctrl_pressed && ImGui::IsKeyPressed(ImGuiKey_Comma, false)) {
        std::cout << "\033[95mSettings:\033[0m Popup window toggled" << std::endl;
        gFileExplorer.setShowWelcomeScreen(false);
        gSettings.toggleSettingsWindow();
    }
    if (ctrl_pressed && ImGui::IsKeyPressed(ImGuiKey_Slash, false)) {
        std::cout << "\033[32mMain:\033[0m Ctrl+/ pressed - Resetting to welcome screen" << std::endl;
        ClosePopper::closeAll();
        gFileExplorer.setShowWelcomeScreen(!gFileExplorer.getShowWelcomeScreen());
        if (gTerminal.isTerminalVisible()) {
            gTerminal.toggleVisibility();
        }
        gFileExplorer.saveCurrentFile();
    }
    if (ctrl_pressed && ImGui::IsKeyPressed(ImGuiKey_O, false)) {
        std::cout << "\033[32mMain:\033[0m Ctrl+O pressed - triggering file dialog" << std::endl;
        ClosePopper::closeAll();
        gFileExplorer.setShowWelcomeScreen(false);
        gFileExplorer.saveCurrentFile();
        gFileExplorer.setShowFileDialog(true);
    }
}

// Render file explorer section
void renderFileExplorer(float explorerWidth) {
    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_ChildBorderSize, 1.0f);
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.5f, 0.5f, 0.5f, 0.0f));
    ImGui::BeginChild("File Explorer", ImVec2(explorerWidth, -1), true, ImGuiWindowFlags_NoScrollbar);

    ImGui::Text("File Explorer");
    ImGui::Separator();
    if (!gFileExplorer.getSelectedFolder().empty()) {
        gFileExplorer.displayFileTree(gFileExplorer.getRootNode());
    }
    ImGui::EndChild();
    ImGui::PopStyleColor();
    ImGui::PopStyleVar(2);
}

// Render editor header
void renderEditorHeader(ImFont *currentFont) {
    // Create a group for the header line
    ImGui::BeginGroup();

    // Push font for the text
    ImGui::PushFont(currentFont);

    // Display the file path and get line height while font is pushed
    float lineHeight = ImGui::GetTextLineHeight();
    ImGui::Text("Editor - %s", gFileExplorer.getCurrentFile().empty() ? "No file selected" : gFileExplorer.getCurrentFile().c_str());

    // Calculate sizes
    float iconSize = lineHeight - 5;
    float verticalOffset = (lineHeight - iconSize) / 2;

    // Position for the icon
    ImGui::SameLine(ImGui::GetContentRegionAvail().x - iconSize - 8);
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + verticalOffset);

    ImGui::PopFont();

    renderSettingsIcon(iconSize);
    ImGui::EndGroup();
    ImGui::Separator();
}

// Render settings icon
void renderSettingsIcon(float iconSize) {
    bool settingsOpen = gSettings.showSettingsWindow;

    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0, 0, 0, 0));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0, 0, 0, 0));

    if (!settingsOpen) {
        ImVec2 cursor_pos = ImGui::GetCursorPos();
        if (ImGui::InvisibleButton("##gear-hitbox", ImVec2(iconSize, iconSize))) {
            gSettings.toggleSettingsWindow();
        }
        bool isHovered = ImGui::IsItemHovered();
        ImGui::SetCursorPos(cursor_pos);
        ImTextureID icon = isHovered ? gFileExplorer.getIcon("gear-hover") : gFileExplorer.getIcon("gear");
        ImGui::Image(icon, ImVec2(iconSize, iconSize));
    } else {
        ImGui::Image(gFileExplorer.getIcon("gear"), ImVec2(iconSize, iconSize));
    }

    ImGui::PopStyleColor(3);
    ImGui::PopStyleVar();
}

void renderSplitter(float padding, float availableWidth) {
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.9f, 0.9f, 0.9f, 0.2f));
    ImGui::SameLine(0, 0);
    float splitterWidth = 2.0f;
    ImGui::Button("##vsplitter", ImVec2(splitterWidth, -1));
    ImGui::PopStyleColor();

    if (ImGui::IsItemActive()) {
        float mousePosInWindow = ImGui::GetMousePos().x - ImGui::GetWindowPos().x;
        float leftPadding = padding * 2;
        float rightPadding = padding * 2 + 6;
        float newSplitPos = (mousePosInWindow - leftPadding) / (availableWidth - leftPadding - rightPadding);
        newSplitPos = Clamp(newSplitPos, 0.1f, 0.9f);
        gSettings.setSplitPos(newSplitPos);
    }
}

void renderEditor(ImFont *currentFont, float editorWidth) {
    ImGui::SameLine(0, 0); // Add this to ensure proper layout
    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 0.0f);
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.2f, 0.2f, 0.2f, 0.0f));

    // Begin the editor child window
    ImGui::BeginChild("Editor", ImVec2(editorWidth, -1), true);

    // Render the editor header and content
    renderEditorHeader(currentFont);
    gFileExplorer.renderFileContent();

    ImGui::EndChild();
    ImGui::PopStyleColor();
    ImGui::PopStyleVar();
}

void RenderMainWindow(ImFont *currentFont, float &explorerWidth, float &editorWidth) {
    handleKeyboardShortcuts();

    if (gTerminal.isTerminalVisible()) {
        // ImGui::PushFont(currentFont);
        gTerminal.render();
        // ImGui::PopFont();
        return;
    }

    if (gFileExplorer.getShowWelcomeScreen()) {
        gWelcome.render();
        return;
    }

    ImGui::PushFont(currentFont);
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);

    ImGui::Begin("Main Window", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize);

    ImGui::PopFont();

    // Calculate window dimensions
    float windowWidth = ImGui::GetWindowWidth();
    float padding = ImGui::GetStyle().WindowPadding.x;
    float splitterWidth = 2.0f;
    float availableWidth = windowWidth - padding * 3 - splitterWidth;
    if (showSidebar) {
        // show sidebar
        explorerWidth = availableWidth * gSettings.getSplitPos();
        editorWidth = availableWidth - explorerWidth - 6;
        
        renderFileExplorer(explorerWidth);
        renderSplitter(padding, availableWidth);
    } else {
        // When sidebar is hidden, editor takes full width
        editorWidth = availableWidth;
    }
    renderEditor(currentFont, editorWidth);

    ImGui::End();
}

void updateFileExplorer() {
    static float last_refresh_time = 0.0f;
    float current_time = ImGui::GetTime();

    if (current_time - last_refresh_time > 5.0f) { // Refresh every 5 seconds
        gFileExplorer.refreshFileTree();
        last_refresh_time = current_time;
    }
}

void handleEvents(GLFWwindow *window) {
    if (glfwGetWindowAttrib(window, GLFW_FOCUSED)) {
        glfwPollEvents();
    } else {
        glfwWaitEventsTimeout(0.016); // 16ms ~60Hz timeout
    }
}

void handleBackgroundUpdates(double currentTime, double &lastSettingsTime, double &lastTreeTime) {
    if (currentTime - lastSettingsTime >= SETTINGS_CHECK_INTERVAL) {
        gSettings.checkSettingsFile();
        lastSettingsTime = currentTime;
    }

    if (currentTime - lastTreeTime >= FILE_TREE_REFRESH_INTERVAL) {
        gFileExplorer.refreshFileTree();
        lastTreeTime = currentTime;
    }
}

void handleFramebuffer(int width, int height, GLuint &fb, GLuint &tex, GLuint &rbo, int &last_w, int &last_h, bool &initialized) {

    if (width == last_w && height == last_h && initialized) {
        return;
    }

    if (initialized) {
        glDeleteFramebuffers(1, &fb);
        glDeleteTextures(1, &tex);
        glDeleteRenderbuffers(1, &rbo);
    }

    glGenFramebuffers(1, &fb);
    glGenTextures(1, &tex);
    glGenRenderbuffers(1, &rbo);

    // Setup texture
    glBindFramebuffer(GL_FRAMEBUFFER, fb);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex, 0);

    // Setup renderbuffer
    glBindRenderbuffer(GL_RENDERBUFFER, rbo);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rbo);

    last_w = width;
    last_h = height;
    initialized = true;
}

// ImGui frame setup/handling
void setupImGuiFrame() {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}

void handleWindowFocus(GLFWwindow *window, bool &windowFocused) {
    bool currentFocus = glfwGetWindowAttrib(window, GLFW_FOCUSED) != 0;
    if (windowFocused && !currentFocus) {
        gFileExplorer.saveCurrentFile();
    }
    windowFocused = currentFocus;
}

// Settings and font handling
void handleSettingsChanges(ImGuiStyle &style, bool &needFontReload) {
    if (gSettings.hasSettingsChanged()) {
        ApplySettings(style);
        if (gSettings.hasThemeChanged()) {
            gEditor.setTheme(gSettings.getCurrentTheme());
            gFileExplorer.refreshSyntaxHighlighting();
            gSettings.resetThemeChanged();
        }
        if (gSettings.hasFontChanged() || gSettings.hasFontSizeChanged()) {
            needFontReload = true;
        }
        gSettings.resetSettingsChanged();
    }
}

void handleFontReload(ImFont *&currentFont, bool &needFontReload) {
    if (needFontReload) {
        ImGui_ImplOpenGL3_DestroyFontsTexture();
        ImGui::GetIO().Fonts->Clear();
        currentFont = LoadFont(gSettings.getCurrentFont(), gSettings.getFontSize());
        ImGui::GetIO().Fonts->Build();
        ImGui_ImplOpenGL3_CreateFontsTexture();
        gSettings.resetFontChanged();
        gSettings.resetFontSizeChanged();
        needFontReload = false;
    }
}

// File dialog handling
void handleFileDialog() {
    if (gFileExplorer.showFileDialog()) {
        gFileExplorer.openFolderDialog();
        if (!gFileExplorer.getSelectedFolder().empty()) {
            auto &rootNode = gFileExplorer.getRootNode();
            rootNode.name = fs::path(gFileExplorer.getSelectedFolder()).filename().string();
            rootNode.fullPath = gFileExplorer.getSelectedFolder();
            rootNode.isDirectory = true;
            rootNode.children.clear();
            gFileExplorer.buildFileTree(gFileExplorer.getSelectedFolder(), rootNode);
            gFileExplorer.setShowWelcomeScreen(false);
        }
    }
}

void beginFrame(int display_w, int display_h) {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, display_w, display_h);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
}

void renderMainContent(ImFont *currentFont, float &explorerWidth, float &editorWidth) {
    RenderMainWindow(currentFont, explorerWidth, editorWidth);
    gBookmarks.renderBookmarksWindow();
    gSettings.renderSettingsWindow();
}

void renderWithShader(Shader &shader, GLuint fullFramebuffer, GLuint fullRenderTexture, GLuint quadVAO, int display_w, int display_h, double currentTime) {
    glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fullFramebuffer);
    glBlitFramebuffer(0, 0, display_w, display_h, 0, 0, display_w, display_h, GL_COLOR_BUFFER_BIT, GL_NEAREST);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glClear(GL_COLOR_BUFFER_BIT);
    glUseProgram(shader.shaderProgram);

    GLint timeLocation = glGetUniformLocation(shader.shaderProgram, "time");
    GLint screenTextureLocation = glGetUniformLocation(shader.shaderProgram, "screenTexture");
    GLint resolutionLocation = glGetUniformLocation(shader.shaderProgram, "resolution");

    if (timeLocation != -1)
        glUniform1f(timeLocation, currentTime);
    if (screenTextureLocation != -1)
        glUniform1i(screenTextureLocation, 0);
    if (resolutionLocation != -1) {
        glUniform2f(resolutionLocation, static_cast<float>(display_w), static_cast<float>(display_h));
    }

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, fullRenderTexture);
    glBindVertexArray(quadVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
}

void handleFrameTiming(std::chrono::high_resolution_clock::time_point frame_start) {
    auto frame_end = std::chrono::high_resolution_clock::now();
    auto frame_duration = frame_end - frame_start;
    std::this_thread::sleep_for(TARGET_FRAME_DURATION - frame_duration);
}

bool initializeGraphics(GLFWwindow *&window, Shader &crtShader) {
    InitializeGLFW();
    window = CreateWindow();
    glfwSetWindowRefreshCallback(window, [](GLFWwindow *window) { glfwPostEmptyEvent(); });

    glewExperimental = GL_TRUE;
    if (GLenum err = glewInit(); GLEW_OK != err) {
        std::cerr << "GLEW initialization failed: " << glewGetErrorString(err) << std::endl;
        glfwTerminate();
        return false;
    }
    glGetError();

    if (!crtShader.loadShader("shaders/vertex.glsl", "shaders/fragment.glsl")) {
        std::cerr << "Shader load failed" << std::endl;
        glfwTerminate();
        return false;
    }

    return true;
}

void initializeImGuiAndResources(GLFWwindow *window, ImFont *&currentFont) {
    InitializeImGui(window);
    gDebugConsole.toggleVisibility();
    gSettings.loadSettings();
    gEditor.setTheme(gSettings.getCurrentTheme());
    ApplySettings(ImGui::GetStyle());
    gFileExplorer.loadIcons();

    currentFont = LoadFont(gSettings.getCurrentFont(), gSettings.getSettings()["fontSize"].get<float>());
    if (currentFont == nullptr) {
        std::cerr << "Failed to load font, using default font" << std::endl;
        currentFont = ImGui::GetIO().Fonts->AddFontDefault();
    }
}

void cleanup(GLFWwindow *window, FramebufferState &fb, ShaderQuad &quad) {
    quad.cleanup();
    if (fb.initialized) {
        glDeleteFramebuffers(1, &fb.framebuffer);
        glDeleteTextures(1, &fb.renderTexture);
        glDeleteRenderbuffers(1, &fb.rbo);
    }

    gSettings.saveSettings();
    gFileExplorer.saveCurrentFile();

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(window);
    glfwTerminate();
}
