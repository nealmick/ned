#include "settings.h"
#include <iostream>
#include <fstream>
#include "imgui.h"
#include <GLFW/glfw3.h>
#include "../editor.h" 
#include "../files.h" 
//testomg asdfasdf 
Settings gSettings;

Settings::Settings() : splitPos(0.3f) {}

void Settings::loadSettings() {
    settingsPath = std::string(getenv("HOME")) + "/.ned.json";
    std::cout << "Attempting to load settings from: " << settingsPath << std::endl;
    std::ifstream settingsFile(settingsPath);
    if (settingsFile.is_open()) {
        try {
            settingsFile >> settings;
            std::cout << "Settings loaded successfully" << std::endl;
        } catch (json::parse_error& e) {
            std::cerr << "Error parsing settings file: " << e.what() << std::endl;
            settings = json::object(); // Reset to empty object
        }
    } else {
        std::cout << "Settings file not found, using defaults" << std::endl;
        settings = json::object(); // Create an empty JSON object
    }
    if (!settings.contains("font")) {
        settings["font"] = "default";
    }
    // Set default values if not present
    if (!settings.contains("backgroundColor")) {
        settings["backgroundColor"] = {0.45f, 0.55f, 0.60f, 1.00f};
    }
    if (!settings.contains("fontSize")) {
        settings["fontSize"] = 16.0f;
    }
    if (!settings.contains("splitPos")) {
        settings["splitPos"] = 0.3f;
    }
    if (!settings.contains("theme")) {
        settings["theme"] = "default";
    }
    if (!settings.contains("themes")) {
    settings["themes"] = {
        {"default", {
            {"text", {1.0f, 1.0f, 1.0f, 1.0f}},
            {"background", {0.2f, 0.2f, 0.2f, 1.0f}},
            {"keyword", {0.0f, 0.4f, 1.0f, 1.0f}},
            {"string", {0.87f, 0.87f, 0.0f, 1.0f}},
            {"number", {0.0f, 0.8f, 0.8f, 1.0f}},
            {"comment", {0.5f, 0.5f, 0.5f, 1.0f}},
            {"heading", {0.9f, 0.5f, 0.2f, 1.0f}},
            {"bold", {1.0f, 0.7f, 0.7f, 1.0f}},
            {"italic", {0.7f, 1.0f, 0.7f, 1.0f}},
            {"link", {0.4f, 0.4f, 1.0f, 1.0f}},
            {"code_block", {0.8f, 0.8f, 0.8f, 1.0f}},
            {"inline_code", {0.7f, 0.7f, 0.7f, 1.0f}},
            {"tag", {0.3f, 0.7f, 0.9f, 1.0f}},
            {"attribute", {0.9f, 0.7f, 0.3f, 1.0f}},
            {"selector", {0.9f, 0.4f, 0.6f, 1.0f}},
            {"property", {0.6f, 0.9f, 0.4f, 1.0f}},
            {"value", {0.4f, 0.6f, 0.9f, 1.0f}},
            {"key", {0.9f, 0.6f, 0.4f, 1.0f}}
        }}
    };
}
    
    splitPos = settings["splitPos"].get<float>();
    // Only update lastSettingsModification if the file exists
    if (fs::exists(settingsPath)) {
        lastSettingsModification = fs::last_write_time(settingsPath);
    } else {
        lastSettingsModification = fs::file_time_type::min();
    }

    std::cout << "Current settings: " << settings.dump(4) << std::endl;
}

void Settings::saveSettings() {
    std::ofstream settingsFile(settingsPath);
    if (settingsFile.is_open()) {
        settingsFile << std::setw(4) << settings << std::endl;
    }
}
void Settings::checkSettingsFile() {
    if (!fs::exists(settingsPath)) {
        std::cout << "Settings file does not exist, skipping check" << std::endl;
        return;
    }

    auto currentModification = fs::last_write_time(settingsPath);
    
    if (currentModification > lastSettingsModification) {
        std::cout << "Settings file has been modified, reloading" << std::endl;
        json oldSettings = settings;
        loadSettings();
        lastSettingsModification = currentModification;
        
        // Check if relevant settings have changed
        if (oldSettings["backgroundColor"] != settings["backgroundColor"] ||
            oldSettings["fontSize"] != settings["fontSize"] ||
            oldSettings["splitPos"] != settings["splitPos"] ||
            oldSettings["theme"] != settings["theme"] ||
            oldSettings["themes"] != settings["themes"] ||
            oldSettings["font"] != settings["font"]) {
            settingsChanged = true;
            
            // Check if theme or themes have changed
            if (oldSettings["theme"] != settings["theme"] || oldSettings["themes"] != settings["themes"]) {
                themeChanged = true;
            }

            // Check if font has changed
            if (oldSettings["font"] != settings["font"]) {
                fontChanged = true;
            }
        }
    }
}
void Settings::renderSettingsWindow() {
    if (!showSettingsWindow) return;

    // Center the window - make height auto-adjustable
    ImGui::SetNextWindowSize(ImVec2(700, 0), ImGuiCond_Always);
    ImGui::SetNextWindowPos(
        ImVec2(ImGui::GetIO().DisplaySize.x * 0.5f, ImGui::GetIO().DisplaySize.y * 0.35f),
        ImGuiCond_Always,
        ImVec2(0.5f, 0.5f)
    );
    
    ImGuiWindowFlags windowFlags = 
        ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoScrollbar |
        ImGuiWindowFlags_NoScrollWithMouse |
        ImGuiWindowFlags_AlwaysAutoResize;

    // Push custom styles for the window
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 10.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.0f);
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.2f, 0.2f, 0.2f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.3f, 0.3f, 0.3f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.15f, 0.15f, 0.15f, 1.0f));

    ImGui::Begin("Settings", nullptr, windowFlags);
    
    // Check for clicks outside window
    if (ImGui::IsMouseClicked(0) && !ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByPopup | ImGuiHoveredFlags_ChildWindows)) {
        showSettingsWindow = false;
    }

    // Start header group with proper spacing
    ImGui::BeginGroup();

    // Get text height for consistent sizing
    float textHeight = ImGui::GetTextLineHeight();

    // Display "Settings" text
    ImGui::TextUnformatted("Settings");

    // Calculate position for close button to align with text
    float closeIconSize = textHeight - 5;  // Match text height exactly
    ImGui::SameLine(ImGui::GetWindowWidth() - closeIconSize - 20); // More padding from right edge

    // Create invisible button with icon
    ImVec2 cursor_pos = ImGui::GetCursorPos();
    if (ImGui::InvisibleButton("##close-settings", ImVec2(closeIconSize, closeIconSize))) {
        showSettingsWindow = false;
        saveSettings();
    }

    bool isHovered = ImGui::IsItemHovered();
    ImGui::SetCursorPos(cursor_pos);

    // Draw the close icon
    ImTextureID closeIcon = gFileExplorer.getIcon("close");
    ImGui::Image(closeIcon, ImVec2(closeIconSize, closeIconSize), 
                 ImVec2(0,0), ImVec2(1,1),
                 isHovered ? ImVec4(1,1,1,0.6f) : ImVec4(1,1,1,1));

    ImGui::EndGroup();
    ImGui::Separator();

    ImGui::BeginChild("ScrollingRegion", ImVec2(0, 300));    ImGui::Spacing();
    
    // Font Size Settings
    float fontSize = settings["fontSize"].get<float>();
    if (ImGui::SliderFloat("Font Size", &fontSize, 8.0f, 72.0f, "%.0f")) {
        settings["fontSize"] = fontSize;
        setFontSize(fontSize);
        fontSizeChanged = true;
        settingsChanged = true;
        saveSettings();
    }
    ImGui::Spacing();
    
    // Background Color
    auto& bgColorArray = settings["backgroundColor"];
    ImVec4 bgColor(
        bgColorArray[0].get<float>(),
        bgColorArray[1].get<float>(),
        bgColorArray[2].get<float>(),
        bgColorArray[3].get<float>()
    );

    if (ImGui::ColorEdit3("Background Color", (float*)&bgColor)) {
        settings["backgroundColor"] = {bgColor.x, bgColor.y, bgColor.z, bgColor.w};
        settingsChanged = true;
        saveSettings();
    }

    // Text Color
    std::string currentTheme = getCurrentTheme();
    auto& themeColors = settings["themes"][currentTheme];

    // Create helper lambda for color editing
    auto editThemeColor = [&](const char* label, const char* colorKey) {
        auto& colorArray = themeColors[colorKey];
        ImVec4 color(
            colorArray[0].get<float>(),
            colorArray[1].get<float>(),
            colorArray[2].get<float>(),
            colorArray[3].get<float>()
        );
        
        ImGui::TextUnformatted(label); ImGui::SameLine(200);
        if (ImGui::ColorEdit3(("##" + std::string(colorKey)).c_str(), (float*)&color)) {
            themeColors[colorKey] = {color.x, color.y, color.z, color.w};
            themeChanged = true;
            settingsChanged = true;
            gEditor.forceColorUpdate(); 
        }
    };

    ImGui::Spacing();
    ImGui::TextUnformatted("Syntax Colors");
    ImGui::Separator();
    
    editThemeColor("Text Color", "text");
    editThemeColor("Keywords", "keyword");
    editThemeColor("Strings", "string");
    editThemeColor("Numbers", "number");
    editThemeColor("Comments", "comment");
    
    ImGui::EndChild();

    ImGui::Separator();
    ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Press Cmd+, to close this window");

    if (ImGui::IsKeyPressed(ImGuiKey_Escape)) {
        showSettingsWindow = false;
        saveSettings();
    }

    ImGui::End();

    // Pop all styles
    ImGui::PopStyleColor(3);
    ImGui::PopStyleVar(2);
}