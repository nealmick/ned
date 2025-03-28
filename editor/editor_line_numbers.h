#pragma once
#include "editor_types.h"
#include "editor_utils.h"
#include "imgui.h"
#include <string>
#include <vector>

// Forward declarations
class Editor;
extern Editor gEditor;

class EditorLineNumbers
{
  public:
    EditorLineNumbers() = default;
    ~EditorLineNumbers() = default;

    // Main rendering function
    void renderLineNumbers();

    // Layout functions
    ImVec2 createLineNumbersPanel();

  private:
    // Color calculation helpers
    ImU32 calculateRainbowColor(float blink_time) const;
    ImU32 determineLineNumberColor(int line_index, int current_line, int selection_start_line, int selection_end_line, bool is_selecting, bool rainbow_mode, ImU32 rainbow_color) const;

    // Selection helpers
    void calculateSelectionLines(int &selection_start_line, int &selection_end_line);

    // Layout/positioning helpers
    float calculateTextRightAlignedPosition(const char *text, float line_number_width, float right_margin = 8.0f) const;

    // Constants
    const ImU32 DEFAULT_LINE_NUMBER_COLOR = IM_COL32(128, 128, 128, 255);
    const ImU32 CURRENT_LINE_COLOR = IM_COL32(255, 255, 255, 255);
    const ImU32 SELECTED_LINE_COLOR = IM_COL32(0, 40, 255, 200); // Neon blue color
    static constexpr size_t LINE_NUMBER_BUFFER_SIZE = 16;
};

// Global instance
extern EditorLineNumbers gEditorLineNumbers;