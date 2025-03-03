#include "editor_keyboard.h"
#include "editor.h"
#include "editor_line_jump.h"
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
            int start = gEditor.getSelectionStart(state);
            int end = gEditor.getSelectionEnd(state);
            if (start < 0 || end > static_cast<int>(text.size()) || start > end) {
                std::cerr << "Error: Invalid selection range "
                             "in HandleCharacterInput"
                          << std::endl;
                return;
            }
            text.erase(start, end - start);
            colors.erase(colors.begin() + start, colors.begin() + end);
            state.cursor_pos = start;
        }

        // Insert new text
        if (state.cursor_pos < 0 || state.cursor_pos > static_cast<int>(text.size())) {
            std::cerr << "Error: Invalid cursor position in "
                         "HandleCharacterInput"
                      << std::endl;
            return;
        }
        text.insert(state.cursor_pos, input);
        colors.insert(colors.begin() + state.cursor_pos, input.size(), ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
        state.cursor_pos += input.size();

        // Reset selection state
        state.selection_start = state.selection_end = state.cursor_pos;
        state.is_selecting = false;

        text_changed = true;
        input_end = state.cursor_pos;
    }
}

void EditorKeyboard::handleEnterKey(std::string &text, std::vector<ImVec4> &colors, EditorState &state, bool &text_changed, int &input_end)
{
    if (gLineJump.hasJustJumped()) {
        return;
    }

    if (ImGui::IsKeyPressed(ImGuiKey_Enter)) {
        // Insert the newline character
        text.insert(state.cursor_pos, 1, '\n');

        // Safely insert the color
        if (state.cursor_pos <= colors.size()) {
            colors.insert(colors.begin() + state.cursor_pos, 1, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
        } else {
            std::cerr << "Warning: Invalid cursor position for colors" << std::endl;
            colors.push_back(ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
        }

        state.cursor_pos++;
        state.selection_start = state.selection_end = state.cursor_pos;
        state.is_selecting = false;
        text_changed = true;
        input_end = state.cursor_pos;
    }
}

void EditorKeyboard::handleDeleteKey(std::string &text, std::vector<ImVec4> &colors, EditorState &state, bool &text_changed, int &input_end)
{
    if (ImGui::IsKeyPressed(ImGuiKey_Delete)) {
        if (state.selection_start != state.selection_end) {
            // There's a selection, delete it
            int start = gEditor.getSelectionStart(state);
            int end = gEditor.getSelectionEnd(state);
            text.erase(start, end - start);
            colors.erase(colors.begin() + start, colors.begin() + end - 1);
            state.cursor_pos = start;
            text_changed = true;
            input_end = start;
        } else if (state.cursor_pos < text.size()) {
            // No selection, delete the character at cursor position
            text.erase(state.cursor_pos, 1);
            colors.erase(colors.begin() + state.cursor_pos - 1);
            text_changed = true;
            input_end = state.cursor_pos + 1;
        }

        // Clear selection after deletion
        state.selection_start = state.selection_end = state.cursor_pos;
        state.is_selecting = false;
    }
}

void EditorKeyboard::handleBackspaceKey(std::string &text, std::vector<ImVec4> &colors, EditorState &state, bool &text_changed, int &input_start)
{
    if (ImGui::IsKeyPressed(ImGuiKey_Backspace)) {
        if (state.selection_start != state.selection_end) {
            // There's a selection, delete it
            int start = gEditor.getSelectionStart(state);
            int end = gEditor.getSelectionEnd(state);
            text.erase(start, end - start);
            colors.erase(colors.begin() + start, colors.begin() + end);
            state.cursor_pos = start;
            text_changed = true;
            input_start = start;
        } else if (state.cursor_pos > 0) {
            // No selection, delete the character before cursor position
            text.erase(state.cursor_pos - 1, 1);
            colors.erase(colors.begin() + state.cursor_pos - 1);
            state.cursor_pos--;
            text_changed = true;
            input_start = state.cursor_pos;
        }

        // Clear selection after deletion
        state.selection_start = state.selection_end = state.cursor_pos;
        state.is_selecting = false;
    }
}