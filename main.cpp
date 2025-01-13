#define GL_SILENCE_DEPRECATION
#define GLEW_NO_GLU

// Include GLEW first
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
#include "util/settings.h"
#include "files.h"
#include "editor.h"
#include <filesystem>
#include <unistd.h>
#include <chrono>
#include "util/bookmarks.h"
#include "util/terminal.h"
#include "util/shader.h"

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
    
    ImFont* font = io.Fonts->AddFontFromFileTTF(fontPath.c_str(), fontSize);
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
    style.Colors[ImGuiCol_TextSelectedBg] = ImVec4(textCol.x * 0.3f, textCol.y * 0.3f, textCol.z * 0.7f, 0.5f);

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
        gTerminal.setWorkingDirectory(gFileExplorer.getSelectedFolder());
        gFileExplorer.displayFileTree(gFileExplorer.getRootNode());
        
    }
    ImGui::EndChild();
    ImGui::PopStyleColor();
    ImGui::PopStyleVar(2);

    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.9f, 0.9f, 0.9f, 0.2f));
    ImGui::SameLine(0, padding);
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

    ImGui::SameLine(0, padding);

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
    gBookmarks.renderBookmarksWindow();
    gSettings.renderSettingsWindow();


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

    // Frame timing setup
    const double target_fps = 60.0;
    const std::chrono::duration<double> target_frame_duration(1.0 / target_fps);
    
    bool is_window_moving = false;
    bool needFontReload = false;
    bool windowFocused = true;

    float explorerWidth = 0.0f, editorWidth = 0.0f;
    auto last_frame_time = std::chrono::high_resolution_clock::now();
    float quadVertices[] = {
        // positions   // texCoords
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

    while (!glfwWindowShouldClose(window)) {
        auto frame_start = std::chrono::high_resolution_clock::now();

        // Get current framebuffer size
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);

        // Event handling
        glfwPollEvents();

        // ImGui frame preparation
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // Window focus handling
        bool currentFocus = glfwGetWindowAttrib(window, GLFW_FOCUSED) != 0;
        if (windowFocused && !currentFocus) {
            gFileExplorer.saveCurrentFile();
        }
        windowFocused = currentFocus;

        // Refresh and check settings
        gFileExplorer.refreshFileTree();
        gSettings.checkSettingsFile();

        // Settings change handling
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

        // File dialog handling
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

        // Render main window
        RenderMainWindow(currentFont, explorerWidth, editorWidth);

        // Prepare multi-sampled framebuffer for full rendering
        GLuint fullFramebuffer, fullRenderTexture, fullRbo;
        
        // Generate framebuffer and texture
        glGenFramebuffers(1, &fullFramebuffer);
        glGenTextures(1, &fullRenderTexture);
        glGenRenderbuffers(1, &fullRbo);

        // Bind the framebuffer
        glBindFramebuffer(GL_FRAMEBUFFER, fullFramebuffer);

        // Configure texture
        glBindTexture(GL_TEXTURE_2D, fullRenderTexture);
        glTexImage2D(
            GL_TEXTURE_2D, 0, GL_RGBA, 
            display_w, display_h, 0, 
            GL_RGBA, GL_UNSIGNED_BYTE, NULL
        );
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        
        // Attach texture to framebuffer
        glFramebufferTexture2D(
            GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, 
            GL_TEXTURE_2D, fullRenderTexture, 0
        );

        // Create renderbuffer for depth and stencil
        glBindRenderbuffer(GL_RENDERBUFFER, fullRbo);
        glRenderbufferStorage(
            GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, 
            display_w, display_h
        );
        
        // Attach renderbuffer to framebuffer
        glFramebufferRenderbuffer(
            GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, 
            GL_RENDERBUFFER, fullRbo
        );

        // Verify framebuffer is complete
        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
            std::cerr << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!" << std::endl;
        }

        // Clear the framebuffer
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

        // Render entire ImGui frame to framebuffer
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        // Switch back to default framebuffer
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glViewport(0, 0, display_w, display_h);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Prepare and use shader program
        glUseProgram(crtShader.shaderProgram);
        
        // Set shader uniforms
        GLint timeLocation = glGetUniformLocation(crtShader.shaderProgram, "time");
        GLint screenTextureLocation = glGetUniformLocation(crtShader.shaderProgram, "screenTexture");
        GLint resolutionLocation = glGetUniformLocation(crtShader.shaderProgram, "resolution");
        
        // Update uniforms
        if (timeLocation != -1) {
            glUniform1f(timeLocation, glfwGetTime());
        }
        
        if (screenTextureLocation != -1) {
            glUniform1i(screenTextureLocation, 0);
        }

        if (resolutionLocation != -1) {
            glUniform2f(resolutionLocation, 
                static_cast<float>(display_w), 
                static_cast<float>(display_h)
            );
        }

        // Bind the rendered texture
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, fullRenderTexture);

        // Render full-screen quad with shader
        glBindVertexArray(quadVAO);
        glDrawArrays(GL_TRIANGLES, 0, 6);

        // Ensure ImGui is still drawn on top
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        // Swap buffers
        glfwSwapBuffers(window);

        // Cleanup resources
        glDeleteFramebuffers(1, &fullFramebuffer);
        glDeleteTextures(1, &fullRenderTexture);
        glDeleteRenderbuffers(1, &fullRbo);

        // Optional: Frame timing control
        auto frame_end = std::chrono::high_resolution_clock::now();
        auto frame_duration = std::chrono::duration_cast<std::chrono::milliseconds>(frame_end - frame_start);
        
        if (frame_duration.count() < target_frame_duration.count()) {
            std::this_thread::sleep_for(
                target_frame_duration - std::chrono::duration_cast<std::chrono::duration<double>>(frame_duration)
            );
        }
    }
    // Cleanup VAO and VBO
    glDeleteVertexArrays(1, &quadVAO);
    glDeleteBuffers(1, &quadVBO);

    // Cleanup
    gSettings.saveSettings();
    gFileExplorer.saveCurrentFile();

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}

