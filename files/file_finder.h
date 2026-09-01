#pragma once

#include <atomic>
#include <filesystem>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

namespace fs = std::filesystem;

class FileExplorer;
class Settings;

struct FileEntry
{
	std::string fullPath;
	std::string relativePath;
	std::string relativePathLower; // precomputed for filtering
	std::string filenameLower;	   // for dotfile skip rule
};

// Fuzzy-ish project file picker (Ctrl+P): background scan + filter.
// Opens the selected file only on Enter (no live preview — embed multi-tab
// would otherwise spawn a tab per arrow key).
class FileFinder
{
  public:
	FileExplorer *fileExplorer = nullptr;
	Settings *settings = nullptr;

	bool showFFWindow = false;

	FileFinder();
	~FileFinder();

	void startBackgroundThread();
	void toggleWindow();

	// Called by the UI layer (friend renderFileFinder) — logic, no widgets.
	void commitSelection(); // Enter: open selected file, close
	void cancelAndClose();	// Esc / click-outside: close without opening
	void updateFilteredList();

  private:
	friend void renderFileFinder(FileFinder &f);
	friend void renderFileFinderHeader(FileFinder &f);
	friend bool renderFileFinderSearchInput(FileFinder &f);
	friend void renderFileFinderList(FileFinder &f);

	static constexpr size_t INPUT_CAP = 256;
	static constexpr int SCAN_INTERVAL_SEC = 3;

	char searchBuffer[INPUT_CAP] = {};
	std::string previousSearch;

	std::vector<FileEntry> fileList;	 // full project list (worker)
	std::vector<FileEntry> filteredList; // search results (UI thread)
	int selectedIndex = 0;

	// Background scanner
	std::thread workerThread;
	std::mutex fileListMutex;
	std::atomic<bool> stopThread{false};
	std::string currentProjectDir;
	bool workerStarted = false;

	void backgroundRefresh();
	void refreshFileListBackground(const std::string &projectDir);
};
