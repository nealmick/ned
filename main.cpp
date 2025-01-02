
#define GL_SILENCE_DEPRECATION
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
        std::cout << "Current working directory: " << cwd << std::endl;
    } else {
        std::cerr << "getcwd() error" << std::endl;
    }
    
    std::cout << "Attempting to load font from: " << fontPath << std::endl;
    
    if (!std::filesystem::exists(fontPath)) {
        std::cerr << "Font file does not exist: " << fontPath << std::endl;
        return io.Fonts->AddFontDefault();
    }
    
    ImFont* font = io.Fonts->AddFontFromFileTTF(fontPath.c_str(), fontSize);
    if (font == nullptr) {
        std::cerr << "Failed to load font: " << fontName << std::endl;
        return io.Fonts->AddFontDefault();
    }
    std::cout << "Successfully loaded font: " << fontName << std::endl;
    return font;
}

void InitializeGLFW() {
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        exit(1);
    }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    //glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);//launched window without title bar
}

GLFWwindow* CreateWindow() {
    GLFWwindow* window = glfwCreateWindow(1200, 750, "Text Editor", NULL, NULL);
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
    ImGui_ImplOpenGL3_Init("#version 120");
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
        std::cout << "Settings window toggled with Cmd+Comma" << std::endl;
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
    InitializeGLFW();
    GLFWwindow* window = CreateWindow();
    InitializeImGui(window);
    bool showSettingsWindow = false;
    gSettings.loadSettings();   
    gEditor.setTheme(gSettings.getCurrentTheme());

    bool windowFocused = true;
    ApplySettings(ImGui::GetStyle());
    gFileExplorer.loadIcons();
    
    ImFont* currentFont = LoadFont(gSettings.getCurrentFont(), gSettings.getSettings()["fontSize"].get<float>());
    if (currentFont == nullptr) {
        std::cerr << "Failed to load font, using default font" << std::endl;
        currentFont = ImGui::GetIO().Fonts->AddFontDefault();
    }

    const double target_fps = 60.0;
    const std::chrono::duration<double> target_frame_duration(1.0 / target_fps);
    bool is_window_moving = false;
    auto last_frame_time = std::chrono::high_resolution_clock::now();

    float explorerWidth = 0.0f, editorWidth = 0.0f;
    bool needFontReload = false;

    while (!glfwWindowShouldClose(window)) {
        auto frame_start = std::chrono::high_resolution_clock::now();

        int window_x, window_y;
        glfwGetWindowPos(window, &window_x, &window_y);
        static int last_window_x = window_x, last_window_y = window_y;
        is_window_moving = (window_x != last_window_x) || (window_y != last_window_y);
        last_window_x = window_x;
        last_window_y = window_y;

        if (is_window_moving) {
            glfwPollEvents();
        } else {
            glfwWaitEventsTimeout(0.1);
        }

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        bool currentFocus = glfwGetWindowAttrib(window, GLFW_FOCUSED) != 0;
        if (windowFocused && !currentFocus) {
            gFileExplorer.saveCurrentFile();
        }
        windowFocused = currentFocus;
        gFileExplorer.refreshFileTree();
        gSettings.checkSettingsFile();

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

        RenderMainWindow(currentFont, explorerWidth, editorWidth);

        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

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

        glfwSwapBuffers(window);

        auto frame_end = std::chrono::high_resolution_clock::now();
        auto frame_duration = frame_end - frame_start;
        if (!is_window_moving && frame_duration < target_frame_duration) {
            std::this_thread::sleep_for(target_frame_duration - frame_duration);
        }

        last_frame_time = frame_start;
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



