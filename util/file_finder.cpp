/*
    util/file_finder.cpp
    Implementation of the file finder utility.
*/
#include "util/file_finder.h"

FileFinder gFileFinder;

void FileFinder::refreshFileList() {
    fileList.clear();
    filteredList.clear();
    selectedIndex = 0;
    
    // Get the current project directory from FileExplorer
    std::string projectDir = gFileExplorer.getSelectedFolder();
    if (projectDir.empty()) return;

    // Recursively get all files
    for (const auto& entry : fs::recursive_directory_iterator(projectDir)) {
        if (entry.is_regular_file()) {
            fileList.push_back(entry.path().string());
        }
    }
    
    // Initialize filtered list with all files
    filteredList = fileList;
}
void FileFinder::updateFilteredList() {
    std::string searchTerm(searchBuffer);
    // Convert search term to lowercase for case-insensitive comparison
    std::transform(searchTerm.begin(), searchTerm.end(), searchTerm.begin(), ::tolower);
    
    // Only reset selection if the search term has changed
    if (searchTerm != previousSearch) {
        selectedIndex = 0;
        previousSearch = searchTerm;
    }
    
    filteredList.clear();
    
    // Filter files based on search term
    for (const auto& file : fileList) {
        std::string filename = file;
        std::transform(filename.begin(), filename.end(), filename.begin(), ::tolower);
        
        if (filename.find(searchTerm) != std::string::npos) {
            filteredList.push_back(file);
        }
    }
}


void FileFinder::handleNavigation() {
    if (filteredList.empty()) return;

    // Now handled in renderWindow
}

void FileFinder::toggleWindow() {
    showWindow = !showWindow;
    if (showWindow) {
        memset(searchBuffer, 0, sizeof(searchBuffer));
        wasKeyboardFocusSet = false;
        refreshFileList();
        std::cout << "\033[36mFileFinder:\033[0m Window opened" << std::endl;
    } else {
        std::cout << "\033[36mFileFinder:\033[0m Window closed" << std::endl;
    }
}

bool FileFinder::isWindowOpen() const { 
    return showWindow; 
}

void FileFinder::renderWindow() {
    // Always check for Ctrl+P toggle, even if window is closed
    bool ctrl_pressed = ImGui::GetIO().KeyCtrl;
    if (ctrl_pressed && ImGui::IsKeyPressed(ImGuiKey_P)) {
        toggleWindow();
        return;
    }

    if (!showWindow) return;
    
    // Window setup
    ImGui::SetNextWindowSize(ImVec2(500, 300), ImGuiCond_Always);
    ImGui::SetNextWindowPos(
        ImVec2(ImGui::GetIO().DisplaySize.x * 0.5f, ImGui::GetIO().DisplaySize.y * 0.35f),
        ImGuiCond_Always,
        ImVec2(0.5f, 0.5f)
    );

    ImGuiWindowFlags windowFlags = 
        ImGuiWindowFlags_NoTitleBar | 
        ImGuiWindowFlags_NoResize | 
        ImGuiWindowFlags_NoMove | 
        ImGuiWindowFlags_NoScrollbar | 
        ImGuiWindowFlags_NoScrollWithMouse;

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

    if (!wasKeyboardFocusSet) {
        ImGui::SetKeyboardFocusHere();
        wasKeyboardFocusSet = true;
    }

    if (ImGui::IsKeyPressed(ImGuiKey_Escape)) {
        toggleWindow();
        ImGui::End();
        ImGui::PopStyleColor(3);
        ImGui::PopStyleVar(3);
        return;
    }

    // Handle up/down navigation
    handleNavigation();

    // Search input
    float inputWidth = ImGui::GetContentRegionAvail().x;
    ImGui::PushItemWidth(inputWidth);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(8, 8));
    bool enterPressed = ImGui::InputText("##SearchInput", searchBuffer, sizeof(searchBuffer),
        ImGuiInputTextFlags_AutoSelectAll | ImGuiInputTextFlags_EnterReturnsTrue);
    ImGui::PopStyleVar(2);
    ImGui::PopItemWidth();

    if (enterPressed) {
        // On Enter press, load the selected file
        if (!filteredList.empty() && selectedIndex >= 0 && selectedIndex < filteredList.size()) {
            const std::string& selectedFile = filteredList[selectedIndex];
            gFileExplorer.loadFileContent(selectedFile);
            toggleWindow();  // Close the file finder after opening file
            ImGui::End();
            ImGui::PopStyleColor(3);
            ImGui::PopStyleVar(3);
            return;
        }
    }

    // Update search results in real-time
    updateFilteredList();

    
    ImGui::Spacing();
    ImGui::Spacing();

    ImGui::Dummy(ImVec2(0, 10.0f)); 

    // Handle navigation keys (moved outside of input text focus)
    if (ImGui::IsKeyPressed(ImGuiKey_UpArrow)) {
        if (selectedIndex > 0) {
            selectedIndex--;
        }
    }
    if (ImGui::IsKeyPressed(ImGuiKey_DownArrow)) {
        if (selectedIndex < filteredList.size() - 1) {
            selectedIndex++;
        }
    }
    
    // File list display with clipping
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0, 0, 0, 0));
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0, 0, 0, 0));
    
    ImGui::BeginChild("SearchResults", ImVec2(0, -ImGui::GetFrameHeightWithSpacing()), false, 
        ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
    
    ImGui::PushStyleVar(ImGuiStyleVar_SelectableTextAlign, ImVec2(0.0f, 0.5f));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(8, 4));
    
    ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.3f, 0.3f, 0.7f, 0.5f));
    ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.3f, 0.3f, 0.7f, 0.5f));
    ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(0.3f, 0.3f, 0.7f, 0.5f));

    // Display filtered list instead of full list
    for (int i = 0; i < filteredList.size(); i++) {
        bool is_selected = (i == selectedIndex);
        
        fs::path fullPath(filteredList[i]);
        fs::path projectPath(gFileExplorer.getSelectedFolder());
        fs::path relativePath = fs::relative(fullPath, projectPath);
        
        ImGui::PushID(i);
        ImGui::Selectable(relativePath.string().c_str(), is_selected, 
            ImGuiSelectableFlags_Disabled | ImGuiSelectableFlags_SpanAllColumns);

        if (is_selected) {
            ImGui::SetScrollHereY(0.5f);
        }
        ImGui::PopID();
    }

    ImGui::PopStyleVar(3);
    ImGui::PopStyleColor(5);
    ImGui::EndChild();

    ImGui::Separator();
    ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Press Ctrl+P or ESC to close");

    ImGui::End();
    ImGui::PopStyleColor(3);
    ImGui::PopStyleVar(3);
}