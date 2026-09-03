#include "file_tree.h"
#include "../editor/editor_api.h"
#include "../editor/util/text_columns.h"
#include "../files/files.h"
#include "../util/imgui_icons.h"
#include "../util/settings.h"
#include <algorithm>
#include <iostream>

// ---------------------------------------------------------------------------
// Tree building
// ---------------------------------------------------------------------------

void FileTree::buildFileTree(const fs::path &path, FileNode &node)
{
	// Closed nodes that already have children: keep them as-is (lazy).
	if (!node.isOpen && !node.children.empty())
		return;

	auto shouldSkip = [](const std::string &name) {
		return name == ".DS_Store" || name == "thumbs.db";
	};

	std::vector<FileNode> newChildren;
	try
	{
		for (const auto &entry : fs::directory_iterator(path))
		{
			const std::string filename = entry.path().filename().string();
			if (shouldSkip(filename))
				continue;

			FileNode child;
			child.name = filename;
			child.fullPath = entry.path().string();
			child.isDirectory = entry.is_directory();

			// Carry over open state + already-loaded grandchildren.
			auto existing = std::find_if(
				node.children.begin(), node.children.end(), [&](const FileNode &n) {
					return n.fullPath == child.fullPath;
				});
			if (existing != node.children.end())
			{
				child.isOpen = existing->isOpen;
				child.children = std::move(existing->children);
			}

			if (child.isDirectory && child.isOpen)
				buildFileTree(child.fullPath, child);

			newChildren.push_back(std::move(child));
		}
	} catch (const fs::filesystem_error &e)
	{
		std::cerr << "Error accessing directory " << path << ": " << e.what()
				  << std::endl;
	}

	// Directories first, then alphabetical.
	std::sort(
		newChildren.begin(), newChildren.end(), [](const FileNode &a, const FileNode &b) {
			if (a.isDirectory != b.isDirectory)
				return a.isDirectory > b.isDirectory;
			return a.name < b.name;
		});

	node.children = std::move(newChildren);
}

void FileTree::refreshFileTree()
{
	if (!fileExplorer || fileExplorer->projectRoot.empty())
		return;

	const std::string &folder = fileExplorer->projectRoot;

	rootNode.name = fs::path(folder).filename().string();
	rootNode.fullPath = folder;
	rootNode.isDirectory = true;
	rootNode.isOpen = true;

	buildFileTree(folder, rootNode);
}

// ---------------------------------------------------------------------------
// Display
// ---------------------------------------------------------------------------
