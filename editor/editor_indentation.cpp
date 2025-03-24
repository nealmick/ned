#include "editor_indentation.h"
#include "../files/files.h"
#include "editor.h"
#include "editor_highlight.h"
#include <algorithm>
#include <iostream>

// Global instance
EditorIndentation gEditorIndentation;

int EditorIndentation::getSelectionStart(const EditorState &state) const { return std::min(state.selection_start, state.selection_end); }

int EditorIndentation::getSelectionEnd(const EditorState &state) const { return std::max(state.selection_start, state.selection_end); }

int EditorIndentation::findLineStart(const std::string &text, int position)
{
    int lineStart = position;
    while (lineStart > 0 && text[lineStart - 1] != '\n') {
        lineStart--;
    }
    return lineStart;
}

int EditorIndentation::findLineEnd(const std::string &text, int position, size_t textLength)
{
    int lineEnd = position;
    while (lineEnd < textLength && text[lineEnd] != '\n') {
        lineEnd++;
    }
    return lineEnd;
}

void EditorIndentation::handleTabKey(std::string &text, std::vector<ImVec4> &colors, bool &text_changed, int &input_end)
{
    if (ImGui::IsKeyPressed(ImGuiKey_Tab)) {
        if (editor_state.selection_active) {
            handleMultiLineIndentation(text, colors, editor_state, input_end);
        } else {
            handleSingleLineIndentation(text, colors, editor_state, input_end);
        }

        finishIndentationChange(text, colors, editor_state, input_end, text_changed);
    }
}

void EditorIndentation::handleMultiLineIndentation(std::string &text, std::vector<ImVec4> &colors, EditorState &state, int &input_end)
{
    int start = getSelectionStart(state);
    int end = getSelectionEnd(state);

    // Find the start of the first line
    int firstLineStart = findLineStart(text, start);

    // Find the end of the last line
    int lastLineEnd = findLineEnd(text, end, text.length());

    int tabsInserted = 0;
    int lineStart = firstLineStart;

    // Insert tabs at the beginning of each selected line
    while (lineStart < lastLineEnd) {
        // Insert tab at the beginning of the line
        text.insert(lineStart, 1, '\t');
        colors.insert(colors.begin() + lineStart, 1, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
        tabsInserted++;

        // Move to the next line
        lineStart = text.find('\n', lineStart) + 1;
        if (lineStart == 0)
            break; // If we've reached the end of the text
    }

    // Adjust cursor and selection positions
    state.cursor_index += (state.cursor_index >= start) ? tabsInserted : 0;
    state.selection_start += (state.selection_start > start) ? tabsInserted : 0;
    state.selection_end += tabsInserted;

    input_end = lastLineEnd + tabsInserted;
}

void EditorIndentation::handleSingleLineIndentation(std::string &text, std::vector<ImVec4> &colors, EditorState &state, int &input_end)
{
    // Insert a single tab character at cursor position
    text.insert(state.cursor_index, 1, '\t');
    colors.insert(colors.begin() + state.cursor_index, 1, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
    state.cursor_index++;
    state.selection_start = state.selection_end = state.cursor_index;
    input_end = state.cursor_index;
}

void EditorIndentation::finishIndentationChange(std::string &text, std::vector<ImVec4> &colors, EditorState &state, int &input_end, bool &text_changed)
{
    // Mark text as changed and update
    text_changed = true;
    gEditor.updateLineStarts();
    gFileExplorer.setUnsavedChanges(true);

    // Trigger syntax highlighting for the affected area
    gEditorHighlight.highlightContent(text, colors, std::min(state.selection_start, state.selection_end), std::max(state.selection_end, input_end));
}

bool EditorIndentation::processIndentRemoval(std::string &text, bool &text_changed, CursorVisibility &ensure_cursor_visible)
{
    // If Shift+Tab is pressed, remove indentation and exit early.
    if (ImGui::GetIO().KeyShift && ImGui::IsKeyPressed(ImGuiKey_Tab, false)) {
        removeIndentation(text);
        text_changed = true;
        ensure_cursor_visible.horizontal = true;
        ensure_cursor_visible.vertical = true;
        ImGui::SetKeyboardFocusHere(-1); // Prevent default tab behavior
        return true;
    }
    return false;
}

void EditorIndentation::removeIndentation(std::string &text)
{
    // Determine the range to process
    int start, end;
    if (editor_state.selection_active) {
        start = getSelectionStart(editor_state);
        end = getSelectionEnd(editor_state);
    } else {
        // If no selection, work on the current line
        start = end = editor_state.cursor_index;
    }

    // Find the start of the first line
    int firstLineStart = findLineStart(text, start);

    // Find the end of the last line
    int lastLineEnd = findLineEnd(text, end, text.length());

    int totalSpacesRemoved = 0;
    std::string newText;
    newText.reserve(text.length());

    // Copy text before the affected lines
    newText.append(text.substr(0, firstLineStart));

    // Process each line
    size_t lineStart = firstLineStart;
    while (lineStart <= lastLineEnd) {
        size_t lineEnd = text.find('\n', lineStart);
        if (lineEnd == std::string::npos || lineEnd > lastLineEnd)
            lineEnd = lastLineEnd;

        processLineIndentRemoval(text, newText, lineStart, lineEnd, lastLineEnd, totalSpacesRemoved);
        lineStart = lineEnd + 1;
    }

    // Copy text after the affected lines
    newText.append(text.substr(lastLineEnd));

    // Update text and adjust cursor and selection
    updateStateAfterIndentRemoval(text, editor_state, newText, firstLineStart, lastLineEnd, totalSpacesRemoved);

    // Update colors and trigger highlighting
    updateColorsAfterIndentRemoval(text, editor_state, firstLineStart, lastLineEnd, totalSpacesRemoved);
}

void EditorIndentation::processLineIndentRemoval(const std::string &text, std::string &newText, size_t lineStart, size_t lineEnd, size_t lastLineEnd, int &totalSpacesRemoved)
{
    // Check for spaces or tab at the beginning of the line
    int spacesToRemove = 0;
    if (lineStart + 4 <= text.length() && text.substr(lineStart, 4) == "    ") {
        spacesToRemove = 4;
    } else if (lineStart < text.length() && text[lineStart] == '\t') {
        spacesToRemove = 1;
    }

    // Append the line without leading indentation
    newText.append(text.substr(lineStart + spacesToRemove, lineEnd - lineStart - spacesToRemove));

    // Add newline if not the last line
    if (lineEnd < lastLineEnd)
        newText.push_back('\n');

    totalSpacesRemoved += spacesToRemove;
}

void EditorIndentation::updateStateAfterIndentRemoval(std::string &text, EditorState &state, std::string &newText, int firstLineStart, int lastLineEnd, int totalSpacesRemoved)
{
    // Update text
    text = std::move(newText);

    // Adjust cursor position
    state.cursor_index = std::max(state.cursor_index - totalSpacesRemoved, firstLineStart);

    // Adjust selection if one exists
    if (state.selection_active) {
        state.selection_start = std::max(state.selection_start - totalSpacesRemoved, firstLineStart);
        state.selection_end = std::max(state.selection_end - totalSpacesRemoved, firstLineStart);
    } else {
        state.selection_start = state.selection_end = state.cursor_index;
    }
}

void EditorIndentation::updateColorsAfterIndentRemoval(std::string &text, EditorState &state, int firstLineStart, int lastLineEnd, int totalSpacesRemoved)
{
    // Update colors vector
    auto &colors = gFileExplorer.getFileColors();
    colors.erase(colors.begin() + firstLineStart, colors.begin() + lastLineEnd);
    colors.insert(colors.begin() + firstLineStart, lastLineEnd - firstLineStart - totalSpacesRemoved, ImVec4(1.0f, 1.0f, 1.0f, 1.0f)); // Insert default color

    // Update line starts
    gEditor.updateLineStarts();

    // Mark text as changed
    gFileExplorer.setUnsavedChanges(true);

    // Trigger syntax highlighting for the affected area
    gEditorHighlight.highlightContent(text, colors, firstLineStart, lastLineEnd - totalSpacesRemoved);
}
