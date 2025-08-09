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
#include <unistd.h>
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

	// Use libgit2 for lock-free operation
	std::map<std::string, std::vector<int>> fileChanges = gitWrapper.getEditedLines();

	// Update animations before changing anything
	updateLineAnimations(fileChanges);

	// Only update if we have valid data to prevent flashing from empty results
	// If git returns empty, keep the existing data until we get real results
	if (!fileChanges.empty() || editedLines.empty())
	{
		editedLines = fileChanges;
	}
}

void EditorGit::backgroundTask()
{
	while (git_enabled)
	{
		if (gSettings.getSettings()["git_changed_lines"])
		{
			auto gitData = gitWrapper.getAllGitData();

			editedLines = gitData.editedLines;
			modifiedFiles = gitData.modifiedFiles;

			// Update current file stats
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

				auto statsIt = gitData.fileStats.find(relativePath);
				if (statsIt != gitData.fileStats.end())
				{
					auto &stats = statsIt->second;
					if (stats.additions > 0 || stats.deletions > 0)
					{
						currentGitChanges = "+" + std::to_string(stats.additions) + "-" +
											std::to_string(stats.deletions);
					} else
					{
						currentGitChanges = "";
					}
				} else
				{
					currentGitChanges = "";
				}
			}
		}
		cleanupCompletedAnimations();
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
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

void EditorGit::updateModifiedFiles()
{
	if (!git_enabled || gFileExplorer.selectedFolder.empty())
		return;

	// Use libgit2 for lock-free operation
	modifiedFiles = gitWrapper.getModifiedFiles();
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
	// Get relative path for consistency
	std::string relativePath = filePath;
	if (filePath.find(gFileExplorer.selectedFolder) == 0)
	{
		size_t folderLength = gFileExplorer.selectedFolder.length();
		if (folderLength < filePath.length())
		{
			relativePath = filePath.substr(folderLength + 1);
		}
	}

	auto fileIt = lineAnimations.find(relativePath);
	if (fileIt == lineAnimations.end())
	{
		// No animation data, check if line is edited
		return isLineEdited(filePath, lineNumber) ? 1.0f : 0.0f;
	}

	auto lineIt = fileIt->second.find(lineNumber);
	if (lineIt == fileIt->second.end())
	{
		// No animation for this line, check if it's edited
		return isLineEdited(filePath, lineNumber) ? 1.0f : 0.0f;
	}

	const LineAnimation &anim = lineIt->second;
	auto now = std::chrono::steady_clock::now();
	auto elapsed =
		std::chrono::duration_cast<std::chrono::milliseconds>(now - anim.startTime).count();

	const float animationDurationMs = 250.0f; // 0.25 seconds
	float progress = std::min(elapsed / animationDurationMs, 1.0f);

	if (anim.fadingIn)
	{
		return progress; // 0 to 1
	} else
	{
		return 1.0f - progress; // 1 to 0
	}
}

void EditorGit::cleanupCompletedAnimations()
{
	auto now = std::chrono::steady_clock::now();
	const float animationDurationMs = 250.0f;

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

EditorGit gEditorGit;
