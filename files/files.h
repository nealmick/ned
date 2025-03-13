/*
    File: files.h
    Description: Main file logic, handles saving after changes, and more...
*/

#pragma once
#include "imgui.h"
#include <GLFW/glfw3.h>

#include "../editor/editor.h"
#include "../editor/editor_lsp.h"
#include "file_content_search.h"
#include "file_tree.h"
#include "file_undo_redo.h"

#include <filesystem>
#include <map>
#include <string>
#include <vector>

namespace fs = std::filesystem;

class FileExplorer
{
  public:
    EditorState editor_state;
    const size_t MAX_FILE_SIZE = 1 * 1024 * 1024; // 1mb

    UndoRedoManager *currentUndoManager = nullptr;
    std::map<std::string, UndoRedoManager> fileUndoManagers;
    std::string currentOpenFile;
    std::string previousOpenFile;
    std::string currentFile;
    ImVec4 openedFileColor = ImVec4(0.65f, 0.65f, 0.65f, 1.0f);
    bool showWelcomeScreen = true;

    // File operations
    void loadFileContent(const std::string &path, std::function<void()> afterLoadCallback = nullptr);
    void saveCurrentFile();
    void setFileContent(const std::string &content)
    {
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

    // UI functions
    void openFolderDialog();
    void renderFileContent();
    void refreshSyntaxHighlighting();

    // Icon handling
    void loadIcons();
    ImTextureID getIconForFile(const std::string &filename);
    ImTextureID getIcon(const std::string &iconName) const
    {
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

    void initializeLSP(const std::string &workspacePath);
    void notifyLSPFileOpen(const std::string &filePath);

  private:
    bool lspInitialized = false;

    std::string selectedFolder;
    bool _showFileDialog = false;
    bool _unsavedChanges = false;
    std::string fileContent;
    std::vector<ImVec4> fileColors;
    std::map<std::string, ImTextureID> fileTypeIcons;

    // Icon loading helpers
    struct IconDimensions
    {
        static constexpr int WIDTH = 32;
        static constexpr int HEIGHT = 32;
        static constexpr float SVG_DPI = 96.0f;
    };

    GLuint createTexture(const unsigned char *pixels, int width, int height);
    bool loadSingleIcon(const std::string &iconFile);
    void createDefaultIcon();

    // File loading helpers
    void resetEditorState();
    bool readFileContent(const std::string &path);
    void updateFileColorBuffer();
    void setupUndoManager(const std::string &path);
    void initializeSyntaxHighlighting(const std::string &path);
    void handleLoadError();
    void updateFilePathStates(const std::string &path);

    // Undo/Redo helpers
    void applyContentChange(const UndoRedoManager::State &state, bool preAllocate = false);
    void adjustColorBuffer(int changeStart, int lengthDiff);
    void rehighlightChangedRegion(int changeStart, int changeEnd);

    // Find box helpers
    void renderEditor(bool &text_changed);
};

extern Editor gEditor;
extern FileExplorer gFileExplorer;