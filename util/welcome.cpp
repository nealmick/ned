#include "welcome.h"
#include "../files.h"
#include "util/debug_console.h"
#include <iostream>

Welcome &gWelcome = Welcome::getInstance();

void Welcome::calculateFPS() {
    frameCount++;
    double currentTime = glfwGetTime();

    if (currentTime - lastTime >= 1.0) {
        fps = frameCount;
        frameCount = 0;
        lastTime = currentTime;
    }
}

void Welcome::render() {
    calculateFPS(); // Update FPS count

    ImGuiViewport *viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->Pos);
    ImGui::SetNextWindowSize(viewport->Size);

    ImGui::Begin("##WelcomeScreen", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings);

    float windowWidth = ImGui::GetWindowWidth();
    float windowHeight = ImGui::GetWindowHeight();

    // FPS Counter in top right
    ImGui::SetCursorPos(ImVec2(windowWidth - 80, 10));
    ImVec4 fpsColor;
    if (fps >= 55) {
        fpsColor = ImVec4(0.0f, 1.0f, 0.0f, 1.0f); // Green for good FPS
    } else if (fps >= 30) {
        fpsColor = ImVec4(1.0f, 1.0f, 0.0f, 1.0f); // Yellow for okay FPS
    } else {
        fpsColor = ImVec4(1.0f, 0.0f, 0.0f, 1.0f); // Red for bad FPS
    }
    ImGui::TextColored(fpsColor, "FPS: %d", fps);

    // Title - "Welcome to NED"
    ImGui::SetCursorPosY(windowHeight * 0.2f);
    float titleScale = 2.0f;
    ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[0]);
    ImGui::SetWindowFontScale(titleScale);
    const char *title = "Welcome to NED";
    float titleWidth = ImGui::CalcTextSize(title).x;
    ImGui::SetCursorPosX((windowWidth - titleWidth) * 0.5f);
    ImGui::TextColored(ImVec4(0.5f, 0.8f, 1.0f, 1.0f), "%s", title);
    ImGui::SetWindowFontScale(1.0f);
    ImGui::PopFont();

    // GitHub link right under title
    const char *github = "github.com/nealmick/ned";
    float githubWidth = ImGui::CalcTextSize(github).x;
    ImGui::SetCursorPosY(windowHeight * 0.25f);
    ImGui::SetCursorPosX((windowWidth - githubWidth) * 0.5f);
    ImGui::TextColored(ImVec4(0.4f, 0.7f, 1.0f, 1.0f), "%s", github);
    ImGui::SetItemAllowOverlap();

    // Description text
    ImGui::SetCursorPosY(windowHeight * 0.35f);
    const char *description = "A lightweight, feature-rich text editor built with C++ and ImGui";
    float descWidth = ImGui::CalcTextSize(description).x;
    ImGui::SetCursorPosX((windowWidth - descWidth) * 0.5f);
    ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "%s", description);

    // Keybinds section
    ImGui::SetCursorPosY(windowHeight * 0.42f);

    // First row of keybinds
    ImGui::SetCursorPosX(windowWidth * 0.3f);
    ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "CMD + O  Open Folder");
    ImGui::SameLine(windowWidth * 0.6f);
    ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "CMD + T  Terminal");

    // Second row
    ImGui::SetCursorPosX(windowWidth * 0.3f);
    ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "CMD + B  Bookmarks");
    ImGui::SameLine(windowWidth * 0.6f);
    ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "CMD + :  Line Jump");

    // Third row
    ImGui::SetCursorPosX(windowWidth * 0.3f);
    ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "CMD + F  Find");
    ImGui::SameLine(windowWidth * 0.6f);
    ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "CMD + /  Show this window");

    // Open Folder button
    float buttonWidth = 300;
    float buttonHeight = 50;
    ImGui::SetCursorPosY(windowHeight * 0.60f);
    ImGui::SetCursorPosX((windowWidth - buttonWidth) * 0.5f);

    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.6f, 0.8f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.7f, 0.9f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.1f, 0.5f, 0.7f, 1.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 5.0f);

    if (ImGui::Button("Open Folder", ImVec2(buttonWidth, buttonHeight))) {
        std::cout << "\033[32mMain:\033[0m Welcome screen - Open Folder clicked" << std::endl;
        gFileExplorer.setShowFileDialog(true);
    }
    // Debug Console
    ImGui::SetCursorPosY(windowHeight * 0.71f);                        // Position below button
    ImGui::SetCursorPosX((windowWidth - (windowWidth * 0.6f)) * 0.5f); // Center console

    ImGui::SetCursorPosY(windowHeight * 0.71f);                        // Position below button
    ImGui::SetCursorPosX((windowWidth - (windowWidth * 0.6f)) * 0.5f); // Center console
    gDebugConsole.render();

    ImGui::PopStyleVar();
    ImGui::PopStyleColor(3);
    ImGui::End();
}