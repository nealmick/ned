// file_finder.h

#pragma once
#include "files.h"
#include "imgui.h"
#include <filesystem>
#include <vector>

namespace fs = std::filesystem;

struct FileEntry
{
	std::string fullPath;
	std::string relativePath;
	std::string fullPathLower; // Lower-case full path for searching
	std::string filenameLower; // Lower-case file name (from relativePath.filename())
};

class FileFinder
{
  private:
	char searchBuffer[256] = ""; // Buffer for the search input
	bool wasKeyboardFocusSet = false;

	std::string previousSearch;
	std::string originalFile;
	std::string currentlyLoadedFile;

	std::vector<FileEntry> fileList;
	std::vector<FileEntry> filteredList;
	bool isInitialSelection = true; // Track if this is the first selection after opening

	int selectedIndex = 0;
	void updateFilteredList();
	std::thread workerThread;
	std::mutex fileListMutex;
	std::atomic<bool> stopThread{false};
	std::string currentProjectDir;

	void backgroundRefresh();
	void refreshFileListBackground(const std::string &projectDir);
	// Helper functions to break up the renderWindow() logic:
	void renderHeader();
	bool renderSearchInput();
	void renderFileList();

	void handleSelectionChange();
	int orginal_cursor_index;

	std::chrono::steady_clock::time_point lastSelectionTime;
	std::string pendingFile;
	bool hasPendingSelection = false;

	void checkPendingSelection(); // Add this declaration

	// Embedded mode support
	bool isEmbedded = false;
	ImVec2 editorPanePos;
	ImVec2 editorPaneSize;

  public:
	bool showFFWindow = false;
	// TODO: Add embedded positioning support when needed
	void toggleWindow();
	bool isWindowOpen() const;
	void renderWindow();

	// Embedded mode support
	void setEmbedded(bool embedded) { isEmbedded = embedded; }
	void setEditorPaneBounds(const ImVec2 &pos, const ImVec2 &size)
	{
		editorPanePos = pos;
		editorPaneSize = size;
	}

	FileFinder();
	~FileFinder();
};

extern FileFinder gFileFinder;
