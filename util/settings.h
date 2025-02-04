/*  
    util/settings.h
    This utility handles various settings such as font size and background color. 
*/


#pragma once
#include <string>
#include <filesystem>
#include "../lib/json.hpp"
#include "close_popper.h"

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
        settingsChanged = true;
    }
    bool hasSettingsChanged() const { return settingsChanged; }
    void resetSettingsChanged() { settingsChanged = false; }
    std::string getCurrentTheme() const { return settings["theme"].get<std::string>(); }
    bool hasThemeChanged() const { return themeChanged; }
    void resetThemeChanged() { themeChanged = false; }
    std::string getCurrentFont() const { return settings["font"].get<std::string>(); }
    bool hasFontChanged() const { return fontChanged; }
    void resetFontChanged() { fontChanged = false; }
    
    float getFontSize() const {  return currentFontSize; }

    void setFontSize(float size) {
        if (size != currentFontSize) {
            settings["fontSize"] = size;
            currentFontSize = size;
            fontSizeChanged = true;
            settingsChanged = true;
        }
    }
    bool hasFontSizeChanged() const { return fontSizeChanged; }
    void resetFontSizeChanged() { fontSizeChanged = false; }
    void ScrollRegion() const{}
    bool showSettingsWindow = false;
    void renderSettingsWindow();
    void toggleSettingsWindow() { 
        showSettingsWindow = !showSettingsWindow;
        if(showSettingsWindow) {  // Only close others if we're opening
           ClosePopper::closeAllExcept(ClosePopper::Type::Settings);   // RIGHT

        }
        blockInput = showSettingsWindow;
    }
    bool isBlockingInput() const { return blockInput; }
    bool getRainbowMode() const { return settings["rainbow"].get<bool>(); }
private:
    json settings;
    std::string settingsPath;
    float splitPos;
    fs::file_time_type lastSettingsModification;
    bool settingsChanged = false;
    bool themeChanged = false;
    bool fontChanged = false;
    bool fontSizeChanged = false;
    bool blockInput = false;
    float currentFontSize = 16.0f;
    int settingsCheckFrameCounter = 0;
    const int SETTINGS_CHECK_INTERVAL = 60; // Check every 60 frames (roughly 1 seconds at 60 FPS)
};

extern Settings gSettings;