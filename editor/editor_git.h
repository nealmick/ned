#pragma once
#include <thread>
#include <atomic>
#include <string>
#include <unordered_map>
#include <set>
#include <mutex>
#include <map>
#include <vector>

class EditorGit {
public:
    void init();
    void gitEditedLines();
    void printGitEditedLines();
    bool isLineEdited(const std::string& filePath, int lineNumber) const;
    std::string gitPlusMinus(const std::string& filePath);
    std::map<std::string, std::vector<int>> editedLines;
    std::string currentGitChanges;  // Store the current git changes string

private:
    bool isGitInitialized();
    void backgroundTask();

    std::atomic<bool> git_enabled{false};
    std::thread backgroundThread;
};

extern EditorGit gEditorGit;
