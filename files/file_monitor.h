/*
	File: file_monitor.h
	Description: Dedicated file monitoring class for external file change detection
*/

#pragma once

#include <filesystem>
#include <functional>
#include <map>
#include <set>
#include <string>
#include <thread>

namespace fs = std::filesystem;

class FileMonitor
{
  public:
	FileMonitor();
	~FileMonitor();

	// Initialize monitoring for a project folder
	void startMonitoring(const std::string &projectFolder);
	void stopMonitoring();

	// Add/remove files from monitoring
	void addFileToMonitoring(const std::string &filePath);
	void removeFileFromMonitoring(const std::string &filePath);

	// Refresh file state after internal save to prevent false external change detection
	void refreshFileState(const std::string &filePath);

	// Get monitoring status
	bool isMonitoring() const { return !_projectFolder.empty(); }
	size_t getMonitoredFileCount() const { return _monitoredFiles.size(); }

	// Check for external file changes (called from main thread)
	void checkForExternalFileChanges();

	// Callback for when files change externally
	std::function<void(const std::string &, const std::string &)> onFileChanged;

  private:
	// Background scanning methods
	void scanProjectFilesForMonitoring();

	// File change detection helpers
	bool shouldReloadFile(const std::string &filePath);
	std::string calculateFileHash(const std::string &content);
	bool shouldIgnoreFile(const std::string &filename);

	// Member variables
	std::string _projectFolder;
	std::set<std::string> _monitoredFiles;
	std::map<std::string, fs::file_time_type> _fileModificationTimes;
	std::map<std::string, std::string> _fileContentHashes;

	// Background thread
	std::thread _fileScanThread;

	// File extensions to monitor
	std::set<std::string> _monitoredExtensions;

	// Throttling for checkForExternalFileChanges
	double _lastChangeCheckTime = 0.0;

	// Initialize monitored extensions
	void initializeMonitoredExtensions();
};
