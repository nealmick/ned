#pragma once

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
};
