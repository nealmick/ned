/*
	files/file_finder.cpp
	Project file finder (Ctrl+P).
*/
#include "file_finder.h"
#include "../editor/editor_api.h"
#include "../editor/editor_events.h"
#include "../editor/views/view_layout.h"
#include "../files/files.h"
#include "../util/keybinds.h"
#include "../util/settings.h"
#include <algorithm>
#include <cctype>
#include <cstring>

namespace {

std::string toLower(std::string s)
{
	std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) {
		return static_cast<char>(std::tolower(c));
	});
	return s;
}

// Path → UTF-8 std::string (Windows-safe).
std::string pathToUtf8(const fs::path &p)
{
#ifdef PLATFORM_WINDOWS
	auto u8 = p.u8string();
	return std::string(u8.begin(), u8.end());
#else
	return p.string();
#endif
}

} // namespace

// --- lifecycle --------------------------------------------------------------

FileFinder::FileFinder() = default;

void FileFinder::startBackgroundThread()
{
	if (workerStarted)
		return;
	workerStarted = true;
	workerThread = std::thread(&FileFinder::backgroundRefresh, this);
}

FileFinder::~FileFinder()
{
	stopThread = true;
	if (workerThread.joinable())
		workerThread.join();
}

// --- background scan --------------------------------------------------------

void FileFinder::backgroundRefresh()
{
	using namespace std::chrono;
	auto lastScanTime = steady_clock::now();

	while (!stopThread)
	{
		auto now = steady_clock::now();
		const std::string projectDir = fileExplorer->projectRoot;

		if (!projectDir.empty())
		{
			const bool directoryChanged = (projectDir != currentProjectDir);
			const bool timeForScan =
				duration_cast<seconds>(now - lastScanTime).count() >= SCAN_INTERVAL_SEC;

			if (directoryChanged || timeForScan)
			{
				currentProjectDir = projectDir;
				refreshFileListBackground(projectDir);
				lastScanTime = now;
			}
		}

		std::this_thread::sleep_for(milliseconds(100));
	}
}

void FileFinder::refreshFileListBackground(const std::string &projectDir)
{
	std::vector<FileEntry> newList;
	try
	{
		for (const auto &entry : fs::recursive_directory_iterator(projectDir))
		{
			try
			{
				if (!entry.is_regular_file())
					continue;

				const fs::path fullPath = entry.path();
				const fs::path relativePath = fs::relative(fullPath, projectDir);

				FileEntry fe;
				fe.fullPath = pathToUtf8(fullPath);
				fe.relativePath = pathToUtf8(relativePath);
				fe.relativePathLower = toLower(fe.relativePath);
				fe.filenameLower = toLower(pathToUtf8(relativePath.filename()));
				newList.push_back(std::move(fe));
			} catch (const std::exception &)
			{
				// Skip entries that fail path conversion / access.
				continue;
			}
		}

		std::lock_guard<std::mutex> lock(fileListMutex);
		fileList = std::move(newList);
	} catch (const std::exception &)
	{
		// Directory gone / permission — leave previous list.
	}
}

// --- filtering --------------------------------------------------------------

void FileFinder::updateFilteredList()
{
	const std::string searchTerm = toLower(searchBuffer);

	if (searchTerm != previousSearch)
	{
		selectedIndex = 0;
		previousSearch = searchTerm;
	}

	std::vector<FileEntry> snapshot;
	{
		std::lock_guard<std::mutex> lock(fileListMutex);
		snapshot = fileList;
	}

	filteredList.clear();
	for (const auto &file : snapshot)
	{
		if (file.relativePathLower.find(searchTerm) == std::string::npos)
			continue;

		// Hide dotfiles unless the query itself contains a '.'
		if (searchTerm.find('.') == std::string::npos && !file.filenameLower.empty() &&
			file.filenameLower[0] == '.')
			continue;

		filteredList.push_back(file);
	}

	// Prefer shorter paths (usually better matches) first.
	std::sort(filteredList.begin(),
			  filteredList.end(),
			  [](const FileEntry &a, const FileEntry &b) {
				  return a.relativePath.size() < b.relativePath.size();
			  });
}

// --- selection --------------------------------------------------------------

void FileFinder::commitSelection()
{
	showFFWindow = false;
	if (!fileExplorer)
		return;

	fileExplorer->setEditorsBlockInput(false);

	// Open only on confirm — no live preview while arrowing/searching.
	if (!filteredList.empty() && selectedIndex >= 0 &&
		selectedIndex < static_cast<int>(filteredList.size()))
	{
		fileExplorer->loadFileContent(
			filteredList[static_cast<size_t>(selectedIndex)].fullPath);
		if (fileExplorer->api)
			fileExplorer->api->requestFocus();
	}
}

void FileFinder::cancelAndClose()
{
	showFFWindow = false;
	if (fileExplorer)
		fileExplorer->setEditorsBlockInput(false);
}

// --- window open/close ------------------------------------------------------

void FileFinder::toggleWindow()
{
	showFFWindow = !showFFWindow;
	if (fileExplorer && fileExplorer->api)
		fileExplorer->api->requestExclusiveOverlay(
			EditorEvents::DidRequestExclusiveOverlay::Keep::FileFinder);

	if (fileExplorer)
		fileExplorer->setEditorsBlockInput(showFFWindow);

	if (!showFFWindow)
		return;

	std::memset(searchBuffer, 0, sizeof(searchBuffer));
	previousSearch.clear();
	selectedIndex = 0;
	updateFilteredList();
}

ImVec4 FileFinder::dimmedBackground() const
{
	return ImVec4(settings->settings["backgroundColor"][0].get<float>() * 0.8f,
				  settings->settings["backgroundColor"][1].get<float>() * 0.8f,
				  settings->settings["backgroundColor"][2].get<float>() * 0.8f,
				  1.0f);
}

// --- UI pieces --------------------------------------------------------------

void FileFinder::renderHeader()
{
	const float fs = ImGui::GetFontSize();
	const ImVec2 windowSize(fs * 30.0f, fs * 17.5f);
	ImVec2 windowPos;

	// Embedded: center on editor pane (ViewLayout metrics — no duplicated rect).
	const ViewLayout *layout =
		(fileExplorer && fileExplorer->api) ? &fileExplorer->api->layout() : nullptr;
	if (settings && settings->isEmbedded && layout &&
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
	ImGui::PushStyleColor(ImGuiCol_WindowBg, dimmedBackground());
	ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.3f, 0.3f, 0.3f, 1.0f));
	ImGui::PushStyleColor(ImGuiCol_FrameBg, dimmedBackground());

	ImGui::Begin("FileFinder", nullptr, flags);
	ImGui::TextUnformatted("Find File");
	ImGui::Spacing();
	ImGui::Spacing();
}

bool FileFinder::renderSearchInput()
{
	const float fs = ImGui::GetFontSize();
	ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x);
	ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, fs * 0.2f);
	ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(fs * 0.4f, fs * 0.4f));
	ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.3f, 0.3f, 0.3f, 1.0f));
	ImGui::PushStyleColor(ImGuiCol_FrameBg, dimmedBackground());

	ImGui::SetKeyboardFocusHere();
	const bool enterPressed = ImGui::InputText(
		"##SearchInput",
		searchBuffer,
		INPUT_CAP,
		ImGuiInputTextFlags_AutoSelectAll | ImGuiInputTextFlags_EnterReturnsTrue);

	ImGui::PopStyleColor(2);
	ImGui::PopStyleVar(3);
	ImGui::PopItemWidth();
	return enterPressed;
}

void FileFinder::renderFileList()
{
	const float itemHeight = ImGui::GetTextLineHeightWithSpacing();
	const float availableHeight = ImGui::GetContentRegionAvail().y;
	const int visibleCount = std::max(1, static_cast<int>(availableHeight / itemHeight));
	const int totalItems = static_cast<int>(filteredList.size());

	int startIdx = std::max(0, selectedIndex - visibleCount / 2);
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
		const bool selected = (i == selectedIndex);
		const FileEntry &entry = filteredList[i];

		ImGui::PushID(i);
		ImGui::Selectable("", selected, ImGuiSelectableFlags_SpanAllColumns);
		ImGui::SameLine();

		const std::string filename = fs::path(entry.fullPath).filename().string();
		const ImTextureID icon = fileExplorer->icons.getForFile(filename);
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

void FileFinder::renderWindow()
{
	const bool ctrl = ImGui::GetIO().KeyCtrl;
	const ImGuiKey toggleKey = settings->keybinds.getActionKey("toggle_file_finder");

	if (ctrl && ImGui::IsKeyPressed(toggleKey, false))
	{
		toggleWindow();
		return;
	}

	if (showFFWindow && ImGui::IsKeyPressed(ImGuiKey_Escape))
	{
		cancelAndClose();
		return;
	}

	if (!showFFWindow)
		return;

	// Own keyboard while open (document input checks blockInput only).
	if (fileExplorer)
		fileExplorer->setEditorsBlockInput(true);

	renderHeader(); // Begin + styles (popped below)

	// Arrow navigation (list highlight only — open on Enter).
	if (ImGui::IsKeyPressed(ImGuiKey_UpArrow) && selectedIndex > 0)
		--selectedIndex;
	if (ImGui::IsKeyPressed(ImGuiKey_DownArrow) &&
		selectedIndex < static_cast<int>(filteredList.size()) - 1)
		++selectedIndex;

	// Click outside: dismiss without opening.
	if (ImGui::IsMouseClicked(ImGuiMouseButton_Left))
	{
		const ImVec2 wpos = ImGui::GetWindowPos();
		const ImVec2 wsize = ImGui::GetWindowSize();
		const ImVec2 mouse = ImGui::GetIO().MousePos;
		if (mouse.x < wpos.x || mouse.x > wpos.x + wsize.x || mouse.y < wpos.y ||
			mouse.y > wpos.y + wsize.y)
		{
			cancelAndClose();
			ImGui::End();
			ImGui::PopStyleColor(3);
			ImGui::PopStyleVar(3);
			return;
		}
	}

	if (renderSearchInput())
	{
		commitSelection();
		ImGui::End();
		ImGui::PopStyleColor(3);
		ImGui::PopStyleVar(3);
		return;
	}

	// Rebuild results when the query changes.
	const std::string term = toLower(searchBuffer);
	if (term != previousSearch)
		updateFilteredList();

	ImGui::Spacing();
	ImGui::Spacing();
	ImGui::Dummy(ImVec2(0, ImGui::GetFontSize() * 0.5f));

	renderFileList();

	ImGui::Separator();
	ImGui::Text("Press Ctrl+P or ESC to close");
	ImGui::End();
	ImGui::PopStyleColor(3);
	ImGui::PopStyleVar(3);
}
