#include "editor_git.h"
#include "../files/files.h"
#include <array>
#include <atomic>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <memory>
#include <sstream>
#include <thread>
#ifndef PLATFORM_WINDOWS
#include <unistd.h>
#endif
#include <vector>

namespace fs = std::filesystem;

bool EditorGit::isGitInitialized()
{
	if (gFileExplorer.selectedFolder.empty())
	{
		return false;
	}

	fs::path gitDir = fs::path(gFileExplorer.selectedFolder) / ".git";
	return fs::exists(gitDir) && fs::is_directory(gitDir);
}

void EditorGit::printGitEditedLines()
{
	for (const auto &pair : editedLines)
	{
		std::cout << "file: " << pair.first << std::endl;
		for (int line : pair.second)
		{
			std::cout << line << std::endl;
		}
	}
}
void EditorGit::gitEditedLines()
{
	if (gFileExplorer.selectedFolder.empty())
	{
		return;
	}

	auto start_git_edited = std::chrono::high_resolution_clock::now();
	std::cout << "[GIT TIMING] gitEditedLines() called" << std::endl;

	// Use libgit2 for lock-free operation
	std::map<std::string, std::vector<int>> fileChanges = gitWrapper.getEditedLines();

	auto end_git_edited = std::chrono::high_resolution_clock::now();
	auto duration_git_edited = std::chrono::duration_cast<std::chrono::microseconds>(
		end_git_edited - start_git_edited);
	std::cout << "[GIT TIMING] gitEditedLines getEditedLines: "
			  << duration_git_edited.count() << " μs" << std::endl;

	// Update animations before changing anything
	updateLineAnimations(fileChanges);

	// Only update if we have valid data to prevent flashing from empty results
	// Don't update to empty if we already have data (prevents flicker on save)
	if (!fileChanges.empty())
	{
		editedLines = fileChanges;
	}
	// If fileChanges is empty but we have no existing data, then clear it
	else if (editedLines.empty())
	{
		editedLines = fileChanges; // This will be empty, but that's correct for first run
	}
	// If fileChanges is empty but we have existing data, keep the existing data
	// This prevents the flicker when saving files
}

void EditorGit::backgroundTask()
{
	auto lastRegularUpdate = std::chrono::steady_clock::now();
	const auto regularInterval = std::chrono::milliseconds(500);

	while (git_enabled)
	{
		bool shouldUpdate =
			immediateUpdateRequested.exchange(false); // Check and clear flag
		auto now = std::chrono::steady_clock::now();

		// Check if it's time for regular update
		bool timeForRegularUpdate = (now - lastRegularUpdate) >= regularInterval;

		if (shouldUpdate ||
			(timeForRegularUpdate && gSettings.getSettings()["git_changed_lines"]))
		{
			// Update current file only
			if (!gFileExplorer.currentFile.empty())
			{
				std::string relativePath = gFileExplorer.currentFile;
				if (relativePath.find(gFileExplorer.selectedFolder) == 0)
				{
					size_t folderLength = gFileExplorer.selectedFolder.length();
					if (folderLength < relativePath.length())
					{
						relativePath = relativePath.substr(folderLength + 1);
					}
				}

				// Get current file changes and stats in single operation (FAST)
				auto currentFileData = gitWrapper.getCurrentFileData(relativePath);

				// Always update the actual data (clears old data when file becomes clean)
				editedLines[relativePath] = currentFileData.editedLines;

				if (currentFileData.stats.additions > 0 ||
					currentFileData.stats.deletions > 0)
				{
					currentGitChanges =
						"+" + std::to_string(currentFileData.stats.additions) + "-" +
						std::to_string(currentFileData.stats.deletions);
				} else
				{
					currentGitChanges = "";
				}
			}

			// Update timestamp if this was a regular update
			if (timeForRegularUpdate)
			{
				lastRegularUpdate = now;
			}
		}

		// Always sleep for short time to check for immediate updates frequently
		std::this_thread::sleep_for(std::chrono::milliseconds(25));
	}
}

void EditorGit::init()
{
	if (backgroundThread.joinable())
	{
		git_enabled = false;
		backgroundThread.join();
	}

	git_enabled = isGitInitialized();

	if (git_enabled)
	{
		gitWrapper.init(gFileExplorer.selectedFolder);

		// Initial scan to pick up existing changes
		std::cout << "[GIT TIMING] Initial scan on startup" << std::endl;
		auto initialData = gitWrapper.getAllGitData();
		editedLines = initialData.editedLines;

		backgroundThread = std::thread(&EditorGit::backgroundTask, this);
	}
}
bool EditorGit::isLineEdited(const std::string &filePath, int lineNumber) const
{
	std::string relativePath = filePath;
	if (filePath.find(gFileExplorer.selectedFolder) == 0)
	{
		size_t folderLength = gFileExplorer.selectedFolder.length();
		if (folderLength < filePath.length())
		{
			relativePath = filePath.substr(folderLength + 1);
		}
	}
	auto it = editedLines.find(relativePath);
	if (it != editedLines.end())
	{
		const auto &lines = it->second;
		return std::find(lines.begin(), lines.end(), lineNumber) != lines.end();
	}
	return false;
}

std::string EditorGit::gitPlusMinus(const std::string &filePath)
{
	if (!git_enabled || gFileExplorer.selectedFolder.empty())
		return "";

	// Get relative path if needed
	std::string relativePath = filePath;
	if (filePath.find(gFileExplorer.selectedFolder) == 0)
	{
		size_t folderLength = gFileExplorer.selectedFolder.length();
		if (folderLength < filePath.length())
		{
			relativePath = filePath.substr(folderLength + 1);
		}
	}

	// Use libgit2 for lock-free operation
	GitDiffStats stats = gitWrapper.getPlusMinusStats(relativePath);

	if (stats.additions == 0 && stats.deletions == 0)
	{
		return "";
	}

	return "+" + std::to_string(stats.additions) + "-" + std::to_string(stats.deletions);
}

void EditorGit::updateLineAnimations(
	const std::map<std::string, std::vector<int>> &newEditedLines)
{
	auto now = std::chrono::steady_clock::now();

	// For each file in the new changes
	for (const auto &filePair : newEditedLines)
	{
		const std::string &filePath = filePair.first;
		const std::vector<int> &newLines = filePair.second;

		// Get current edited lines for this file
		std::set<int> oldLines;
		auto oldIt = editedLines.find(filePath);
		if (oldIt != editedLines.end())
		{
			oldLines.insert(oldIt->second.begin(), oldIt->second.end());
		}

		std::set<int> newLinesSet(newLines.begin(), newLines.end());

		// Start fade-in animation for newly added lines
		for (int line : newLines)
		{
			if (oldLines.find(line) == oldLines.end())
			{
				// New line - start fade in
				lineAnimations[filePath][line] = {now, true};
			}
		}

		// Start fade-out animation for removed lines
		for (int line : oldLines)
		{
			if (newLinesSet.find(line) == newLinesSet.end())
			{
				// Line removed - start fade out
				lineAnimations[filePath][line] = {now, false};
			}
		}
	}
}

float EditorGit::getLineAnimationAlpha(const std::string &filePath, int lineNumber) const
{
	// No animations - just return 1.0f if line is edited, 0.0f otherwise
	return isLineEdited(filePath, lineNumber) ? 1.0f : 0.0f;
}

void EditorGit::cleanupCompletedAnimations()
{
	auto now = std::chrono::steady_clock::now();
	const float animationDurationMs = 25.0f;

	for (auto fileIt = lineAnimations.begin(); fileIt != lineAnimations.end();)
	{
		for (auto lineIt = fileIt->second.begin(); lineIt != fileIt->second.end();)
		{
			auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
							   now - lineIt->second.startTime)
							   .count();

			if (elapsed > animationDurationMs)
			{
				// Animation completed
				if (!lineIt->second.fadingIn)
				{
					// Fade out completed - remove the animation
					lineIt = fileIt->second.erase(lineIt);
				} else
				{
					// Fade in completed - keep the line but remove animation tracking
					lineIt = fileIt->second.erase(lineIt);
				}
			} else
			{
				++lineIt;
			}
		}

		// Remove empty file entries
		if (fileIt->second.empty())
		{
			fileIt = lineAnimations.erase(fileIt);
		} else
		{
			++fileIt;
		}
	}
}

EditorGit::~EditorGit()
{
	// Clean shutdown
	if (backgroundThread.joinable())
	{
		git_enabled = false;
		backgroundThread.join();
	}
}

std::set<std::string> EditorGit::getModifiedFilePaths()
{
	std::set<std::string> modifiedFiles;

	if (!git_enabled || gFileExplorer.selectedFolder.empty())
		return modifiedFiles;

	// Simple libgit2 git status check
	git_repository *repo = nullptr;
	int open_result = git_repository_open(&repo, gFileExplorer.selectedFolder.c_str());
	if (open_result != 0)
	{
		std::cout << "[FILE TREE GIT] Failed to open repository: "
				  << git_error_last()->message << std::endl;
		return {};
	}

	git_status_options opts = GIT_STATUS_OPTIONS_INIT;
	// Use default options - should include workdir and index changes

	git_status_list *status_list = nullptr;
	int status_result = git_status_list_new(&status_list, repo, &opts);
	if (status_result == 0)
	{
		size_t count = git_status_list_entrycount(status_list);

		for (size_t i = 0; i < count; i++)
		{
			const git_status_entry *entry = git_status_byindex(status_list, i);

			// Check all possible status flags
			if (entry->status != GIT_STATUS_CURRENT && entry->status != 0)
			{
				const char *path = nullptr;
				if (entry->index_to_workdir && entry->index_to_workdir->new_file.path)
					path = entry->index_to_workdir->new_file.path;
				else if (entry->head_to_index && entry->head_to_index->new_file.path)
					path = entry->head_to_index->new_file.path;

				if (path)
				{
					modifiedFiles.insert(path);
				}
			}
		}

		git_status_list_free(status_list);
	}

	git_repository_free(repo);

	return modifiedFiles;
}

void EditorGit::triggerImmediateUpdate()
{
	// Signal the background thread to run immediately (non-blocking)
	immediateUpdateRequested = true;
}

EditorGit gEditorGit;
