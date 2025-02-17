/*
    files.h
    Main file logic, handles rendering file tree, saving after changes, and more...
*/

#pragma once
#include "editor.h"
#include "imgui.h"
#include "util/undo_redo_manager.h"
#include <GLFW/glfw3.h>
#include <filesystem>
#include <map>
#include <string>
#include <vector>

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
    UndoRedoManager *currentUndoManager = nullptr;

    std::map<std::string, UndoRedoManager> fileUndoManagers;
    std::string currentOpenFile;
    std::string previousOpenFile;
    std::string currentFile;
    bool findBoxActive = false;
    std::string findText = "";
    size_t lastFoundPos = std::string::npos;
    ImVec4 openedFileColor = ImVec4(0.65f, 0.65f, 0.65f, 1.0f);
    bool showWelcomeScreen = true;

    // File tree operations
    void buildFileTree(const fs::path &path, FileNode &node);
    void refreshFileTree();
    void preserveOpenStates(const FileNode &oldNode, FileNode &newNode);
    FileNode &getRootNode() { return rootNode; }

    // File operations
    void loadFileContent(const std::string &path, std::function<void()> afterLoadCallback = nullptr);
    void saveCurrentFile();
    void setFileContent(const std::string &content) {
        fileContent = content;
        fileColors.resize(content.size(), ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
    }

    // Content getters/setters
    std::string &getFileContent() { return fileContent; }
    std::vector<ImVec4> &getFileColors() { return fileColors; }
    std::string getCurrentFile() const { return currentFile; }
    bool hasUnsavedChanges() const { return _unsavedChanges; }
    void setUnsavedChanges(bool value) { _unsavedChanges = value; }
    std::string getSelectedFolder() const { return selectedFolder; }

    // Undo/Redo
    void handleUndo();
    void handleRedo();
    void addUndoState(int changeStart, int changeEnd);

    // Search functionality
    void findNext();
    void findPrevious();

    // UI functions
    void openFolderDialog();
    void displayFileTree(FileNode &node);
    void renderFileContent();
    void refreshSyntaxHighlighting();

    // Icon handling
    void loadIcons();
    ImTextureID getIconForFile(const std::string &filename);
    ImTextureID getIcon(const std::string &iconName) const {
        auto it = fileTypeIcons.find(iconName);
        if (it != fileTypeIcons.end()) {
            return it->second;
        }
        return fileTypeIcons.at("default");
    }

    // Dialog state
    bool showFileDialog() const { return _showFileDialog; }
    void setShowFileDialog(bool show) { _showFileDialog = show; }
    void setShowWelcomeScreen(bool show) { showWelcomeScreen = show; }
    bool getShowWelcomeScreen() const { return showWelcomeScreen; }

  private:
    FileNode rootNode;
    std::string selectedFolder;
    bool _showFileDialog = false;
    bool _unsavedChanges = false;
    std::string fileContent;
    std::vector<ImVec4> fileColors;
    std::map<std::string, ImTextureID> fileTypeIcons;
    double lastFileTreeRefreshTime = 0.0;
    const double FILE_TREE_REFRESH_INTERVAL = 1.0;

    // Icon loading helpers
    GLuint createTexture(const unsigned char *pixels, int width, int height);
    bool loadSingleIcon(const std::string &iconFile);
    void createDefaultIcon();

    struct IconDimensions {
        static constexpr int WIDTH = 32;
        static constexpr int HEIGHT = 32;
        static constexpr float SVG_DPI = 96.0f;
    };

    // File loading helpers
    void resetEditorState();
    bool readFileContent(const std::string &path);
    void updateFileColorBuffer();
    void setupUndoManager(const std::string &path);
    void initializeSyntaxHighlighting(const std::string &path);
    void handleLoadError();
    void updateFilePathStates(const std::string &path);

    // File tree display helpers
    struct TreeDisplayMetrics {
        float currentFontSize;
        float folderIconSize;
        float fileIconSize;
        float itemHeight;
        float indentWidth;
        ImVec2 cursorPos;
    };

    struct TreeStyleSettings {
        static constexpr float FRAME_ROUNDING = 6.0f;
        static constexpr float HORIZONTAL_PADDING = 8.0f;
        static constexpr float TEXT_PADDING = 14.0f;
        static constexpr float LEFT_MARGIN = 3.0f;
        static constexpr ImVec2 FRAME_PADDING = ImVec2(4, 2);
        static constexpr ImVec2 ITEM_SPACING = ImVec2(1, 3);
        static constexpr ImVec4 TRANSPARENT_BG = ImVec4(0, 0, 0, 0);
        static constexpr ImVec4 HOVER_COLOR = ImVec4(0.26f, 0.59f, 0.98f, 0.31f);
        static constexpr ImVec4 INACTIVE_TEXT = ImVec4(0.7f, 0.7f, 0.7f, 1.0f);
    };

    TreeDisplayMetrics calculateDisplayMetrics();
    void pushTreeStyles();
    void displayDirectoryNode(const FileNode &node, const TreeDisplayMetrics &metrics, int &depth);
    void displayFileNode(const FileNode &node, const TreeDisplayMetrics &metrics, int depth);
    ImTextureID getFolderIcon(bool isOpen);
    void renderNodeText(const std::string &name, bool isCurrentFile);

    // Undo/Redo helpers
    void applyContentChange(const UndoRedoManager::State &state, bool preAllocate = false);
    void adjustColorBuffer(int changeStart, int lengthDiff);
    void rehighlightChangedRegion(int changeStart, int changeEnd);

    void handleFindBoxActivation();
    void renderFindBox();
    void handleFindBoxKeyboardShortcuts();
    void renderEditor(bool &text_changed);
};

extern Editor gEditor;
extern FileExplorer gFileExplorer;