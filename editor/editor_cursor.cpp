#include "editor_cursor.h"
#include "../util/settings.h"
#include "editor_utils.h"
#include <algorithm>
#include <cctype> // For std::isalnum, std::isspace

// Global instance
EditorCursor gEditorCursor;

EditorCursor::EditorCursor() {}

void EditorCursor::renderCursor(ImDrawList *draw_list, const ImVec2 &cursor_screen_pos, float line_height, float blink_time)
{
    float blink_alpha = (sinf(blink_time * 4.0f) + 1.0f) * 0.5f; // Blink frequency
    ImU32 cursor_color;
    bool rainbow_mode = gSettings.getRainbowMode(); // Get setting here

    if (rainbow_mode) {
        // Use the shared rainbow color function - no need to pass blink_time
        ImVec4 rainbow = EditorUtils::GetRainbowColor();
        cursor_color = ImGui::ColorConvertFloat4ToU32(rainbow);
    } else {
        cursor_color = IM_COL32(255, 255, 255, (int)(blink_alpha * 255));
    }

    draw_list->AddLine(cursor_screen_pos, ImVec2(cursor_screen_pos.x, cursor_screen_pos.y + line_height - 1), cursor_color);
}

// Cursor time management
void EditorCursor::updateBlinkTime(EditorState &state, float deltaTime) { state.cursor_blink_time += deltaTime; }

// Implementation of helper functions
int EditorCursor::calculateVisualColumn(const std::string &text, int line_start, int cursor_pos)
{
    const int TAB_WIDTH = 4; // Tab width in spaces
    int visual_column = 0;

    for (int i = line_start; i < cursor_pos && i < text.length(); i++) {
        if (text[i] == '\t') {
            // Calculate next tab stop (tabs align to positions divisible by TAB_WIDTH)
            visual_column = ((visual_column / TAB_WIDTH) + 1) * TAB_WIDTH;
        } else {
            visual_column++;
        }
    }

    return visual_column;
}

int EditorCursor::findPositionFromVisualColumn(const std::string &text, int line_start, int line_end, int visual_column)
{
    const int TAB_WIDTH = 4; // Tab width in spaces
    int current_visual_column = 0;
    int pos = line_start;

    while (pos < line_end && current_visual_column < visual_column && pos < text.length()) {
        if (text[pos] == '\t') {
            // Calculate next tab stop
            int next_tab_stop = ((current_visual_column / TAB_WIDTH) + 1) * TAB_WIDTH;

            // If going to or past the next tab stop would exceed our target,
            // we've found the closest position
            if (next_tab_stop > visual_column) {
                break;
            }

            current_visual_column = next_tab_stop;
        } else {
            current_visual_column++;
        }

        pos++;
    }

    return pos;
}

void EditorCursor::cursorLeft(EditorState &state)
{
    if (state.cursor_index > 0) {
        state.cursor_index--;

        // We can't update the preferred visual column here since we don't have access to text
        // It will be updated next time cursor moves vertically
        int current_line = EditorUtils::GetLineFromPosition(state.editor_content_lines, state.cursor_index);
        state.cursor_column_prefered = state.cursor_index - state.editor_content_lines[current_line];
    }
}

// Fix for cursorRight
void EditorCursor::cursorRight(const std::string &text, EditorState &state)
{
    if (state.cursor_index < text.size()) {
        state.cursor_index++;

        // Update preferred column using visual positions
        int current_line = EditorUtils::GetLineFromPosition(state.editor_content_lines, state.cursor_index);
        int line_start = state.editor_content_lines[current_line];
        state.cursor_column_prefered = calculateVisualColumn(text, line_start, state.cursor_index);
    }
}

void EditorCursor::cursorUp(const std::string &text, EditorState &state, float line_height, float window_height)
{
    int current_line = EditorUtils::GetLineFromPosition(state.editor_content_lines, state.cursor_index);
    if (current_line > 0) {
        // Calculate current visual column if preferred_column hasn't been set yet
        if (state.cursor_column_prefered == 0) {
            int line_start = state.editor_content_lines[current_line];
            state.cursor_column_prefered = calculateVisualColumn(text, line_start, state.cursor_index);
        }

        // Target the previous line
        int target_line = current_line - 1;
        int new_line_start = state.editor_content_lines[target_line];
        int new_line_end = state.editor_content_lines[current_line] - 1;

        // Find position in new line that corresponds to our visual column
        state.cursor_index = findPositionFromVisualColumn(text, new_line_start, new_line_end, state.cursor_column_prefered);

        // Update scroll position through EditorScroll
        ImVec2 currentPos = gEditorScroll.getScrollPosition();
        gEditorScroll.setScrollPosition(ImVec2(currentPos.x, std::max(0.0f, currentPos.y - line_height)));
    }
}

void EditorCursor::cursorDown(const std::string &text, EditorState &state, float line_height, float window_height)
{
    int current_line = EditorUtils::GetLineFromPosition(state.editor_content_lines, state.cursor_index);
    if (current_line < state.editor_content_lines.size() - 1) {
        // Calculate current visual column if preferred_column hasn't been set yet
        if (state.cursor_column_prefered == 0) {
            int line_start = state.editor_content_lines[current_line];
            state.cursor_column_prefered = calculateVisualColumn(text, line_start, state.cursor_index);
        }

        // Target the next line
        int target_line = current_line + 1;
        int new_line_start = state.editor_content_lines[target_line];
        int new_line_end = (target_line + 1 < state.editor_content_lines.size()) ? state.editor_content_lines[target_line + 1] - 1 : text.size();

        // Find position in new line that corresponds to our visual column
        state.cursor_index = findPositionFromVisualColumn(text, new_line_start, new_line_end, state.cursor_column_prefered);

        // Update scroll position through EditorScroll
        ImVec2 currentPos = gEditorScroll.getScrollPosition();
        gEditorScroll.setScrollPosition(ImVec2(currentPos.x, currentPos.y + line_height));
    }
}

void EditorCursor::moveCursorVertically(std::string &text, EditorState &state, int line_delta)
{
    int current_line = EditorUtils::GetLineFromPosition(state.editor_content_lines, state.cursor_index);
    int target_line = std::max(0, std::min(static_cast<int>(state.editor_content_lines.size()) - 1, current_line + line_delta));

    // Calculate current visual column if preferred_column hasn't been set yet
    if (state.cursor_column_prefered == 0) {
        int line_start = state.editor_content_lines[current_line];
        state.cursor_column_prefered = calculateVisualColumn(text, line_start, state.cursor_index);
    }

    // Set the new cursor position using preferred visual column
    int new_line_start = state.editor_content_lines[target_line];
    int new_line_end = (target_line + 1 < state.editor_content_lines.size()) ? state.editor_content_lines[target_line + 1] - 1 : text.size();

    // Find position in new line that corresponds to our visual column
    state.cursor_index = findPositionFromVisualColumn(text, new_line_start, new_line_end, state.cursor_column_prefered);
}

void EditorCursor::moveWordForward(const std::string &text, EditorState &state)
{
    size_t pos = state.cursor_index;
    size_t len = text.length();

    // Skip current word
    while (pos < len && std::isalnum(text[pos]))
        pos++;
    // Skip spaces
    while (pos < len && std::isspace(text[pos]))
        pos++;
    // Find start of next word
    while (pos < len && !std::isalnum(text[pos]) && !std::isspace(text[pos]))
        pos++;

    // If we've reached the end, wrap around to the beginning
    if (pos == len)
        pos = 0;

    state.cursor_index = pos;
    state.selection_start = state.selection_end = pos;
}

void EditorCursor::moveWordBackward(const std::string &text, EditorState &state)
{
    size_t pos = state.cursor_index;

    // If at the beginning, wrap around to the end
    if (pos == 0)
        pos = text.length();

    // Move back to the start of the current word
    while (pos > 0 && !std::isalnum(text[pos - 1]))
        pos--;
    while (pos > 0 && std::isalnum(text[pos - 1]))
        pos--;

    state.cursor_index = pos;
    state.selection_start = state.selection_end = pos;
}

float EditorCursor::getCursorYPosition(const EditorState &state, float line_height)
{
    int cursor_line = EditorUtils::GetLineFromPosition(state.editor_content_lines, state.cursor_index);
    return cursor_line * line_height;
}

float EditorCursor::getCursorXPosition(const ImVec2 &text_pos, const std::string &text, int cursor_pos)
{
    float x = text_pos.x;
    for (int i = 0; i < cursor_pos; i++) {
        if (text[i] == '\n') {
            x = text_pos.x;
        } else {
            x += ImGui::CalcTextSize(&text[i], &text[i + 1]).x;
        }
    }
    return x;
}

void EditorCursor::handleCursorMovement(const std::string &text, EditorState &state, const ImVec2 &text_pos, float line_height, float window_height, float window_width)
{
    bool shift_pressed = ImGui::GetIO().KeyShift;

    // Start a new selection only if Shift is pressed and we're not already selecting
    if (shift_pressed && !state.selection_active) {
        state.selection_active = true;
        state.selection_start = state.cursor_index;
    }

    // Clear selection only if a movement key is pressed without Shift
    if (!shift_pressed && (ImGui::IsKeyPressed(ImGuiKey_UpArrow) || ImGui::IsKeyPressed(ImGuiKey_DownArrow) || ImGui::IsKeyPressed(ImGuiKey_LeftArrow) || ImGui::IsKeyPressed(ImGuiKey_RightArrow))) {
        state.selection_active = false;
        state.selection_start = state.selection_end = state.cursor_index;
    }

    // Handle cursor movement based on arrow keys
    if (ImGui::IsKeyPressed(ImGuiKey_UpArrow)) {
        cursorUp(text, state, line_height, window_height);
    }
    if (ImGui::IsKeyPressed(ImGuiKey_DownArrow)) {
        cursorDown(text, state, line_height, window_height);
    }
    if (ImGui::IsKeyPressed(ImGuiKey_LeftArrow)) {
        cursorLeft(state);
    }
    if (ImGui::IsKeyPressed(ImGuiKey_RightArrow)) {
        cursorRight(text, state);
    }

    // Update selection end if we're in selection mode
    if (state.selection_active) {
        state.selection_end = state.cursor_index;
    }

    // Use the EditorScroll class to handle scroll adjustments
    gEditorScroll.handleCursorMovementScroll(text_pos, text, state, line_height, window_height, window_width);
}

void EditorCursor::processCursorJump(std::string &text, EditorState &state, CursorVisibility &ensure_cursor_visible)
{
    if (ImGui::IsKeyPressed(ImGuiKey_LeftArrow)) {
        int current_line = EditorUtils::GetLineFromPosition(state.editor_content_lines, state.cursor_index);
        state.cursor_index = state.editor_content_lines[current_line] + 1;
        ensure_cursor_visible.horizontal = true;
    } else if (ImGui::IsKeyPressed(ImGuiKey_RightArrow)) {
        int current_line = EditorUtils::GetLineFromPosition(state.editor_content_lines, state.cursor_index);
        int next_line = current_line + 1;
        if (next_line < state.editor_content_lines.size()) {
            state.cursor_index = state.editor_content_lines[next_line] - 2; // Position before the newline
        } else {
            state.cursor_index = text.size();
        }
        ensure_cursor_visible.horizontal = true;
    } else if (ImGui::IsKeyPressed(ImGuiKey_UpArrow)) {
        moveCursorVertically(text, state, -5);
        ensure_cursor_visible.vertical = true;
        ensure_cursor_visible.horizontal = true;
    } else if (ImGui::IsKeyPressed(ImGuiKey_DownArrow)) {
        moveCursorVertically(text, state, 5);
        ensure_cursor_visible.vertical = true;
        ensure_cursor_visible.horizontal = true;
    }
}

void EditorCursor::processWordMovement(std::string &text, EditorState &state, CursorVisibility &ensure_cursor_visible, bool shift_pressed)
{
    if (ImGui::IsKeyPressed(ImGuiKey_W)) {
        if (shift_pressed) {
            moveWordBackward(text, state);
        } else {
            moveWordForward(text, state);
        }
        ensure_cursor_visible.horizontal = true;
        ensure_cursor_visible.vertical = true;
    }
}
