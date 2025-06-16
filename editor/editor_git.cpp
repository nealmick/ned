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
    current_filepath = filepath;
}

void EditorGit::initializeFileTracking(const std::string& filepath) {
    if (!hasGitRepository(filepath)) {
        return;
    }
    edited_lines_map[filepath] = getEditedLines(filepath);
}

bool EditorGit::isLineEdited(int line_number) const {
    auto it = edited_lines_map.find(current_filepath);
    if (it == edited_lines_map.end()) {
        return false;
    }
    return it->second.find(line_number) != it->second.end();
}

void EditorGit::updateFileChanges(const std::string& filepath) {
    if (!hasGitRepository(filepath)) {
        return;
    }
    edited_lines_map[filepath] = getEditedLines(filepath);
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
    
    std::string cmd = "cd " + git_root + " && git diff-index --quiet HEAD -- " + rel_path;
    int status = system(cmd.c_str());
    
    if (status != 0) {
        cmd = "cd " + git_root + " && git diff --unified=0 HEAD -- " + rel_path;
        FILE* pipe = popen(cmd.c_str(), "r");
        if (pipe) {
            char buffer[128];
            std::string result = "";
            while (fgets(buffer, sizeof(buffer), pipe) != NULL) {
                result += buffer;
            }
            pclose(pipe);
            
            std::istringstream iss(result);
            std::string line;
            while (std::getline(iss, line)) {
                if (line.length() >= 2 && line.substr(0, 2) == "@@") {
                    size_t minus_pos = line.find("-");
                    size_t plus_pos = line.find("+");
                    if (minus_pos != std::string::npos && plus_pos != std::string::npos) {
                        size_t old_comma_pos = line.find(",", minus_pos);
                        int old_start = 0;
                        int old_count = 0;
                        if (old_comma_pos != std::string::npos) {
                            old_start = std::stoi(line.substr(minus_pos + 1, old_comma_pos - minus_pos - 1));
                            old_count = std::stoi(line.substr(old_comma_pos + 1, plus_pos - old_comma_pos - 1));
                        }
                        
                        size_t new_comma_pos = line.find(",", plus_pos);
                        int new_start = 0;
                        int new_count = 0;
                        if (new_comma_pos != std::string::npos) {
                            new_start = std::stoi(line.substr(plus_pos + 1, new_comma_pos - plus_pos - 1));
                            new_count = std::stoi(line.substr(new_comma_pos + 1));
                        } else {
                            new_start = std::stoi(line.substr(plus_pos + 1));
                            new_count = 1;
                        }
                        
                        if (old_count > 0 && new_count > 0) {
                            for (int i = 0; i < old_count; ++i) {
                                edited_lines.insert(old_start + i);
                            }
                        }
                        else if (new_count > 0) {
                            for (int i = 0; i < new_count; ++i) {
                                edited_lines.insert(new_start + i);
                            }
                        }
                        else if (old_count > 0) {
                            for (int i = 0; i < old_count; ++i) {
                                edited_lines.insert(old_start + i);
                            }
                        }
                    }
                }
            }
        }
    }
    
    cmd = "cd " + git_root + " && git diff --staged --unified=0 -- " + rel_path;
    FILE* pipe = popen(cmd.c_str(), "r");
    if (pipe) {
        char buffer[128];
        std::string result = "";
        while (fgets(buffer, sizeof(buffer), pipe) != NULL) {
            result += buffer;
        }
        pclose(pipe);
        
        std::istringstream iss(result);
        std::string line;
        while (std::getline(iss, line)) {
            if (line.length() >= 2 && line.substr(0, 2) == "@@") {
                size_t minus_pos = line.find("-");
                size_t plus_pos = line.find("+");
                if (minus_pos != std::string::npos && plus_pos != std::string::npos) {
                    size_t old_comma_pos = line.find(",", minus_pos);
                    int old_start = 0;
                    int old_count = 0;
                    if (old_comma_pos != std::string::npos) {
                        old_start = std::stoi(line.substr(minus_pos + 1, old_comma_pos - minus_pos - 1));
                        old_count = std::stoi(line.substr(old_comma_pos + 1, plus_pos - old_comma_pos - 1));
                    }
                    
                    size_t new_comma_pos = line.find(",", plus_pos);
                    int new_start = 0;
                    int new_count = 0;
                    if (new_comma_pos != std::string::npos) {
                        new_start = std::stoi(line.substr(plus_pos + 1, new_comma_pos - plus_pos - 1));
                        new_count = std::stoi(line.substr(new_comma_pos + 1));
                    } else {
                        new_start = std::stoi(line.substr(plus_pos + 1));
                        new_count = 1;
                    }
                    
                    if (old_count > 0 && new_count > 0) {
                        for (int i = 0; i < old_count; ++i) {
                            edited_lines.insert(old_start + i);
                        }
                    }
                    else if (new_count > 0) {
                        for (int i = 0; i < new_count; ++i) {
                            edited_lines.insert(new_start + i);
                        }
                    }
                    else if (old_count > 0) {
                        for (int i = 0; i < old_count; ++i) {
                            edited_lines.insert(old_start + i);
                        }
                    }
                }
            }
        }
    }
    
    return edited_lines;
} 