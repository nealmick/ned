/*  
    files.h
    Main file logic, handles rendering file tree, saving after changes, and more...
*/



#pragma once
#include <map>
#include <string>
#include <vector>
#include <filesystem>
#include <GLFW/glfw3.h>

#include "editor.h"
#include "imgui.h"
#include "util/undo_redo_manager.h"


namespace fs = std::filesystem;

struct FileNode {
    std::string name;
    std::string fullPath;
    bool isDirectory;
    std::vector<FileNode> children;
    bool isOpen = false;
    ImTextureID iconTexture = nullptr;
};
class FileExplorer {
public:
    EditorState editor_state;
    void openFolderDialog();
    void buildFileTree(const fs::path& path, FileNode& node);
    void loadFileContent(const std::string& path, std::function<void()> afterLoadCallback = nullptr);

    void displayFileTree(FileNode& node);
    std::string getSelectedFolder() const { return selectedFolder; }
    std::string getCurrentFile() const { return currentFile; }
    bool showFileDialog() const { return _showFileDialog; }
    FileNode& getRootNode() { return rootNode; }

    void saveCurrentFile();
    bool hasUnsavedChanges() const { return _unsavedChanges; }
    void setUnsavedChanges(bool value) { _unsavedChanges = value; }

    void renderFileContent();
    std::string& getFileContent() { return fileContent; }
    
    std::vector<ImVec4>& getFileColors() { return fileColors; }
    void refreshSyntaxHighlighting();
    bool findBoxActive = false;
    std::string findText = "";
    void findNext();
    void findPrevious();
    size_t lastFoundPos = std::string::npos;
    
    void loadIcons();
    ImTextureID getIconForFile(const std::string& filename);
    
    void handleUndo();
    void handleRedo();
    void addUndoState(int changeStart, int changeEnd);
    std::map<std::string, UndoRedoManager> fileUndoManagers;
    UndoRedoManager* currentUndoManager = nullptr;
    void setFileContent(const std::string& content) {
        fileContent = content;
        fileColors.resize(content.size(), ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
    }
    std::string currentOpenFile;
    std::string previousOpenFile;
    ImVec4 openedFileColor = ImVec4(0.65f, 0.65f, 0.65f, 1.0f);  //grey vistedfiles
    void refreshFileTree();
    void preserveOpenStates(const FileNode& oldNode, FileNode& newNode);
    std::string currentFile;
    ImTextureID getIcon(const std::string& iconName) const {
        auto it = fileTypeIcons.find(iconName);
        if (it != fileTypeIcons.end()) {
            return it->second;
        }
        return fileTypeIcons.at("default"); // Return default icon if not found
    }
private:
    std::string selectedFolder;
    bool _showFileDialog = true;
    FileNode rootNode;
    bool _unsavedChanges = false;

    std::string fileContent;
    std::vector<ImVec4> fileColors;
    std::map<std::string, ImTextureID> fileTypeIcons;

    
    double lastFileTreeRefreshTime = 0.0;
    const double FILE_TREE_REFRESH_INTERVAL = 1.0; // Refresh every 1 second
};

extern Editor gEditor;
extern FileExplorer gFileExplorer;

