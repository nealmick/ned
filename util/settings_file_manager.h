#pragma once
#include "../lib/json.hpp"
#include <filesystem>
#include <string>
#include <vector>

namespace fs = std::filesystem;
using json = nlohmann::json;

class SettingsFileManager
{
  public:
	SettingsFileManager();

	// File path management
	static std::string getAppResourcesPath();
	static std::string getUserSettingsPath();

	// File operations
	void loadSettings(json &settings, std::string &settingsPath);
	void saveSettings(const json &settings, const std::string &settingsPath);
	void checkSettingsFile(const std::string &settingsPath,
						   json &settings,
						   bool &settingsChanged,
						   bool &fontChanged,
						   bool &fontSizeChanged,
						   bool &themeChanged);

	// Profile management
	std::vector<std::string> getAvailableProfileFiles();
	void switchProfile(const std::string &newProfile,
					   json &settings,
					   std::string &settingsPath,
					   bool &settingsChanged,
					   bool &fontChanged,
					   bool &themeChanged);

	// Default settings
	void applyDefaultSettings(json &settings);
	void applyDefaultThemes(json &settings);

	bool loadJsonFile(const std::string &filePath, json &jsonData);
	void saveJsonFile(const std::string &filePath, const json &jsonData);

	// OpenRouter key management
	static std::string getOpenRouterKeyFilePath();
	std::string getOpenRouterKey();
	void setOpenRouterKey(const std::string &key);

  private:
	fs::file_time_type lastSettingsModification;

	// Helper methods
	void createSettingsDirectory(const fs::path &dir);
	void copyDefaultSettingsFiles(const fs::path &destDir);
	void updateLastModificationTime(const std::string &filePath);
};

extern SettingsFileManager gSettingsFileManager;