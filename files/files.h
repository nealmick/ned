/*
	File: files.h
	Description: Main file logic, handles saving after changes, and more...
*/

#pragma once
#include "imgui.h"
#include <GLFW/glfw3.h>

#include "../editor/editor.h"
#include "../lsp/lsp.h"
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
	const size_t MAX_FILE_SIZE = 1 * 1024 * 1024 * 1; // 1mb

	UndoRedoManager *currentUndoManager = nullptr;
	std::map<std::string, UndoRedoManager> fileUndoManagers;

	std::string currentOpenFile;
	std::string previousOpenFile;
	std::string currentFile;

	bool showWelcomeScreen = true;

	// File operations
	void loadFileContent(const std::string &path,
						 std::function<void()> afterLoadCallback = nullptr);

	void saveCurrentFile();

	// Undo/Redo
	void handleUndo();
	void handleRedo();
	void addUndoState();
	void saveUndoRedoState();
	void loadUndoRedoState();

	// UI functions
	void openFolderDialog();
	void renderFileContent();

	// Icon handling
	// by file exntension for example .py or .cpp
	void loadIcons();

	ImTextureID getIconForFile(const std::string &filename)
	{
		// Get the filename without path
		std::string fileName = fs::path(filename).filename().string();

		// First check for special filenames without extensions

		if (fileName == "CMakeLists.txt" || fileName == "cmake")
		{
			auto it = fileTypeIcons.find("cmake");
			if (it != fileTypeIcons.end())
				return it->second;
		}
		if (fileName == ".clangd" || fileName == ".clang-format")
		{
			auto it = fileTypeIcons.find("clangd");
			if (it != fileTypeIcons.end())
				return it->second;
		}
		if (fileName == "Dockerfile")
		{
			auto it = fileTypeIcons.find("Dockerfile");
			if (it != fileTypeIcons.end())
				return it->second;
		} else if (fileName == ".gitignore")
		{
			auto it = fileTypeIcons.find("gitignore");
			if (it != fileTypeIcons.end())
				return it->second;
		} else if (fileName == ".gitmodules")
		{
			auto it = fileTypeIcons.find("gitmodule");
			if (it != fileTypeIcons.end())
				return it->second;
		}

		// Proceed with extension-based lookup for other files
		std::string extension = fs::path(filename).extension().string();
		if (!extension.empty() && extension[0] == '.')
		{
			extension = extension.substr(1);
		}

		auto it = fileTypeIcons.find(extension);
		return (it != fileTypeIcons.end()) ? it->second : fileTypeIcons["default"];
	}

	// by icon name for example folder or folder-open
	ImTextureID getIcon(const std::string &iconName) const
	{
		auto it = fileTypeIcons.find(iconName);
		if (it != fileTypeIcons.end())
		{
			return it->second;
		}
		return fileTypeIcons.at("default");
	}

	// Dialog state
	bool showFileDialog() const { return _showFileDialog; }

	void notifyLSPFileOpen(const std::string &filePath);

	bool _unsavedChanges = false;
	std::string selectedFolder;
	bool _showFileDialog = false;

  private:
	std::map<std::string, ImTextureID> fileTypeIcons;

	std::unordered_map<std::string, int> _documentVersions;

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
	bool readFileContent(const std::string &path);
	void updateFileColorBuffer();
	void setupUndoManager(const std::string &path);
	void handleLoadError();
	void updateFilePathStates(const std::string &path);

	// Undo/Redo helpers
	void applyOperation(const UndoRedoManager::Operation& op, bool isUndo);


	void resetColorBuffer();

	// Find box helpers
	void renderEditor(bool &text_changed);
};

extern Editor gEditor;
extern FileExplorer gFileExplorer;