#include "file_tree.h"
#include "../editor/editor_git.h"
#include "../editor/editor_utils.h"
#include "../files/files.h"
#include "../util/settings.h"
#include "editor.h"
#include <GLFW/glfw3.h>
#include <algorithm>
#include <iostream>

void FileTree::startGitStatusTracking()
{
	gitStatusEnabled = true;
	gitStatusThread = std::thread(&FileTree::gitStatusBackgroundTask, this);
}

void FileTree::stopGitStatusTracking()
{
	if (gitStatusThread.joinable())
	{
		gitStatusEnabled = false;
		gitStatusThread.join();
	}
}

FileTree::~FileTree() { stopGitStatusTracking(); }

void FileTree::gitStatusBackgroundTask()
{
	while (gitStatusEnabled)
	{
		std::set<std::string> modifiedFiles = gEditorGit.getModifiedFilePaths();

		// Cache the results thread-safely
		{
			std::lock_guard<std::mutex> lock(modifiedFilesMutex);
			cachedModifiedFiles = modifiedFiles;
		}

		std::this_thread::sleep_for(std::chrono::seconds(1));
	}
}

FileTree gFileTree;

FileTree::FileTree()
	: initialRefreshDone(false), hasAutoOpenedReadme(false), shouldCheckForReadme(true)
{
}

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

void FileTree::renderNodeText(const std::string &name,
							  const std::string &fullPath,
							  bool isCurrentFile)
{
	// Get current theme text color as default
	extern Settings gSettings;
	ImVec4 textColor = gSettings.getCurrentTextColor();

	// Convert fullPath to relative path for Git status check
	std::string relativePath = fullPath;
	if (fullPath.rfind(gFileExplorer.selectedFolder, 0) == 0)
	{
		if (gFileExplorer.selectedFolder.length() < fullPath.length())
		{
			relativePath = fullPath.substr(gFileExplorer.selectedFolder.length() + 1);
		} else
		{
			relativePath = "."; // Root folder itself
		}
	}

	// Check if file is modified (use cached results from background thread)
	bool isModified = false;
	{
		std::lock_guard<std::mutex> lock(modifiedFilesMutex);
		isModified = cachedModifiedFiles.find(relativePath) != cachedModifiedFiles.end();
	}

	// Prioritize active file rainbow color
	if (isCurrentFile && gSettings.getRainbowMode())
	{
		textColor = EditorUtils::GetRainbowColor();
	}
	// Fallback for current file if not rainbow mode - use theme color
	else if (isCurrentFile)
	{
		textColor = gSettings.getCurrentTextColor();
	}
	// Dark grey color for modified files (but not the current file)
	else if (isModified)
	{
		textColor = ImVec4(0.4f, 0.4f, 0.4f, 1.0f); // Dark grey for modified files
	}
	// Default color for other files - use theme color
	else
	{
		textColor = gSettings.getCurrentTextColor();
	}

	ImGui::PushStyleColor(ImGuiCol_Text, textColor);
	ImGui::Text("%s", name.c_str());
	ImGui::PopStyleColor();
}
std::string FileTree::findReadmeInRoot()
{
	if (!rootNode.isDirectory)
		return "";

	// Case-insensitive search for readme.md variations
	for (const auto &child : rootNode.children)
	{
		if (!child.isDirectory)
		{
			std::string lowerName = child.name;
			std::transform(
				lowerName.begin(), lowerName.end(), lowerName.begin(), ::tolower);

			if (lowerName == "readme.md" || lowerName == "readme")
			{
				return child.fullPath;
			}
		}
	}
	return "";
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

	float requiredWidth = (depth * metrics.indentWidth) +
						  TreeStyleSettings::HORIZONTAL_PADDING + iconDimensions.x +
						  TreeStyleSettings::TEXT_PADDING // Spacing between icon and text
														  // (matches SameLine offset)
						  + textSize.x;

	float availableWidth = ImGui::GetContentRegionAvail().x;

	float buttonWidth = std::max(requiredWidth, availableWidth);

	ImGui::PushStyleColor(ImGuiCol_Button, TreeStyleSettings::TRANSPARENT_BG);
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, TreeStyleSettings::HOVER_COLOR);
	ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));

	bool isOpen = ImGui::Button(("##" + node.fullPath).c_str(),
								ImVec2(buttonWidth, metrics.itemHeight));

	ImGui::PopStyleVar();
	ImGui::PopStyleColor(2);

	float lineCenterY = metrics.cursorPos.y + (metrics.itemHeight / 2.0f);
	float iconTopY = lineCenterY - (iconDimensions.y / 2.0f);
	float textHeight = ImGui::GetTextLineHeight();
	float textTopY = lineCenterY - (textHeight / 2.0f);

	// Position icon and text on the same line with proper alignment
	ImGui::SetCursorPosX(metrics.cursorPos.x + depth * metrics.indentWidth +
						 TreeStyleSettings::HORIZONTAL_PADDING);
	ImGui::SetCursorPosY(iconTopY);
	ImGui::Image(folderIcon, iconDimensions);

	ImGui::SameLine(metrics.cursorPos.x + depth * metrics.indentWidth + iconDimensions.x +
					TreeStyleSettings::HORIZONTAL_PADDING +
					TreeStyleSettings::TEXT_PADDING);
	ImGui::SetCursorPosY(textTopY);

	// Use theme text color for folder names
	extern Settings gSettings;
	ImVec4 folderTextColor = gSettings.getCurrentTextColor();
	ImGui::PushStyleColor(ImGuiCol_Text, folderTextColor);
	ImGui::Text("%s", node.name.c_str());
	ImGui::PopStyleColor();

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

void FileTree::displayFileNode(const FileNode &node,
							   const TreeDisplayMetrics &metrics,
							   int depth)
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
	bool clicked = ImGui::Button(("##" + node.fullPath).c_str(),
								 ImVec2(buttonWidth, metrics.itemHeight));

	ImGui::PopStyleVar();
	ImGui::PopStyleColor(2);

	// Position icon and text as before...
	float lineCenterY = metrics.cursorPos.y + (metrics.itemHeight / 2.0f);
	float iconTopY = lineCenterY - (iconDimensions.y / 2.0f);
	float textHeight = ImGui::GetTextLineHeight();
	float textTopY = lineCenterY - (textHeight / 2.0f);

	// Position icon and text on the same line with proper alignment
	ImGui::SetCursorPosX(metrics.cursorPos.x + depth * metrics.indentWidth +
						 TreeStyleSettings::LEFT_MARGIN);
	ImGui::SetCursorPosY(iconTopY);
	ImGui::Image(fileIcon, iconDimensions);

	ImGui::SameLine(depth * metrics.indentWidth + iconDimensions.x +
					TreeStyleSettings::LEFT_MARGIN + 10);
	ImGui::SetCursorPosY(textTopY);
	renderNodeText(node.name, node.fullPath, node.fullPath == gFileExplorer.currentFile);

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

		// Rebuild root node
		rootNode.name = fs::path(gFileExplorer.selectedFolder).filename().string();
		rootNode.fullPath = gFileExplorer.selectedFolder;
		rootNode.isDirectory = true;
		rootNode.isOpen = true;

		buildFileTree(gFileExplorer.selectedFolder, rootNode);
		preserveOpenStates(oldRoot, rootNode);

		// Auto-open README on first refresh
		if (shouldCheckForReadme && initialRefreshDone)
		{
			std::string readmePath = findReadmeInRoot();
			if (!readmePath.empty() && gFileExplorer.currentFile.empty())
			{
				gFileExplorer.loadFileContent(readmePath);
				hasAutoOpenedReadme = true;
			}
			shouldCheckForReadme = false;
		}
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
	// Helper function to check if file should be skipped
	auto shouldSkipFile = [](const std::string &filename) {
		return filename == ".DS_Store" || filename == "thumbs.db";
	};

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
			// Skip hidden system files and specific unwanted files
			std::string filename = entry.path().filename().string();
			if (shouldSkipFile(filename))
			{
				continue;
			}

			FileNode child;
			child.name = filename;
			child.fullPath = entry.path().string();
			child.isDirectory = entry.is_directory();

			// Find existing child to preserve its state
			auto existingChild =
				std::find_if(node.children.begin(),
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
		std::cerr << "Error accessing directory " << path << ": " << e.what()
				  << std::endl;
	}

	// Sort directories first, then files by name (existing code)
	std::sort(newChildren.begin(),
			  newChildren.end(),
			  [](const FileNode &a, const FileNode &b) {
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
	metrics.indentWidth = 18.0f;
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
