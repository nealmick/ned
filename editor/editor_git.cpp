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

std::string execCommand(const char *cmd)
{
	std::array<char, 128> buffer;
	std::string result;
	std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd, "r"), pclose);
	if (!pipe)
	{
		throw std::runtime_error("popen() failed!");
	}
	while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr)
	{
		result += buffer.data();
	}
	return result;
}

void EditorGit::gitEditedLines()
{
	if (gFileExplorer.selectedFolder.empty())
	{
		return;
	}

	// Check if git is busy (lock file exists)
	fs::path gitLockFile = fs::path(gFileExplorer.selectedFolder) / ".git" / "index.lock";
	if (fs::exists(gitLockFile))
	{
		return; // Git is busy, skip this update
	}

	// Change to the project directory before running git commands
	std::string originalDir = fs::current_path().string();
	if (chdir(gFileExplorer.selectedFolder.c_str()) != 0)
	{
		return; // Failed to change directory
	}

	const char *cmd = "git diff --unified=0 --no-color | awk '\n/^diff --git "
					  "a\\// {\n    if (file) print "
					  "\"\";\n    file = substr($3, 3);\n    print \"file: \" "
					  "file;\n    next\n}\n/^@@/ {\n    "
					  "plus = index($0, \"+\");\n    comma = index(substr($0, "
					  "plus+1), \",\");\n    if (comma > "
					  "0) {\n        new_line = substr($0, plus+1, comma-1) + "
					  "0;\n    } else {\n        new_line "
					  "= substr($0, plus+1) + 0;\n    }\n    next\n}\n/^\\+/ "
					  "&& !/^\\+\\+\\+/ {\n    print "
					  "new_line;\n    new_line++;\n}\n/^ / { new_line++; }\n'";
	std::array<char, 256> buffer;
	std::string result;
	std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd, "r"), pclose);
	if (!pipe)
	{
		std::cerr << "Failed to run git diff awk command!" << std::endl;
		chdir(originalDir.c_str()); // Restore original directory
		return;
	}
	while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr)
	{
		result += buffer.data();
	}

	// Restore original directory
	chdir(originalDir.c_str());

	std::istringstream iss(result);
	std::string line;
	std::string currentFile;
	std::map<std::string, std::vector<int>> fileChanges;
	while (std::getline(iss, line))
	{
		if (line.compare(0, 6, "file: ") == 0)
		{
			currentFile = line.substr(6);
		} else if (!line.empty())
		{
			int lineNum = std::stoi(line);
			fileChanges[currentFile].push_back(lineNum);
		}
	}

	// Update animations for changed lines
	updateLineAnimations(fileChanges);

	// Update the map with the new changes
	editedLines = fileChanges;
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

void EditorGit::updateModifiedFiles()
{
	if (gFileExplorer.selectedFolder.empty())
	{
		return;
	}

	// Check if git is busy (lock file exists)
	fs::path gitLockFile = fs::path(gFileExplorer.selectedFolder) / ".git" / "index.lock";
	if (fs::exists(gitLockFile))
	{
		return; // Git is busy, skip this update
	}

	std::string originalDir = fs::current_path().string();
	if (chdir(gFileExplorer.selectedFolder.c_str()) != 0)
	{
		return; // Failed to change directory
	}

	std::string cmd = "git status --porcelain";
	std::string result = execCommand(cmd.c_str());

	chdir(originalDir.c_str()); // Restore original directory

	std::istringstream iss(result);
	std::string line;
	std::set<std::string> newModifiedFiles;

	while (std::getline(iss, line))
	{
		if (line.length() > 3)
		{
			// Extract the file path (after the 2-char status and space)
			std::string filePath = line.substr(3);
			newModifiedFiles.insert(filePath);
		}
	}
	modifiedFiles = newModifiedFiles;
}

void EditorGit::backgroundTask()
{
	while (git_enabled)
	{
		if (gSettings.getSettings()["git_changed_lines"])
		{
			// std::cout << "scanning for git changes" << std::endl;
			gitEditedLines();
			// Update git changes string for current file
			if (!gFileExplorer.currentFile.empty())
			{
				currentGitChanges = gitPlusMinus(gFileExplorer.currentFile);
			} else
			{
				currentGitChanges.clear();
			}
		}
		// Always update modified files, regardless of git_changed_lines setting
		updateModifiedFiles();

		// Clean up completed animations
		cleanupCompletedAnimations();

		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}
}

void EditorGit::init()
{
	// Kill any existing background thread
	if (backgroundThread.joinable())
	{
		git_enabled = false;
		backgroundThread.join();
	}

	// Check if Git is initialized
	git_enabled = isGitInitialized();

	// Only start background task if Git is enabled
	if (git_enabled)
	{
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

	// Check if git is busy (lock file exists)
	fs::path gitLockFile = fs::path(gFileExplorer.selectedFolder) / ".git" / "index.lock";
	if (fs::exists(gitLockFile))
	{
		return ""; // Git is busy, skip this update
	}

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

	// Change to the project directory before running git commands
	std::string originalDir = fs::current_path().string();
	if (chdir(gFileExplorer.selectedFolder.c_str()) != 0)
	{
		return ""; // Failed to change directory
	}

	// Check if file exists in git
	std::string checkCmd =
		"git ls-files --error-unmatch -- " + relativePath + " >/dev/null 2>&1";
	int checkResult = system(checkCmd.c_str());
	if (checkResult != 0)
	{
		chdir(originalDir.c_str()); // Restore original directory
		return "";					// File not tracked by git
	}

	// Run git diff command to get added/removed lines
	std::string cmd = "git diff --numstat -- " + relativePath + " | awk '{print $1,$2}'";
	std::array<char, 128> buffer;
	std::string result;
	std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd.c_str(), "r"), pclose);

	if (!pipe)
	{
		chdir(originalDir.c_str()); // Restore original directory
		return "";
	}

	while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr)
	{
		result += buffer.data();
	}

	// Restore original directory
	chdir(originalDir.c_str());

	// Parse the result
	std::istringstream iss(result);
	int added = 0, removed = 0;
	iss >> added >> removed;

	if (added == 0 && removed == 0)
	{
		return "";
	}

	return "+" + std::to_string(added) + "-" + std::to_string(removed);
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

EditorGit gEditorGit;
