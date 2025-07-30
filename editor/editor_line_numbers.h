#pragma once
#include "editor_types.h"
#include "editor_utils.h"
#include "imgui.h"
#include <string>
#include <vector>

// Forward declarations
class Editor;
extern Editor gEditor;

// Constants for line number colors
constexpr ImU32 DEFAULT_LINE_NUMBER_COLOR = IM_COL32(128, 128, 128, 150);
constexpr ImU32 CURRENT_LINE_COLOR = IM_COL32(255, 255, 255, 255);
constexpr int LINE_NUMBER_BUFFER_SIZE = 32;

class EditorLineNumbers
{
  public:
	EditorLineNumbers() = default;
	~EditorLineNumbers() = default;

	// Main rendering function
	void renderLineNumbers();

	// Layout functions
	ImVec2 createLineNumbersPanel();

	void setCurrentFilePath(const std::string &filepath);

  private:
	// Color calculation helpers
	ImU32 calculateRainbowColor(float blink_time) const;
	ImU32 determineLineNumberColor(int line_index,
								   int current_line,
								   int selection_start_line,
								   int selection_end_line,
								   bool is_selecting,
								   bool rainbow_mode,
								   ImU32 rainbow_color) const;

	// Selection helpers
	void calculateSelectionLines(int &selection_start_line, int &selection_end_line);

	// Layout/positioning helpers
	float calculateTextRightAlignedPosition(const char *text,
											float line_number_width,
											float right_margin = 4.0f) const;
	float calculateRequiredLineNumberWidth() const;

	std::string current_filepath;
};

// Global instance
extern EditorLineNumbers gEditorLineNumbers;