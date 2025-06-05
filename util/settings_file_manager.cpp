#include "settings_file_manager.h"
#include <unistd.h>  
#include <fstream>
#include <iostream>
#include <iomanip>
#include <algorithm>
#include <libgen.h> 

#ifdef __APPLE__
#include <mach-o/dyld.h>
#include <sys/param.h>   
#endif
#ifdef __linux__
#include <linux/limits.h>
#include <sys/types.h>  // For ssize_t
#endif

#include <map>


SettingsFileManager::SettingsFileManager() {}

std::string SettingsFileManager::getAppResourcesPath() {
#ifdef __APPLE__
    char exePath[MAXPATHLEN];
    uint32_t size = sizeof(exePath);
    if (_NSGetExecutablePath(exePath, &size) == 0) {
        char resolvedPath[MAXPATHLEN];
        if (realpath(exePath, resolvedPath) != nullptr) {
            std::string p = resolvedPath;
            p = dirname((char *)p.c_str());
            p = dirname((char *)p.c_str());
            std::string resourcesPath = p + "/Resources";
            if (fs::exists(resourcesPath) && fs::is_directory(resourcesPath)) {
                return resourcesPath;
            } else {
                return p;
            }
        } else {
            std::string p = exePath;
            p = dirname((char *)p.c_str());
            p = dirname((char *)p.c_str());
            p += "/Resources";
            return p;
        }
    }
#elif defined(__linux__)
    char exePath[PATH_MAX];
    ssize_t count = readlink("/proc/self/exe", exePath, PATH_MAX);
    if (count != -1) {
        exePath[count] = '\0';
        std::string p = dirname(exePath);

        std::vector<std::string> pathsToCheck = {
            p + "/../share/Ned",
            p + "/resources",
            p
        };

        for (const auto &path : pathsToCheck) {
            if (fs::exists(path) && fs::is_directory(path)) {
                std::cout << "[Settings] Using app resource path: " << path << std::endl;
                return path;
            }
        }
        if (fs::exists(p) && fs::is_directory(p)) return p;
    }
    return ".";
#endif
}

std::string SettingsFileManager::getUserSettingsPath() {
    const char *home = getenv("HOME");
    if (!home) {
        std::cerr << "[Settings] WARNING: HOME environment variable not found. Using './ned/settings/ned.json' for primary settings." << std::endl;
        return "ned/settings/ned.json";
    }
    return std::string(home) + "/ned/settings/ned.json";
}

void SettingsFileManager::createSettingsDirectory(const fs::path& dir) {
    if (!fs::exists(dir)) {
        try {
            fs::create_directories(dir);
            std::cout << "[Settings] Created settings directory: " << dir.string() << std::endl;
        } catch (const fs::filesystem_error &e) {
            std::cerr << "[Settings] Error creating settings directory " << dir.string() << ": " << e.what() << std::endl;
        }
    }
}

void SettingsFileManager::copyDefaultSettingsFiles(const fs::path& destDir) {
    std::string bundleSettingsDir = getAppResourcesPath() + "/settings";
    std::vector<std::string> filesToCopy = {
        "ned.json", "amber.json", "test.json", "solarized.json", 
        "solarized-light.json", "custom1.json", "custom2.json", "custom3.json"
    };

    for (const auto &filename : filesToCopy) {
        std::string sourcePath = bundleSettingsDir + "/" + filename;
        std::string destPath = (destDir / filename).string();

        if (fs::exists(sourcePath)) {
            try {
                fs::copy_file(sourcePath, destPath, fs::copy_options::overwrite_existing);
                std::cout << "[Settings] Copied " << filename << " from bundle to " << destPath << std::endl;
            } catch (const fs::filesystem_error &e) {
                std::cerr << "[Settings] Error copying " << filename << ": " << e.what() << std::endl;
                if (filename == "ned.json") {
                    std::cerr << "CRITICAL: Failed to copy primary settings file" << std::endl;
                }
            }
        } else {
            std::cerr << (filename == "ned.json" ? "CRITICAL" : "WARNING") << ": Bundle file " << sourcePath << " not found" << std::endl;
        }
    }
}

bool SettingsFileManager::loadJsonFile(const std::string& filePath, json& jsonData) {
    try {
        std::ifstream file(filePath);
        if (file.is_open()) {
            file >> jsonData;
            return true;
        }
        std::cerr << "[Settings] Failed to open file " << filePath << " for reading." << std::endl;
    } catch (const json::parse_error &e) {
        std::cerr << "[Settings] Error parsing " << filePath << ": " << e.what() << std::endl;
    } catch (const std::exception &e) {
        std::cerr << "[Settings] Generic error loading " << filePath << ": " << e.what() << std::endl;
    }
    return false;
}

void SettingsFileManager::saveJsonFile(const std::string& filePath, const json& jsonData) {
    try {
        std::ofstream file(filePath);
        if (file.is_open()) {
            file << std::setw(4) << jsonData << std::endl;
            file.close();
            updateLastModificationTime(filePath);
        } else {
            std::cerr << "[Settings] Failed to open " << filePath << " for saving." << std::endl;
        }
    } catch (const std::exception &e) {
        std::cerr << "[Settings] Error saving " << filePath << ": " << e.what() << std::endl;
    }
}

void SettingsFileManager::updateLastModificationTime(const std::string& filePath) {
    try {
        lastSettingsModification = fs::last_write_time(filePath);
    } catch (const fs::filesystem_error &e) {
        std::cerr << "[Settings] Error getting last write time for " << filePath << ": " << e.what() << std::endl;
        lastSettingsModification = fs::file_time_type::min();
    }
}

void SettingsFileManager::loadSettings(json& settings, std::string& settingsPath) {
    std::string primarySettingsFilePath = getUserSettingsPath();
    fs::path primarySettingsDir = fs::path(primarySettingsFilePath).parent_path();

    createSettingsDirectory(primarySettingsDir);

    json primaryJsonConfig;
    bool primaryJsonModified = false;

    if (!fs::exists(primarySettingsFilePath)) {
        copyDefaultSettingsFiles(primarySettingsDir);
    }

    if (!loadJsonFile(primarySettingsFilePath, primaryJsonConfig)) {
        primaryJsonConfig = json::object();
    }

    std::string activeSettingsFilename;
    if (primaryJsonConfig.contains("settings_file") && primaryJsonConfig["settings_file"].is_string()) {
        activeSettingsFilename = primaryJsonConfig["settings_file"].get<std::string>();
    } else {
        activeSettingsFilename = "ned.json";
        primaryJsonConfig["settings_file"] = activeSettingsFilename;
        primaryJsonModified = true;
    }

    if (primaryJsonModified) {
        saveJsonFile(primarySettingsFilePath, primaryJsonConfig);
    }

    settingsPath = (primarySettingsDir / activeSettingsFilename).string();
    std::cout << "[Settings] Active settings file target: " << settingsPath << std::endl;

    if (activeSettingsFilename != "ned.json" && !fs::exists(settingsPath)) {
        if (fs::exists(primarySettingsFilePath)) {
            try {
                fs::copy_file(primarySettingsFilePath, settingsPath, fs::copy_options::overwrite_existing);
                std::cout << "[Settings] Copied " << primarySettingsFilePath << " to " << settingsPath << " as a base." << std::endl;
            } catch (const fs::filesystem_error &e) {
                std::cerr << "[Settings] Error copying " << primarySettingsFilePath << " to " << settingsPath << ": " << e.what() << std::endl;
                settingsPath = primarySettingsFilePath;
            }
        } else {
            settingsPath = primarySettingsFilePath;
        }
    }

    settings = json::object();
    if (!loadJsonFile(settingsPath, settings)) {
        settings = json::object();
    }

    if (activeSettingsFilename != "ned.json" && settings.contains("settings_file")) {
        settings.erase("settings_file");
        saveJsonFile(settingsPath, settings);
    }

    applyDefaultSettings(settings);
    applyDefaultThemes(settings);

    updateLastModificationTime(settingsPath);
}

void SettingsFileManager::saveSettings(const json& settings, const std::string& settingsPath) {
    if (settingsPath.empty()) {
        std::cerr << "[Settings] Error: settingsPath is empty, cannot save settings." << std::endl;
        return;
    }
    saveJsonFile(settingsPath, settings);
}

void SettingsFileManager::checkSettingsFile(const std::string& settingsPath, json& settings,
                                          bool& settingsChanged, bool& fontChanged,
                                          bool& fontSizeChanged, bool& themeChanged) {
    if (settingsPath.empty() || !fs::exists(settingsPath)) {
        return;
    }

    try {
        auto currentModification = fs::last_write_time(settingsPath);
        if (currentModification <= lastSettingsModification) {
            return;
        }

        std::cout << "[Settings] Active settings file " << settingsPath << " was modified externally, reloading." << std::endl;

        json oldSettings = settings;
        loadSettings(settings, const_cast<std::string&>(settingsPath));

        if (oldSettings.contains("fontSize") && settings.contains("fontSize") &&
            oldSettings["fontSize"] != settings["fontSize"]) {
            fontSizeChanged = true;
        }
        if (oldSettings.contains("font") && settings.contains("font") &&
            oldSettings["font"] != settings["font"]) {
            fontChanged = true;
        }
        if (oldSettings.contains("theme") && settings.contains("theme") &&
            oldSettings["theme"] != settings["theme"]) {
            themeChanged = true;
        }
        if (oldSettings.contains("themes") && settings.contains("themes") &&
            oldSettings["themes"] != settings["themes"]) {
            themeChanged = true;
        }

        const std::vector<std::string> checkKeys = {
            "backgroundColor", "splitPos", "rainbow", "treesitter", "shader_toggle",
            "scanline_intensity", "burnin_intensity", "curvature_intensity",
            "colorshift_intensity", "bloom_intensity", "static_intensity",
            "jitter_intensity", "pixelation_intensity", "pixel_width",
            "vignet_intensity", "mac_background_opacity", "mac_blur_enabled"
        };

        for (const auto &key : checkKeys) {
            if (oldSettings.contains(key) && settings.contains(key)) {
                if (oldSettings[key] != settings[key]) {
                    settingsChanged = true;
                    break;
                }
            } else if (oldSettings.contains(key) != settings.contains(key)) {
                settingsChanged = true;
                break;
            }
        }

    } catch (const fs::filesystem_error &e) {
        std::cerr << "[Settings] Filesystem error checking settings file " << settingsPath << ": " << e.what() << std::endl;
    } catch (const std::exception &e) {
        std::cerr << "[Settings] Error during settings file check for " << settingsPath << ": " << e.what() << std::endl;
    }
}

std::vector<std::string> SettingsFileManager::getAvailableProfileFiles() {
    std::vector<std::string> availableProfileFiles;
    fs::path userSettingsDir = fs::path(getUserSettingsPath()).parent_path();

    if (fs::exists(userSettingsDir) && fs::is_directory(userSettingsDir)) {
        for (const auto &entry : fs::directory_iterator(userSettingsDir)) {
            if (entry.is_regular_file() && entry.path().extension() == ".json") {
                availableProfileFiles.push_back(entry.path().filename().string());
            }
        }
        std::sort(availableProfileFiles.begin(), availableProfileFiles.end());
    }
    return availableProfileFiles;
}

void SettingsFileManager::switchProfile(const std::string& newProfile, json& settings,
                                      std::string& settingsPath, bool& settingsChanged,
                                      bool& fontChanged, bool& themeChanged) {
    std::string primarySettingsFilePath = getUserSettingsPath();
    json primaryJson;
    
    if (loadJsonFile(primarySettingsFilePath, primaryJson)) {
        primaryJson["settings_file"] = newProfile;
        saveJsonFile(primarySettingsFilePath, primaryJson);
        loadSettings(settings, settingsPath);
        settingsChanged = true;
        fontChanged = true;
        themeChanged = true;
    }
}

void SettingsFileManager::applyDefaultSettings(json& settings) {
    const std::vector<std::pair<std::string, json>> defaults = {
        {"backgroundColor", json::array({0.05816289037466049, 0.19437342882156372, 0.1578674018383026, 1.0000001192092896})},
        {"bloom_intensity", 0.75},
        {"burnin_intensity", 0.9525200128555298},
        {"colorshift_intensity", 0.8999999761581421},
        {"curvature_intensity", 0.0},
        {"font", "SourceCodePro-Regular"},
        {"fontSize", 20.0f},
        {"jitter_intensity", 2.809999942779541},
        {"pixel_width", 5000.0},
        {"pixelation_intensity", -0.10999999940395355},
        {"rainbow", true},
        {"scanline_intensity", 0.20000000298023224},
        {"shader_toggle", true},
        {"splitPos", 0.2142857164144516},
        {"static_intensity", 0.20800000429153442},
        {"theme", "default"},
        {"treesitter", true},
        {"vignet_intensity", 0.25},
        {"mac_background_opacity", 0.5},
        {"mac_blur_enabled", true}
    };

    for (const auto &[key, value] : defaults) {
        if (!settings.contains(key)) {
            settings[key] = value;
        }
    }
}

void SettingsFileManager::applyDefaultThemes(json& settings) {
    if (!settings.contains("themes") || !settings["themes"].is_object()) {
        settings["themes"] = {
            {"default", {
                {"background", json::array({0.2, 0.2, 0.2, 1.0})},
                {"comment", json::array({0.4565933346748352, 0.4565933346748352, 0.4565933346748352, 0.8999999761581421})},
                {"function", json::array({0.6752017140388489, 0.3185149133205414, 0.7726563811302185, 1.0})},
                {"keyword", json::array({0.0, 0.5786033272743225, 0.9643363952636719, 1.0})},
                {"number", json::array({0.6439791321754456, 0.3536583185195923, 0.7698838710784912, 1.0})},
                {"string", json::array({0.08516374975442886, 0.6660587787628174, 0.6660587787628174, 1.0})},
                {"text", json::array({0.680115282535553, 0.680115282535553, 0.680115282535553, 1.0})},
                {"type", json::array({0.6007077097892761, 0.7665653228759766, 0.44595927000045776, 1.0})},
                {"variable", json::array({0.3506303131580353, 0.7735447883605957, 0.8863282203674316, 1.0})}
            }}
        };
    }
} 