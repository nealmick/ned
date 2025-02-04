#define GL_SILENCE_DEPRECATION
#define GLEW_NO_GLU
#include <GL/glew.h>
#if defined(__APPLE__)
#define GL_GLEXT_PROTOTYPES 1
#include <OpenGL/gl.h>
#else
#include <GL/gl.h>
#endif
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <GLFW/glfw3.h>
#include <string>
#include <iostream>
#include <chrono>
#include <thread>
#include "files.h"
#include "editor.h"
#include <filesystem>
#include <unistd.h>
#include <chrono>
#include "util/bookmarks.h"
#include "util/terminal.h"
#include "util/settings.h"
#include "util/close_popper.h"

#include "shaders/shader.h"


Bookmarks gBookmarks;

float Clamp(float value, float min, float max) {
    if (value < min) return min;
    if (value > max) return max;
    return value;
}

ImFont* LoadFont(const std::string& fontName, float fontSize) {
    ImGuiIO& io = ImGui::GetIO();
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
        0x0020, 0x00FF,  // Basic Latin + Latin Supplement
        0x2500, 0x257F,  // Box Drawing Characters
        0x2580, 0x259F,  // Block Elements
        0x25A0, 0x25FF,  // Geometric Shapes
        0x2600, 0x26FF,  // Miscellaneous Symbols
        0x2700, 0x27BF,  // Dingbats
        0x2900, 0x297F,  // Supplemental Arrows-B
        0x2B00, 0x2BFF,  // Miscellaneous Symbols and Arrows
        0x3000, 0x303F,  // CJK Symbols and Punctuation
        0xE000, 0xE0FF,  // Private Use Area
        0,
    };

    // Create config for the main font (from settings)
    ImFontConfig config_main;
    config_main.MergeMode = false;
    config_main.GlyphRanges = ranges;

    // Load the main font first (from settings)
    ImFont* font = io.Fonts->AddFontFromFileTTF(fontPath.c_str(), fontSize, &config_main, ranges);

    // Then merge DejaVu Sans just for the Braille range
    ImFontConfig config_braille;
    config_braille.MergeMode = true;  // Important! This will merge with previous font
    static const ImWchar braille_ranges[] = { 0x2800, 0x28FF, 0 };
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

GLFWwindow* CreateWindow() {
    GLFWwindow* window = glfwCreateWindow(1200, 750, "NED", NULL, NULL);
    if (window == NULL) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        exit(1);
    }
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);
    return window;
}

void InitializeImGui(GLFWwindow* window) {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330");
}

void ApplySettings(ImGuiStyle& style) {
    // Background color
    style.Colors[ImGuiCol_WindowBg] = ImVec4(
        gSettings.getSettings()["backgroundColor"][0].get<float>(),
        gSettings.getSettings()["backgroundColor"][1].get<float>(),
        gSettings.getSettings()["backgroundColor"][2].get<float>(),
        gSettings.getSettings()["backgroundColor"][3].get<float>()
    );

    // Get text color from current theme
    std::string currentTheme = gSettings.getCurrentTheme();
    auto& textColor = gSettings.getSettings()["themes"][currentTheme]["text"];
    ImVec4 textCol(
        textColor[0].get<float>(),
        textColor[1].get<float>(),
        textColor[2].get<float>(),
        textColor[3].get<float>()
    );

    // Apply text color to all text-related ImGui elements
    style.Colors[ImGuiCol_Text] = textCol;                // Regular text
    style.Colors[ImGuiCol_TextDisabled] = ImVec4(textCol.x * 0.6f, textCol.y * 0.6f, textCol.z * 0.6f, textCol.w);
    style.Colors[ImGuiCol_TextSelectedBg] = ImVec4(1.0f, 0.1f, 0.7f, 0.3f);  // Neon pink with 30% alpha

    // Rest of your existing settings
    style.ScrollbarSize = 30.0f;
    style.ScaleAllSizes(1.0f);
    
    style.Colors[ImGuiCol_ScrollbarBg] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
    style.Colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
    style.Colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
    style.Colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);

    ImGui::GetIO().FontGlobalScale = gSettings.getSettings()["fontSize"].get<float>() / 16.0f;
}

void RenderMainWindow(ImFont* currentFont, float& explorerWidth, float& editorWidth) {
    bool ctrl_pressed = ImGui::GetIO().KeyCtrl;
        // Add terminal toggle with Cmd/Ctrl+T
    if (ctrl_pressed && ImGui::IsKeyPressed(ImGuiKey_T, false)) {
        gTerminal.toggleVisibility();
        if (gTerminal.isTerminalVisible()) {
            ClosePopper::closeAll();  // Closes all popups, no exceptions
        }
    }
    if (ctrl_pressed && ImGui::IsKeyPressed(ImGuiKey_Comma, false)) {
        std::cout << "\033[95mSettings:\033[0m Popup window toggled" << std::endl;
        gSettings.toggleSettingsWindow();
    }
    if (gTerminal.isTerminalVisible()) {
        gTerminal.render();
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
    explorerWidth = availableWidth * gSettings.getSplitPos();
    editorWidth = availableWidth - explorerWidth - 6;

    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_ChildBorderSize, 1.0f);
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.5f, 0.5f, 0.5f, 0.0f));
    ImGui::BeginChild("File Explorer", ImVec2(explorerWidth, -1), true, ImGuiWindowFlags_NoScrollbar);

    ImGui::Text("File Explorer");
    ImGui::Separator();
    if (!gFileExplorer.getSelectedFolder().empty()) {
        //gTerminal.setWorkingDirectory(gFileExplorer.getSelectedFolder());
        gFileExplorer.displayFileTree(gFileExplorer.getRootNode());
        
    }
    ImGui::EndChild();
    ImGui::PopStyleColor();
    ImGui::PopStyleVar(2);

    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.9f, 0.9f, 0.9f, 0.2f));
    ImGui::SameLine(0, 0);
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

    ImGui::SameLine(0, 0);

    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 0.0f);
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.2f, 0.2f, 0.2f, 0.0f));
    ImGui::BeginChild("Editor", ImVec2(editorWidth, -1), true);

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

    // Pop font before handling the icon
    ImGui::PopFont();

    bool settingsOpen = gSettings.showSettingsWindow;

    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0,0));
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0,0,0,0));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0,0,0,0));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0,0,0,0));

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

    ImGui::EndGroup();
    ImGui::Separator();

    gFileExplorer.renderFileContent();
    ImGui::EndChild();
    ImGui::PopStyleColor();
    ImGui::PopStyleVar();
    ImGui::End();



}
void updateFileExplorer() {
    static float last_refresh_time = 0.0f;
    float current_time = ImGui::GetTime();
    
    if (current_time - last_refresh_time > 5.0f) {  // Refresh every 5 seconds
        gFileExplorer.refreshFileTree();
        last_refresh_time = current_time;
    }
}


int main() {
    // Initialize GLFW
    InitializeGLFW();
    
    // Create window
   GLFWwindow* window = CreateWindow();
    glfwSetWindowRefreshCallback(window, [](GLFWwindow* window) {
        glfwPostEmptyEvent();
    });
    
    // Initialize GLEW
    glewExperimental = GL_TRUE;
    GLenum err = glewInit();
    if (GLEW_OK != err) {
        std::cerr << "GLEW initialization failed: " << glewGetErrorString(err) << std::endl;
        glfwTerminate();
        return -1;
    }
    glGetError(); // Clear any error that GLEW init might have caused

    // Initialize ImGui
    InitializeImGui(window);
    
    // Load settings
    gSettings.loadSettings();   
    gEditor.setTheme(gSettings.getCurrentTheme());

    // Apply initial settings
    ApplySettings(ImGui::GetStyle());
    gFileExplorer.loadIcons();
    
    // Load font
    ImFont* currentFont = LoadFont(gSettings.getCurrentFont(), gSettings.getSettings()["fontSize"].get<float>());
    if (currentFont == nullptr) {
        std::cerr << "Failed to load font, using default font" << std::endl;
        currentFont = ImGui::GetIO().Fonts->AddFontDefault();
    }

    // Shader and Framebuffer setup
    Shader crtShader;
    if (!crtShader.loadShader("shaders/vertex.glsl", "shaders/fragment.glsl")) {
        std::cerr << "Shader load failed" << std::endl;
        glfwTerminate();
        return -1;
    }

    // Persistent framebuffer objects
    GLuint fullFramebuffer = 0, fullRenderTexture = 0, fullRbo = 0;
    int last_display_w = 0, last_display_h = 0;
    bool frameBufferInitialized = false;

    // Frame timing and performance tracking
    int frameCount = 0;
    double lastFPSTime = glfwGetTime();
    double lastSettingsCheckTime = lastFPSTime;
    double lastFileTreeRefreshTime = lastFPSTime;

    // Performance constants
    const double SETTINGS_CHECK_INTERVAL = 2.0;
    const double FILE_TREE_REFRESH_INTERVAL = 2.0;
    const double TARGET_FPS = 60.0;
    const std::chrono::duration<double> TARGET_FRAME_DURATION(1.0 / TARGET_FPS);

    // Render state tracking
    bool is_window_moving = false;
    bool needFontReload = false;
    bool windowFocused = true;

    // Quad vertices for shader rendering
    float quadVertices[] = {
        -1.0f, -1.0f,  0.0f, 0.0f,
         1.0f, -1.0f,  1.0f, 0.0f,
         1.0f,  1.0f,  1.0f, 1.0f,
        -1.0f, -1.0f,  0.0f, 0.0f,
         1.0f,  1.0f,  1.0f, 1.0f,
        -1.0f,  1.0f,  0.0f, 1.0f
    };

    // Setup quad VAO and VBO
    GLuint quadVAO, quadVBO;
    glGenVertexArrays(1, &quadVAO);
    glGenBuffers(1, &quadVBO);
    glBindVertexArray(quadVAO);
    glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);

    // Position attribute
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    // Texture coordinate attribute
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);

    float explorerWidth = 0.0f, editorWidth = 0.0f;

    while (!glfwWindowShouldClose(window)) {
        auto frame_start = std::chrono::high_resolution_clock::now();

        // Always poll events
        if (glfwGetWindowAttrib(window, GLFW_FOCUSED)) {
            // Use polling when focused for responsive input
            glfwPollEvents();
        } else {
            // Use event waiting with timeout when not focused
            glfwWaitEventsTimeout(0.016);  // 16ms ~60Hz timeout
        }

        // Performance tracking
        double currentTime = glfwGetTime();
        frameCount++;
        
        // FPS reporting
        /*
        if (currentTime - lastFPSTime >= 1.0) {
            // Color codes
            const char* color = "\033[31m";  // Default to red
            
            if (frameCount >= 50) {
                color = "\033[32m";  // Green for 50+ FPS
            } else if (frameCount >= 30) {
                color = "\033[33m";  // Orange (yellow) for 30-49 FPS
            }
            
            std::cout << color << "FPS: " << frameCount << "\033[0m" << std::endl;
            frameCount = 0;
            lastFPSTime += 1.0;
        }
        */
        // Occasional settings and file tree updates
        if (currentTime - lastSettingsCheckTime >= SETTINGS_CHECK_INTERVAL) {
            gSettings.checkSettingsFile();
            lastSettingsCheckTime = currentTime;
        }

        if (currentTime - lastFileTreeRefreshTime >= FILE_TREE_REFRESH_INTERVAL) {
            gFileExplorer.refreshFileTree();
            lastFileTreeRefreshTime = currentTime;
        }

        // Get framebuffer size
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);

        // Reinitialize framebuffer if window size changes
        if (display_w != last_display_w || display_h != last_display_h || !frameBufferInitialized) {
            if (frameBufferInitialized) {
                glDeleteFramebuffers(1, &fullFramebuffer);
                glDeleteTextures(1, &fullRenderTexture);
                glDeleteRenderbuffers(1, &fullRbo);
            }

            glGenFramebuffers(1, &fullFramebuffer);
            glGenTextures(1, &fullRenderTexture);
            glGenRenderbuffers(1, &fullRbo);

            glBindFramebuffer(GL_FRAMEBUFFER, fullFramebuffer);
            glBindTexture(GL_TEXTURE_2D, fullRenderTexture);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, display_w, display_h, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, fullRenderTexture, 0);

            glBindRenderbuffer(GL_RENDERBUFFER, fullRbo);
            glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, display_w, display_h);
            glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, fullRbo);

            last_display_w = display_w;
            last_display_h = display_h;
            frameBufferInitialized = true;
        }

        // Start ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // Window focus handling
        bool currentFocus = glfwGetWindowAttrib(window, GLFW_FOCUSED) != 0;
        if (windowFocused && !currentFocus) {
            gFileExplorer.saveCurrentFile();
        }
        windowFocused = currentFocus;

        // Handle settings changes
        if (gSettings.hasSettingsChanged()) {
            ApplySettings(ImGui::GetStyle());
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

        // File dialog handling (restored from previous version)
        if (gFileExplorer.showFileDialog()) {
            gFileExplorer.openFolderDialog();
            if (!gFileExplorer.getSelectedFolder().empty()) {
                auto& rootNode = gFileExplorer.getRootNode();
                rootNode.name = fs::path(gFileExplorer.getSelectedFolder()).filename().string();
                rootNode.fullPath = gFileExplorer.getSelectedFolder();
                rootNode.isDirectory = true;
                rootNode.children.clear();
                gFileExplorer.buildFileTree(gFileExplorer.getSelectedFolder(), rootNode);
            }
        }

        // Render to default framebuffer
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glViewport(0, 0, display_w, display_h);
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        // Render ImGui content
        RenderMainWindow(currentFont, explorerWidth, editorWidth);
        gBookmarks.renderBookmarksWindow();
        gSettings.renderSettingsWindow();

        // Complete ImGui frame
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        // Rest of the shader rendering remains the same as in previous optimized version
        // (Copy framebuffer, apply shader effect)
        glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fullFramebuffer);
        glBlitFramebuffer(0, 0, display_w, display_h, 
                         0, 0, display_w, display_h,
                         GL_COLOR_BUFFER_BIT, GL_NEAREST);

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glClear(GL_COLOR_BUFFER_BIT);

        glUseProgram(crtShader.shaderProgram);
        
        GLint timeLocation = glGetUniformLocation(crtShader.shaderProgram, "time");
        GLint screenTextureLocation = glGetUniformLocation(crtShader.shaderProgram, "screenTexture");
        GLint resolutionLocation = glGetUniformLocation(crtShader.shaderProgram, "resolution");

        if (timeLocation != -1) glUniform1f(timeLocation, currentTime);
        if (screenTextureLocation != -1) glUniform1i(screenTextureLocation, 0);
        if (resolutionLocation != -1) {
            glUniform2f(resolutionLocation, static_cast<float>(display_w), static_cast<float>(display_h));
        }

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, fullRenderTexture);
        glBindVertexArray(quadVAO);
        glDrawArrays(GL_TRIANGLES, 0, 6);

        // Handle font reloading if needed
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

        // Swap buffers
        glfwSwapBuffers(window);

        // Frame timing
        auto frame_end = std::chrono::high_resolution_clock::now();
        auto frame_duration = frame_end - frame_start;
        
        // Optional: Add sleep to maintain target FPS
        std::this_thread::sleep_for(TARGET_FRAME_DURATION - frame_duration);
    }

    // Cleanup
    glDeleteVertexArrays(1, &quadVAO);
    glDeleteBuffers(1, &quadVBO);
    if (frameBufferInitialized) {
        glDeleteFramebuffers(1, &fullFramebuffer);
        glDeleteTextures(1, &fullRenderTexture);
        glDeleteRenderbuffers(1, &fullRbo);
    }

    gSettings.saveSettings();
    gFileExplorer.saveCurrentFile();

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
