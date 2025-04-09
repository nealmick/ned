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

    void refreshFileList();    // Refreshes the list of files
    void updateFilteredList(); // Updates filtered list based on search

    // Helper functions to break up the renderWindow() logic:
    void renderHeader();
    bool renderSearchInput();
    void renderFileList();

    void handleSelectionChange();
    int orginal_cursor_index;

  public:
    bool showFFWindow = false;
    FileFinder() = default;
    void toggleWindow();
    bool isWindowOpen() const;
    void renderWindow();
};

extern FileFinder gFileFinder;
