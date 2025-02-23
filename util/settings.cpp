/*
    util/settings.cpp
    This utility handles various settings such as font size and background color.
    The settings state is saved to the build folder as .ned.json
    This utility also draws a pop up window for adjusting settings, open with cmd ,(comma)
    The settings json file can edited manually, once saved the changes take effect.
*/

#include "settings.h"
#include "../editor.h"
#include "../files.h"
#include "config.h"
#include "imgui.h"
#include <GLFW/glfw3.h>
#include <fstream>
#include <iostream>
#include <unistd.h>

#include "../files.h"

Settings gSettings;

Settings::Settings() : splitPos(0.3f) {}
void Settings::loadSettings()
{
    settingsPath = std::string(SOURCE_DIR) + "/.ned.json";
    // std::cout << "\033[95mSettings:\033[0m Attempting to load settings from: " << settingsPath << std::endl;

    std::ifstream settingsFile(settingsPath);
    if (settingsFile.is_open())
    {
        try
        {
            settingsFile >> settings;
            std::cout << "\033[95mSettings:\033[0m Json preferences loaded successfully" << std::endl;
        }
        catch (json::parse_error &e)
        {
            std::cerr << "\033[95mSettings:\033[0m Error parsing settings file: " << e.what() << std::endl;
            settings = json::object(); // Reset to empty object
        }
    }
    else
    {
        std::cout << "\033[95mSettings:\033[0m json file not found, using defaults" << std::endl;
        settings = json::object(); // Create an empty JSON object
    }
    if (!settings.contains("font"))
    {
        settings["font"] = "default";
    }
    // Set default values if not present
    if (!settings.contains("backgroundColor"))
    {
        settings["backgroundColor"] = {0.45f, 0.55f, 0.60f, 1.00f};
    }
    if (!settings.contains("fontSize"))
    {
        currentFontSize = settings["fontSize"].get<float>();
    }
    if (!settings.contains("splitPos"))
    {
        settings["splitPos"] = 0.3f;
    }
    if (!settings.contains("theme"))
    {
        settings["theme"] = "default";
    }
    if (!settings.contains("rainbow"))
    {
        settings["rainbow"] = true; // default to true
    }
    if (!settings.contains("shader_toggle"))
    {
        settings["shader_toggle"] = true; // default to shader enabled
    }
    if (!settings.contains("themes"))
    {
        settings["themes"] = {{"default", {{"function", {1.0f, 1.0f, 1.0f, 1.0f}}, {"text", {1.0f, 1.0f, 1.0f, 1.0f}}, {"background", {0.2f, 0.2f, 0.2f, 1.0f}}, {"keyword", {0.0f, 0.4f, 1.0f, 1.0f}}, {"string", {0.87f, 0.87f, 0.0f, 1.0f}}, {"number", {0.0f, 0.8f, 0.8f, 1.0f}}, {"comment", {0.5f, 0.5f, 0.5f, 1.0f}}, {"heading", {0.9f, 0.5f, 0.2f, 1.0f}}, {"bold", {1.0f, 0.7f, 0.7f, 1.0f}}, {"italic", {0.7f, 1.0f, 0.7f, 1.0f}}, {"link", {0.4f, 0.4f, 1.0f, 1.0f}}, {"code_block", {0.8f, 0.8f, 0.8f, 1.0f}}, {"inline_code", {0.7f, 0.7f, 0.7f, 1.0f}}, {"tag", {0.3f, 0.7f, 0.9f, 1.0f}}, {"attribute", {0.9f, 0.7f, 0.3f, 1.0f}}, {"selector", {0.9f, 0.4f, 0.6f, 1.0f}}, {"property", {0.6f, 0.9f, 0.4f, 1.0f}}, {"value", {0.4f, 0.6f, 0.9f, 1.0f}}, {"key", {0.9f, 0.6f, 0.4f, 1.0f}}}}};
    }

    splitPos = settings["splitPos"].get<float>();
    // Only update lastSettingsModification if the file exists
    if (fs::exists(settingsPath))
    {
        lastSettingsModification = fs::last_write_time(settingsPath);
    }
    else
    {
        lastSettingsModification = fs::file_time_type::min();
    }

    // std::cout << "Settings: Current preferences : " << settings.dump(4) << std::endl;
}

void Settings::saveSettings()
{
    std::ofstream settingsFile(settingsPath);
    if (settingsFile.is_open())
    {
        settingsFile << std::setw(4) << settings << std::endl;
    }
}
void Settings::checkSettingsFile()
{

    if (!fs::exists(settingsPath))
    {
        std::cout << "Settings file does not exist, skipping check" << std::endl;
        return;
    }

    auto currentModification = fs::last_write_time(settingsPath);

    if (currentModification > lastSettingsModification)
    {
        std::cout << "Settings file has been modified, reloading" << std::endl;
        json oldSettings = settings;
        loadSettings();
        lastSettingsModification = currentModification;

        // Rest of the existing check logic remains the same
        if (oldSettings["backgroundColor"] != settings["backgroundColor"] || oldSettings["fontSize"] != settings["fontSize"] || oldSettings["splitPos"] != settings["splitPos"] || oldSettings["theme"] != settings["theme"] || oldSettings["themes"] != settings["themes"] || oldSettings["rainbow"] != settings["rainbow"] || oldSettings["shader_toggle"] != settings["shader_toggle"] || oldSettings["font"] != settings["font"])
        {
            settingsChanged = true;

            // Check if theme or themes have changed
            if (oldSettings["theme"] != settings["theme"] || oldSettings["themes"] != settings["themes"])
            {
                themeChanged = true;
            }

            // Check if font has changed
            if (oldSettings["font"] != settings["font"])
            {
                fontChanged = true;
            }
        }
    }
}
void Settings::renderSettingsWindow()
{
    if (!showSettingsWindow)
        return;
    ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 1.0f);
    // Center the window - make height auto-adjustable
    ImGui::SetNextWindowSize(ImVec2(900, 600), ImGuiCond_Always); // Fixed height
    ImGui::SetNextWindowPos(ImVec2(ImGui::GetIO().DisplaySize.x * 0.5f, ImGui::GetIO().DisplaySize.y * 0.50f), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
    ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_Modal;
    // Push custom styles for the window
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 10.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.0f);
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.2f, 0.2f, 0.2f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.3f, 0.3f, 0.3f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.15f, 0.15f, 0.15f, 1.0f));

    ImGui::Begin("Settings", nullptr, windowFlags);

    // Add focus detection
    static bool wasFocused = false;
    bool isFocused = ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);

    if (wasFocused && !isFocused)
    {
        std::cout << "Settings window lost focus!" << std::endl;
        std::cout << "Settings window lost focus!" << std::endl;
        showSettingsWindow = false;
        saveSettings();
    }
    else if (!wasFocused && isFocused)
    {
        std::cout << "Settings window gained focus!" << std::endl;
    }
    wasFocused = isFocused;

    // Check for clicks outside window
    if (ImGui::IsMouseClicked(0))
    {
        ImVec2 mousePos = ImGui::GetMousePos();
        ImVec2 windowPos = ImGui::GetWindowPos();
        ImVec2 windowSize = ImGui::GetWindowSize();

        bool isOutside = mousePos.x < windowPos.x || mousePos.x > (windowPos.x + windowSize.x) || mousePos.y < windowPos.y || mousePos.y > (windowPos.y + windowSize.y);

        if (isOutside && !ImGui::IsPopupOpen("", ImGuiPopupFlags_AnyPopup))
        {
            showSettingsWindow = false;
            saveSettings();
        }
    }

    // Start header group with proper spacing
    ImGui::BeginGroup();

    float textHeight = ImGui::GetTextLineHeight();
    ImGui::TextUnformatted("Settings");

    float closeIconSize = textHeight - 5;
    ImGui::SameLine(ImGui::GetWindowWidth() - closeIconSize - 20);

    ImVec2 cursor_pos = ImGui::GetCursorPos();
    if (ImGui::InvisibleButton("##close-settings", ImVec2(closeIconSize, closeIconSize)))
    {
        showSettingsWindow = false;
        saveSettings();
    }

    bool isHovered = ImGui::IsItemHovered();
    ImGui::SetCursorPos(cursor_pos);

    ImTextureID closeIcon = gFileExplorer.getIcon("close");
    ImGui::Image(closeIcon, ImVec2(closeIconSize, closeIconSize), ImVec2(0, 0), ImVec2(1, 1), isHovered ? ImVec4(1, 1, 1, 0.6f) : ImVec4(1, 1, 1, 1));

    ImGui::EndGroup();
    ImGui::Separator();
    ImGui::Spacing();

    static float tempFontSize = currentFontSize; // Use currentFontSize instead of reading from settings

    if (ImGui::SliderFloat("Font Size", &tempFontSize, 4.0f, 32.0f, "%.0f"))
    {
        settings["fontSize"] = tempFontSize; // Update settings but don't change display yet
    }

    if (ImGui::IsItemDeactivatedAfterEdit())
    {
        setFontSize(tempFontSize); // This will update currentFontSize and trigger changes
        saveSettings();
    }
    ImGui::Spacing();

    // Background Color
    auto &bgColorArray = settings["backgroundColor"];
    ImVec4 bgColor(bgColorArray[0].get<float>(), bgColorArray[1].get<float>(), bgColorArray[2].get<float>(), bgColorArray[3].get<float>());

    if (ImGui::ColorEdit3("Background Color", (float *)&bgColor))
    {
        settings["backgroundColor"] = {bgColor.x, bgColor.y, bgColor.z, bgColor.w};
        settingsChanged = true;
        saveSettings();
    }

    // Text Color
    std::string currentTheme = getCurrentTheme();
    auto &themeColors = settings["themes"][currentTheme];

    // Create helper lambda for color editing
    auto editThemeColor = [&](const char *label, const char *colorKey)
    {
        auto &colorArray = themeColors[colorKey];
        ImVec4 color(colorArray[0].get<float>(), colorArray[1].get<float>(), colorArray[2].get<float>(), colorArray[3].get<float>());

        ImGui::TextUnformatted(label);
        ImGui::SameLine(200);
        if (ImGui::ColorEdit3(("##" + std::string(colorKey)).c_str(), (float *)&color))
        {
            themeColors[colorKey] = {color.x, color.y, color.z, color.w};
            themeChanged = true;
            settingsChanged = true;
            gEditor.forceColorUpdate();
        }
    };

    ImGui::Spacing();
    ImGui::TextUnformatted("Syntax Colors");
    ImGui::Separator();
    ImGui::Spacing();

    editThemeColor("Text Color", "text");
    editThemeColor("Keywords", "keyword");
    editThemeColor("Strings", "string");
    editThemeColor("Numbers", "number");
    editThemeColor("Comments", "comment");
    editThemeColor("Functions", "function");

    ImGui::Spacing();
    ImGui::TextUnformatted("Toggle");
    ImGui::Separator();
    ImGui::Spacing();

    // Rainbow Mode Toggle
    bool rainbowMode = settings["rainbow"].get<bool>();
    if (ImGui::Checkbox("Rainbow Mode", &rainbowMode))
    {
        settings["rainbow"] = rainbowMode;
        settingsChanged = true;
        saveSettings();
    }
    ImGui::SameLine();
    ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "(Rainbow cursor and line numbers)");
    ImGui::Spacing();
    bool shaderEnabled = settings["shader_toggle"].get<bool>();
    if (ImGui::Checkbox("Enable Shader Effects", &shaderEnabled))
    {
        settings["shader_toggle"] = shaderEnabled;
        settingsChanged = true;
        saveSettings();
    }
    ImGui::SameLine();
    ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "(CRT and visual effects)");

    ImGui::Separator();
    ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Press Cmd+, to close this window");

    if (ImGui::IsKeyPressed(ImGuiKey_Escape))
    {
        showSettingsWindow = false;
        saveSettings();
    }

    ImGui::End();

    // Pop all styles
    ImGui::PopStyleColor(3);
    ImGui::PopStyleVar(3);
}
