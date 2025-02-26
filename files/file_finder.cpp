/*
    util/file_finder.cpp
    Implementation of the file finder utility.
*/
#include "file_finder.h"
#include "../util/close_popper.h"

FileFinder gFileFinder;

void FileFinder::refreshFileList()
{
    fileList.clear();
    filteredList.clear();
    selectedIndex = 0;

    std::string projectDir = gFileExplorer.getSelectedFolder();
    if (projectDir.empty())
        return;

    for (const auto &entry : fs::recursive_directory_iterator(projectDir)) {
        if (entry.is_regular_file()) {
            fs::path fullPath = entry.path();
            fs::path relativePath = fs::relative(fullPath, projectDir);

            std::string filenameLower = relativePath.filename().string();
            std::transform(filenameLower.begin(), filenameLower.end(), filenameLower.begin(), ::tolower);

            std::string fullPathStr = fullPath.string();
            std::string fullPathLower = fullPathStr;
            std::transform(fullPathLower.begin(), fullPathLower.end(), fullPathLower.begin(), ::tolower);

            fileList.push_back({fullPathStr, relativePath.string(), fullPathLower, filenameLower});
        }
    }
    filteredList = fileList;
}

void FileFinder::updateFilteredList()
{
    std::string searchTerm(searchBuffer);
    std::transform(searchTerm.begin(), searchTerm.end(), searchTerm.begin(), ::tolower);

    if (searchTerm != previousSearch) {
        selectedIndex = 0;
        previousSearch = searchTerm;
    }

    filteredList.clear();
    // For each file, filter based on the relative path (from the project folder)
    for (const auto &file : fileList) {
        std::string relativeLower = file.relativePath;
        std::transform(relativeLower.begin(), relativeLower.end(), relativeLower.begin(), ::tolower);

        // Only add if the relative path contains the search term
        if (relativeLower.find(searchTerm) != std::string::npos) {
            // If the search term doesn't include a dot, skip files whose filename begins with '.'
            if (searchTerm.find('.') == std::string::npos && !file.filenameLower.empty() && file.filenameLower[0] == '.') {
                continue;
            }
            filteredList.push_back(file);
        }
    }

    // (Optional) Sort by the length of the relative path (shortest first)
    std::sort(filteredList.begin(), filteredList.end(), [](const FileEntry &a, const FileEntry &b) { return a.relativePath.size() < b.relativePath.size(); });
}

void FileFinder::handleSelectionChange()
{
    if (!filteredList.empty() && selectedIndex >= 0 && selectedIndex < static_cast<int>(filteredList.size())) {
        const std::string &selectedFile = filteredList[selectedIndex].fullPath;

        // Only load if it's not the initial selection and the file isn't already loaded
        // AND we've had some user interaction (arrow keys or search)
        if (!isInitialSelection && selectedFile != currentlyLoadedFile) {
            currentlyLoadedFile = selectedFile;
            gFileExplorer.loadFileContent(selectedFile);
        }
    }
}
void FileFinder::toggleWindow()
{
    showFFWindow = !showFFWindow;
    ClosePopper::closeAllExcept(ClosePopper::Type::FileFinder);

    if (showFFWindow) {
        originalFile = gFileExplorer.getCurrentFile();
        currentlyLoadedFile = originalFile; // Initialize with original file
        memset(searchBuffer, 0, sizeof(searchBuffer));
        previousSearch = ""; // Initialize this to empty string
        wasKeyboardFocusSet = false;
        isInitialSelection = true;
        refreshFileList();
        std::cout << "\033[36mFileFinder:\033[0m Window opened" << std::endl;
    } else {
        std::cout << "\033[36mFileFinder:\033[0m Window closed" << std::endl;
    }
}

bool FileFinder::isWindowOpen() const { return showFFWindow; }

// Helper: Render window header (setup and title)
void FileFinder::renderHeader()
{
    // Window setup (size, position, flags)
    ImGui::SetNextWindowSize(ImVec2(500, 300), ImGuiCond_Always);
    ImGui::SetNextWindowPos(ImVec2(ImGui::GetIO().DisplaySize.x * 0.5f, ImGui::GetIO().DisplaySize.y * 0.35f), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
    ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;
    // Push window style (3 style vars, 3 style colors)
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 10.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(16.0f, 16.0f));
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.2f, 0.2f, 0.2f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.3f, 0.3f, 0.3f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.15f, 0.15f, 0.15f, 1.0f));
    ImGui::Begin("FileFinder", nullptr, windowFlags);

    ImGui::TextUnformatted("Find File");
    ImGui::Spacing();
    ImGui::Spacing();

    // Ensure keyboard focus is set on first render
    if (!wasKeyboardFocusSet) {
        ImGui::SetKeyboardFocusHere();
        wasKeyboardFocusSet = true;
    }
}

// Helper: Render the search input box and force keyboard focus.
bool FileFinder::renderSearchInput()
{
    float inputWidth = ImGui::GetContentRegionAvail().x;
    ImGui::PushItemWidth(inputWidth);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(8, 8));
    // Force keyboard focus each frame so the input stays focused
    ImGui::SetKeyboardFocusHere();
    bool enterPressed = ImGui::InputText("##SearchInput", searchBuffer, sizeof(searchBuffer), ImGuiInputTextFlags_AutoSelectAll | ImGuiInputTextFlags_EnterReturnsTrue);
    ImGui::PopStyleVar(2);
    ImGui::PopItemWidth();
    return enterPressed;
}

// Helper: Render the file list with manual clipping and no mouse hover effect.
void FileFinder::renderFileList()
{
    // Compute available space for list items.
    float itemHeight = ImGui::GetTextLineHeightWithSpacing();
    float availableHeight = ImGui::GetContentRegionAvail().y;
    int visibleCount = static_cast<int>(availableHeight / itemHeight);
    int totalItems = static_cast<int>(filteredList.size());
    int startIdx = std::max(0, selectedIndex - visibleCount / 2);
    int endIdx = std::min(totalItems, startIdx + visibleCount);
    if (endIdx == totalItems)
        startIdx = std::max(0, totalItems - visibleCount);

    // Begin a child window for the file list.
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0, 0, 0, 0));
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0, 0, 0, 0));
    ImGui::BeginChild("SearchResults", ImVec2(0, -ImGui::GetFrameHeightWithSpacing()), false, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

    // Push styling for list items.
    ImGui::PushStyleVar(ImGuiStyleVar_SelectableTextAlign, ImVec2(0.0f, 0.5f));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(8, 4));
    // Disable hover effect by making hover color transparent and keeping selection color
    ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(1.0f, 0.1f, 0.7f, 1.0f));        // Selection color
    ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.0f, 0.0f, 0.0f, 0.0f)); // Transparent hover
    ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));  // Transparent active

    for (int i = startIdx; i < endIdx; ++i) {
        bool is_selected = (i == selectedIndex);
        const FileEntry &entry = filteredList[i];
        ImGui::PushID(i);
        ImGui::Selectable("", is_selected, ImGuiSelectableFlags_SpanAllColumns);
        ImGui::SameLine();
        std::string filename = fs::path(entry.fullPath).filename().string();
        ImTextureID fileIcon = gFileExplorer.getIconForFile(filename);
        float iconSize = ImGui::GetTextLineHeight();
        ImGui::Image(fileIcon, ImVec2(iconSize, iconSize));
        ImGui::SameLine();
        ImGui::TextUnformatted(entry.relativePath.c_str());
        if (is_selected)
            ImGui::SetScrollHereY(0.5f);
        ImGui::PopID();
    }
    // Pop the style colors and variables for list items.
    ImGui::PopStyleColor(3); // header colors
    ImGui::PopStyleVar(3);
    ImGui::PopStyleColor(2); // child window colors
    ImGui::EndChild();
}

// The main renderWindow() now calls the helpers.
void FileFinder::renderWindow()
{
    // Toggle with Ctrl+P
    bool ctrl_pressed = ImGui::GetIO().KeyCtrl;
    if (ctrl_pressed && ImGui::IsKeyPressed(ImGuiKey_P)) {
        toggleWindow();
        return;
    }
    if (showFFWindow && ImGui::IsKeyPressed(ImGuiKey_Escape)) {
        // Restore the original file before closing
        if (!originalFile.empty()) {
            gFileExplorer.loadFileContent(originalFile);
        }
        toggleWindow();
        return;
    }

    if (!showFFWindow)
        return;

    // Render header (window setup and title)
    renderHeader();

    if (ImGui::IsKeyPressed(ImGuiKey_UpArrow)) {
        if (selectedIndex > 0) {
            isInitialSelection = false; // User made an intentional selection
            selectedIndex--;
            handleSelectionChange(); // Load file when moving up
        }
    }
    if (ImGui::IsKeyPressed(ImGuiKey_DownArrow)) {
        if (selectedIndex < static_cast<int>(filteredList.size()) - 1) {
            isInitialSelection = false; // User made an intentional selection
            selectedIndex++;
            handleSelectionChange(); // Load file when moving down
        }
    }

    // Render search input; if Enter is pressed, load the selected file.
    bool enterPressed = renderSearchInput();
    if (enterPressed) {
        toggleWindow(); // Just close the finder
        ImGui::End();
        ImGui::PopStyleColor(3);
        ImGui::PopStyleVar(3);
        return;
    }

    // Update filtered list based on search input.
    std::string oldSearchTerm = previousSearch; // Store the old search term
    updateFilteredList();

    // If the search term changed (meaning we have new filtered results)
    // or if we're navigating with arrow keys, handle the selection
    if (oldSearchTerm != previousSearch) {
        isInitialSelection = false; // No longer initial when search changes
        handleSelectionChange();
    }

    ImGui::Spacing();
    ImGui::Spacing();
    ImGui::Dummy(ImVec2(0, 10.0f));

    // Render the file list with manual clipping.
    renderFileList();

    ImGui::Separator();
    ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Press Ctrl+P or ESC to close");
    ImGui::End();
    // Pop the window style colors and vars pushed in renderHeader()
    ImGui::PopStyleColor(3);
    ImGui::PopStyleVar(3);
}