#include "editor_git.h"
#include <filesystem>
#include <fstream>
#include <sstream>
#include <cstdlib>
#include <iostream>

namespace fs = std::filesystem;

EditorGit& EditorGit::getInstance() {
    static EditorGit instance;
    return instance;
}

void EditorGit::setCurrentFile(const std::string& filepath) {
    std::cout << "Setting current file to: " << filepath << std::endl;
    current_filepath = filepath;
}

void EditorGit::initializeFileTracking(const std::string& filepath) {
    std::cout << "Initializing Git tracking for: " << filepath << std::endl;
    
    if (!hasGitRepository(filepath)) {
        std::cout << "No Git repository found for: " << filepath << std::endl;
        return;
    }
    
    std::cout << "Found Git repository, getting edited lines..." << std::endl;
    // Get initial state of edited lines
    edited_lines_map[filepath] = getEditedLines(filepath);
    std::cout << "Found " << edited_lines_map[filepath].size() << " edited lines" << std::endl;
}

bool EditorGit::isLineEdited(int line_number) const {
    // Use the current filepath to look up edited lines
    auto it = edited_lines_map.find(current_filepath);
    if (it == edited_lines_map.end()) {
        return false;
    }
    bool is_edited = it->second.find(line_number) != it->second.end();
    if (is_edited) {
        std::cout << "Line " << line_number << " is edited" << std::endl;
    }
    return is_edited;
}

void EditorGit::updateFileChanges(const std::string& filepath) {
    std::cout << "Updating Git changes for: " << filepath << std::endl;
    if (!hasGitRepository(filepath)) {
        std::cout << "No Git repository found for: " << filepath << std::endl;
        return;
    }
    
    // Update the edited lines for this file
    edited_lines_map[filepath] = getEditedLines(filepath);
    std::cout << "Updated edited lines count: " << edited_lines_map[filepath].size() << std::endl;
}

void EditorGit::clearFileTracking(const std::string& filepath) {
    edited_lines_map.erase(filepath);
}

bool EditorGit::hasGitRepository(const std::string& path) const {
    fs::path current_path = fs::absolute(path);
    while (!current_path.empty()) {
        if (fs::exists(current_path / ".git")) {
            return true;
        }
        current_path = current_path.parent_path();
    }
    return false;
}

std::string EditorGit::getGitRoot(const std::string& path) const {
    fs::path current_path = fs::absolute(path);
    while (!current_path.empty()) {
        if (fs::exists(current_path / ".git")) {
            return current_path.string();
        }
        current_path = current_path.parent_path();
    }
    return "";
}

std::string EditorGit::getRelativePath(const std::string& filepath) const {
    std::string git_root = getGitRoot(filepath);
    if (git_root.empty()) {
        return filepath;
    }
    
    fs::path abs_path = fs::absolute(filepath);
    fs::path rel_path = fs::relative(abs_path, git_root);
    return rel_path.string();
}

std::unordered_set<int> EditorGit::getEditedLines(const std::string& filepath) const {
    std::unordered_set<int> edited_lines;
    std::string git_root = getGitRoot(filepath);
    if (git_root.empty()) {
        return edited_lines;
    }
    
    std::string rel_path = getRelativePath(filepath);
    
    // Execute git diff command
    std::string cmd = "cd " + git_root + " && git diff --unified=0 " + rel_path;
    FILE* pipe = popen(cmd.c_str(), "r");
    if (!pipe) {
        return edited_lines;
    }
    
    char buffer[128];
    std::string result = "";
    while (fgets(buffer, sizeof(buffer), pipe) != NULL) {
        result += buffer;
    }
    pclose(pipe);
    
    // Parse the diff output to find edited lines
    std::istringstream iss(result);
    std::string line;
    while (std::getline(iss, line)) {
        // Use string comparison instead of starts_with
        if (line.length() >= 2 && line.substr(0, 2) == "@@") {
            // Parse the line numbers from the diff header
            size_t pos = line.find("+");
            if (pos != std::string::npos) {
                size_t comma_pos = line.find(",", pos);
                if (comma_pos != std::string::npos) {
                    int start_line = std::stoi(line.substr(pos + 1, comma_pos - pos - 1));
                    int num_lines = std::stoi(line.substr(comma_pos + 1));
                    for (int i = 0; i < num_lines; ++i) {
                        edited_lines.insert(start_line + i);
                    }
                }
            }
        }
    }
    
    return edited_lines;
} 