#pragma once
#include <thread>
#include <atomic>
#include <string>
#include <unordered_map>
#include <set>
#include <mutex>

class EditorGit {
private:
    std::thread backgroundThread;
    std::atomic<bool> git_enabled{false};
    void backgroundTask();
    void gitEditedLines();
    void printGitEditedLines();

    // Data structure to track edited lines
    // Map of filename -> set of line numbers that are modified
    std::unordered_map<std::string, std::set<int>> editedLines;
    std::mutex editedLinesMutex; // Protect access to editedLines

public:
    void init();
    bool isGitInitialized();
    
};

extern EditorGit gEditorGit;
