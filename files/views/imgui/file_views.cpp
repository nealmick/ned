/*
	File: views/imgui/file_views.cpp
	Description: ImGui rendering for the files module — sidebar (file tree),
	project file finder popup. All widget code lives here; files/ keeps the
	scan/filter/open logic.
*/

#include "file_views.h"
#include "../../../files/file_finder.h"
#include "../../../files/file_tree.h"
#include "../../../files/files.h"
#include "../../../util/icons.h"
#include "../../../util/settings.h"
#include "../../../editor/editor_api.h"
#include "../../../editor/util/editor_utils.h"
#include "imgui.h"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <string>

namespace fs = std::filesystem;

namespace {

std::string toLower(std::string s)
{
	std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) {
		return static_cast<char>(std::tolower(c));
	});
	return s;
}

// Row styling constants (were FileTree privates).
constexpr float ICON_SCALE = 1.1f;
constexpr ImVec4 TRANSPARENT_COLOR{0, 0, 0, 0};
constexpr ImVec4 HOVER_COLOR{0.06f, 0.35f, 0.60f, 0.85f};
// Hard dim so dirty names are obvious on both light and dark themes.
constexpr ImVec4 MODIFIED_TEXT{0.45f, 0.45f, 0.45f, 1.0f};

} // namespace

// ---- tree row helpers (below) ----
static void drawNodeRow(FileTree &tree, FileNode &node, int depth);
static void drawNodeLabel(FileTree &tree,
						  const std::string &name,
						  const std::string &fullPath,
						  bool isCurrentFile);
static ImVec4 themeTextColor(FileTree &tree);
static ImTextureID folderIcon(FileTree &tree, bool isOpen);

void renderFileSidebar(FileExplorer &fx, float explorerWidth)
{
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 0.0f));
	ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 0.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_ChildBorderSize, 0.0f);

	const float treeW = explorerWidth <= 0.0f ? 0.0f : explorerWidth;
	// Standalone macOS / Windows: explorer/terminal/fx.settings live in the
	// window title bar, so the explorer doesn't grow a second toolbar.
	const bool nativeTitlebar =
#if defined(__APPLE__) || defined(_WIN32)
		fx.settings && !fx.settings->isEmbedded;
#else
		false;
#endif
	const float icon = ImGui::GetFontSize() * 1.05f;
	const float pad = ImGui::GetFontSize() * 0.5f;
	const float barH = nativeTitlebar ? 0.0f : icon + ImGui::GetFontSize() * 0.8f;
	ImGui::BeginChild("File Tree", ImVec2(treeW, barH > 0.0f ? -barH : 0.0f));
	if (!fx.projectRoot.empty())
		renderFileTree(fx.fileTree, fx.fileTree.rootNode);
	ImGui::EndChild();

	if (barH > 0.0f)
	{
		const ImVec4 bg = ImGui::GetStyleColorVec4(ImGuiCol_ChildBg);
		const ImVec4 tx = ImGui::GetStyleColorVec4(ImGuiCol_Text);
		ImGui::PushStyleColor(ImGuiCol_ChildBg,
							  ImVec4(bg.x + (tx.x - bg.x) * 0.1f,
									 bg.y + (tx.y - bg.y) * 0.1f,
									 bg.z + (tx.z - bg.z) * 0.1f,
									 bg.w));
		ImGui::BeginChild(
			"##explorer_activity", ImVec2(0.0f, barH), 0, ImGuiWindowFlags_NoScrollbar);
		ImGui::SetCursorPos(ImVec2(pad, (barH - icon) * 0.5f));
		const ImVec2 sz(icon, icon);
		auto btn = [&](const char *id, const char *off, const char *on, const char *tip) {
			const ImVec2 p = ImGui::GetCursorPos();
			const bool hit = ImGui::InvisibleButton(id, sz);
			const bool hov = ImGui::IsItemHovered();
			if (hov)
				ImGui::SetTooltip("%s", tip);
			ImGui::SetCursorPos(p);
			ImGui::Image(fx.icons.get(hov ? on : off), sz);
			return hit;
		};
		if (btn("##settings", "gear", "gear-hover", "Settings") && fx.api && fx.settings)
			fx.settings->toggleSettingsWindow(*fx.api);
		ImGui::SameLine(0.0f, pad);
		if (btn("##terminal", "terminal", "terminal-hover", "Terminal") && fx.settings)
			fx.settings->toggleTerminal();
		ImGui::EndChild();
		ImGui::PopStyleColor();
	}
	ImGui::PopStyleVar(4);
}

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

static ImVec4 fileFinderDimmedBackground(FileFinder &f)
{
	return ImVec4(f.settings->settings["backgroundColor"][0].get<float>() * 0.8f,
				  f.settings->settings["backgroundColor"][1].get<float>() * 0.8f,
				  f.settings->settings["backgroundColor"][2].get<float>() * 0.8f,
				  1.0f);
}

// --- UI pieces --------------------------------------------------------------

void renderFileFinderHeader(FileFinder &f)
{
	const float fs = ImGui::GetFontSize();
	const ImVec2 windowSize(fs * 30.0f, fs * 17.5f);
	ImVec2 windowPos;

	// Embedded: center on editor pane (ViewLayout metrics — no duplicated rect).
	const ViewLayout *layout = (f.fileExplorer && f.fileExplorer->api)
								   ? &f.fileExplorer->api->layout()
								   : nullptr;
	if (f.settings && f.settings->isEmbedded && layout &&
		(layout->paneSize.x > 0.0f || layout->paneSize.y > 0.0f))
	{
		windowPos =
			ImVec2(layout->panePos.x + layout->paneSize.x * 0.5f - windowSize.x * 0.5f,
				   layout->panePos.y + layout->paneSize.y * 0.5f - windowSize.y * 0.5f);
		ImGui::SetNextWindowSize(windowSize, ImGuiCond_Always);
		ImGui::SetNextWindowPos(windowPos, ImGuiCond_Always);
	} else
	{
		// Standalone (or pane not ready yet): center on display.
		windowPos = ImVec2(ImGui::GetIO().DisplaySize.x * 0.5f,
						   ImGui::GetIO().DisplaySize.y * 0.35f);
		ImGui::SetNextWindowSize(windowSize, ImGuiCond_Always);
		ImGui::SetNextWindowPos(windowPos, ImGuiCond_Always, ImVec2(0.5f, 0.5f));
	}

	const ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar |
								   ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
								   ImGuiWindowFlags_NoScrollbar |
								   ImGuiWindowFlags_NoScrollWithMouse;

	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, fs * 0.5f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(fs * 0.8f, fs * 0.8f));
	ImGui::PushStyleColor(ImGuiCol_WindowBg, fileFinderDimmedBackground(f));
	ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.3f, 0.3f, 0.3f, 1.0f));
	ImGui::PushStyleColor(ImGuiCol_FrameBg, fileFinderDimmedBackground(f));

	ImGui::Begin("FileFinder", nullptr, flags);
	ImGui::TextUnformatted("Find File");
	ImGui::Spacing();
	ImGui::Spacing();
}

bool renderFileFinderSearchInput(FileFinder &f)
{
	const float fs = ImGui::GetFontSize();
	ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x);
	ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, fs * 0.2f);
	ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(fs * 0.4f, fs * 0.4f));
	ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.3f, 0.3f, 0.3f, 1.0f));
	ImGui::PushStyleColor(ImGuiCol_FrameBg, fileFinderDimmedBackground(f));

	ImGui::SetKeyboardFocusHere();
	const bool enterPressed = ImGui::InputText(
		"##SearchInput",
		f.searchBuffer,
		f.INPUT_CAP,
		ImGuiInputTextFlags_AutoSelectAll | ImGuiInputTextFlags_EnterReturnsTrue);

	ImGui::PopStyleColor(2);
	ImGui::PopStyleVar(3);
	ImGui::PopItemWidth();
	return enterPressed;
}

void renderFileFinderList(FileFinder &f)
{
	const float itemHeight = ImGui::GetTextLineHeightWithSpacing();
	const float availableHeight = ImGui::GetContentRegionAvail().y;
	const int visibleCount = std::max(1, static_cast<int>(availableHeight / itemHeight));
	const int totalItems = static_cast<int>(f.filteredList.size());

	int startIdx = std::max(0, f.selectedIndex - visibleCount / 2);
	int endIdx = std::min(totalItems, startIdx + visibleCount);
	if (endIdx == totalItems)
		startIdx = std::max(0, totalItems - visibleCount);

	ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0, 0, 0, 0));
	ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0, 0, 0, 0));
	ImGui::BeginChild("SearchResults",
					  ImVec2(0, -ImGui::GetFrameHeightWithSpacing()),
					  false,
					  ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse |
						  ImGuiWindowFlags_NoMouseInputs);

	const float fs = ImGui::GetFontSize();
	ImGui::PushStyleVar(ImGuiStyleVar_SelectableTextAlign, ImVec2(0.0f, 0.5f));
	ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, fs * 0.2f);
	ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(fs * 0.4f, fs * 0.2f));
	ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(1.0f, 0.1f, 0.7f, 0.4f));
	ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0, 0, 0, 0));
	ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(0, 0, 0, 0));

	for (int i = startIdx; i < endIdx; ++i)
	{
		const bool selected = (i == f.selectedIndex);
		const FileEntry &entry = f.filteredList[i];

		ImGui::PushID(i);
		ImGui::Selectable("", selected, ImGuiSelectableFlags_SpanAllColumns);
		ImGui::SameLine();

		const std::string filename = fs::path(entry.fullPath).filename().string();
		const ImTextureID icon = f.fileExplorer->icons.getForFile(filename);
		const float iconSize = ImGui::GetTextLineHeight();
		ImGui::Image(icon, ImVec2(iconSize, iconSize));
		ImGui::SameLine();
		ImGui::TextUnformatted(entry.relativePath.c_str());

		if (selected)
			ImGui::SetScrollHereY(0.5f);
		ImGui::PopID();
	}

	ImGui::PopStyleColor(3);
	ImGui::PopStyleVar(3);
	ImGui::PopStyleColor(2);
	ImGui::EndChild();
}

// --- main frame -------------------------------------------------------------

void renderFileFinder(FileFinder &f)
{
	const bool ctrl = ImGui::GetIO().KeyCtrl;
	const ImGuiKey toggleKey = f.settings->keybinds.getActionKey("toggle_file_finder");

	if (ctrl && ImGui::IsKeyPressed(toggleKey, false))
	{
		f.toggleWindow();
		return;
	}

	if (f.showFFWindow && ImGui::IsKeyPressed(ImGuiKey_Escape))
	{
		f.cancelAndClose();
		return;
	}

	if (!f.showFFWindow)
		return;

	// Own keyboard while open (document input checks blockInput only).
	if (f.fileExplorer)
		f.fileExplorer->setEditorsBlockInput(true);

	renderFileFinderHeader(f); // Begin + styles (popped below)

	// Arrow navigation (list highlight only — open on Enter).
	if (ImGui::IsKeyPressed(ImGuiKey_UpArrow) && f.selectedIndex > 0)
		--f.selectedIndex;
	if (ImGui::IsKeyPressed(ImGuiKey_DownArrow) &&
		f.selectedIndex < static_cast<int>(f.filteredList.size()) - 1)
		++f.selectedIndex;

	// Click outside: dismiss without opening.
	if (ImGui::IsMouseClicked(ImGuiMouseButton_Left))
	{
		const ImVec2 wpos = ImGui::GetWindowPos();
		const ImVec2 wsize = ImGui::GetWindowSize();
		const ImVec2 mouse = ImGui::GetIO().MousePos;
		if (mouse.x < wpos.x || mouse.x > wpos.x + wsize.x || mouse.y < wpos.y ||
			mouse.y > wpos.y + wsize.y)
		{
			f.cancelAndClose();
			ImGui::End();
			ImGui::PopStyleColor(3);
			ImGui::PopStyleVar(3);
			return;
		}
	}

	if (renderFileFinderSearchInput(f))
	{
		f.commitSelection();
		ImGui::End();
		ImGui::PopStyleColor(3);
		ImGui::PopStyleVar(3);
		return;
	}

	// Rebuild results when the query changes.
	const std::string term = toLower(f.searchBuffer);
	if (term != f.previousSearch)
		f.updateFilteredList();

	ImGui::Spacing();
	ImGui::Spacing();
	ImGui::Dummy(ImVec2(0, ImGui::GetFontSize() * 0.5f));

	renderFileFinderList(f);

	ImGui::Separator();
	ImGui::Text("Press Ctrl+P or ESC to close");
	ImGui::End();
	ImGui::PopStyleColor(3);
	ImGui::PopStyleVar(3);
}
