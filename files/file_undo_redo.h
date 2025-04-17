/*
	util/undo_redo_manager.h
	This utility tracks changes made to files and can both undo changes and
   redo changes.
*/

#pragma once
#include <algorithm>
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

  private:
	std::vector<State> undoStack;
	std::vector<State> redoStack;
	size_t maxStackSize = 100;

  public:
	void addState(const std::string &state, int changeStart, int changeEnd)
	{
		if (changeStart < 0 || changeEnd < changeStart ||
			changeEnd > static_cast<int>(state.length()))
		{
			std::cerr << "Invalid change range: " << changeStart << " to " << changeEnd
					  << std::endl;
			return;
		}

		if (undoStack.empty() || state != undoStack.back().content)
		{
			undoStack.push_back({state, changeStart, changeEnd});
			if (undoStack.size() > maxStackSize)
			{
				undoStack.erase(undoStack.begin());
			}
			redoStack.clear();
		}
	}

	State undo(const std::string &currentState)
	{
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
};