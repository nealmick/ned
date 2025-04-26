#include "file_tree.h"
#include "../editor/editor_utils.h"
#include "../files/files.h"
#include "../util/settings.h"
#include "editor.h"
#include <GLFW/glfw3.h>
#include <algorithm>
#include <iostream>

FileTree gFileTree;

FileTree::FileTree() {}

void FileTree::displayFileTree(FileNode &node)
{
	static int current_depth = 0;
	TreeDisplayMetrics metrics = calculateDisplayMetrics();

	pushTreeStyles();
	ImGui::PushID(node.fullPath.c_str());

	ImGui::SetCursorPosX(ImGui::GetCursorPosX() + current_depth * metrics.indentWidth);

	if (node.isDirectory)
	{
		displayDirectoryNode(node, metrics, current_depth);
	} else
	{
		displayFileNode(node, metrics, current_depth);
	}

	ImGui::PopID();
	ImGui::PopStyleVar(3);
}

void FileTree::renderNodeText(const std::string &name, bool isCurrentFile)
{
	if (!isCurrentFile)
	{
		ImGui::Text("%s", name.c_str());
		return;
	}

	if (gSettings.getRainbowMode())
	{
		// Get synchronized rainbow color - no need to pass blink_time
		ImVec4 rainbowColor = EditorUtils::GetRainbowColor();
		// Use the rainbow color for the text
		ImGui::PushStyleColor(ImGuiCol_Text, rainbowColor);
		ImGui::Text("%s", name.c_str());
		ImGui::PopStyleColor();
	} else
	{
		ImGui::PushStyleColor(ImGuiCol_Text, TreeStyleSettings::INACTIVE_TEXT);
		ImGui::Text("%s", name.c_str());
		ImGui::PopStyleColor();
	}
}

void FileTree::displayDirectoryNode(const FileNode &node,
									const TreeDisplayMetrics &metrics,
									int &depth)
{
	float multiplier = 1.1f;
	float iconSize = metrics.folderIconSize * multiplier;
	ImVec2 iconDimensions(iconSize, iconSize);
	ImTextureID folderIcon = getFolderIcon(node.isOpen);

	ImVec2 textSize = ImGui::CalcTextSize(node.name.c_str());

	float requiredWidth =
		(depth * metrics.indentWidth) + TreeStyleSettings::HORIZONTAL_PADDING + iconDimensions.x +
		TreeStyleSettings::TEXT_PADDING // Spacing between icon and text (matches SameLine offset)
		+ textSize.x;

	float availableWidth = ImGui::GetContentRegionAvail().x;

	float buttonWidth = std::max(requiredWidth, availableWidth);

	ImGui::PushStyleColor(ImGuiCol_Button, TreeStyleSettings::TRANSPARENT_BG);
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, TreeStyleSettings::HOVER_COLOR);
	ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));

	bool isOpen =
		ImGui::Button(("##" + node.fullPath).c_str(), ImVec2(buttonWidth, metrics.itemHeight));

	ImGui::PopStyleVar();
	ImGui::PopStyleColor(2);

	float lineCenterY = metrics.cursorPos.y + (metrics.itemHeight / 2.0f);
	float iconTopY = lineCenterY - (iconDimensions.y / 2.0f);

	ImGui::SetCursorPosX(metrics.cursorPos.x + depth * metrics.indentWidth +
						 TreeStyleSettings::HORIZONTAL_PADDING);
	ImGui::SetCursorPosY(iconTopY);
	ImGui::Image(folderIcon, iconDimensions);

	float textHeight = ImGui::GetTextLineHeight();
	float textTopY = lineCenterY - (textHeight / 2.0f);

	ImGui::SameLine(metrics.cursorPos.x + depth * metrics.indentWidth + iconDimensions.x +
					TreeStyleSettings::HORIZONTAL_PADDING + TreeStyleSettings::TEXT_PADDING);
	ImGui::SetCursorPosY(textTopY);
	ImGui::Text("%s", node.name.c_str());

	if (isOpen)
	{
		const_cast<FileNode &>(node).isOpen = !node.isOpen;
		if (node.isOpen)
		{
			buildFileTree(node.fullPath, const_cast<FileNode &>(node));
		}
	}

	if (node.isOpen)
	{
		depth++;
		for (const auto &child : node.children)
		{
			displayFileTree(const_cast<FileNode &>(child));
		}
		depth--;
	}
}

void FileTree::displayFileNode(const FileNode &node, const TreeDisplayMetrics &metrics, int depth)
{
	float multiplier = 1.1f;
	float iconSize = metrics.fileIconSize * multiplier;
	ImVec2 iconDimensions(iconSize, iconSize);
	ImTextureID fileIcon = gFileExplorer.getIconForFile(node.name);

	// Calculate the width of the node's text
	ImVec2 textSize = ImGui::CalcTextSize(node.name.c_str());

	// Calculate the required width to fit indentation, icon, spacing, and text
	float requiredWidth = (depth * metrics.indentWidth) + TreeStyleSettings::LEFT_MARGIN +
						  iconSize +
						  10.0f // Spacing between icon and text (matches SameLine offset)
						  + textSize.x;

	// Get the available width in the content region
	float availableWidth = ImGui::GetContentRegionAvail().x;

	// Use the larger of required or available width to prevent cutoff
	float buttonWidth = std::max(requiredWidth, availableWidth);

	ImGui::PushStyleColor(ImGuiCol_Button, TreeStyleSettings::TRANSPARENT_BG);
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, TreeStyleSettings::HOVER_COLOR);
	ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));

	// Create button with calculated width to cover full content
	bool clicked =
		ImGui::Button(("##" + node.fullPath).c_str(), ImVec2(buttonWidth, metrics.itemHeight));

	ImGui::PopStyleVar();
	ImGui::PopStyleColor(2);

	// Position icon and text as before...
	float lineCenterY = metrics.cursorPos.y + (metrics.itemHeight / 2.0f);
	float iconTopY = lineCenterY - (iconDimensions.y / 2.0f);

	ImGui::SetCursorPosX(metrics.cursorPos.x + depth * metrics.indentWidth +
						 TreeStyleSettings::LEFT_MARGIN);
	ImGui::SetCursorPosY(iconTopY);
	ImGui::Image(fileIcon, iconDimensions);

	float textHeight = ImGui::GetTextLineHeight();
	float textTopY = lineCenterY - (textHeight / 2.0f);

	ImGui::SameLine(depth * metrics.indentWidth + iconDimensions.x +
					TreeStyleSettings::LEFT_MARGIN + 10);
	ImGui::SetCursorPosY(textTopY);
	renderNodeText(node.name, node.fullPath == gFileExplorer.currentFile);

	if (clicked)
	{
		editor_state.cursor_index = 0;
		editor_state.selection_start = 0;
		editor_state.selection_end = 0;
		editor_state.selection_active = false;
		gFileExplorer.loadFileContent(node.fullPath);
	}
}

void FileTree::refreshFileTree()
{
	// Always allow immediate refresh for initial load
	if (!initialRefreshDone)
	{
		// Force initial refresh
		lastFileTreeRefreshTime = 0;
		initialRefreshDone = true;
	} else
	{
		// Use normal interval checks after initial load
		double currentTime = glfwGetTime();
		if (currentTime - lastFileTreeRefreshTime < FILE_TREE_REFRESH_INTERVAL)
		{
			return;
		}
		lastFileTreeRefreshTime = currentTime;
	}

	if (!gFileExplorer.selectedFolder.empty())
	{
		FileNode oldRoot = rootNode;

		rootNode.name = fs::path(gFileExplorer.selectedFolder).filename().string();
		rootNode.fullPath = gFileExplorer.selectedFolder;
		rootNode.isDirectory = true;
		rootNode.isOpen = true; // Root should stay open

		buildFileTree(gFileExplorer.selectedFolder, rootNode);

		preserveOpenStates(oldRoot, rootNode);
	}
}
void FileTree::preserveOpenStates(const FileNode &oldNode, FileNode &newNode)
{
	for (auto &newChild : newNode.children)
	{
		auto it = std::find_if(oldNode.children.begin(),
							   oldNode.children.end(),
							   [&newChild](const FileNode &oldChild) {
								   return oldChild.fullPath == newChild.fullPath;
							   });

		if (it != oldNode.children.end())
		{
			newChild.isOpen = it->isOpen;
			if (newChild.isDirectory && newChild.isOpen)
			{
				preserveOpenStates(*it, newChild);
			}
		}
	}
}

void FileTree::buildFileTree(const fs::path &path, FileNode &node)
{
	// Don't clear children if they already exist and the node is open
	if (!node.isOpen && !node.children.empty())
	{
		return;
	}

	std::vector<FileNode> newChildren;
	try
	{
		for (const auto &entry : fs::directory_iterator(path))
		{
			FileNode child;
			child.name = entry.path().filename().string();
			child.fullPath = entry.path().string();
			child.isDirectory = entry.is_directory();

			// Find existing child to preserve its state
			auto existingChild = std::find_if(node.children.begin(),
											  node.children.end(),
											  [&child](const FileNode &existing) {
												  return existing.fullPath == child.fullPath;
											  });

			if (existingChild != node.children.end())
			{
				// Preserve the existing child's state and children
				child.isOpen = existingChild->isOpen;
				child.children = std::move(existingChild->children);
			}

			// If it's a directory and it's open, build its tree
			if (child.isDirectory && child.isOpen)
			{
				buildFileTree(child.fullPath, child);
			}

			newChildren.push_back(std::move(child));
		}
	} catch (const fs::filesystem_error &e)
	{
		std::cerr << "Error accessing directory " << path << ": " << e.what() << std::endl;
	}

	// Sort directories first, then files by name
	std::sort(newChildren.begin(), newChildren.end(), [](const FileNode &a, const FileNode &b) {
		if (a.isDirectory != b.isDirectory)
		{
			return a.isDirectory > b.isDirectory;
		}
		return a.name < b.name;
	});

	node.children = std::move(newChildren);
}

FileTree::TreeDisplayMetrics FileTree::calculateDisplayMetrics()
{
	TreeDisplayMetrics metrics;
	metrics.currentFontSize = gSettings.getFontSize();
	metrics.folderIconSize = metrics.currentFontSize * 0.8f;
	metrics.fileIconSize = metrics.currentFontSize * 1.2f;
	metrics.itemHeight = ImGui::GetFrameHeight();
	metrics.indentWidth = 28.0f;
	metrics.cursorPos = ImGui::GetCursorPos();
	return metrics;
}

void FileTree::pushTreeStyles()
{
	ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, TreeStyleSettings::FRAME_ROUNDING);
	ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, TreeStyleSettings::FRAME_PADDING);
	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, TreeStyleSettings::ITEM_SPACING);
}

ImTextureID FileTree::getFolderIcon(bool isOpen)
{
	ImTextureID icon =
		isOpen ? gFileExplorer.getIcon("folder-open") : gFileExplorer.getIcon("folder");
	if (!icon)
	{
		icon = gFileExplorer.getIcon("folder");
	}
	return icon ? icon : gFileExplorer.getIcon("default");
}
