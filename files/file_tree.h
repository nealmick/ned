#pragma once

#include "imgui.h"
#include <filesystem>
#include <string>
#include <vector>

namespace fs = std::filesystem;

// Move FileNode struct from files.h
struct FileNode
{
	std::string name;
	std::string fullPath;
	bool isDirectory;
	std::vector<FileNode> children;
	bool isOpen = false;
	ImTextureID iconTexture = nullptr;
};

class FileTree
{
  public:
	FileTree();

	// Core file tree operations
	void buildFileTree(const fs::path &path, FileNode &node);
	void refreshFileTree();
	void preserveOpenStates(const FileNode &oldNode, FileNode &newNode);
	void displayFileTree(FileNode &node);

	// Getters/Setters
	FileNode &getRootNode() { return rootNode; }
	FileNode rootNode;

  private:
	double lastFileTreeRefreshTime = 0.0;
	const double FILE_TREE_REFRESH_INTERVAL = 0.5;
	bool initialRefreshDone = false; // Track if initial refresh has occurred

	struct TreeDisplayMetrics
	{
		float currentFontSize;
		float folderIconSize;
		float fileIconSize;
		float itemHeight;
		float indentWidth;
		ImVec2 cursorPos;
	};

	struct TreeStyleSettings
	{
		static constexpr float FRAME_ROUNDING = 6.0f;
		static constexpr float HORIZONTAL_PADDING = 4.0f;
		static constexpr float TEXT_PADDING = 7.0f;
		static constexpr float LEFT_MARGIN = 0.0f;
		static constexpr ImVec2 FRAME_PADDING = ImVec2(4, 2);
		static constexpr ImVec2 ITEM_SPACING = ImVec2(1, 3);
		static constexpr ImVec4 TRANSPARENT_BG = ImVec4(0, 0, 0, 0);
		static constexpr ImVec4 HOVER_COLOR = ImVec4(0.26f, 0.59f, 0.98f, 0.31f);
		static constexpr ImVec4 INACTIVE_TEXT = ImVec4(0.7f, 0.7f, 0.7f, 1.0f);
	};

	// Display helper methods
	TreeDisplayMetrics calculateDisplayMetrics();
	void pushTreeStyles();
	void displayDirectoryNode(const FileNode &node, const TreeDisplayMetrics &metrics, int &depth);
	void displayFileNode(const FileNode &node, const TreeDisplayMetrics &metrics, int depth);
	ImTextureID getFolderIcon(bool isOpen);
	void renderNodeText(const std::string &name, bool isCurrentFile);

	bool hasAutoOpenedReadme = false;
	bool shouldCheckForReadme = true;

	std::string findReadmeInRoot();
};

extern FileTree gFileTree;