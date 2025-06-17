#include "editor_git.h"
#include "files.h"
#include "../util/settings.h"
#include <filesystem>
#include <fstream>
#include <sstream>
#include <cstdlib>
#include <chrono>
#include <thread>
#include <mutex>

namespace fs = std::filesystem;

EditorGit::EditorGit() {
    worker_thread = std::thread(&EditorGit::checkGitStatus, this);
}

EditorGit::~EditorGit() {
    should_stop = true;
    if (worker_thread.joinable()) {
        worker_thread.join();
    }
}

EditorGit& EditorGit::getInstance() {
    static EditorGit instance;
    return instance;
}

void EditorGit::checkGitStatus() {
    while (!should_stop) {
        if (gSettings.getSettings().value("git_changed_lines", true)) {
            std::lock_guard<std::mutex> lock(lines_mutex);
            if (!current_filepath.empty() && hasGitRepository(current_filepath)) {
                edited_lines_map[current_filepath] = getEditedLines(current_filepath);
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(250));
    }
}

void EditorGit::setCurrentFile(const std::string& filepath) {
    current_filepath = filepath;
}

void EditorGit::initializeFileTracking(const std::string& filepath) {
    if (!gSettings.getSettings().value("git_changed_lines", true)) {
        std::lock_guard<std::mutex> lock(lines_mutex);
        edited_lines_map[filepath] = std::unordered_set<int>();
        return;
    }
    
    if (!hasGitRepository(filepath)) {
        std::lock_guard<std::mutex> lock(lines_mutex);
        edited_lines_map[filepath] = std::unordered_set<int>();
        return;
    }
    
    std::lock_guard<std::mutex> lock(lines_mutex);
    edited_lines_map[filepath] = getEditedLines(filepath);
}

bool EditorGit::isLineEdited(int line_number) const {
    if (!gSettings.getSettings().value("git_changed_lines", true)) {
        return false;
    }
    
    std::lock_guard<std::mutex> lock(lines_mutex);
    auto it = edited_lines_map.find(current_filepath);
    if (it == edited_lines_map.end()) {
        return false;
    }
    return it->second.find(line_number) != it->second.end();
}

void EditorGit::updateFileChanges(const std::string& filepath) {
    // No-op - background thread handles updates
}

void EditorGit::clearFileTracking(const std::string& filepath) {
    std::lock_guard<std::mutex> lock(lines_mutex);
    edited_lines_map.erase(filepath);
}

bool EditorGit::hasGitRepository(const std::string& path) const {
    if (!gSettings.getSettings().value("git_changed_lines", true)) {
        return false;
    }
    
    std::string project_dir = gFileExplorer.selectedFolder;
    
    if (project_dir.empty()) {
        return false;
    }
    
    fs::path git_path = fs::path(project_dir) / ".git";
    return fs::exists(git_path);
}

std::string EditorGit::getGitRoot(const std::string& path) const {
    if (!gSettings.getSettings().value("git_changed_lines", true)) {
        return "";
    }
    
    std::string project_dir = gFileExplorer.selectedFolder;
    
    if (project_dir.empty()) {
        return "";
    }
    
    fs::path git_path = fs::path(project_dir) / ".git";
    if (!fs::exists(git_path)) {
        return "";
    }
    
    return project_dir;
}

std::string EditorGit::getRelativePath(const std::string& filepath) const {
    if (!gSettings.getSettings().value("git_changed_lines", true)) {
        return filepath;
    }
    
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
    
    if (!gSettings.getSettings().value("git_changed_lines", true)) {
        return edited_lines;
    }
    
    if (!hasGitRepository(filepath)) {
        return edited_lines;
    }
    
    std::string git_root = getGitRoot(filepath);
    if (git_root.empty()) {
        return edited_lines;
    }
    
    std::string rel_path = getRelativePath(filepath);
    if (rel_path.empty()) {
        return edited_lines;
    }
    
    // Get both staged and unstaged changes
    std::string cmd = "cd \"" + git_root + "\" && git diff --unified=0 --no-color HEAD -- \"" + rel_path + "\" && git diff --staged --unified=0 --no-color -- \"" + rel_path + "\"";
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
    
    // Parse the diff output
    std::istringstream iss(result);
    std::string line;
    edited_lines.reserve(100); // Pre-allocate space for common case
    
    while (std::getline(iss, line)) {
        if (line.length() >= 2 && line.substr(0, 2) == "@@") {
            size_t minus_pos = line.find("-");
            size_t plus_pos = line.find("+");
            if (minus_pos != std::string::npos && plus_pos != std::string::npos) {
                // For both deletions and additions, we want to use the new file's line numbers
                size_t new_comma_pos = line.find(",", plus_pos);
                if (new_comma_pos != std::string::npos) {
                    int new_start = std::stoi(line.substr(plus_pos + 1, new_comma_pos - plus_pos - 1));
                    int new_count = std::stoi(line.substr(new_comma_pos + 1));
                    for (int i = 0; i < new_count; ++i) {
                        edited_lines.insert(new_start + i);
                    }
                } else {
                    int new_start = std::stoi(line.substr(plus_pos + 1));
                    edited_lines.insert(new_start);
                }
            }
        }
    }
    
    return edited_lines;
} 