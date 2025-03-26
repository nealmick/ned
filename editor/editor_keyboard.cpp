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
#include "editor_render.h"
#include "editor_scroll.h"
#include "editor_selection.h"
#include "editor_utils.h"

#include <iostream>

// Global instance
EditorKeyboard gEditorKeyboard;

EditorKeyboard::EditorKeyboard() {}

void EditorKeyboard::handleCharacterInput(std::string &text, std::vector<ImVec4> &colors, bool &text_changed, int &input_start, int &input_end)
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
        if (editor_state.selection_start != editor_state.selection_end) {
            int start = gEditorSelection.getSelectionStart();
            int end = gEditorSelection.getSelectionEnd();
            if (start < 0 || end > static_cast<int>(text.size()) || start > end) {
                std::cerr << "Error: Invalid selection range "
                             "in HandleCharacterInput"
                          << std::endl;
                return;
            }
            text.erase(start, end - start);
            colors.erase(colors.begin() + start, colors.begin() + end);
            editor_state.cursor_index = start;
        }

        // Insert new text
        if (editor_state.cursor_index < 0 || editor_state.cursor_index > static_cast<int>(text.size())) {
            std::cerr << "Error: Invalid cursor position in "
                         "HandleCharacterInput"
                      << std::endl;
            return;
        }
        text.insert(editor_state.cursor_index, input);
        colors.insert(colors.begin() + editor_state.cursor_index, input.size(), ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
        editor_state.cursor_index += input.size();

        // Reset selection state
        editor_state.selection_start = editor_state.selection_end = editor_state.cursor_index;
        editor_state.selection_active = false;

        text_changed = true;
        input_end = editor_state.cursor_index;
    }
}

void EditorKeyboard::handleEnterKey(std::string &text, std::vector<ImVec4> &colors, bool &text_changed, int &input_end)
{
    if (gLineJump.hasJustJumped()) {
        return;
    }

    if (ImGui::IsKeyPressed(ImGuiKey_Enter)) {
        // Insert the newline character
        text.insert(editor_state.cursor_index, 1, '\n');

        // Safely insert the color
        if (editor_state.cursor_index <= colors.size()) {
            colors.insert(colors.begin() + editor_state.cursor_index, 1, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
        } else {
            std::cerr << "Warning: Invalid cursor position for colors" << std::endl;
            colors.push_back(ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
        }

        editor_state.cursor_index++;
        editor_state.selection_start = editor_state.selection_end = editor_state.cursor_index;
        editor_state.selection_active = false;
        text_changed = true;
        input_end = editor_state.cursor_index;
    }
}

void EditorKeyboard::handleDeleteKey(std::string &text, std::vector<ImVec4> &colors, bool &text_changed, int &input_end)
{
    if (ImGui::IsKeyPressed(ImGuiKey_Delete)) {
        if (editor_state.selection_start != editor_state.selection_end) {
            // There's a selection, delete it
            int start = gEditorSelection.getSelectionStart();
            int end = gEditorSelection.getSelectionEnd();
            text.erase(start, end - start);
            colors.erase(colors.begin() + start, colors.begin() + end - 1);
            editor_state.cursor_index = start;
            text_changed = true;
            input_end = start;
        } else if (editor_state.cursor_index < text.size()) {
            // No selection, delete the character at cursor position
            text.erase(editor_state.cursor_index, 1);
            colors.erase(colors.begin() + editor_state.cursor_index - 1);
            text_changed = true;
            input_end = editor_state.cursor_index + 1;
        }

        // Clear selection after deletion
        editor_state.selection_start = editor_state.selection_end = editor_state.cursor_index;
        editor_state.selection_active = false;
    }
}

void EditorKeyboard::handleBackspaceKey(std::string &text, std::vector<ImVec4> &colors, bool &text_changed, int &input_start)
{
    if (ImGui::IsKeyPressed(ImGuiKey_Backspace)) {
        if (editor_state.selection_start != editor_state.selection_end) {
            // There's a selection, delete it
            int start = gEditorSelection.getSelectionStart();
            int end = gEditorSelection.getSelectionEnd();
            text.erase(start, end - start);
            colors.erase(colors.begin() + start, colors.begin() + end);
            editor_state.cursor_index = start;
            text_changed = true;
            input_start = start;
        } else if (editor_state.cursor_index > 0) {
            // No selection, delete the character before cursor position
            text.erase(editor_state.cursor_index - 1, 1);
            colors.erase(colors.begin() + editor_state.cursor_index - 1);
            editor_state.cursor_index--;
            text_changed = true;
            input_start = editor_state.cursor_index;
        }

        // Clear selection after deletion
        editor_state.selection_start = editor_state.selection_end = editor_state.cursor_index;
        editor_state.selection_active = false;
    }
}

void EditorKeyboard::handleTextInput(std::string &text, std::vector<ImVec4> &colors, bool &text_changed)
{
    int input_start = editor_state.cursor_index;
    int input_end = editor_state.cursor_index;

    // Handle selection deletion only for Enter key
    if (editor_state.selection_start != editor_state.selection_end && ImGui::IsKeyPressed(ImGuiKey_Enter)) {
        int start = gEditorSelection.getSelectionStart();
        int end = gEditorSelection.getSelectionEnd();
        text.erase(start, end - start);
        colors.erase(colors.begin() + start, colors.begin() + end);
        editor_state.cursor_index = start;
        editor_state.selection_start = editor_state.selection_end = start;
        text_changed = true;
        input_start = input_end = start;
    }

    handleCharacterInput(text, colors, text_changed, input_start, input_end);
    handleEnterKey(text, colors, text_changed, input_end);
    handleBackspaceKey(text, colors, text_changed, input_start);
    handleDeleteKey(text, colors, text_changed, input_end);
    gEditorIndentation.handleTabKey(text, colors, text_changed, input_end);

    if (text_changed) {
        // Get the start of the line where the change began
        int line_start = editor_state.editor_content_lines[gEditor.getLineFromPos(input_start)];

        // Get the end of the line where the change ended (or the end of
        // the text if it's the last line)
        int line_end = input_end < text.size() ? editor_state.editor_content_lines[gEditor.getLineFromPos(input_end)] : text.size();

        // Update syntax highlighting only for the affected lines
        gEditorHighlight.highlightContent(text, colors, line_start, line_end);

        // Update line starts
        gEditor.updateLineStarts();

        // Add undo state with change range
        gFileExplorer.addUndoState(line_start, line_end);
        gFileExplorer._unsavedChanges = true;
        gFileExplorer.saveCurrentFile();
        editor_state.text_changed = false;
        editor_state.ensure_cursor_visible.horizontal = true;
        editor_state.ensure_cursor_visible.vertical = true;
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

void EditorKeyboard::processSelectAll(std::string &text, CursorVisibility &ensure_cursor_visible)
{
    if (ImGui::IsKeyPressed(ImGuiKey_A)) {
        gEditorSelection.selectAllText(text);
        ensure_cursor_visible.vertical = true;
        ensure_cursor_visible.horizontal = true;
        std::cout << "Ctrl+A: Selected all text" << std::endl;
    }
}

// New method implementations for the refactored code

void EditorKeyboard::processTextEditorInput()
{
    if (!editor_state.block_input) {
        handleEditorKeyboardInput(editor_state.fileContent, editor_state.text_pos, editor_state.line_height, editor_state.text_changed, editor_state.fileColors, editor_state.ensure_cursor_visible);
    } else {
        editor_state.ensure_cursor_visible.vertical = true;
        editor_state.ensure_cursor_visible.horizontal = true;
    }

    if (gEditorScroll.getEnsureCursorVisibleFrames() > 0) {
        editor_state.ensure_cursor_visible.vertical = true;
        editor_state.ensure_cursor_visible.horizontal = true;
        gEditorScroll.decrementEnsureCursorVisibleFrames();
    }
}

void EditorKeyboard::handleEditorKeyboardInput(std::string &text, const ImVec2 &text_start_pos, float line_height, bool &text_changed, std::vector<ImVec4> &colors, CursorVisibility &ensure_cursor_visible)
{
    bool ctrl_pressed = ImGui::GetIO().KeyCtrl;
    bool shift_pressed = ImGui::GetIO().KeyShift;

    // block input if searching for file...
    if (gFileFinder.isWindowOpen()) {
        return;
    }

    // Process bookmarks first
    gBookmarks.handleBookmarkInput(gFileExplorer);

    if (ImGui::IsWindowFocused() && !editor_state.block_input) {
        // Process Shift+Tab for indentation removal. If handled, exit early.
        if (gEditorIndentation.processIndentRemoval(text, text_changed, ensure_cursor_visible))
            return;

        if (ctrl_pressed) {
            processFontSizeAdjustment(ensure_cursor_visible);
            processSelectAll(text, ensure_cursor_visible);
            gEditorKeyboard.processUndoRedo(text, colors, text_changed, ensure_cursor_visible, shift_pressed);
            gEditorCursor.processWordMovement(text, ensure_cursor_visible, shift_pressed);
            gEditorCursor.processCursorJump(text, ensure_cursor_visible);
        }
    }

    if (ImGui::IsWindowHovered()) {
        gEditorMouse.handleMouseInput(text, text_start_pos, line_height);
        gEditorScroll.processMouseWheelScrolling(line_height);
    }

    // Handle arrow key visibility
    handleArrowKeyVisibility(ensure_cursor_visible);

    // Pass the correct variables to handleCursorMovement
    float window_height = ImGui::GetWindowHeight();
    float window_width = ImGui::GetWindowWidth();
    gEditorCursor.handleCursorMovement(text, text_start_pos, line_height, window_height, window_width);

    // Call the refactored method in EditorKeyboard
    handleTextInput(text, colors, text_changed);

    if (ImGui::IsWindowFocused() && ctrl_pressed)
        gEditorCopyPaste.processClipboardShortcuts(text, colors, text_changed, ensure_cursor_visible);

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

void EditorKeyboard::processUndoRedo(std::string &text, std::vector<ImVec4> &colors, bool &text_changed, CursorVisibility &ensure_cursor_visible, bool shift_pressed)
{
    if (ImGui::IsKeyPressed(ImGuiKey_Z)) {
        std::cout << "Z key pressed. Ctrl: " << ImGui::GetIO().KeyCtrl << ", Shift: " << shift_pressed << std::endl;

        int oldCursorPos = editor_state.cursor_index;
        int oldLine = EditorUtils::GetLineFromPosition(editor_state.editor_content_lines, oldCursorPos);
        int oldColumn = oldCursorPos - editor_state.editor_content_lines[oldLine];

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
        gEditor.updateLineStarts();

        int newLine = std::min(oldLine, static_cast<int>(editor_state.editor_content_lines.size()) - 1);
        int lineStart = editor_state.editor_content_lines[newLine];
        int lineEnd = (newLine + 1 < editor_state.editor_content_lines.size()) ? editor_state.editor_content_lines[newLine + 1] - 1 : text.size();
        int lineLength = lineEnd - lineStart;

        editor_state.cursor_index = lineStart + std::min(oldColumn, lineLength);
        editor_state.selection_start = editor_state.selection_end = editor_state.cursor_index;
        text_changed = true;
        ensure_cursor_visible.vertical = true;
        ensure_cursor_visible.horizontal = true;

        gFileExplorer.currentUndoManager->printStacks();
    }
}