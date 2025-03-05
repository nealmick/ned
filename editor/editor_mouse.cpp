#include "editor_mouse.h"
#include "editor.h"
#include <algorithm>
#include <iostream>

// Global instance
EditorMouse gEditorMouse;

EditorMouse::EditorMouse() : is_dragging(false), anchor_pos(-1) {}

void EditorMouse::handleMouseInput(const std::string &text, EditorState &state, const ImVec2 &text_start_pos, float line_height)
{
    ImVec2 mouse_pos = ImGui::GetMousePos();
    int char_index = getCharIndexFromCoords(text, mouse_pos, text_start_pos, state.editor_content_lines, line_height);

    // Handle click
    if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
        handleMouseClick(state, char_index, state.editor_content_lines);
    }
    // Handle drag
    else if (ImGui::IsMouseDragging(ImGuiMouseButton_Left) && is_dragging) {
        handleMouseDrag(state, char_index);
    }
    // Handle release
    else if (ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
        handleMouseRelease();
    }
}

void EditorMouse::handleMouseClick(EditorState &state, int char_index, const std::vector<int> &line_starts)
{
    if (ImGui::GetIO().KeyShift) {
        // If shift is held and we're not already selecting, set the anchor to the current
        // cursor position.
        if (!state.selection_active) {
            anchor_pos = state.cursor_column;
            state.selection_start = anchor_pos;
            state.selection_active = true;
        }
        // Update the selection end based on the new click.
        state.selection_end = char_index;
        state.cursor_column = char_index;
    } else {
        // On a regular click (without shift), reset the selection and update the anchor.
        state.cursor_column = char_index;
        anchor_pos = char_index;
        state.selection_start = char_index;
        state.selection_end = char_index;
        state.selection_active = false;
        int current_line = gEditor.getLineFromPos(line_starts, state.cursor_column);
        state.cursor_column_prefered = state.cursor_column - line_starts[current_line];
    }
    is_dragging = true;
}

void EditorMouse::handleMouseDrag(EditorState &state, int char_index)
{
    if (ImGui::GetIO().KeyShift) {
        // If shift is held, use the existing anchor for updating selection.
        if (anchor_pos == -1) {
            anchor_pos = state.cursor_column;
            state.selection_start = anchor_pos;
        }
        state.selection_end = char_index;
        state.cursor_column = char_index;
    } else {
        // Normal drag selection without shift: use the initial click as the anchor.
        state.selection_active = true;
        if (anchor_pos == -1) {
            anchor_pos = state.cursor_column;
        }
        state.selection_start = anchor_pos;
        state.selection_end = char_index;
        state.cursor_column = char_index;
    }
}

void EditorMouse::handleMouseRelease()
{
    is_dragging = false;
    anchor_pos = -1;
}

int EditorMouse::getCharIndexFromCoords(const std::string &text, const ImVec2 &click_pos, const ImVec2 &text_start_pos, const std::vector<int> &line_starts, float line_height)
{
    // Determine which line was clicked (clamped to valid indices)
    int clicked_line = std::clamp(static_cast<int>((click_pos.y - text_start_pos.y) / line_height), 0, static_cast<int>(line_starts.size()) - 1);

    // Get start/end indices for that line in the text.
    int line_start = line_starts[clicked_line];
    int line_end = (clicked_line + 1 < line_starts.size()) ? line_starts[clicked_line + 1] : text.size();
    int n = line_end - line_start; // number of characters in the line

    // If the line is empty, return its start.
    if (n <= 0)
        return line_start;

    // Build an array of "insertion positions" (i.e. the x-coordinate for placing the cursor)
    // For i=0, the insertion position is at 0 (the very start), and for i>0 it is the width
    // of the substring [line_start, line_start+i].
    std::vector<float> insertionPositions(n + 1, 0.0f);
    insertionPositions[0] = 0.0f;
    for (int i = 1; i <= n; i++) {
        insertionPositions[i] = ImGui::CalcTextSize(&text[line_start], &text[line_start + i]).x;
    }

    // Compute the click's x-coordinate relative to the beginning of the text.
    float click_x = click_pos.x - text_start_pos.x;

    // Find the insertion index (0...n) whose position is closest to click_x.
    int bestIndex = 0;
    float bestDist = std::abs(click_x - insertionPositions[0]);
    for (int i = 1; i <= n; i++) {
        float d = std::abs(click_x - insertionPositions[i]);
        if (d < bestDist) {
            bestDist = d;
            bestIndex = i;
        }
    }

    // Return the character index in the full text corresponding to that insertion point.
    return line_start + bestIndex;
}