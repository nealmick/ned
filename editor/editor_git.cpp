#include "editor_git.h"
#include <iostream>
#include <chrono>
#include <thread>
#include <atomic>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <cstdio>
#include <memory>
#include <array>
#include "../files/files.h"

namespace fs = std::filesystem;

bool EditorGit::isGitInitialized() {
    if (gFileExplorer.selectedFolder.empty()) {
        return false;
    }
    
    fs::path gitDir = fs::path(gFileExplorer.selectedFolder) / ".git";
    return fs::exists(gitDir) && fs::is_directory(gitDir);
}

std::string execCommand(const char* cmd) {
    std::array<char, 128> buffer;
    std::string result;
    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd, "r"), pclose);
    if (!pipe) {
        throw std::runtime_error("popen() failed!");
    }
    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
        result += buffer.data();
    }
    return result;
}

void EditorGit::gitEditedLines() {
    if (gFileExplorer.selectedFolder.empty()) {
        return;
    }

    // Change to the project directory
    fs::current_path(gFileExplorer.selectedFolder);

    // Clear the edited lines map at the start
    {
        std::lock_guard<std::mutex> lock(editedLinesMutex);
        editedLines.clear();
    }

    // Get modified files (both tracked and untracked)
    std::string gitStatus = execCommand("git status --porcelain");
    std::istringstream statusStream(gitStatus);
    std::string line;
    
    std::unordered_map<std::string, std::set<int>> newEditedLines;

    while (std::getline(statusStream, line)) {
        if (line.empty()) continue;

        // Parse the status line
        std::string status = line.substr(0, 2);
        std::string filepath = line.substr(3);

        // Skip deleted files
        if (status[0] == 'D' || status[1] == 'D') continue;

        // Use git diff-index to compare against HEAD
        std::string diffCmd = "git diff-index --unified=0 HEAD -- " + filepath;
        std::string diff = execCommand(diffCmd.c_str());

        // If no changes found with diff-index, try diff for staged changes
        if (diff.empty()) {
            diffCmd = "git diff --cached --unified=0 -- " + filepath;
            diff = execCommand(diffCmd.c_str());
        }

        std::istringstream diffStream(diff);
        std::string diffLine;
        int currentLine = 0;
        bool inHunk = false;

        while (std::getline(diffStream, diffLine)) {
            if (diffLine.compare(0, 2, "@@") == 0) {
                // Parse the line number from the diff header
                // Format: @@ -a,b +c,d @@
                size_t plusPos = diffLine.find("+");
                if (plusPos != std::string::npos) {
                    size_t commaPos = diffLine.find(",", plusPos);
                    if (commaPos != std::string::npos) {
                        try {
                            currentLine = std::stoi(diffLine.substr(plusPos + 1, commaPos - plusPos - 1));
                            inHunk = true;
                        } catch (const std::exception& e) {
                            inHunk = false;
                            continue;
                        }
                    }
                }
            } else if (inHunk) {
                if (diffLine.compare(0, 1, "+") == 0 && diffLine.compare(0, 3, "+++") != 0) {
                    // This is an added line
                    newEditedLines[filepath].insert(currentLine);
                    currentLine++;
                } else if (diffLine.compare(0, 1, "-") == 0 && diffLine.compare(0, 3, "---") != 0) {
                    // This is a removed line
                    newEditedLines[filepath].insert(currentLine);
                } else if (diffLine.compare(0, 1, " ") != 0 && diffLine.compare(0, 3, "---") != 0 && diffLine.compare(0, 3, "+++") != 0) {
                    // Regular line, increment counter
                    currentLine++;
                }
            }
        }
    }

    // Update the edited lines map
    {
        std::lock_guard<std::mutex> lock(editedLinesMutex);
        editedLines = std::move(newEditedLines);
    }
}

void EditorGit::printGitEditedLines() {
    if (gFileExplorer.currentOpenFile.empty()) {
        return;
    }

    std::lock_guard<std::mutex> lock(editedLinesMutex);
    
    // Get the filename without the path
    std::string currentFile = fs::path(gFileExplorer.currentOpenFile).filename().string();
    
    // Try both the full path and just the filename
    auto it = editedLines.find(gFileExplorer.currentOpenFile);
    if (it == editedLines.end()) {
        it = editedLines.find(currentFile);
    }
    
    std::cout << "Current file: " << gFileExplorer.currentOpenFile << std::endl;
    std::cout << "Edited lines:" << std::endl;
    
    if (it != editedLines.end() && !it->second.empty()) {
        for (int line : it->second) {
            std::cout << "line " << line << std::endl;
        }
    } else {
        std::cout << "No edited lines found" << std::endl;
    }
    std::cout << "-------------------" << std::endl;
}

void EditorGit::backgroundTask() {
    while (git_enabled) {
        std::cout << "Checking for git changes" << std::endl;
        gitEditedLines();
        printGitEditedLines();
        std::this_thread::sleep_for(std::chrono::milliseconds(300));
    }
}

void EditorGit::init() {
    // Kill any existing background thread
    if (backgroundThread.joinable()) {
        git_enabled = false;
        backgroundThread.join();
    }

    // Check if Git is initialized
    git_enabled = isGitInitialized();

    // Only start background task if Git is enabled
    if (git_enabled) {
        backgroundThread = std::thread(&EditorGit::backgroundTask, this);
    }
}

EditorGit gEditorGit;
