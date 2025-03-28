#include "editor_cursor.h"
#include "../util/settings.h"
#include "editor.h"
#include "editor_utils.h"
#include <algorithm>
#include <cctype> // For std::isalnum, std::isspace

// Global instance
EditorCursor gEditorCursor;

EditorCursor::EditorCursor() {}

void EditorCursor::renderCursor()
{
    int cursor_line = gEditor.getLineFromPos(editor_state.cursor_index);
    float cursor_x = gEditorCursor.getCursorXPosition(editor_state.text_pos, editor_state.fileContent, editor_state.cursor_index);

    ImVec2 cursor_screen_pos = editor_state.text_pos;
    cursor_screen_pos.x = cursor_x;
    cursor_screen_pos.y = editor_state.text_pos.y + cursor_line * editor_state.line_height;

    float blink_alpha = (sinf(editor_state.cursor_blink_time * 4.0f) + 1.0f) * 0.5f; // Blink frequency
    ImU32 cursor_color;
    bool rainbow_mode = gSettings.getRainbowMode(); // Get setting here

    if (rainbow_mode) {
        // Use the shared rainbow color function - no need to pass blink_time
        ImVec4 rainbow = EditorUtils::GetRainbowColor();
        cursor_color = ImGui::ColorConvertFloat4ToU32(rainbow);
    } else {
        cursor_color = IM_COL32(255, 255, 255, (int)(blink_alpha * 255));
    }

    ImGui::GetWindowDrawList()->AddLine(cursor_screen_pos, ImVec2(cursor_screen_pos.x, cursor_screen_pos.y + editor_state.line_height - 1), cursor_color);
}

// Cursor time management
void EditorCursor::updateBlinkTime() { editor_state.cursor_blink_time += ImGui::GetIO().DeltaTime; }

// Implementation of helper functions
void EditorCursor::calculateVisualColumn()
{
    const int TAB_WIDTH = 4; // Tab width in spaces
    int visual_column = 0;
    int current_line = EditorUtils::GetLineFromPosition(editor_state.editor_content_lines, editor_state.cursor_index);
    int line_start = editor_state.editor_content_lines[current_line];

    for (int i = line_start; i < editor_state.cursor_index && i < editor_state.fileContent.length(); i++) {
        if (editor_state.fileContent[i] == '\t') {
            // Calculate next tab stop (tabs align to positions divisible by TAB_WIDTH)
            visual_column = ((visual_column / TAB_WIDTH) + 1) * TAB_WIDTH;
        } else {
            visual_column++;
        }
    }
    editor_state.cursor_column_prefered = visual_column;
}

void EditorCursor::findPositionFromVisualColumn(int line_start, int line_end)
{
    const int TAB_WIDTH = 4; // Tab width in spaces
    int current_visual_column = 0;
    int pos = line_start;

    while (pos < line_end && current_visual_column < editor_state.cursor_column_prefered && pos < editor_state.fileContent.length()) {
        if (editor_state.fileContent[pos] == '\t') {
            // Calculate next tab stop
            int next_tab_stop = ((current_visual_column / TAB_WIDTH) + 1) * TAB_WIDTH;

            // If going to or past the next tab stop would exceed our target,
            // we've found the closest position
            if (next_tab_stop > editor_state.cursor_column_prefered) {
                break;
            }

            current_visual_column = next_tab_stop;
        } else {
            current_visual_column++;
        }

        pos++;
    }
    editor_state.cursor_index = pos;
}

void EditorCursor::cursorLeft()
{
    if (editor_state.cursor_index > 0) {
        editor_state.cursor_index--;

        // We can't update the preferred visual column here since we don't have access to text
        // It will be updated next time cursor moves vertically
        int current_line = EditorUtils::GetLineFromPosition(editor_state.editor_content_lines, editor_state.cursor_index);
        editor_state.cursor_column_prefered = editor_state.cursor_index - editor_state.editor_content_lines[current_line];
    }
}

// Fix for cursorRight
void EditorCursor::cursorRight()
{
    if (editor_state.cursor_index < editor_state.fileContent.size()) {
        editor_state.cursor_index++;
        calculateVisualColumn();
    }
}

void EditorCursor::cursorUp()
{
    int current_line = EditorUtils::GetLineFromPosition(editor_state.editor_content_lines, editor_state.cursor_index);
    if (current_line > 0) {
        // Calculate current visual column if preferred_column hasn't been set yet
        if (editor_state.cursor_column_prefered == 0) {
            calculateVisualColumn();
        }

        // Target the previous line
        int target_line = current_line - 1;
        int new_line_start = editor_state.editor_content_lines[target_line];
        int new_line_end = editor_state.editor_content_lines[current_line] - 1;

        findPositionFromVisualColumn(new_line_start, new_line_end);

        // Update scroll position through EditorScroll
        ImVec2 currentPos = gEditorScroll.getScrollPosition();
        gEditorScroll.setScrollPosition(ImVec2(currentPos.x, std::max(0.0f, currentPos.y - editor_state.line_height)));
    }
}

void EditorCursor::cursorDown()
{
    int current_line = EditorUtils::GetLineFromPosition(editor_state.editor_content_lines, editor_state.cursor_index);
    if (current_line < editor_state.editor_content_lines.size() - 1) {
        // Calculate current visual column if preferred_column hasn't been set yet
        if (editor_state.cursor_column_prefered == 0) {
            calculateVisualColumn();
        }

        // Target the next line
        int target_line = current_line + 1;
        int new_line_start = editor_state.editor_content_lines[target_line];
        int new_line_end = (target_line + 1 < editor_state.editor_content_lines.size()) ? editor_state.editor_content_lines[target_line + 1] - 1 : editor_state.fileContent.size();

        // Find position in new line that corresponds to our visual column
        findPositionFromVisualColumn(new_line_start, new_line_end);

        // Update scroll position through EditorScroll
        ImVec2 currentPos = gEditorScroll.getScrollPosition();
        gEditorScroll.setScrollPosition(ImVec2(currentPos.x, currentPos.y + editor_state.line_height));
    }
}

void EditorCursor::moveCursorVertically(std::string &text, int line_delta)
{
    int current_line = EditorUtils::GetLineFromPosition(editor_state.editor_content_lines, editor_state.cursor_index);
    int target_line = std::max(0, std::min(static_cast<int>(editor_state.editor_content_lines.size()) - 1, current_line + line_delta));

    // Calculate current visual column if preferred_column hasn't been set yet
    if (editor_state.cursor_column_prefered == 0) {
        calculateVisualColumn();
    }

    // Set the new cursor position using preferred visual column
    int new_line_start = editor_state.editor_content_lines[target_line];
    int new_line_end = (target_line + 1 < editor_state.editor_content_lines.size()) ? editor_state.editor_content_lines[target_line + 1] - 1 : text.size();

    // Find position in new line that corresponds to our visual column
    findPositionFromVisualColumn(new_line_start, new_line_end);
}

void EditorCursor::moveWordForward(const std::string &text)
{
    size_t pos = editor_state.cursor_index;
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

    editor_state.cursor_index = pos;
    editor_state.selection_start = editor_state.selection_end = pos;
}

void EditorCursor::moveWordBackward(const std::string &text)
{
    size_t pos = editor_state.cursor_index;

    // If at the beginning, wrap around to the end
    if (pos == 0)
        pos = text.length();

    // Move back to the start of the current word
    while (pos > 0 && !std::isalnum(text[pos - 1]))
        pos--;
    while (pos > 0 && std::isalnum(text[pos - 1]))
        pos--;

    editor_state.cursor_index = pos;
    editor_state.selection_start = editor_state.selection_end = pos;
}

float EditorCursor::getCursorYPosition(float line_height)
{
    int cursor_line = EditorUtils::GetLineFromPosition(editor_state.editor_content_lines, editor_state.cursor_index);
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

void EditorCursor::handleCursorMovement(const std::string &text, const ImVec2 &text_pos, float line_height, float window_height, float window_width)
{
    bool shift_pressed = ImGui::GetIO().KeyShift;

    // Start a new selection only if Shift is pressed and we're not already selecting
    if (shift_pressed && !editor_state.selection_active) {
        editor_state.selection_active = true;
        editor_state.selection_start = editor_state.cursor_index;
    }

    // Clear selection only if a movement key is pressed without Shift
    if (!shift_pressed && (ImGui::IsKeyPressed(ImGuiKey_UpArrow) || ImGui::IsKeyPressed(ImGuiKey_DownArrow) || ImGui::IsKeyPressed(ImGuiKey_LeftArrow) || ImGui::IsKeyPressed(ImGuiKey_RightArrow))) {
        editor_state.selection_active = false;
        editor_state.selection_start = editor_state.selection_end = editor_state.cursor_index;
    }

    // Handle cursor movement based on arrow keys
    if (ImGui::IsKeyPressed(ImGuiKey_UpArrow)) {
        cursorUp();
    }
    if (ImGui::IsKeyPressed(ImGuiKey_DownArrow)) {
        cursorDown();
    }
    if (ImGui::IsKeyPressed(ImGuiKey_LeftArrow)) {
        cursorLeft();
    }
    if (ImGui::IsKeyPressed(ImGuiKey_RightArrow)) {
        cursorRight();
    }

    // Update selection end if we're in selection mode
    if (editor_state.selection_active) {
        editor_state.selection_end = editor_state.cursor_index;
    }

    // Use the EditorScroll class to handle scroll adjustments
    gEditorScroll.handleCursorMovementScroll();
}

void EditorCursor::processCursorJump(std::string &text, CursorVisibility &ensure_cursor_visible)
{
    if (ImGui::IsKeyPressed(ImGuiKey_LeftArrow)) {
        int current_line = EditorUtils::GetLineFromPosition(editor_state.editor_content_lines, editor_state.cursor_index);
        editor_state.cursor_index = editor_state.editor_content_lines[current_line] + 1;
        ensure_cursor_visible.horizontal = true;
    } else if (ImGui::IsKeyPressed(ImGuiKey_RightArrow)) {
        int current_line = EditorUtils::GetLineFromPosition(editor_state.editor_content_lines, editor_state.cursor_index);
        int next_line = current_line + 1;
        if (next_line < editor_state.editor_content_lines.size()) {
            editor_state.cursor_index = editor_state.editor_content_lines[next_line] - 2; // Position before the newline
        } else {
            editor_state.cursor_index = text.size();
        }
        ensure_cursor_visible.horizontal = true;
    } else if (ImGui::IsKeyPressed(ImGuiKey_UpArrow)) {
        moveCursorVertically(text, -5);
        ensure_cursor_visible.vertical = true;
        ensure_cursor_visible.horizontal = true;
    } else if (ImGui::IsKeyPressed(ImGuiKey_DownArrow)) {
        moveCursorVertically(text, 5);
        ensure_cursor_visible.vertical = true;
        ensure_cursor_visible.horizontal = true;
    }
}

void EditorCursor::processWordMovement(std::string &text, CursorVisibility &ensure_cursor_visible, bool shift_pressed)
{
    if (ImGui::IsKeyPressed(ImGuiKey_W)) {
        if (shift_pressed) {
            moveWordBackward(text);
        } else {
            moveWordForward(text);
        }
        ensure_cursor_visible.horizontal = true;
        ensure_cursor_visible.vertical = true;
    }
}
