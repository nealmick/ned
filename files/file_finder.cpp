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
