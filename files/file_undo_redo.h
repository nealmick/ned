#pragma once
#include "../lib/json.hpp"
#include <algorithm>
#include <chrono>
#include <iostream>
#include <string>
#include <vector>

using json = nlohmann::json;

class UndoRedoManager
{
  public:
	struct Operation
	{
		int position;		  // Where the change occurred
		std::string removed;  // Text that was removed
		std::string inserted; // Text that was inserted
		int cursor_before;	  // Cursor position before the change
		int cursor_after;	  // Cursor position after the change

		// Helper to apply this operation to a string
		std::string apply(const std::string &current) const
		{
			std::string result = current;
			if (position >= 0 && position <= static_cast<int>(result.length()))
			{
				result.replace(position, inserted.length(), removed);
			}
			return result;
		}

		// Helper to apply inverse (redo) operation
		std::string applyInverse(const std::string &current) const
		{
			std::string result = current;
			if (position >= 0 && position <= static_cast<int>(result.length()))
			{
				result.replace(position, removed.length(), inserted);
			}
			return result;
		}
	};

	json toJson() const
	{
		json j;
		j["maxStackSize"] = maxStackSize;
		j["lastCommittedState"] = lastCommittedState;
		j["undoStack"] = json::array();
		j["redoStack"] = json::array();

		// Only serialize recent operations to reduce memory usage
		size_t undoCount = std::min(undoStack.size(), static_cast<size_t>(50));
		for (size_t i = undoStack.size() - undoCount; i < undoStack.size(); ++i)
		{
			j["undoStack"].push_back({{"position", undoStack[i].position},
									  {"removed", undoStack[i].removed},
									  {"inserted", undoStack[i].inserted},
									  {"cursor_before", undoStack[i].cursor_before},
									  {"cursor_after", undoStack[i].cursor_after}});
		}

		size_t redoCount = std::min(redoStack.size(), static_cast<size_t>(20));
		for (size_t i = redoStack.size() - redoCount; i < redoStack.size(); ++i)
		{
			j["redoStack"].push_back({{"position", redoStack[i].position},
									  {"removed", redoStack[i].removed},
									  {"inserted", redoStack[i].inserted},
									  {"cursor_before", redoStack[i].cursor_before},
									  {"cursor_after", redoStack[i].cursor_after}});
		}
		return j;
	}

	void fromJson(const json &j)
	{
		try
		{
			maxStackSize = j.value("maxStackSize", 50); // Reduced default
			lastCommittedState = j.value("lastCommittedState", "");
			undoStack.clear();
			redoStack.clear();

			if (j.contains("undoStack") && j["undoStack"].is_array())
			{
				for (const auto &item : j["undoStack"])
				{
					if (item.is_object() && item.contains("position") &&
						item["position"].is_number() && item.contains("removed") &&
						item["removed"].is_string() && item.contains("inserted") &&
						item["inserted"].is_string() && item.contains("cursor_before") &&
						item["cursor_before"].is_number() &&
						item.contains("cursor_after") && item["cursor_after"].is_number())
					{

						undoStack.push_back({item["position"].get<int>(),
											 item["removed"].get<std::string>(),
											 item["inserted"].get<std::string>(),
											 item["cursor_before"].get<int>(),
											 item["cursor_after"].get<int>()});
					}
				}
			}

			if (j.contains("redoStack") && j["redoStack"].is_array())
			{
				for (const auto &item : j["redoStack"])
				{
					if (item.is_object() && item.contains("position") &&
						item["position"].is_number() && item.contains("removed") &&
						item["removed"].is_string() && item.contains("inserted") &&
						item["inserted"].is_string() && item.contains("cursor_before") &&
						item["cursor_before"].is_number() &&
						item.contains("cursor_after") && item["cursor_after"].is_number())
					{

						redoStack.push_back({item["position"].get<int>(),
											 item["removed"].get<std::string>(),
											 item["inserted"].get<std::string>(),
											 item["cursor_before"].get<int>(),
											 item["cursor_after"].get<int>()});
					}
				}
			}
		} catch (const std::exception &e)
		{
			// If there's any error loading the JSON, just reset to empty state
			std::cerr << "Error loading undo/redo state: " << e.what() << std::endl;
			undoStack.clear();
			redoStack.clear();
			lastCommittedState = "";
			maxStackSize = 50;
		}
	}

	// Simplified API: Only current content and cursor index
	void addState(const std::string &currentContent, int cursor_after)
	{
		std::cout << "Adding state: " << cursor_after << std::endl;
		if (!hasPending)
		{
			// First change in a series
			pendingInitialContent = lastCommittedState;
			pendingInitialCursor = pendingFinalCursor;
			hasPending = true;
			redoStack.clear(); // Clear redo stack on new changes
		}

		pendingFinalContent = currentContent;
		pendingFinalCursor = cursor_after;
		lastAddTime = std::chrono::steady_clock::now();
	}

	void update()
	{
		if (!hasPending)
			return;

		auto now = std::chrono::steady_clock::now();
		auto elapsed =
			std::chrono::duration_cast<std::chrono::milliseconds>(now - lastAddTime)
				.count();

		if (elapsed >= 300)
		{ // Reduced debounce time for better responsiveness
			computeAndSaveOperation();
			hasPending = false;
		}
	}

	std::pair<Operation, bool> undo()
	{
		commitPending();
		if (undoStack.empty())
		{
			return {{}, false};
		}

		Operation op = undoStack.back();
		undoStack.pop_back();

		// Update last committed state by applying inverse operation
		lastCommittedState = op.apply(lastCommittedState);
		redoStack.push_back(op);

		return {op, true};
	}

	std::pair<Operation, bool> redo()
	{
		commitPending();
		if (redoStack.empty())
		{
			return {{}, false};
		}

		Operation op = redoStack.back();
		redoStack.pop_back();

		// Update last committed state by applying operation
		lastCommittedState = op.applyInverse(lastCommittedState);
		undoStack.push_back(op);

		return {op, true};
	}

	void initialize(const std::string &content, int cursor)
	{
		lastCommittedState = content;
		pendingFinalCursor = cursor;
	}

	void printStacks() const
	{
		std::cout << "Undo stack: " << undoStack.size()
				  << " Redo stack: " << redoStack.size() << std::endl;
	}

	// Check if there are any operations to save
	bool hasOperations() const { return !undoStack.empty() || !redoStack.empty(); }

	// Force commit pending state immediately (useful for paste operations)
	void forceCommitPending() { commitPending(); }

	void updatePendingFinalCursor(int cursor)
	{
		if (!hasPending)
		{
			pendingFinalCursor = cursor;
		}
	}

  private:
	std::vector<Operation> undoStack;
	std::vector<Operation> redoStack;
	size_t maxStackSize = 50;		// Reduced from 100 to 50
	std::string lastCommittedState; // Last known state after commit

	// Debounce members
	std::string pendingInitialContent;
	std::string pendingFinalContent;
	int pendingInitialCursor = 0;
	int pendingFinalCursor = 0;
	bool hasPending = false;
	std::chrono::steady_clock::time_point lastAddTime;

	void commitPending()
	{
		if (hasPending)
		{
			computeAndSaveOperation();
			hasPending = false;
		}
	}

	// Optimized diff computation
	void computeAndSaveOperation()
	{
		// Compute diff between initial and final states
		const std::string &oldStr = pendingInitialContent;
		const std::string &newStr = pendingFinalContent;

		// Find first difference
		size_t start = 0;
		size_t minLen = std::min(oldStr.length(), newStr.length());

		while (start < minLen && oldStr[start] == newStr[start])
		{
			start++;
		}

		// Find last difference
		size_t oldEnd = oldStr.length();
		size_t newEnd = newStr.length();

		while (oldEnd > start && newEnd > start &&
			   oldStr[oldEnd - 1] == newStr[newEnd - 1])
		{
			oldEnd--;
			newEnd--;
		}

		// Create operation
		Operation op{static_cast<int>(start),
					 oldStr.substr(start, oldEnd - start),
					 newStr.substr(start, newEnd - start),
					 pendingInitialCursor,
					 pendingFinalCursor};

		// Save operation and update state
		undoStack.push_back(op);
		if (undoStack.size() > maxStackSize)
		{
			undoStack.erase(undoStack.begin());
		}
		lastCommittedState = pendingFinalContent;
	}
};