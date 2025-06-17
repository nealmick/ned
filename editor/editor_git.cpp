#include "editor_git.h"
#include "files.h"
#include "../util/settings.h"
#include <filesystem>
#include <fstream>
#include <sstream>
#include <cstdlib>
#include <chrono>

namespace fs = std::filesystem;

EditorGit& EditorGit::getInstance() {
    static EditorGit instance;
    return instance;
}

void EditorGit::setCurrentFile(const std::string& filepath) {
    current_filepath = filepath;
}

void EditorGit::initializeFileTracking(const std::string& filepath) {
    if (!gSettings.getSettings().value("git_changed_lines", true)) {
        edited_lines_map[filepath] = std::unordered_set<int>();  // Initialize with empty set
        return;
    }
    
    if (!hasGitRepository(filepath)) {
        edited_lines_map[filepath] = std::unordered_set<int>();  // Initialize with empty set
        return;
    }
    edited_lines_map[filepath] = getEditedLines(filepath, true);  // Force refresh on initialization
}

bool EditorGit::isLineEdited(int line_number) const {
    if (!gSettings.getSettings().value("git_changed_lines", true)) {
        return false;
    }
    
    auto it = edited_lines_map.find(current_filepath);
    if (it == edited_lines_map.end()) {
        return false;
    }
    return it->second.find(line_number) != it->second.end();
}

void EditorGit::updateFileChanges(const std::string& filepath) {
    if (!gSettings.getSettings().value("git_changed_lines", true)) {
        edited_lines_map[filepath] = std::unordered_set<int>();  // Set empty set when disabled
        return;
    }
    
    if (!hasGitRepository(filepath)) {
        edited_lines_map[filepath] = std::unordered_set<int>();  // Set empty set for non-git files
        return;
    }
    edited_lines_map[filepath] = getEditedLines(filepath, true);  // Force refresh on file change
}

void EditorGit::forceRefresh(const std::string& filepath) {
    if (!gSettings.getSettings().value("git_changed_lines", true)) {
        return;
    }
    
    if (!hasGitRepository(filepath)) {
        return;
    }
    
    edited_lines_map[filepath] = getEditedLines(filepath, true);
}

void EditorGit::clearFileTracking(const std::string& filepath) {
    edited_lines_map.erase(filepath);
    git_cache.erase(filepath);
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

bool EditorGit::isCacheValid(const std::string& filepath) const {
    auto it = git_cache.find(filepath);
    if (it == git_cache.end()) {
        return false;
    }
    auto now = std::chrono::system_clock::now();
    return (now - it->second.last_update) < CACHE_DURATION;
}

void EditorGit::updateCache(const std::string& filepath, const std::unordered_set<int>& lines) {
    GitCache cache;
    cache.edited_lines = lines;
    cache.last_update = std::chrono::system_clock::now();
    cache.is_valid = true;
    git_cache[filepath] = cache;
}

std::unordered_set<int> EditorGit::getEditedLines(const std::string& filepath, bool force_refresh) const {
    if (!gSettings.getSettings().value("git_changed_lines", true)) {
        return std::unordered_set<int>();
    }
    
    if (!hasGitRepository(filepath)) {
        return std::unordered_set<int>();
    }
    
    // Check cache first, unless force refresh is requested
    if (!force_refresh && isCacheValid(filepath)) {
        return git_cache.at(filepath).edited_lines;
    }
    
    std::string git_root = getGitRoot(filepath);
    if (git_root.empty()) {
        return std::unordered_set<int>();
    }
    
    std::string rel_path = getRelativePath(filepath);
    if (rel_path.empty()) {
        return std::unordered_set<int>();
    }
    
    // Combine git commands into a single call
    std::string cmd = "cd \"" + git_root + "\" && git diff --unified=0 HEAD -- \"" + rel_path + "\" && git diff --staged --unified=0 -- \"" + rel_path + "\"";
    FILE* pipe = popen(cmd.c_str(), "r");
    if (!pipe) {
        return std::unordered_set<int>();
    }
    
    std::unordered_set<int> edited_lines;
    char buffer[128];
    std::string result = "";
    while (fgets(buffer, sizeof(buffer), pipe) != NULL) {
        result += buffer;
    }
    pclose(pipe);
    
    // Parse the combined diff output
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
                
                // Optimize line insertion by pre-allocating space
                if (old_count > 0) {
                    edited_lines.reserve(edited_lines.size() + old_count);
                    for (int i = 0; i < old_count; ++i) {
                        edited_lines.insert(old_start + i);
                    }
                }
                if (new_count > 0) {
                    edited_lines.reserve(edited_lines.size() + new_count);
                    for (int i = 0; i < new_count; ++i) {
                        edited_lines.insert(new_start + i);
                    }
                }
            }
        }
    }
    
    // Update cache
    const_cast<EditorGit*>(this)->updateCache(filepath, edited_lines);
    
    return edited_lines;
} 