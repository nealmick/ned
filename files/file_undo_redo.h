/*
	util/undo_redo_manager.h
	This utility tracks changes made to files and can both undo changes and
   redo changes.
*/

#pragma once
#include <algorithm>
#include <chrono>
#include <iostream>
#include <string>
#include <vector>

class UndoRedoManager
{

  public:
	struct State
	{
		std::string content;
		int changeStart;
		int changeEnd;
	};

	void addState(const std::string &state, int changeStart, int changeEnd)
	{
		if (changeStart < 0 || changeEnd < changeStart ||
			changeEnd > static_cast<int>(state.length()))
		{
			std::cerr << "Invalid change range: " << changeStart << " to " << changeEnd
					  << std::endl;
			return;
		}

		// Update pending state and reset timer
		pendingState = {state, changeStart, changeEnd};
		hasPending = true;
		lastAddTime = std::chrono::steady_clock::now();
	}

	void update()
	{
		if (!hasPending)
			return;

		auto now = std::chrono::steady_clock::now();
		auto elapsed =
			std::chrono::duration_cast<std::chrono::milliseconds>(now - lastAddTime).count();

		if (elapsed >= 500) // 500ms debounce period
		{
			// Check if state is different from the last in undo stack
			if (undoStack.empty() || pendingState.content != undoStack.back().content)
			{
				undoStack.push_back(pendingState);
				if (undoStack.size() > maxStackSize)
				{
					undoStack.erase(undoStack.begin());
				}
				redoStack.clear();
			}
			hasPending = false; // Reset pending state
		}
	}

	State undo(const std::string &currentState)
	{
		commitPending(); // Ensure any pending state is added before undo
		if (undoStack.size() <= 1)
		{
			return {currentState, 0, static_cast<int>(currentState.length())};
		}
		redoStack.push_back(undoStack.back());
		undoStack.pop_back();
		return undoStack.back();
	}

	State redo(const std::string &currentState)
	{
		commitPending(); // Ensure any pending state is added before redo
		if (redoStack.empty())
		{
			return {currentState, 0, static_cast<int>(currentState.length())};
		}
		State nextState = redoStack.back();
		undoStack.push_back(nextState);
		redoStack.pop_back();
		return nextState;
	}

	void printStacks() const
	{
		std::cout << "Undo stack size: " << undoStack.size() << std::endl;
		std::cout << "Redo stack size: " << redoStack.size() << std::endl;
	}

  private:
	std::vector<State> undoStack;
	std::vector<State> redoStack;
	size_t maxStackSize = 100;

	// Debounce members
	State pendingState;
	bool hasPending = false;
	std::chrono::steady_clock::time_point lastAddTime;

	void commitPending()
	{
		if (hasPending)
		{
			// Directly commit if called during undo/redo
			if (undoStack.empty() || pendingState.content != undoStack.back().content)
			{
				undoStack.push_back(pendingState);
				if (undoStack.size() > maxStackSize)
				{
					undoStack.erase(undoStack.begin());
				}
				redoStack.clear();
			}
			hasPending = false;
		}
	}
};