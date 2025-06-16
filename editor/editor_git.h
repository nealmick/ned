#pragma once

#include <string>
#include <vector>
#include <unordered_set>
#include <memory>
#include <unordered_map>

class EditorGit {
public:
    static EditorGit& getInstance();
    
    // Initialize Git tracking for a file
    void initializeFileTracking(const std::string& filepath);
    
    // Check if a line has been edited
    bool isLineEdited(int line_number) const;
    
    // Update tracking when file changes
    void updateFileChanges(const std::string& filepath);
    
    // Clear tracking for a file
    void clearFileTracking(const std::string& filepath);

    // Set the current file being tracked
    void setCurrentFile(const std::string& filepath);

private:
    EditorGit() = default;
    ~EditorGit() = default;
    EditorGit(const EditorGit&) = delete;
    EditorGit& operator=(const EditorGit&) = delete;

    // Check if a Git repository exists for the given path
    bool hasGitRepository(const std::string& path) const;
    
    // Get the Git root directory for a given path
    std::string getGitRoot(const std::string& path) const;
    
    // Get the relative path from Git root to the file
    std::string getRelativePath(const std::string& filepath) const;
    
    // Get the list of edited lines from Git
    std::unordered_set<int> getEditedLines(const std::string& filepath) const;

    // Map of filepath to edited lines
    std::unordered_map<std::string, std::unordered_set<int>> edited_lines_map;
    
    // Current file being tracked
    std::string current_filepath;
}; 