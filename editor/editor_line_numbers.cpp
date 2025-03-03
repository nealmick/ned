#include "editor_line_numbers.h"
#include "../util/settings.h"
#include <algorithm>
#include <cmath>

// Global instance
EditorLineNumbers gEditorLineNumbers;

void EditorLineNumbers::renderLineNumbers(const ImVec2 &pos, float line_number_width, float line_height, int num_lines, float scroll_y, float window_height, const EditorState &editor_state, float blink_time)
{
    static char line_number_buffer[LINE_NUMBER_BUFFER_SIZE];
    ImDrawList *draw_list = ImGui::GetWindowDrawList();

    // Calculate visible line range
    int start_line = static_cast<int>(scroll_y / line_height);
    int end_line = std::min(num_lines, static_cast<int>((scroll_y + window_height) / line_height) + 1);

    // Pre-calculate rainbow color if needed
    bool rainbow_mode = gSettings.getRainbowMode();
    ImU32 rainbow_color = CURRENT_LINE_COLOR;
    if (rainbow_mode) {
        rainbow_color = calculateRainbowColor(blink_time);
    }

    // Calculate selection lines
    int selection_start_line = 0;
    int selection_end_line = 0;
    calculateSelectionLines(editor_state, selection_start_line, selection_end_line);

    // Render each visible line number
    for (int i = start_line; i < end_line; i++) {
        // Calculate vertical position
        float y_pos = pos.y + (i * line_height) - scroll_y;

        // Format line number text
        snprintf(line_number_buffer, sizeof(line_number_buffer), "%d", i + 1);

        // Determine color based on selection and current line
        ImU32 line_number_color = determineLineNumberColor(i, editor_state.current_line, selection_start_line, selection_end_line, editor_state.is_selecting, rainbow_mode, rainbow_color);

        // Calculate position for right-aligned text
        float text_width = ImGui::CalcTextSize(line_number_buffer).x;
        float x_pos = calculateTextRightAlignedPosition(line_number_buffer, line_number_width);

        // Draw the line number
        draw_list->AddText(ImVec2(x_pos, y_pos), line_number_color, line_number_buffer);
    }
}

ImVec2 EditorLineNumbers::createLineNumbersPanel(float line_number_width, float editor_top_margin)
{
    ImGui::BeginGroup();
    ImGui::PushStyleVar(ImGuiStyleVar_ChildBorderSize, 0.0f);
    ImGui::BeginChild("LineNumbers", ImVec2(line_number_width, ImGui::GetContentRegionAvail().y), false, ImGuiWindowFlags_NoScrollbar);
    ImVec2 line_numbers_pos = ImGui::GetCursorScreenPos();
    line_numbers_pos.y += editor_top_margin;
    ImGui::EndChild();
    ImGui::PopStyleVar();
    ImGui::SameLine();
    return line_numbers_pos;
}

float EditorLineNumbers::calculateTextRightAlignedPosition(const char *text, float line_number_width, float right_margin) const
{
    float text_width = ImGui::CalcTextSize(text).x;
    return ImGui::GetCursorScreenPos().x + line_number_width - text_width - right_margin;
}

ImU32 EditorLineNumbers::calculateRainbowColor(float blink_time) const
{
    // Update color less frequently for better performance
    static float last_update_time = 0.0f;
    static ImU32 last_rainbow_color = CURRENT_LINE_COLOR;

    if (blink_time - last_update_time > 0.05f) {         // Update every 50ms
        ImVec4 rainbow = EditorUtils::GetRainbowColor(); // Use the synchronized version
        last_rainbow_color = ImGui::ColorConvertFloat4ToU32(rainbow);
        last_update_time = blink_time;
    }

    return last_rainbow_color;
}

ImU32 EditorLineNumbers::determineLineNumberColor(int line_index, int current_line, int selection_start_line, int selection_end_line, bool is_selecting, bool rainbow_mode, ImU32 rainbow_color) const
{
    // Selected line takes precedence
    if (line_index >= selection_start_line && line_index < selection_end_line && is_selecting) {
        return SELECTED_LINE_COLOR;
    }
    // Current line comes next
    else if (line_index == current_line) {
        return rainbow_mode ? rainbow_color : CURRENT_LINE_COLOR;
    }
    // Default line color
    else {
        return DEFAULT_LINE_NUMBER_COLOR;
    }
}

void EditorLineNumbers::calculateSelectionLines(const EditorState &editor_state, int &selection_start_line, int &selection_end_line) const
{
    // Get selection range accounting for selection direction
    int selection_start = std::min(editor_state.selection_start, editor_state.selection_end);
    int selection_end = std::max(editor_state.selection_start, editor_state.selection_end);

    // Convert character positions to line indices
    selection_start_line = std::lower_bound(editor_state.line_starts.begin(), editor_state.line_starts.end(), selection_start) - editor_state.line_starts.begin();

    selection_end_line = std::lower_bound(editor_state.line_starts.begin(), editor_state.line_starts.end(), selection_end) - editor_state.line_starts.begin();
}