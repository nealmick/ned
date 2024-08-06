#pragma once

#include <string>
#include <filesystem>
#include "lib/json.hpp"

namespace fs = std::filesystem;
using json = nlohmann::json;

class Settings {
public:
    Settings();
    void loadSettings();
    void saveSettings();
    void checkSettingsFile();
    json& getSettings() { return settings; }
    float getSplitPos() const { return splitPos; }
    void setSplitPos(float pos) { 
        splitPos = pos; 
        settings["splitPos"] = pos;
    }
    bool hasSettingsChanged() const { return settingsChanged; }
    void resetSettingsChanged() { settingsChanged = false; }
    std::string getCurrentTheme() const { return settings["theme"].get<std::string>(); }
    bool hasThemeChanged() const { return themeChanged; }
    void resetThemeChanged() { themeChanged = false; }

    std::string getCurrentFont() const { return settings["font"].get<std::string>(); }
    bool hasFontChanged() const { return fontChanged; }
    void resetFontChanged() { fontChanged = false; }
private:
    json settings;
    std::string settingsPath;
    float splitPos;
    fs::file_time_type lastSettingsModification;
    bool settingsChanged = false;
    bool themeChanged = false;
    bool fontChanged = false;
};

extern Settings gSettings;