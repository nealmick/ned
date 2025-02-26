/*
    File: ned.cpp
    Description: Main application class implementation for NED text editor.
*/

#include "ned.h"
#include "editor/bookmarks.h"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "util/debug_console.h"
#include "util/settings.h"
#include "util/terminal.h"
#include "util/welcome.h"

#include <filesystem>
#include <iostream>
#include <thread>

extern bool shader_toggle;
extern bool showSidebar;

Ned::Ned() : window(nullptr), currentFont(nullptr), needFontReload(false), windowFocused(true), explorerWidth(0.0f), editorWidth(0.0f), initialized(false) {}

Ned::~Ned()
{
    if (initialized) {
        cleanup();
    }
}
void ApplySettings(ImGuiStyle &style);

void Ned::ShaderQuad::initialize()
{
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

void Ned::ShaderQuad::cleanup()
{
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
}

bool Ned::initialize()
{
    if (!initializeGraphics()) {
        return false;
    }

    // Set up the scroll callback for smooth scrolling
    glfwSetWindowUserPointer(window, this);
    glfwSetScrollCallback(window, [](GLFWwindow *window, double xoffset, double yoffset) {
        Ned *ned = static_cast<Ned *>(glfwGetWindowUserPointer(window));
        ned->handleScrollEvent(xoffset, yoffset);
    });

    initializeImGui();
    initializeResources();

    quad.initialize();
    initialized = true;

    return true;
}

bool Ned::initializeGraphics()
{
    if (!glfwInit()) {
        std::cerr << "🔴 Failed to initialize GLFW" << std::endl;
        return false;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    window = glfwCreateWindow(1200, 750, "NED", NULL, NULL);
    if (!window) {
        std::cerr << "🔴 Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return false;
    }

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);
    glfwSetWindowRefreshCallback(window, [](GLFWwindow *window) { glfwPostEmptyEvent(); });

    glewExperimental = GL_TRUE;
    if (GLenum err = glewInit(); GLEW_OK != err) {
        std::cerr << "🔴 GLEW initialization failed: " << glewGetErrorString(err) << std::endl;
        glfwTerminate();
        return false;
    }
    glGetError(); // Clear any GLEW startup errors

    if (!crtShader.loadShader("shaders/vertex.glsl", "shaders/fragment.glsl")) {
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
}
void Ned::initializeResources()
{
    gDebugConsole.toggleVisibility();
    gSettings.loadSettings();
    gEditor.setTheme(gSettings.getCurrentTheme());

    // Apply settings to the ImGui style.
    ApplySettings(ImGui::GetStyle());

    shader_toggle = gSettings.getSettings()["shader_toggle"].get<bool>();
    gFileExplorer.loadIcons();

    currentFont = loadFont(gSettings.getCurrentFont(), gSettings.getSettings()["fontSize"].get<float>());
    if (!currentFont) {
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
    // This is a good candidate for moving to a separate FontManager class in the future
    ImGuiIO &io = ImGui::GetIO();
    std::string fontPath = "fonts/" + fontName + ".ttf";

    if (!std::filesystem::exists(fontPath)) {
        std::cerr << "🔴 Font file does not exist: " << fontPath << std::endl;
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

    ImFont *font = io.Fonts->AddFontFromFileTTF(fontPath.c_str(), fontSize, &config_main, ranges);

    // Merge DejaVu Sans for Braille
    ImFontConfig config_braille;
    config_braille.MergeMode = true;
    static const ImWchar braille_ranges[] = {0x2800, 0x28FF, 0};
    io.Fonts->AddFontFromFileTTF("fonts/DejaVuSans.ttf", fontSize, &config_braille, braille_ranges);

    if (!font) {
        std::cerr << "🔴 Failed to load font: " << fontName << std::endl;
        return io.Fonts->AddFontDefault();
    }

    std::cout << "\033[32mNed:\033[0m Successfully loaded font: " << fontName << std::endl;
    return font;
}

void Ned::run()
{
    if (!initialized) {
        std::cerr << "🔴 Cannot run: Not initialized" << std::endl;
        return;
    }

    while (!glfwWindowShouldClose(window)) {
        auto frame_start = std::chrono::high_resolution_clock::now();

        // Handle events and updates
        handleEvents();
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
    if (glfwGetWindowAttrib(window, GLFW_FOCUSED)) {
        glfwPollEvents();
    } else {
        glfwWaitEventsTimeout(0.016); // 16ms ~60Hz timeout
    }
}

void Ned::handleBackgroundUpdates(double currentTime)
{
    if (currentTime - timing.lastSettingsCheck >= SETTINGS_CHECK_INTERVAL) {
        gSettings.checkSettingsFile();
        timing.lastSettingsCheck = currentTime;
    }

    if (currentTime - timing.lastFileTreeRefresh >= FILE_TREE_REFRESH_INTERVAL) {
        gFileExplorer.refreshFileTree();
        timing.lastFileTreeRefresh = currentTime;
    }
}

void Ned::handleFramebuffer(int width, int height)
{
    if (width == fb.last_display_w && height == fb.last_display_h && fb.initialized) {
        return;
    }

    if (fb.initialized) {
        glDeleteFramebuffers(1, &fb.framebuffer);
        glDeleteTextures(1, &fb.renderTexture);
        glDeleteRenderbuffers(1, &fb.rbo);
    }

    glGenFramebuffers(1, &fb.framebuffer);
    glGenTextures(1, &fb.renderTexture);
    glGenRenderbuffers(1, &fb.rbo);

    // Setup texture
    glBindFramebuffer(GL_FRAMEBUFFER, fb.framebuffer);
    glBindTexture(GL_TEXTURE_2D, fb.renderTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, fb.renderTexture, 0);

    // Setup renderbuffer
    glBindRenderbuffer(GL_RENDERBUFFER, fb.rbo);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, fb.rbo);

    fb.last_display_w = width;
    fb.last_display_h = height;
    fb.initialized = true;
}

void Ned::setupImGuiFrame()
{
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();

    // Apply smooth scrolling right before ImGui's NewFrame
    handleScrollInput();

    ImGui::NewFrame();
}

void Ned::handleWindowFocus()
{
    bool currentFocus = glfwGetWindowAttrib(window, GLFW_FOCUSED) != 0;
    if (windowFocused && !currentFocus) {
        gFileExplorer.saveCurrentFile();
    }
    windowFocused = currentFocus;
}
void Ned::handleKeyboardShortcuts()
{
    ImGuiIO &io = ImGui::GetIO();
    // Accept either Ctrl or Super (Command on macOS)
    bool modPressed = io.KeyCtrl || io.KeySuper;

    if (modPressed && ImGui::IsKeyPressed(ImGuiKey_S, false)) {
        showSidebar = !showSidebar; // Toggle sidebar visibility
        std::cout << "\033[32mNed:\033[0m Toggled sidebar visibility" << std::endl;
    }
    if (modPressed && ImGui::IsKeyPressed(ImGuiKey_T, false)) {
        gTerminal.toggleVisibility();
        gFileExplorer.saveCurrentFile();
        if (gTerminal.isTerminalVisible()) {
            ClosePopper::closeAll();
        }
    }
    if (modPressed && ImGui::IsKeyPressed(ImGuiKey_Comma, false)) {
        std::cout << "\033[95mSettings:\033[0m Popup window toggled" << std::endl;
        gFileExplorer.setShowWelcomeScreen(false);
        gSettings.toggleSettingsWindow();
    }
    if (modPressed && ImGui::IsKeyPressed(ImGuiKey_Slash, false)) {
        std::cout << "\033[32mNed:\033[0m Ctrl+/ pressed - Resetting to welcome screen" << std::endl;
        ClosePopper::closeAll();
        gFileExplorer.setShowWelcomeScreen(!gFileExplorer.getShowWelcomeScreen());
        if (gTerminal.isTerminalVisible()) {
            gTerminal.toggleVisibility();
        }
        gFileExplorer.saveCurrentFile();
    }
    if (modPressed && ImGui::IsKeyPressed(ImGuiKey_O, false)) {
        std::cout << "\033[32mNed:\033[0m Ctrl+O pressed - triggering file dialog" << std::endl;
        ClosePopper::closeAll();
        gFileExplorer.setShowWelcomeScreen(false);
        gFileExplorer.saveCurrentFile();
        gFileExplorer.setShowFileDialog(true);
    }
}

void Ned::renderFileExplorer(float explorerWidth)
{
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
void Ned::renderEditorHeader(ImFont *currentFont)
{
    ImGui::BeginGroup();
    ImGui::PushFont(currentFont);

    // Determine the base icon size (equal to font size, or adjust with a multiplier)
    float iconSize = ImGui::GetFontSize();
    std::string currentFile = gFileExplorer.getCurrentFile();

    // Render the left part: file icon (if available) and file path text.
    if (currentFile.empty()) {
        ImGui::Text("Editor - No file selected");
    } else {
        // Get the file type icon.
        ImTextureID fileIcon = gFileExplorer.getIconForFile(currentFile);
        if (fileIcon) {
            // Calculate the center point of the line
            float textHeight = ImGui::GetTextLineHeight();
            float lineCenterY = ImGui::GetCursorPosY() + (textHeight / 2.0f);

            // Position the icon so its center aligns with the line center
            float iconTopY = lineCenterY - (iconSize / 2.0f);
            ImGui::SetCursorPosY(iconTopY);

            ImGui::Image(fileIcon, ImVec2(iconSize, iconSize));
            ImGui::SameLine();

            // Reset cursor Y for text
            ImGui::SetCursorPosY(ImGui::GetCursorPosY() + (iconSize - textHeight) / 2.0f);
        }
        ImGui::Text("Editor - %s", currentFile.c_str());
    }

    // Now, to place the settings toggle on the far right,
    // get the available width of the current window.
    float availableWidth = ImGui::GetWindowContentRegionMax().x;
    float rightPadding = 10.0f;               // Add padding from the right edge
    float adjustedIconSize = iconSize * 0.8f; // Make the icon slightly smaller

    // Calculate the line's center point
    float textHeight = ImGui::GetTextLineHeight();
    float lineCenterY = ImGui::GetCursorPosY() - textHeight + (textHeight / 2.0f);

    // Position the icon so its center aligns with the line center
    float iconTopY = lineCenterY - (adjustedIconSize / 2.0f);

    // Move the cursor to a position near the far right with padding
    ImGui::SameLine(availableWidth - adjustedIconSize - rightPadding);

    // Apply vertical centering by directly setting cursor Y
    ImGui::SetCursorPosY(iconTopY);

    // Render the settings icon
    renderSettingsIcon(adjustedIconSize * 0.80f);

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

void Ned::renderSplitter(float padding, float availableWidth)
{
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
        newSplitPos = clamp(newSplitPos, 0.1f, 0.9f);
        gSettings.setSplitPos(newSplitPos);
    }
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
    handleKeyboardShortcuts();

    if (gTerminal.isTerminalVisible()) {
        gTerminal.render();
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

    float windowWidth = ImGui::GetWindowWidth();
    float padding = ImGui::GetStyle().WindowPadding.x;
    float splitterWidth = 2.0f;
    float availableWidth = windowWidth - padding * 3 - splitterWidth;

    if (showSidebar) {
        explorerWidth = availableWidth * gSettings.getSplitPos();
        editorWidth = availableWidth - explorerWidth - 6;

        renderFileExplorer(explorerWidth);
        renderSplitter(padding, availableWidth);
    } else {
        editorWidth = availableWidth;
    }
    renderEditor(currentFont, editorWidth);

    ImGui::End();
}
void Ned::renderFrame()
{
    int display_w, display_h;
    glfwGetFramebufferSize(window, &display_w, &display_h);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, display_w, display_h);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    // Render the main window content.
    renderMainWindow();
    // Render the pop-up windows.
    gBookmarks.renderBookmarksWindow();
    gSettings.renderSettingsWindow();

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    // If shader post-processing is enabled, run the shader pass.
    if (shader_toggle) {
        renderWithShader(display_w, display_h, glfwGetTime());
    }

    glfwSwapBuffers(window);
}

void Ned::handleFileDialog()
{
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
void Ned::renderWithShader(int display_w, int display_h, double currentTime)
{
    glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fb.framebuffer);
    glBlitFramebuffer(0, 0, display_w, display_h, 0, 0, display_w, display_h, GL_COLOR_BUFFER_BIT, GL_NEAREST);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glClear(GL_COLOR_BUFFER_BIT);
    glUseProgram(crtShader.shaderProgram);

    GLint timeLocation = glGetUniformLocation(crtShader.shaderProgram, "time");
    GLint screenTextureLocation = glGetUniformLocation(crtShader.shaderProgram, "screenTexture");
    GLint resolutionLocation = glGetUniformLocation(crtShader.shaderProgram, "resolution");

    if (timeLocation != -1)
        glUniform1f(timeLocation, currentTime);
    if (screenTextureLocation != -1)
        glUniform1i(screenTextureLocation, 0);
    if (resolutionLocation != -1)
        glUniform2f(resolutionLocation, static_cast<float>(display_w), static_cast<float>(display_h));

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, fb.renderTexture);
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
    if (gSettings.hasSettingsChanged()) {
        ImGuiStyle &style = ImGui::GetStyle();
        style.Colors[ImGuiCol_WindowBg] = ImVec4(gSettings.getSettings()["backgroundColor"][0].get<float>(), gSettings.getSettings()["backgroundColor"][1].get<float>(), gSettings.getSettings()["backgroundColor"][2].get<float>(), gSettings.getSettings()["backgroundColor"][3].get<float>());

        // Update shader toggle here:
        shader_toggle = gSettings.getSettings()["shader_toggle"].get<bool>();

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

void Ned::handleFontReload()
{
    if (needFontReload) {
        ImGui_ImplOpenGL3_DestroyFontsTexture();
        ImGui::GetIO().Fonts->Clear();
        currentFont = loadFont(gSettings.getCurrentFont(), gSettings.getSettings()["fontSize"].get<float>());
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

void ApplySettings(ImGuiStyle &style)
{
    // Set the window background color from settings.
    style.Colors[ImGuiCol_WindowBg] = ImVec4(gSettings.getSettings()["backgroundColor"][0].get<float>(), gSettings.getSettings()["backgroundColor"][1].get<float>(), gSettings.getSettings()["backgroundColor"][2].get<float>(), gSettings.getSettings()["backgroundColor"][3].get<float>());
    shader_toggle = gSettings.getSettings()["shader_toggle"].get<bool>();

    // Set text colors from the current theme.
    std::string currentTheme = gSettings.getCurrentTheme();
    auto &textColor = gSettings.getSettings()["themes"][currentTheme]["text"];
    ImVec4 textCol(textColor[0].get<float>(), textColor[1].get<float>(), textColor[2].get<float>(), textColor[3].get<float>());
    style.Colors[ImGuiCol_Text] = textCol;
    style.Colors[ImGuiCol_TextDisabled] = ImVec4(textCol.x * 0.6f, textCol.y * 0.6f, textCol.z * 0.6f, textCol.w);
    style.Colors[ImGuiCol_TextSelectedBg] = ImVec4(1.0f, 0.1f, 0.7f, 0.3f);

    // Hide scrollbars by setting their alpha to 0.
    style.ScrollbarSize = 30.0f;
    style.ScaleAllSizes(1.0f);
    style.Colors[ImGuiCol_ScrollbarBg] = ImVec4(0, 0, 0, 0);
    style.Colors[ImGuiCol_ScrollbarGrab] = ImVec4(0, 0, 0, 0);
    style.Colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0, 0, 0, 0);
    style.Colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0, 0, 0, 0);

    // Set the global font scale.
    ImGui::GetIO().FontGlobalScale = gSettings.getSettings()["fontSize"].get<float>() / 16.0f;
}

void Ned::handleScrollEvent(double xoffset, double yoffset)
{
    // Convert raw input to target velocity
    // Use larger values for more pronounced effect
    const float velocityScale = 15.0f;

    // Add to the target velocity (where we want to go)
    targetScrollVelocity.x += static_cast<float>(xoffset) * velocityScale;
    targetScrollVelocity.y += static_cast<float>(yoffset) * velocityScale;

    // Optional: Cap maximum velocity
    const float maxVelocity = 50.0f;
    if (targetScrollVelocity.x > maxVelocity)
        targetScrollVelocity.x = maxVelocity;
    if (targetScrollVelocity.x < -maxVelocity)
        targetScrollVelocity.x = -maxVelocity;
    if (targetScrollVelocity.y > maxVelocity)
        targetScrollVelocity.y = maxVelocity;
    if (targetScrollVelocity.y < -maxVelocity)
        targetScrollVelocity.y = -maxVelocity;
}

void Ned::handleScrollInput()
{
    ImGuiIO &io = ImGui::GetIO();
    float dt = io.DeltaTime;

    // Spring physics parameters with enhanced smoothing
    const float stiffness = 3.0f; // Reduced for smoother response (was 8.0f)
    const float damping = 8.0f;   // Reduced for more gentle deceleration (was 12.0f)

    // Friction to gradually bring target velocity to zero
    const float friction = 5.0f; // Reduced for longer-lasting momentum (was 8.0f)
    targetScrollVelocity.x *= exp(-friction * dt);
    targetScrollVelocity.y *= exp(-friction * dt);

    // Calculate spring force (difference between current and target velocity)
    ImVec2 springForce;
    springForce.x = stiffness * (targetScrollVelocity.x - currentScrollVelocity.x);
    springForce.y = stiffness * (targetScrollVelocity.y - currentScrollVelocity.y);

    // Apply spring force to current velocity
    currentScrollVelocity.x += springForce.x * dt;
    currentScrollVelocity.y += springForce.y * dt;

    // Apply damping to current velocity
    currentScrollVelocity.x *= exp(-damping * dt);
    currentScrollVelocity.y *= exp(-damping * dt);

    // Set ImGui scroll values based on current velocity
    // Use smaller multiplier for more controlled scrolling speed
    const float outputMultiplier = 0.0004f; // Reduced by half for slower scrolling
    io.MouseWheel = currentScrollVelocity.y * outputMultiplier;
    io.MouseWheelH = currentScrollVelocity.x * outputMultiplier;
}