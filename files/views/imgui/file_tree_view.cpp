#include "file_tree_view.h"
#include "../../../editor/editor_api.h"
#include "../../../files/file_tree.h"
#include "../../../util/icons.h"
#include "../../../util/settings.h"
#include "imgui.h"

#include <algorithm>

#include "../../../editor/util/editor_utils.h"
#include "../../../files/files.h"

// Row styling constants (were FileTree privates).
static constexpr float ICON_SCALE = 1.1f;
static constexpr ImVec4 TRANSPARENT_COLOR{0, 0, 0, 0};
static constexpr ImVec4 HOVER_COLOR{0.06f, 0.35f, 0.60f, 0.85f};
// Hard dim so dirty names are obvious on both light and dark themes.
static constexpr ImVec4 MODIFIED_TEXT{0.45f, 0.45f, 0.45f, 1.0f};

static void drawNodeRow(FileTree &tree, FileNode &node, int depth);
static void drawNodeLabel(FileTree &tree,
						  const std::string &name,
						  const std::string &fullPath,
						  bool isCurrentFile);
static ImVec4 themeTextColor(FileTree &tree);
static ImTextureID folderIcon(FileTree &tree, bool isOpen);

void renderFileTree(FileTree &tree, FileNode &node, int depth)
{
	const float fs = ImGui::GetFontSize();
	ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, fs * 0.3f);
	ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(fs * 0.2f, fs * 0.1f));
	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(fs * 0.05f, fs * 0.15f));
	ImGui::PushID(node.fullPath.c_str());

	drawNodeRow(tree, node, depth);

	ImGui::PopID();
	ImGui::PopStyleVar(3);
}

static void drawNodeRow(FileTree &tree, FileNode &node, int depth)
{
	const float fontSize = ImGui::GetFontSize();
	const float baseIcon = node.isDirectory ? fontSize * 0.8f : fontSize * 1.2f;
	const float iconSize = baseIcon * ICON_SCALE;
	const float itemHeight = std::max(ImGui::GetFrameHeight(), iconSize + 2.0f);
	const ImVec2 rowOrigin = ImGui::GetCursorPos();
	const float indent = depth * fontSize * 0.9f;
	const float rowPadX = fontSize * 0.2f;
	const float iconTextGap = fontSize * 0.35f;

	const ImTextureID icon = node.isDirectory
								 ? folderIcon(tree, node.isOpen)
								 : tree.fileExplorer->icons.getForFile(node.name);

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
		ImGui::PushStyleColor(ImGuiCol_Text, themeTextColor(tree));
		ImGui::Text("%s", node.name.c_str());
		ImGui::PopStyleColor();

		if (clicked)
		{
			node.isOpen = !node.isOpen;
			if (node.isOpen)
				tree.buildFileTree(node.fullPath, node);
		}

		if (node.isOpen)
		{
			for (auto &child : node.children)
				renderFileTree(tree, child, depth + 1);
		}
	} else
	{
		const bool isCurrent =
			tree.fileExplorer->api && node.fullPath == tree.fileExplorer->api->path();
		drawNodeLabel(tree, node.name, node.fullPath, isCurrent);

		if (clicked)
		{
			tree.fileExplorer->loadFileContent(node.fullPath);
			// Tree button holds ImGui focus; pull keyboard into the document.
			if (tree.fileExplorer->api)
				tree.fileExplorer->api->requestFocus();
		}
	}
}

static void drawNodeLabel(FileTree &tree,
						  const std::string &name,
						  const std::string &fullPath,
						  bool isCurrentFile)
{
	ImVec4 color = themeTextColor(tree);

	if (isCurrentFile)
	{
		if (tree.settings->settings.value("rainbow", true))
			color = EditorUtils::GetRainbowColor();
	} else if (tree.fileExplorer && tree.fileExplorer->api &&
			   tree.fileExplorer->api->isFileModified(fullPath))
	{
		color = MODIFIED_TEXT;
	}

	ImGui::PushStyleColor(ImGuiCol_Text, color);
	ImGui::Text("%s", name.c_str());
	ImGui::PopStyleColor();
}

static ImVec4 themeTextColor(FileTree &tree)
{
	const auto &theme = tree.settings->settings["themes"][tree.settings->settings.value(
		"theme", std::string("default"))];
	const auto &t = theme["text"];
	return ImVec4(t[0], t[1], t[2], t[3]);
}

static ImTextureID folderIcon(FileTree &tree, bool isOpen)
{
	Icons &icons = tree.fileExplorer->icons;
	ImTextureID icon = isOpen ? icons.get("folder-open") : icons.get("folder");
	if (!icon)
		icon = icons.get("folder");
	return icon ? icon : icons.get("default");
}
