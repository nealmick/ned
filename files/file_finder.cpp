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
		// Same skip list as the Qt finder (FileFinderMatch::shouldSkipDir):
		// .git / build dirs / node_modules otherwise dominate every scan.
		std::error_code ec;
		for (fs::recursive_directory_iterator
				 it(projectDir, fs::directory_options::skip_permission_denied, ec),
			 end;
			 !ec && it != end;
			 it.increment(ec))
		{
			try
			{
				if (it->is_directory(ec))
				{
					if (FileFinderMatch::shouldSkipDir(it->path().filename().string()))
						it.disable_recursion_pending();
					continue;
				}
				if (ec || !it->is_regular_file(ec))
					continue;

				const fs::path fullPath = it->path();
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
