#include "editor_keyboard.h"
#include "../files/file_finder.h"
#include "../files/files.h"
#include "editor.h"
#include "editor_bookmarks.h"
#include "editor_copy_paste.h"
#include "editor_highlight.h"
#include "editor_indentation.h"
#include "editor_line_jump.h"
#include "editor_mouse.h"
#include "editor_scroll.h"
#include "editor_selection.h"
#include "editor_utils.h"

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

void EditorKeyboard::processSelectAll(std::string &text, EditorState &state, CursorVisibility &ensure_cursor_visible)
{
    if (ImGui::IsKeyPressed(ImGuiKey_A)) {
        gEditorSelection.selectAllText(state, text);
        ensure_cursor_visible.vertical = true;
        ensure_cursor_visible.horizontal = true;
        std::cout << "Ctrl+A: Selected all text" << std::endl;
    }
}

// New method implementations for the refactored code

void EditorKeyboard::processTextEditorInput(std::string &text, EditorState &editor_state, const ImVec2 &text_start_pos, float line_height, bool &text_changed, std::vector<ImVec4> &colors, CursorVisibility &ensure_cursor_visible, int initial_cursor_pos)
{
    if (!editor_state.block_input) {
        handleEditorKeyboardInput(text, editor_state, text_start_pos, line_height, text_changed, colors, ensure_cursor_visible);
    } else {
        ensure_cursor_visible.vertical = true;
        ensure_cursor_visible.horizontal = true;
    }

    if (gEditorScroll.getEnsureCursorVisibleFrames() > 0) {
        ensure_cursor_visible.vertical = true;
        ensure_cursor_visible.horizontal = true;
        gEditorScroll.decrementEnsureCursorVisibleFrames();
    }

    if (editor_state.cursor_column != initial_cursor_pos) {
        ensure_cursor_visible.vertical = true;
        ensure_cursor_visible.horizontal = true;
    }
}

void EditorKeyboard::handleEditorKeyboardInput(std::string &text, EditorState &state, const ImVec2 &text_start_pos, float line_height, bool &text_changed, std::vector<ImVec4> &colors, CursorVisibility &ensure_cursor_visible)
{
    bool ctrl_pressed = ImGui::GetIO().KeyCtrl;
    bool shift_pressed = ImGui::GetIO().KeyShift;

    // block input if searching for file...
    if (gFileFinder.isWindowOpen()) {
        return;
    }

    // Process bookmarks first
    gBookmarks.handleBookmarkInput(gFileExplorer, state);

    if (ImGui::IsWindowFocused() && !state.block_input) {
        // Process Shift+Tab for indentation removal. If handled, exit early.
        if (gEditorIndentation.processIndentRemoval(text, state, text_changed, ensure_cursor_visible))
            return;

        if (ctrl_pressed) {
            processFontSizeAdjustment(ensure_cursor_visible);
            processSelectAll(text, state, ensure_cursor_visible);
            gEditorKeyboard.processUndoRedo(text, colors, state, text_changed, ensure_cursor_visible, shift_pressed);
            gEditorCursor.processWordMovement(text, state, ensure_cursor_visible, shift_pressed);
            gEditorCursor.processCursorJump(text, state, ensure_cursor_visible);
        }
    }

    if (ImGui::IsWindowHovered()) {
        gEditorMouse.handleMouseInput(text, state, text_start_pos, line_height);
        gEditorScroll.processMouseWheelScrolling(line_height, state);
    }

    // Handle arrow key visibility
    handleArrowKeyVisibility(ensure_cursor_visible);

    // Pass the correct variables to handleCursorMovement
    float window_height = ImGui::GetWindowHeight();
    float window_width = ImGui::GetWindowWidth();
    gEditorCursor.handleCursorMovement(text, state, text_start_pos, line_height, window_height, window_width);

    // Call the refactored method in EditorKeyboard
    handleTextInput(text, colors, state, text_changed);

    if (ImGui::IsWindowFocused() && ctrl_pressed)
        gEditorCopyPaste.processClipboardShortcuts(text, colors, state, text_changed, ensure_cursor_visible);

    // Update cursor visibility if text has changed
    updateCursorVisibilityOnTextChange(text_changed, ensure_cursor_visible);
}

void EditorKeyboard::handleArrowKeyVisibility(CursorVisibility &ensure_cursor_visible)
{
    // Additional arrow key presses outside the ctrl block
    if (ImGui::IsKeyPressed(ImGuiKey_UpArrow) || ImGui::IsKeyPressed(ImGuiKey_DownArrow))
        ensure_cursor_visible.vertical = true;
    if (ImGui::IsKeyPressed(ImGuiKey_LeftArrow) || ImGui::IsKeyPressed(ImGuiKey_RightArrow))
        ensure_cursor_visible.horizontal = true;
}

void EditorKeyboard::updateCursorVisibilityOnTextChange(bool text_changed, CursorVisibility &ensure_cursor_visible)
{
    // Ensure cursor is visible if text has changed
    if (text_changed) {
        ensure_cursor_visible.vertical = true;
        ensure_cursor_visible.horizontal = true;
    }
}

void EditorKeyboard::processUndoRedo(std::string &text, std::vector<ImVec4> &colors, EditorState &state, bool &text_changed, CursorVisibility &ensure_cursor_visible, bool shift_pressed)
{
    if (ImGui::IsKeyPressed(ImGuiKey_Z)) {
        std::cout << "Z key pressed. Ctrl: " << ImGui::GetIO().KeyCtrl << ", Shift: " << shift_pressed << std::endl;

        int oldCursorPos = state.cursor_column;
        int oldLine = EditorUtils::GetLineFromPosition(state.editor_content_lines, oldCursorPos);
        int oldColumn = oldCursorPos - state.editor_content_lines[oldLine];

        if (shift_pressed) {
            std::cout << "Attempting Redo" << std::endl;
            gFileExplorer.handleRedo();
        } else {
            std::cout << "Attempting Undo" << std::endl;
            gFileExplorer.handleUndo();
        }

        // Update text and colors
        text = gFileExplorer.getFileContent();
        colors = gFileExplorer.getFileColors();
        gEditor.updateLineStarts(text, state.editor_content_lines);

        int newLine = std::min(oldLine, static_cast<int>(state.editor_content_lines.size()) - 1);
        int lineStart = state.editor_content_lines[newLine];
        int lineEnd = (newLine + 1 < state.editor_content_lines.size()) ? state.editor_content_lines[newLine + 1] - 1 : text.size();
        int lineLength = lineEnd - lineStart;

        state.cursor_column = lineStart + std::min(oldColumn, lineLength);
        state.selection_start = state.selection_end = state.cursor_column;
        text_changed = true;
        ensure_cursor_visible.vertical = true;
        ensure_cursor_visible.horizontal = true;

        gFileExplorer.currentUndoManager->printStacks();
    }
}