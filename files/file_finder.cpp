/*
	files/file_finder.cpp
	Project file finder (Ctrl+P).
*/
#include "file_finder.h"
#include "../editor/editor_api.h"
#include "../editor/editor_events.h"
#include "../files/files.h"
#include "../util/keybinds.h"
#include "../util/settings.h"
#include "file_finder_match.h"
#include <algorithm>
#include <cctype>
#include <cstring>
#include <system_error>

namespace {

std::string toLower(std::string s)
{
	std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) {
		return static_cast<char>(std::tolower(c));
	});
	return s;
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
	std::vector<FileEntry> newList = scanWorkspaceFiles(projectDir, stopThread);

	std::lock_guard<std::mutex> lock(fileListMutex);
	fileList = std::move(newList);
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

	// Shared matcher (files/file_finder_match.h): fuzzy subsequence score
	// (a superset of the old substring rule), dotfile hiding, shorter-path
	// tie-breaks — identical results to the Qt finder.
	filteredList = FileFinderMatch::filterFiles(snapshot, searchTerm, SIZE_MAX);
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
