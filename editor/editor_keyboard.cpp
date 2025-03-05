#include "editor_keyboard.h"
#include "../files/files.h" // For gFileExplorer
#include "editor.h"
#include "editor_highlight.h"
#include "editor_indentation.h"
#include "editor_line_jump.h"
#include "editor_selection.h"
#include <iostream>

// Global instance
EditorKeyboard gEditorKeyboard;

EditorKeyboard::EditorKeyboard() {}

void EditorKeyboard::handleCharacterInput(std::string &text, std::vector<ImVec4> &colors, EditorState &state, bool &text_changed, int &input_start, int &input_end)
{
    ImGuiIO &io = ImGui::GetIO();
    std::string input;
    input.reserve(io.InputQueueCharacters.Size);
    for (int n = 0; n < io.InputQueueCharacters.Size; n++) {
        char c = static_cast<char>(io.InputQueueCharacters[n]);
        if (c != 0 && c >= 32) {
            input += c;
        }
    }
    if (!input.empty()) {
        // Clear any existing selection
        if (state.selection_start != state.selection_end) {
            int start = gEditorSelection.getSelectionStart(state);
            int end = gEditorSelection.getSelectionEnd(state);
            if (start < 0 || end > static_cast<int>(text.size()) || start > end) {
                std::cerr << "Error: Invalid selection range "
                             "in HandleCharacterInput"
                          << std::endl;
                return;
            }
            text.erase(start, end - start);
            colors.erase(colors.begin() + start, colors.begin() + end);
            state.cursor_column = start;
        }

        // Insert new text
        if (state.cursor_column < 0 || state.cursor_column > static_cast<int>(text.size())) {
            std::cerr << "Error: Invalid cursor position in "
                         "HandleCharacterInput"
                      << std::endl;
            return;
        }
        text.insert(state.cursor_column, input);
        colors.insert(colors.begin() + state.cursor_column, input.size(), ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
        state.cursor_column += input.size();

        // Reset selection state
        state.selection_start = state.selection_end = state.cursor_column;
        state.selection_active = false;

        text_changed = true;
        input_end = state.cursor_column;
    }
}

void EditorKeyboard::handleEnterKey(std::string &text, std::vector<ImVec4> &colors, EditorState &state, bool &text_changed, int &input_end)
{
    if (gLineJump.hasJustJumped()) {
        return;
    }

    if (ImGui::IsKeyPressed(ImGuiKey_Enter)) {
        // Insert the newline character
        text.insert(state.cursor_column, 1, '\n');

        // Safely insert the color
        if (state.cursor_column <= colors.size()) {
            colors.insert(colors.begin() + state.cursor_column, 1, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
        } else {
            std::cerr << "Warning: Invalid cursor position for colors" << std::endl;
            colors.push_back(ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
        }

        state.cursor_column++;
        state.selection_start = state.selection_end = state.cursor_column;
        state.selection_active = false;
        text_changed = true;
        input_end = state.cursor_column;
    }
}

void EditorKeyboard::handleDeleteKey(std::string &text, std::vector<ImVec4> &colors, EditorState &state, bool &text_changed, int &input_end)
{
    if (ImGui::IsKeyPressed(ImGuiKey_Delete)) {
        if (state.selection_start != state.selection_end) {
            // There's a selection, delete it
            int start = gEditorSelection.getSelectionStart(state);
            int end = gEditorSelection.getSelectionEnd(state);
            text.erase(start, end - start);
            colors.erase(colors.begin() + start, colors.begin() + end - 1);
            state.cursor_column = start;
            text_changed = true;
            input_end = start;
        } else if (state.cursor_column < text.size()) {
            // No selection, delete the character at cursor position
            text.erase(state.cursor_column, 1);
            colors.erase(colors.begin() + state.cursor_column - 1);
            text_changed = true;
            input_end = state.cursor_column + 1;
        }

        // Clear selection after deletion
        state.selection_start = state.selection_end = state.cursor_column;
        state.selection_active = false;
    }
}

void EditorKeyboard::handleBackspaceKey(std::string &text, std::vector<ImVec4> &colors, EditorState &state, bool &text_changed, int &input_start)
{
    if (ImGui::IsKeyPressed(ImGuiKey_Backspace)) {
        if (state.selection_start != state.selection_end) {
            // There's a selection, delete it
            int start = gEditorSelection.getSelectionStart(state);
            int end = gEditorSelection.getSelectionEnd(state);
            text.erase(start, end - start);
            colors.erase(colors.begin() + start, colors.begin() + end);
            state.cursor_column = start;
            text_changed = true;
            input_start = start;
        } else if (state.cursor_column > 0) {
            // No selection, delete the character before cursor position
            text.erase(state.cursor_column - 1, 1);
            colors.erase(colors.begin() + state.cursor_column - 1);
            state.cursor_column--;
            text_changed = true;
            input_start = state.cursor_column;
        }

        // Clear selection after deletion
        state.selection_start = state.selection_end = state.cursor_column;
        state.selection_active = false;
    }
}

void EditorKeyboard::handleTextInput(std::string &text, std::vector<ImVec4> &colors, EditorState &state, bool &text_changed)
{
    int input_start = state.cursor_column;
    int input_end = state.cursor_column;

    // Handle selection deletion only for Enter key
    if (state.selection_start != state.selection_end && ImGui::IsKeyPressed(ImGuiKey_Enter)) {
        int start = gEditorSelection.getSelectionStart(state);
        int end = gEditorSelection.getSelectionEnd(state);
        text.erase(start, end - start);
        colors.erase(colors.begin() + start, colors.begin() + end);
        state.cursor_column = start;
        state.selection_start = state.selection_end = start;
        text_changed = true;
        input_start = input_end = start;
    }

    handleCharacterInput(text, colors, state, text_changed, input_start, input_end);
    handleEnterKey(text, colors, state, text_changed, input_end);
    handleBackspaceKey(text, colors, state, text_changed, input_start);
    handleDeleteKey(text, colors, state, text_changed, input_end);
    gEditorIndentation.handleTabKey(text, colors, state, text_changed, input_end);

    if (text_changed) {
        // Get the start of the line where the change began
        int line_start = state.editor_content_lines[gEditor.getLineFromPos(state.editor_content_lines, input_start)];

        // Get the end of the line where the change ended (or the end of
        // the text if it's the last line)
        int line_end = input_end < text.size() ? state.editor_content_lines[gEditor.getLineFromPos(state.editor_content_lines, input_end)] : text.size();

        // Update syntax highlighting only for the affected lines
        gEditorHighlight.highlightContent(text, colors, line_start, line_end);

        // Update line starts
        gEditor.updateLineStarts(text, state.editor_content_lines);

        // Add undo state with change range
        gFileExplorer.addUndoState(line_start, line_end);
    }
}
void EditorKeyboard::processFontSizeAdjustment(CursorVisibility &ensure_cursor_visible)
{
    if (ImGui::IsKeyPressed(ImGuiKey_Equal)) { // '+' key
        float currentSize = gSettings.getFontSize();
        gSettings.setFontSize(currentSize + 2.0f);
        ensure_cursor_visible.vertical = true;
        ensure_cursor_visible.horizontal = true;
        std::cout << "Cmd++: Font size increased to " << gSettings.getFontSize() << std::endl;
    } else if (ImGui::IsKeyPressed(ImGuiKey_Minus)) { // '-' key
        float currentSize = gSettings.getFontSize();
        gSettings.setFontSize(std::max(currentSize - 2.0f, 8.0f));
        ensure_cursor_visible.vertical = true;
        ensure_cursor_visible.horizontal = true;
        std::cout << "Cmd+-: Font size decreased to " << gSettings.getFontSize() << std::endl;
    }
}