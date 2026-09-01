#include "file_finder_view.h"
#include "../../../editor/views/imgui/ned_key_imgui.h"
#include "../../../editor/editor_api.h"
#include "../../../files/file_finder.h"
#include "../../../util/icons.h"
#include "../../../util/settings.h"
#include "imgui.h"

#include <algorithm>

#include "../../../editor/views/view_layout.h"
#include "../../../files/files.h"
#include "../../../util/keybinds.h"

#include <cctype>
#include <filesystem>

namespace fs = std::filesystem;

namespace {

std::string toLower(std::string s)
{
	std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) {
		return static_cast<char>(std::tolower(c));
	});
	return s;
}

} // namespace

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
	const ImGuiKey toggleKey = imguiKeyFromNed(f.settings->keybinds.getActionKey("toggle_file_finder"));

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
