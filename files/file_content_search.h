#pragma once

#include "../editor/editor.h"
#include "../editor/editor_cursor.h"
#include "imgui.h"
#include <string>

class FileContentSearch
{
  public:
	FileContentSearch();

	// Search operations
	void findNext(bool ignoreCase = false);
	void findPrevious(bool ignoreCase = false);

	// UI functions
	void renderFindBox();
	void handleFindBoxActivation();

	std::vector<size_t> findAllOccurrences(bool ignoreCase);
	bool needsInputUnblock;
	int unblockDelayFrames;

  private:
	// Core members
	std::string findText; // the string we are searching for...

	size_t lastFoundPos = std::string::npos;
	bool findBoxShouldFocus = false;

	// Helper functions
	std::string toLower(const std::string &s);
	void handleFindBoxKeyboardShortcuts(bool ignoreCaseCheckbox);

	// UI Constants
	struct Style
	{
		static constexpr float FRAME_ROUNDING = 6.0f;
		static constexpr float BORDER_SIZE = 1.0f;
		static constexpr ImVec4 FRAME_BG = ImVec4(0.2f, 0.2f, 0.2f, 1.0f);
		static constexpr ImVec4 BORDER_COLOR = ImVec4(0.3f, 0.3f, 0.3f, 1.0f);
	};
};

extern FileContentSearch gFileContentSearch;