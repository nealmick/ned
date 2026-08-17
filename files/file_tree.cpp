#include "file_tree.h"
#include "../editor/editor_api.h"
#include "../editor/util/editor_utils.h"
#include "../files/files.h"
#include "../util/icons.h"
#include "../util/settings.h"
#include <algorithm>
#include <cctype>
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

	if (shouldAutoOpenReadme)
	{
		if (fileExplorer->api && !fileExplorer->api->hasPath())
		{
			const std::string readme = findReadmeInRoot();
			if (!readme.empty())
				fileExplorer->loadFileContent(readme);
		}
		shouldAutoOpenReadme = false;
	}
}

std::string FileTree::findReadmeInRoot() const
{
	if (!rootNode.isDirectory)
		return {};

	for (const auto &child : rootNode.children)
	{
		if (child.isDirectory)
			continue;

		std::string lower = child.name;
		std::transform(lower.begin(), lower.end(), lower.begin(), [](unsigned char c) {
			return static_cast<char>(std::tolower(c));
		});

		if (lower == "readme.md" || lower == "readme")
			return child.fullPath;
	}
	return {};
}

// ---------------------------------------------------------------------------
// Display
// ---------------------------------------------------------------------------

void FileTree::displayFileTree(FileNode &node, int depth)
{
	const float fs = ImGui::GetFontSize();
	ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, fs * 0.3f);
	ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(fs * 0.2f, fs * 0.1f));
	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(fs * 0.05f, fs * 0.15f));
	ImGui::PushID(node.fullPath.c_str());

	drawNodeRow(node, depth);

	ImGui::PopID();
	ImGui::PopStyleVar(3);
}

void FileTree::drawNodeRow(FileNode &node, int depth)
{
	const float fontSize = ImGui::GetFontSize();
	const float baseIcon = node.isDirectory ? fontSize * 0.8f : fontSize * 1.2f;
	const float iconSize = baseIcon * ICON_SCALE;
	const float itemHeight = std::max(ImGui::GetFrameHeight(), iconSize + 2.0f);
	const ImVec2 rowOrigin = ImGui::GetCursorPos();
	const float indent = depth * fontSize * 0.9f;
	const float rowPadX = fontSize * 0.2f;
	const float iconTextGap = fontSize * 0.35f;

	const ImTextureID icon = node.isDirectory ? folderIcon(node.isOpen)
											  : fileExplorer->icons.getForFile(node.name);

	const ImVec2 textSize = ImGui::CalcTextSize(node.name.c_str());
	const float requiredWidth = indent + rowPadX + iconSize + iconTextGap + textSize.x;
	const float buttonWidth = std::max(requiredWidth, ImGui::GetContentRegionAvail().x);

	// Full-width invisible hit target for the row.
	ImGui::PushStyleColor(ImGuiCol_Button, TRANSPARENT_COLOR);
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, HOVER_COLOR);
	ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));
	const bool clicked =
		ImGui::Button(("##" + node.fullPath).c_str(), ImVec2(buttonWidth, itemHeight));
	ImGui::PopStyleVar();
	ImGui::PopStyleColor(2);

	// Icon + label centered vertically on the row (drawn over the button).
	const float centerY = rowOrigin.y + itemHeight * 0.5f;
	const float iconX = rowOrigin.x + indent + rowPadX;
	const float textX = iconX + iconSize + iconTextGap;

	ImGui::SetCursorPos(ImVec2(iconX, centerY - iconSize * 0.5f));
	ImGui::Image(icon, ImVec2(iconSize, iconSize));

	ImGui::SetCursorPos(ImVec2(textX, centerY - ImGui::GetTextLineHeight() * 0.5f));

	if (node.isDirectory)
	{
		ImGui::PushStyleColor(ImGuiCol_Text, themeTextColor());
		ImGui::Text("%s", node.name.c_str());
		ImGui::PopStyleColor();

		if (clicked)
		{
			node.isOpen = !node.isOpen;
			if (node.isOpen)
				buildFileTree(node.fullPath, node);
		}

		if (node.isOpen)
		{
			for (auto &child : node.children)
				displayFileTree(child, depth + 1);
		}
	} else
	{
		const bool isCurrent =
			fileExplorer->api && node.fullPath == fileExplorer->api->path();
		drawNodeLabel(node.name, node.fullPath, isCurrent);

		if (clicked)
		{
			fileExplorer->loadFileContent(node.fullPath);
			// Tree button holds ImGui focus; pull keyboard into the document.
			if (fileExplorer->api)
				fileExplorer->api->requestFocus();
		}
	}
}

void FileTree::drawNodeLabel(const std::string &name,
							 const std::string &fullPath,
							 bool isCurrentFile)
{
	ImVec4 color = themeTextColor();

	if (isCurrentFile)
	{
		if (settings->settings.value("rainbow", true))
			color = EditorUtils::GetRainbowColor();
	} else if (fileExplorer && fileExplorer->api &&
			   fileExplorer->api->isFileModified(fullPath))
	{
		color = MODIFIED_TEXT;
	}

	ImGui::PushStyleColor(ImGuiCol_Text, color);
	ImGui::Text("%s", name.c_str());
	ImGui::PopStyleColor();
}

ImVec4 FileTree::themeTextColor() const
{
	const auto &theme = settings->settings["themes"][settings->settings.value(
		"theme", std::string("default"))];
	const auto &t = theme["text"];
	return ImVec4(t[0], t[1], t[2], t[3]);
}

ImTextureID FileTree::folderIcon(bool isOpen) const
{
	Icons &icons = fileExplorer->icons;
	ImTextureID icon = isOpen ? icons.get("folder-open") : icons.get("folder");
	if (!icon)
		icon = icons.get("folder");
	return icon ? icon : icons.get("default");
}
