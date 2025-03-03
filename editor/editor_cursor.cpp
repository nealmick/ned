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

// Private helper that doesn't reference Editor directly to avoid circular dependency
int EditorCursor::getLineFromPosition(const std::vector<int> &line_starts, int pos) { return EditorUtils::GetLineFromPosition(line_starts, pos); }

void EditorCursor::cursorLeft(EditorState &state)
{
    if (state.cursor_pos > 0) {
        state.cursor_pos--;

        // Update preferred column when moving horizontally
        int current_line = getLineFromPosition(state.line_starts, state.cursor_pos);
        state.preferred_column = state.cursor_pos - state.line_starts[current_line];
    }
}

void EditorCursor::cursorRight(const std::string &text, EditorState &state)
{
    if (state.cursor_pos < text.size()) {
        state.cursor_pos++;

        // Update preferred column when moving horizontally
        int current_line = getLineFromPosition(state.line_starts, state.cursor_pos);
        state.preferred_column = state.cursor_pos - state.line_starts[current_line];
    }
}

void EditorCursor::cursorUp(const std::string &text, EditorState &state, float line_height, float window_height)
{
    int current_line = getLineFromPosition(state.line_starts, state.cursor_pos);
    if (current_line > 0) {
        // Calculate current column only if preferred_column hasn't been set yet
        if (state.preferred_column == 0) {
            state.preferred_column = state.cursor_pos - state.line_starts[current_line];
        }

        // Use the preferred column instead of current column
        int target_line = current_line - 1;
        int new_line_start = state.line_starts[target_line];
        int new_line_end = state.line_starts[current_line] - 1;

        // Try to place cursor at preferred column, but don't exceed line length
        state.cursor_pos = std::min(new_line_start + state.preferred_column, new_line_end);

        // Update scroll position through EditorScroll
        ImVec2 currentPos = gEditorScroll.getScrollPosition();
        gEditorScroll.setScrollPosition(ImVec2(currentPos.x, std::max(0.0f, currentPos.y - line_height)));
    }
}

void EditorCursor::cursorDown(const std::string &text, EditorState &state, float line_height, float window_height)
{
    int current_line = getLineFromPosition(state.line_starts, state.cursor_pos);
    if (current_line < state.line_starts.size() - 1) {
        // Calculate current column only if preferred_column hasn't been set yet
        if (state.preferred_column == 0) {
            state.preferred_column = state.cursor_pos - state.line_starts[current_line];
        }

        // Use the preferred column instead of current column
        int target_line = current_line + 1;
        int new_line_start = state.line_starts[target_line];
        int new_line_end = (target_line + 1 < state.line_starts.size()) ? state.line_starts[target_line + 1] - 1 : text.size();

        // Try to place cursor at preferred column, but don't exceed line length
        state.cursor_pos = std::min(new_line_start + state.preferred_column, new_line_end);

        // Update scroll position through EditorScroll
        ImVec2 currentPos = gEditorScroll.getScrollPosition();
        gEditorScroll.setScrollPosition(ImVec2(currentPos.x, currentPos.y + line_height));
    }
}

void EditorCursor::moveWordForward(const std::string &text, EditorState &state)
{
    size_t pos = state.cursor_pos;
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

    state.cursor_pos = pos;
    state.selection_start = state.selection_end = pos;
}

void EditorCursor::moveWordBackward(const std::string &text, EditorState &state)
{
    size_t pos = state.cursor_pos;

    // If at the beginning, wrap around to the end
    if (pos == 0)
        pos = text.length();

    // Move back to the start of the current word
    while (pos > 0 && !std::isalnum(text[pos - 1]))
        pos--;
    while (pos > 0 && std::isalnum(text[pos - 1]))
        pos--;

    state.cursor_pos = pos;
    state.selection_start = state.selection_end = pos;
}

void EditorCursor::moveCursorVertically(std::string &text, EditorState &state, int line_delta)
{
    int current_line = getLineFromPosition(state.line_starts, state.cursor_pos);
    int target_line = std::max(0, std::min(static_cast<int>(state.line_starts.size()) - 1, current_line + line_delta));

    // Calculate current column only if preferred_column hasn't been set yet
    if (state.preferred_column == 0) {
        state.preferred_column = state.cursor_pos - state.line_starts[current_line];
    }

    // Set the new cursor position using preferred column
    int new_line_start = state.line_starts[target_line];
    int new_line_end = (target_line + 1 < state.line_starts.size()) ? state.line_starts[target_line + 1] - 1 : text.size();

    // Try to maintain the preferred column position, but don't go past the end of the line
    state.cursor_pos = std::min(new_line_start + state.preferred_column, new_line_end);
}

float EditorCursor::getCursorYPosition(const EditorState &state, float line_height)
{
    int cursor_line = getLineFromPosition(state.line_starts, state.cursor_pos);
    return cursor_line * line_height;
}

float EditorCursor::calculateCursorXPosition(const ImVec2 &text_pos, const std::string &text, int cursor_pos)
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

    // Update the current line - we'll use our own getLineFromPosition method
    state.current_line = getLineFromPosition(state.line_starts, state.cursor_pos);

    // Start a new selection only if Shift is pressed and we're not already selecting
    if (shift_pressed && !state.is_selecting) {
        state.is_selecting = true;
        state.selection_start = state.cursor_pos;
    }

    // Clear selection only if a movement key is pressed without Shift
    if (!shift_pressed && (ImGui::IsKeyPressed(ImGuiKey_UpArrow) || ImGui::IsKeyPressed(ImGuiKey_DownArrow) || ImGui::IsKeyPressed(ImGuiKey_LeftArrow) || ImGui::IsKeyPressed(ImGuiKey_RightArrow))) {
        state.is_selecting = false;
        state.selection_start = state.selection_end = state.cursor_pos;
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
    if (state.is_selecting) {
        state.selection_end = state.cursor_pos;
    }

    // Use the EditorScroll class to handle scroll adjustments
    gEditorScroll.handleCursorMovementScroll(text_pos, text, state, line_height, window_height, window_width);
}

void EditorCursor::processCursorJump(std::string &text, EditorState &state, CursorVisibility &ensure_cursor_visible)
{
    if (ImGui::IsKeyPressed(ImGuiKey_LeftArrow)) {
        int current_line = getLineFromPosition(state.line_starts, state.cursor_pos);
        state.cursor_pos = state.line_starts[current_line] + 1;
        ensure_cursor_visible.horizontal = true;
    } else if (ImGui::IsKeyPressed(ImGuiKey_RightArrow)) {
        int current_line = getLineFromPosition(state.line_starts, state.cursor_pos);
        int next_line = current_line + 1;
        if (next_line < state.line_starts.size()) {
            state.cursor_pos = state.line_starts[next_line] - 2; // Position before the newline
        } else {
            state.cursor_pos = text.size();
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