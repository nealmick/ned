#pragma once
#include "../lib/json.hpp"
#include <algorithm>
#include <chrono>
#include <iostream>
#include <string>
#include <vector>

using json = nlohmann::json;

class UndoRedoManager {
public:
    struct Operation {
        int position;          // Where the change occurred
        std::string removed;   // Text that was removed
        std::string inserted;  // Text that was inserted
        int cursor_before;     // Cursor position before the change
        int cursor_after;      // Cursor position after the change

        // Helper to apply this operation to a string
        std::string apply(const std::string& current) const {
            std::string result = current;
            result.replace(position, inserted.length(), removed);
            return result;
        }

        // Helper to apply inverse (redo) operation
        std::string applyInverse(const std::string& current) const {
            std::string result = current;
            result.replace(position, removed.length(), inserted);
            return result;
        }
    };

    json toJson() const {
        json j;
        j["maxStackSize"] = maxStackSize;
        j["lastCommittedState"] = lastCommittedState;
        j["undoStack"] = json::array();
        j["redoStack"] = json::array();

        for (const auto& op : undoStack) {
            j["undoStack"].push_back({
                {"position", op.position},
                {"removed", op.removed},
                {"inserted", op.inserted},
                {"cursor_before", op.cursor_before},
                {"cursor_after", op.cursor_after}
            });
        }

        for (const auto& op : redoStack) {
            j["redoStack"].push_back({
                {"position", op.position},
                {"removed", op.removed},
                {"inserted", op.inserted},
                {"cursor_before", op.cursor_before},
                {"cursor_after", op.cursor_after}
            });
        }
        return j;
    }

    void fromJson(const json& j) {
        maxStackSize = j.value("maxStackSize", 100);
        lastCommittedState = j.value("lastCommittedState", "");
        undoStack.clear();
        redoStack.clear();

        if (j.contains("undoStack")) {
            for (const auto& item : j["undoStack"]) {
                undoStack.push_back({
                    item["position"].get<int>(),
                    item["removed"].get<std::string>(),
                    item["inserted"].get<std::string>(),
                    item["cursor_before"].get<int>(),
                    item["cursor_after"].get<int>()
                });
            }
        }

        if (j.contains("redoStack")) {
            for (const auto& item : j["redoStack"]) {
                redoStack.push_back({
                    item["position"].get<int>(),
                    item["removed"].get<std::string>(),
                    item["inserted"].get<std::string>(),
                    item["cursor_before"].get<int>(),
                    item["cursor_after"].get<int>()
                });
            }
        }
    }

    // Simplified API: Only current content and cursor index
    void addState(const std::string& currentContent, int cursor_after) {
        if (!hasPending) {
            // First change in a series
            pendingInitialContent = lastCommittedState;
            pendingInitialCursor = pendingFinalCursor;
            hasPending = true;
            redoStack.clear();  // Clear redo stack on new changes
        }
        
        pendingFinalContent = currentContent;
        pendingFinalCursor = cursor_after;
        lastAddTime = std::chrono::steady_clock::now();
    }

    void update() {
        if (!hasPending) return;

        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastAddTime).count();

        if (elapsed >= 500) {  // 500ms debounce
            computeAndSaveOperation();
            hasPending = false;
        }
    }

    std::pair<Operation, bool> undo() {
        commitPending();
        if (undoStack.empty()) {
            return {{}, false};
        }

        Operation op = undoStack.back();
        undoStack.pop_back();
        
        // Update last committed state by applying inverse operation
        lastCommittedState = op.apply(lastCommittedState);
        redoStack.push_back(op);
        
        return {op, true};
    }

    std::pair<Operation, bool> redo() {
        commitPending();
        if (redoStack.empty()) {
            return {{}, false};
        }

        Operation op = redoStack.back();
        redoStack.pop_back();
        
        // Update last committed state by applying operation
        lastCommittedState = op.applyInverse(lastCommittedState);
        undoStack.push_back(op);
        
        return {op, true};
    }

    void initialize(const std::string& content, int cursor) {
        lastCommittedState = content;
        pendingFinalCursor = cursor;
    }

    void printStacks() const {
        std::cout << "Undo stack: " << undoStack.size() 
                  << " Redo stack: " << redoStack.size() << std::endl;
    }

private:
    std::vector<Operation> undoStack;
    std::vector<Operation> redoStack;
    size_t maxStackSize = 100;
    std::string lastCommittedState;  // Last known state after commit

    // Debounce members
    std::string pendingInitialContent;
    std::string pendingFinalContent;
    int pendingInitialCursor = 0;
    int pendingFinalCursor = 0;
    bool hasPending = false;
    std::chrono::steady_clock::time_point lastAddTime;

    void commitPending() {
        if (hasPending) {
            computeAndSaveOperation();
            hasPending = false;
        }
    }

    // Computes the difference between initial and final states
    void computeAndSaveOperation() {
        // Compute diff between initial and final states
        const std::string& oldStr = pendingInitialContent;
        const std::string& newStr = pendingFinalContent;
        
        // Find first difference
        size_t start = 0;
        size_t oldEnd = oldStr.size();
        size_t newEnd = newStr.size();
        
        while (start < oldEnd && start < newEnd && oldStr[start] == newStr[start]) {
            start++;
        }
        
        // Find last difference
        while (oldEnd > start && newEnd > start && 
               oldStr[oldEnd - 1] == newStr[newEnd - 1]) {
            oldEnd--;
            newEnd--;
        }
        
        // Create operation
        Operation op{
            static_cast<int>(start),
            oldStr.substr(start, oldEnd - start),
            newStr.substr(start, newEnd - start),
            pendingInitialCursor,
            pendingFinalCursor
        };
        
        // Save operation and update state
        undoStack.push_back(op);
        if (undoStack.size() > maxStackSize) {
            undoStack.erase(undoStack.begin());
        }
        lastCommittedState = pendingFinalContent;
    }
};