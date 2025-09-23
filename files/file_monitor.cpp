/*
	File: file_monitor.cpp
	Description: Simple file monitoring class implementation
*/

#include "file_monitor.h"
#include <GLFW/glfw3.h>
#include <algorithm>
#include <fstream>
#include <iostream>
#include <sstream>

// Constructor
FileMonitor::FileMonitor() { initializeMonitoredExtensions(); }

// Destructor
FileMonitor::~FileMonitor() { stopMonitoring(); }

void FileMonitor::initializeMonitoredExtensions()
{
	_monitoredExtensions = {".cpp",	 ".c",	  ".h",	  ".hpp", ".cc",   ".cxx",
							".py",	 ".js",	  ".ts",  ".tsx", ".jsx",  ".java",
							".go",	 ".rs",	  ".rb",  ".php", ".html", ".css",
							".scss", ".json", ".xml", ".md",  ".txt",  ".yml",
							".yaml", ".toml", ".sh",  ".bat", ".ps1",  ".sql"};
}

void FileMonitor::startMonitoring(const std::string &projectFolder)
{
	if (projectFolder.empty())
	{
		return;
	}

	stopMonitoring(); // Stop any existing monitoring

	_projectFolder = projectFolder;
	std::cout << "[FileMonitor] Starting monitoring for: " << projectFolder << std::endl;

	// Start background scanning
	scanProjectFilesForMonitoring();
}

void FileMonitor::stopMonitoring()
{
	// Clean up background thread
	if (_fileScanThread.joinable())
	{
		_fileScanThread.join();
	}

	// Clear monitoring data
	_monitoredFiles.clear();
	_fileModificationTimes.clear();
	_fileContentHashes.clear();
	_projectFolder.clear();

	std::cout << "[FileMonitor] Monitoring stopped" << std::endl;
}

void FileMonitor::checkForExternalFileChanges()
{
	if (!isMonitoring())
	{
		return;
	}

	// Throttle checks to avoid performance impact - check every second like the old
	// implementation
	double currentTime = glfwGetTime();
	const double FILE_CHANGE_CHECK_INTERVAL = 1.0; // Check every second
	if (currentTime - _lastChangeCheckTime < FILE_CHANGE_CHECK_INTERVAL)
	{
		return;
	}
	_lastChangeCheckTime = currentTime;

	// Check all monitored files for external changes
	std::vector<std::string> filesToRemove;

	for (const auto &filePath : _monitoredFiles)
	{
		try
		{
			std::string filename = fs::path(filePath).filename().string();

			// Skip files that should be ignored
			if (shouldIgnoreFile(filename))
			{
				continue;
			}

			if (!fs::exists(filePath))
			{
				// File was deleted externally
				std::cout << "[FileMonitor] File was deleted externally: " << filename
						  << std::endl;
				filesToRemove.push_back(filePath);
				continue;
			}

			// Check modification time
			auto currentModTime = fs::last_write_time(filePath);
			auto modTimeIt = _fileModificationTimes.find(filePath);

			if (modTimeIt != _fileModificationTimes.end() &&
				currentModTime > modTimeIt->second)
			{
				// File modification time changed, check if content actually changed
				if (shouldReloadFile(filePath))
				{
					std::string filename = fs::path(filePath).filename().string();
					std::cout << "[FileMonitor] File changed externally: " << filename
							  << std::endl;

					// Call the callback if set
					if (onFileChanged)
					{
						onFileChanged(filePath, filename);
					}
				}
			}

			// Update the modification time
			_fileModificationTimes[filePath] = currentModTime;

		} catch (const std::exception &e)
		{
			std::cerr << "Error checking external file changes for " << filePath << ": "
					  << e.what() << std::endl;
		}
	}

	// Remove deleted files and files that should be ignored
	for (const auto &filePath : filesToRemove)
	{
		_monitoredFiles.erase(filePath);
		_fileModificationTimes.erase(filePath);
		_fileContentHashes.erase(filePath);
	}

	// Also remove any files that should be ignored (cleanup for existing monitored files)
	std::vector<std::string> ignoredFilesToRemove;
	for (const auto &filePath : _monitoredFiles)
	{
		std::string filename = fs::path(filePath).filename().string();
		if (shouldIgnoreFile(filename))
		{
			ignoredFilesToRemove.push_back(filePath);
		}
	}
	for (const auto &filePath : ignoredFilesToRemove)
	{
		_monitoredFiles.erase(filePath);
		_fileModificationTimes.erase(filePath);
		_fileContentHashes.erase(filePath);
	}
}

void FileMonitor::addFileToMonitoring(const std::string &filePath)
{
	if (_monitoredFiles.find(filePath) == _monitoredFiles.end())
	{
		_monitoredFiles.insert(filePath);

		if (fs::exists(filePath))
		{
			_fileModificationTimes[filePath] = fs::last_write_time(filePath);

			// Calculate content hash for change detection
			try
			{
				std::ifstream file(filePath, std::ios::binary);
				if (file)
				{
					std::stringstream buffer;
					buffer << file.rdbuf();
					_fileContentHashes[filePath] = calculateFileHash(buffer.str());
				}
			} catch (...)
			{
				// Ignore errors for individual files
			}
		}
	}
}

void FileMonitor::removeFileFromMonitoring(const std::string &filePath)
{
	_monitoredFiles.erase(filePath);
	_fileModificationTimes.erase(filePath);
	_fileContentHashes.erase(filePath);
}

void FileMonitor::refreshFileState(const std::string &filePath)
{
	if (fs::exists(filePath))
	{
		// Update the modification time to current
		_fileModificationTimes[filePath] = fs::last_write_time(filePath);

		// Update the content hash to current
		try
		{
			std::ifstream file(filePath, std::ios::binary);
			if (file)
			{
				std::stringstream buffer;
				buffer << file.rdbuf();
				_fileContentHashes[filePath] = calculateFileHash(buffer.str());
			}
		} catch (...)
		{
			// Ignore errors for individual files
		}
	}
}

void FileMonitor::scanProjectFilesForMonitoring()
{
	if (_projectFolder.empty())
	{
		return;
	}

	// If already scanning, don't start another scan
	if (_fileScanThread.joinable())
	{
		return;
	}

	std::cout << "[FileMonitor] Starting background scan for: " << _projectFolder
			  << std::endl;

	// Launch scan in background thread
	_fileScanThread = std::thread([this]() {
		try
		{
			for (const auto &entry : fs::recursive_directory_iterator(_projectFolder))
			{
				if (entry.is_regular_file())
				{
					std::string filePath = entry.path().string();
					std::string filename = entry.path().filename().string();

					// Skip files that should be ignored
					if (shouldIgnoreFile(filename))
					{
						continue;
					}

					// Check extension
					std::string extension = entry.path().extension().string();
					if (_monitoredExtensions.find(extension) != _monitoredExtensions.end())
					{
						addFileToMonitoring(filePath);
					}
				}
			}

			std::cout << "[FileMonitor] Scan completed! Monitoring "
					  << _monitoredFiles.size() << " files" << std::endl;

		} catch (const std::exception &e)
		{
			std::cerr << "Error in background file scan: " << e.what() << std::endl;
		}
	});
}

bool FileMonitor::shouldReloadFile(const std::string &filePath)
{
	try
	{
		// Read the current file content
		std::ifstream file(filePath, std::ios::binary);
		if (!file)
		{
			return false;
		}

		std::stringstream buffer;
		buffer << file.rdbuf();
		std::string currentContent = buffer.str();

		// Calculate hash of current content
		std::string currentHash = calculateFileHash(currentContent);

		// Compare with stored hash
		auto it = _fileContentHashes.find(filePath);
		if (it != _fileContentHashes.end())
		{
			return it->second != currentHash;
		}

		return false;
	} catch (const std::exception &e)
	{
		std::cerr << "Error checking if file should be reloaded: " << e.what()
				  << std::endl;
		return false;
	}
}

std::string FileMonitor::calculateFileHash(const std::string &content)
{
	std::hash<std::string> hasher;
	std::stringstream ss;
	ss << std::hex << hasher(content);
	return ss.str();
}

bool FileMonitor::shouldIgnoreFile(const std::string &filename)
{
	return filename == ".undo-redo-ned.json" || filename == ".ned-agent-history.json" ||
		   filename == "open_router_key.json";
}
