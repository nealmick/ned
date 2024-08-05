#pragma once
#include <vector>
#include <string>
#include <iostream>
class UndoRedoManager {
private:
    struct State {
        std::string content;
        int changeStart;
        int changeEnd;
    };
    std::vector<State> undoStack;
    std::vector<State> redoStack;
    size_t maxStackSize = 100;

public:
    void addState(const std::string& state, int changeStart, int changeEnd) {
        if (undoStack.empty() || state != undoStack.back().content) {
            undoStack.push_back({state, changeStart, changeEnd});
            if (undoStack.size() > maxStackSize) {
                undoStack.erase(undoStack.begin());
            }
            redoStack.clear();  // Only clear redo stack when a new state is added
        }
    }

    State undo(const std::string& currentState) {
        if (undoStack.size() <= 1) {
            return {currentState, 0, 0};
        }
        redoStack.push_back(undoStack.back());  // Save current state to redo stack
        undoStack.pop_back();  // Remove the current state
        return undoStack.back();  // Return the previous state
    }

    State redo(const std::string& currentState) {
        if (redoStack.empty()) {
            return {currentState, 0, 0};
        }
        State nextState = redoStack.back();
        undoStack.push_back(redoStack.back());  // Move the redone state to the undo stack
        redoStack.pop_back();
        return nextState;
    }

    // Add this method for debugging
    void printStacks() const {
        std::cout << "Undo stack size: " << undoStack.size() << std::endl;
        std::cout << "Redo stack size: " << redoStack.size() << std::endl;
    }
};