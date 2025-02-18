/*
    util/file_finder.h
    This utility provides a fuzzy file finder popup similar to Sublime Text/Vim.
*/

#pragma once
#include "imgui.h"
#include "files.h"
#include <string>
#include <iostream>
#include <cstring>
#include <vector>
#include <filesystem>
#include <algorithm>

namespace fs = std::filesystem;

class FileFinder {
private:
    bool showWindow = false;
    char searchBuffer[256] = "";  // Buffer for the search input
    bool wasKeyboardFocusSet = false;
    
    std::vector<std::string> fileList;      // List of all files
    std::vector<std::string> filteredList;  // List of filtered files
    int selectedIndex = 0;                  // Currently selected file index
    
    void refreshFileList();                 // Refreshes the list of files
    void handleNavigation();               // Handles up/down navigation
    void updateFilteredList();             // Updates filtered list based on search
    std::string previousSearch;
public:
    FileFinder() = default;
    void toggleWindow();
    bool isWindowOpen() const;
    void renderWindow();
};
extern FileFinder gFileFinder;