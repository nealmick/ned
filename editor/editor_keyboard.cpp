
#include "editor_keyboard.h"
#include "../files/file_finder.h"
#include "../files/files.h"
#include "../lsp/lsp_autocomplete.h"
#include "../lsp/lsp_goto_def.h"
#include "../lsp/lsp_goto_ref.h"
#include "../lsp/lsp_symbol_info.h"
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
#include "imgui.h"

#include <iostream>

// Global instance
EditorKeyboard gEditorKeyboard;

EditorKeyboard::EditorKeyboard() {}

void EditorKeyboard::handleCharacterInput()
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
            if (start < 0 || end > static_cast<int>(editor_state.fileContent.size()) || start > end) {
                std::cerr << "Error: Invalid selection range "
                             "in HandleCharacterInput"
                          << std::endl;
                return;
            }
            editor_state.fileContent.erase(start, end - start);
            editor_state.fileColors.erase(editor_state.fileColors.begin() + start, editor_state.fileColors.begin() + end);
            editor_state.cursor_index = start;
        }

        // Insert new text
        if (editor_state.cursor_index < 0 || editor_state.cursor_index > static_cast<int>(editor_state.fileContent.size())) {
            std::cerr << "Error: Invalid cursor position in "
                         "HandleCharacterInput"
                      << std::endl;
            return;
        }
        editor_state.fileContent.insert(editor_state.cursor_index, input);
        editor_state.fileColors.insert(editor_state.fileColors.begin() + editor_state.cursor_index, input.size(), ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
        editor_state.cursor_index += input.size();

        // Reset selection state
        editor_state.selection_start = editor_state.selection_end = editor_state.cursor_index;
        editor_state.selection_active = false;
        // set text changed
        editor_state.text_changed = true;
    }
}

void EditorKeyboard::handleEnterKey()
{
    if (gLineJump.hasJustJumped()) {
        return;
    }

    if (ImGui::IsKeyPressed(ImGuiKey_Enter)) {
        // Insert the newline character
        editor_state.fileContent.insert(editor_state.cursor_index, 1, '\n');

        // Safely insert the color
        if (editor_state.cursor_index <= editor_state.fileColors.size()) {
            editor_state.fileColors.insert(editor_state.fileColors.begin() + editor_state.cursor_index, 1, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
        } else {
            std::cerr << "Warning: Invalid cursor position for colors" << std::endl;
            editor_state.fileColors.push_back(ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
        }

        editor_state.cursor_index++;
        editor_state.selection_start = editor_state.selection_end = editor_state.cursor_index;
        editor_state.selection_active = false;
        editor_state.text_changed = true;
    }
}

void EditorKeyboard::handleDeleteKey()
{
    if (ImGui::IsKeyPressed(ImGuiKey_Delete)) {
        if (editor_state.selection_start != editor_state.selection_end) {
            // There's a selection, delete it
            int start = gEditorSelection.getSelectionStart();
            int end = gEditorSelection.getSelectionEnd();
            editor_state.fileContent.erase(start, end - start);
            editor_state.fileColors.erase(editor_state.fileColors.begin() + start, editor_state.fileColors.begin() + end - 1);
            editor_state.cursor_index = start;
            editor_state.text_changed = true;
        } else if (editor_state.cursor_index < editor_state.fileContent.size()) {
            // No selection, delete the character at cursor position
            editor_state.fileContent.erase(editor_state.cursor_index, 1);
            editor_state.fileColors.erase(editor_state.fileColors.begin() + editor_state.cursor_index - 1);
            editor_state.text_changed = true;
        }

        // Clear selection after deletion
        editor_state.selection_start = editor_state.selection_end = editor_state.cursor_index;
        editor_state.selection_active = false;
    }
}

void EditorKeyboard::handleBackspaceKey()
{
    if (ImGui::IsKeyPressed(ImGuiKey_Backspace)) {
        if (editor_state.selection_start != editor_state.selection_end) {
            // There's a selection, delete it
            int start = gEditorSelection.getSelectionStart();
            int end = gEditorSelection.getSelectionEnd();
            editor_state.fileContent.erase(start, end - start);
            editor_state.fileColors.erase(editor_state.fileColors.begin() + start, editor_state.fileColors.begin() + end);
            editor_state.cursor_index = start;
            editor_state.text_changed = true;
        } else if (editor_state.cursor_index > 0) {
            // No selection, delete the character before cursor position
            editor_state.fileContent.erase(editor_state.cursor_index - 1, 1);
            editor_state.fileColors.erase(editor_state.fileColors.begin() + editor_state.cursor_index - 1);
            editor_state.cursor_index--;
            editor_state.text_changed = true;
        }

        // Clear selection after deletion
        editor_state.selection_start = editor_state.selection_end = editor_state.cursor_index;
        editor_state.selection_active = false;
    }
}

void EditorKeyboard::handleTextInput()
{
    int input_start = editor_state.cursor_index;
    int input_end = editor_state.cursor_index;

    // Handle selection deletion only for Enter key
    if (editor_state.selection_start != editor_state.selection_end && ImGui::IsKeyPressed(ImGuiKey_Enter)) {
        int start = gEditorSelection.getSelectionStart();
        int end = gEditorSelection.getSelectionEnd();
        editor_state.fileContent.erase(start, end - start);
        editor_state.fileColors.erase(editor_state.fileColors.begin() + start, editor_state.fileColors.begin() + end);
        editor_state.cursor_index = start;
        editor_state.selection_start = editor_state.selection_end = start;
        editor_state.text_changed = true;
        input_start = input_end = start;
    }

    handleCharacterInput();
    handleEnterKey();
    handleBackspaceKey();
    handleDeleteKey();
    gLineJump.handleLineJumpInput();
    gEditorIndentation.handleTabKey();

    if (editor_state.text_changed) {
        // Get the start of the line where the change began
        int line_start = editor_state.editor_content_lines[gEditor.getLineFromPos(input_start)];

        // Get the end of the line where the change ended (or the end of
        // the text if it's the last line)
        int line_end = input_end < editor_state.fileContent.size() ? editor_state.editor_content_lines[gEditor.getLineFromPos(input_end)] : editor_state.fileContent.size();

        // Update syntax highlighting only for the affected lines
        gEditorHighlight.highlightContent();

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

void EditorKeyboard::processFontSizeAdjustment()
{
    if (ImGui::IsKeyPressed(ImGuiKey_Equal)) { // '+' key
        float currentSize = gSettings.getFontSize();
        gSettings.setFontSize(currentSize + 2.0f);
        editor_state.ensure_cursor_visible.vertical = true;
        editor_state.ensure_cursor_visible.horizontal = true;
        std::cout << "Cmd++: Font size increased to " << gSettings.getFontSize() << std::endl;
    } else if (ImGui::IsKeyPressed(ImGuiKey_Minus)) { // '-' key
        float currentSize = gSettings.getFontSize();
        gSettings.setFontSize(std::max(currentSize - 2.0f, 8.0f));
        editor_state.ensure_cursor_visible.vertical = true;
        editor_state.ensure_cursor_visible.horizontal = true;
        std::cout << "Cmd+-: Font size decreased to " << gSettings.getFontSize() << std::endl;
    }
}

void EditorKeyboard::processSelectAll()
{
    if (ImGui::IsKeyPressed(ImGuiKey_A)) {
        gEditorSelection.selectAllText(editor_state.fileContent);
        editor_state.ensure_cursor_visible.vertical = true;
        editor_state.ensure_cursor_visible.horizontal = true;
        std::cout << "Ctrl+A: Selected all text" << std::endl;
    }
}

// New method implementations for the refactored code

void EditorKeyboard::processTextEditorInput()
{
    if (!editor_state.block_input) {
        handleEditorKeyboardInput();
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

void EditorKeyboard::handleEditorKeyboardInput()
{
    bool ctrl_pressed = ImGui::GetIO().KeyCtrl;
    bool shift_pressed = ImGui::GetIO().KeyShift;

    // block input if searching for file...
    if (gFileFinder.showFFWindow) {
        return;
    }

    // Process bookmarks first
    if (ImGui::GetIO().KeyAlt) {
        gEditorCursor.processWordMovement(editor_state.fileContent, editor_state.ensure_cursor_visible);
    }

    if (ImGui::IsWindowFocused() && !editor_state.block_input) {
        // Process Shift+Tab for indentation removal. If handled, exit early.
        if (gEditorIndentation.processIndentRemoval())
            return;

        if (ctrl_pressed) {
            processFontSizeAdjustment();
            processSelectAll();
            gEditorKeyboard.processUndoRedo();
            gBookmarks.handleBookmarkInput(gFileExplorer);

            gEditorCursor.processCursorJump(editor_state.fileContent, editor_state.ensure_cursor_visible);
            if (ImGui::IsKeyPressed(ImGuiKey_I)) {

                gLSPSymbolInfo.fetchSymbolInfo(gFileExplorer.currentFile);
            }
            if (ImGui::IsKeyPressed(ImGuiKey_R)) {

                int current_line = gEditor.getLineFromPos(editor_state.cursor_index);
                // Get character offset in current line (same as above)
                int line_start = 0; // Default to 0
                if (current_line >= 0 && current_line < editor_state.editor_content_lines.size()) {
                    line_start = editor_state.editor_content_lines[current_line];
                }
                int char_offset = editor_state.cursor_index - line_start;
                char_offset = std::max(0, char_offset); // Ensure non-negative

                // Call LSP find references using the new global instance
                gLSPGotoRef.findReferences(gFileExplorer.currentFile, current_line, char_offset);
            }
            if (ImGui::IsKeyPressed(ImGuiKey_D)) {

                // Get current line number from editor_state
                int current_line = gEditor.getLineFromPos(editor_state.cursor_index);

                // Get character offset in current line
                int line_start = editor_state.editor_content_lines[current_line];
                int char_offset = editor_state.cursor_index - line_start;

                // Call LSP goto definition
                gLSPGotoDef.gotoDefinition(gFileExplorer.currentFile, current_line, char_offset);
            }
            if (ImGui::IsKeyPressed(ImGuiKey_L)) {

                // Get current line number from editor_state
                int current_line = gEditor.getLineFromPos(editor_state.cursor_index);

                // Get character offset in current line
                int line_start = editor_state.editor_content_lines[current_line];
                int char_offset = editor_state.cursor_index - line_start;

                gLSPAutocomplete.requestCompletion(gFileExplorer.currentFile, current_line, char_offset);
            }
        }
    }

    if (ImGui::IsWindowHovered()) {
        gEditorMouse.handleMouseInput();
        gEditorScroll.processMouseWheelScrolling();
    }

    // Handle arrow key visibility
    handleArrowKeyVisibility();

    // Pass the correct variables to handleCursorMovement
    float window_height = ImGui::GetWindowHeight();
    float window_width = ImGui::GetWindowWidth();
    gEditorCursor.handleCursorMovement(editor_state.fileContent, editor_state.text_pos, editor_state.line_height, window_height, window_width);

    // Call the refactored method in EditorKeyboard
    handleTextInput();

    if (ImGui::IsWindowFocused() && ctrl_pressed)
        gEditorCopyPaste.processClipboardShortcuts();

    // Update cursor visibility if text has changed
    updateCursorVisibilityOnTextChange();
}

void EditorKeyboard::handleArrowKeyVisibility()
{
    // Additional arrow key presses outside the ctrl block
    if (ImGui::IsKeyPressed(ImGuiKey_UpArrow) || ImGui::IsKeyPressed(ImGuiKey_DownArrow))
        editor_state.ensure_cursor_visible.vertical = true;
    if (ImGui::IsKeyPressed(ImGuiKey_LeftArrow) || ImGui::IsKeyPressed(ImGuiKey_RightArrow))
        editor_state.ensure_cursor_visible.horizontal = true;
}

void EditorKeyboard::updateCursorVisibilityOnTextChange()
{
    // Ensure cursor is visible if text has changed
    if (editor_state.text_changed) {
        editor_state.ensure_cursor_visible.vertical = true;
        editor_state.ensure_cursor_visible.horizontal = true;
    }
}

void EditorKeyboard::processUndoRedo()
{
    bool shift_pressed = ImGui::GetIO().KeyShift;
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
        gEditor.updateLineStarts();

        int newLine = std::min(oldLine, static_cast<int>(editor_state.editor_content_lines.size()) - 1);
        int lineStart = editor_state.editor_content_lines[newLine];
        int lineEnd = (newLine + 1 < editor_state.editor_content_lines.size()) ? editor_state.editor_content_lines[newLine + 1] - 1 : editor_state.fileContent.size();
        int lineLength = lineEnd - lineStart;

        editor_state.cursor_index = lineStart + std::min(oldColumn, lineLength);
        editor_state.selection_start = editor_state.selection_end = editor_state.cursor_index;
        editor_state.text_changed = true;
        editor_state.ensure_cursor_visible.vertical = true;
        editor_state.ensure_cursor_visible.horizontal = true;

        gFileExplorer.currentUndoManager->printStacks();
    }
}