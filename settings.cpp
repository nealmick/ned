#include "settings.h"
#include <iostream>
#include <fstream>

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