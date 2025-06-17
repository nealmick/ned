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
#include <map>
#include <vector>

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
    const char* cmd = "git diff --unified=0 --no-color | awk '\n/^diff --git a\\// {\n    if (file) print \"\";\n    file = substr($3, 3);\n    print \"file: \" file;\n    next\n}\n/^@@/ {\n    plus = index($0, \"+\");\n    comma = index(substr($0, plus+1), \",\");\n    if (comma > 0) {\n        new_line = substr($0, plus+1, comma-1) + 0;\n    } else {\n        new_line = substr($0, plus+1) + 0;\n    }\n    next\n}\n/^\\+/ && !/^\\+\\+\\+/ {\n    print new_line;\n    new_line++;\n}\n/^ / { new_line++; }\n'";
    std::array<char, 256> buffer;
    std::string result;
    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd, "r"), pclose);
    if (!pipe) {
        std::cerr << "Failed to run git diff awk command!" << std::endl;
        return;
    }
    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
        result += buffer.data();
    }
    std::istringstream iss(result);
    std::string line;
    std::string currentFile;
    std::map<std::string, std::vector<int>> fileChanges;
    while (std::getline(iss, line)) {
        if (line.compare(0, 6, "file: ") == 0) {
            currentFile = line.substr(6);
        } else if (!line.empty()) {
            int lineNum = std::stoi(line);
            fileChanges[currentFile].push_back(lineNum);
        }
    }
    // Update the map with the new changes
    editedLines = fileChanges;
}

void EditorGit::printGitEditedLines() {
    for (const auto& pair : editedLines) {
        std::cout << "file: " << pair.first << std::endl;
        for (int line : pair.second) {
            std::cout << line << std::endl;
        }
    }
}

void EditorGit::backgroundTask() {
    while (git_enabled) {
        if (gSettings.getSettings()["git_changed_lines"]) {
            //std::cout << "scanning for git changes" << std::endl;
            gitEditedLines();
            // Update git changes string for current file
            if (!gFileExplorer.currentFile.empty()) {
                currentGitChanges = gitPlusMinus(gFileExplorer.currentFile);
            } else {
                currentGitChanges.clear();
            }
        } else {
            editedLines.clear();  // Clear the map when feature is disabled
            currentGitChanges.clear();
        }
        //printGitEditedLines();
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
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
bool EditorGit::isLineEdited(const std::string& filePath, int lineNumber) const {
    std::string relativePath = filePath;
    if (filePath.find(gFileExplorer.selectedFolder) == 0) {
        size_t folderLength = gFileExplorer.selectedFolder.length();
        if (folderLength < filePath.length()) {
            relativePath = filePath.substr(folderLength + 1);
        }
    }
    auto it = editedLines.find(relativePath);
    if (it != editedLines.end()) {
        const auto& lines = it->second;
        return std::find(lines.begin(), lines.end(), lineNumber) != lines.end();
    }
    return false;
}

std::string EditorGit::gitPlusMinus(const std::string& filePath) {
    if (!git_enabled) return "";
    
    // Get relative path if needed
    std::string relativePath = filePath;
    if (filePath.find(gFileExplorer.selectedFolder) == 0) {
        size_t folderLength = gFileExplorer.selectedFolder.length();
        if (folderLength < filePath.length()) {
            relativePath = filePath.substr(folderLength + 1);
        }
    }

    // Run git diff command to get added/removed lines
    std::string cmd = "git diff --numstat " + relativePath + " | awk '{print $1,$2}'";
    std::array<char, 128> buffer;
    std::string result;
    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd.c_str(), "r"), pclose);
    
    if (!pipe) {
        return "";
    }
    
    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
        result += buffer.data();
    }

    // Parse the result
    std::istringstream iss(result);
    int added = 0, removed = 0;
    iss >> added >> removed;

    if (added == 0 && removed == 0) {
        return "";
    }

    return "+" + std::to_string(added) + "-" + std::to_string(removed);
}

EditorGit gEditorGit;
