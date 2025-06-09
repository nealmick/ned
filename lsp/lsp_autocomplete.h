// lsp_autocomplete.h
#pragma once

#include "../editor/editor.h"
#include "../editor/editor_cursor.h"
#include "../lib/json.hpp"
#include "imgui.h"
#include <string>
#include <vector>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <thread>

using json = nlohmann::json;

struct CompletionRequest {
	std::string filePath;
	int line;
	int character;
	int requestId;
};

struct CompletionDisplayItem
{
	std::string label;
	std::string detail;
	std::string insertText;
	std::string sortText;  // Added for LSP server's sorting
	int kind;
	int startLine = -1;
	int startChar = -1;
	int endLine = -1;
	int endChar = -1;
	float score = 0.0f;  // Add score field for sorting
};

class LSPAutocomplete
{
  public:
	LSPAutocomplete();
	~LSPAutocomplete();

	void requestCompletion(const std::string &filePath, int line, int character);
	void renderCompletions();
	void processPendingResponses(); // New method to process responses in main thread

	bool showCompletions = false;
	int selectedCompletionIndex = 0;
	ImVec2 completionPopupPos = ImVec2(0, 0);

	bool blockTab = false;
	bool blockEnter = false;

  private:
	std::vector<CompletionDisplayItem> currentCompletionItems;
	std::queue<CompletionRequest> requestQueue;
	std::mutex queueMutex;
	std::condition_variable queueCondition;
	std::atomic<bool> shouldStop{false};
	std::thread workerThread;

	// --- Private Helper Functions for Rendering ---
	bool shouldRender();
	bool handleInputAndCheckClose(); // Returns true if the window should close
	void calculateWindowGeometry(ImVec2 &outWindowSize, ImVec2 &outSafePos);
	void applyStyling();
	void renderCompletionListItems();
	bool handleClickOutside();
	void finalizeRenderState();

	// State tracking for focus/frame logic
	static bool wasShowingLastFrame;

	// requesting logic
	std::string formCompletionRequest(int requestId, const std::string &filePath, int line, int character);
	bool processResponse(const std::string &response, int requestId);
	void parseCompletionResult(const json &result);
	void updatePopupPosition();
	void workerFunction(); // New method for background thread

	void insertText(int row_start, int col__start, int row_end, int col__end, std::string text);

	std::pair<int, int> getLineAndCharFromIndex(int index) const
	{
		if (editor_state.editor_content_lines.empty() || index < 0)
			return {0, 0};

		// Find the line containing this index
		int line = 0;
		for (; line < static_cast<int>(editor_state.editor_content_lines.size()); ++line)
		{
			if (editor_state.editor_content_lines[line] > index)
			{
				break;
			}
		}
		line = std::max(0, line - 1);

		// Calculate character position within line
		int line_start = editor_state.editor_content_lines[line];
		return {line, index - line_start};
	}
};

// Global instance
extern LSPAutocomplete gLSPAutocomplete;
