#pragma once

#include "imgui.h"
#include <filesystem>
#include <string>
#include <vector>

namespace fs = std::filesystem;

class FileExplorer;
class Settings;

struct FileNode
{
	std::string name;
	std::string fullPath;
	bool isDirectory = false;
	bool isOpen = false;
	std::vector<FileNode> children;
};
class FileTree
{
  public:
	FileTree() = default;

	FileExplorer *fileExplorer = nullptr;
	Settings *settings = nullptr;

	FileNode rootNode;

	void buildFileTree(const fs::path &path, FileNode &node);

	void refreshFileTree();

	void displayFileTree(FileNode &node, int depth = 0);

  private:
	// Row layout
	static constexpr float INDENT_WIDTH = 18.0f;
	static constexpr float ICON_SCALE = 1.1f;
	static constexpr float ICON_TEXT_GAP = 7.0f;
	static constexpr float ROW_PAD_X = 4.0f;
	static constexpr float FRAME_ROUNDING = 6.0f;
	static constexpr ImVec2 FRAME_PADDING{4.0f, 2.0f};
	static constexpr ImVec2 ITEM_SPACING{1.0f, 3.0f};
	static constexpr ImVec4 TRANSPARENT_COLOR{0, 0, 0, 0};
	static constexpr ImVec4 HOVER_COLOR{0.06f, 0.35f, 0.60f, 0.85f};
	// Hard dim so dirty names are obvious on both light and dark themes.
	static constexpr ImVec4 MODIFIED_TEXT{0.45f, 0.45f, 0.45f, 1.0f};

	void drawNodeRow(FileNode &node, int depth);
	void drawNodeLabel(const std::string &name,
					   const std::string &fullPath,
					   bool isCurrentFile);
	ImVec4 themeTextColor() const;
	ImTextureID folderIcon(bool isOpen) const;
	std::string findReadmeInRoot() const;

	// Open README.md once when a project is first loaded with no active file.
	bool shouldAutoOpenReadme = true;
};
