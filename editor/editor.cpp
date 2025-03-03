/*
    File: editor.cpp
    Description: Main editor logic for displaying file content, handling keybinds and more...
*/

#include "editor.h"
#include "editor_bookmarks.h"
#include "editor_copy_paste.h"
#include "editor_cursor.h"
#include "editor_line_jump.h"
#include "editor_line_numbers.h"
#include "editor_types.h"
#include "editor_utils.h"

#include "../files/file_finder.h"
#include "../files/files.h"
#include "../util/settings.h"

#include <algorithm>
#include <cmath>
#include <iostream>

Editor gEditor;
EditorState editor_state;

bool Editor::textEditor(const char *label, std::string &text, std::vector<ImVec4> &colors, EditorState &editor_state)
{

    // Validate and resize colors if needed.
    validateAndResizeColors(text, colors);
    bool text_changed = false;
    CursorVisibility ensure_cursor_visible = {false, false};

    editor_state.cursor_blink_time += ImGui::GetIO().DeltaTime;

    // Setup editor window parameters.
    ImVec2 size;
    float line_number_width, line_height, editor_top_margin, text_left_margin;
    setupEditorWindow(label, size, line_number_width, line_height, editor_top_margin, text_left_margin);

    // Begin group and render line numbers panel.
    ImVec2 line_numbers_pos = renderLineNumbersPanel(line_number_width, editor_top_margin);

    // Calculate dimensions for the main text editor child.
    float remaining_width = size.x - line_number_width;
    float content_width = calculateTextWidth(text, editor_state.line_starts) + ImGui::GetFontSize() * 10.0f;
    float content_height = editor_state.line_starts.size() * line_height;
    float current_scroll_y, current_scroll_x;
    ImVec2 text_pos;
    beginTextEditorChild(label, remaining_width, content_width, content_height, current_scroll_y, current_scroll_x, text_pos, editor_top_margin, text_left_margin, editor_state);

    // Update line starts and store initial state.
    updateLineStarts(text, editor_state.line_starts);
    float total_height = line_height * editor_state.line_starts.size();
    ImVec2 text_start_pos = text_pos;
    int initial_cursor_pos = editor_state.cursor_pos;

    // Process editor input.
    processTextEditorInput(text, editor_state, text_start_pos, line_height, text_changed, colors, ensure_cursor_visible, initial_cursor_pos);

    // Process mouse wheel scrolling.
    gEditorScroll.processMouseWheelForEditor(line_height, current_scroll_y, current_scroll_x, editor_state);

    // Adjust scrolling to ensure the cursor is visible.
    gEditorScroll.adjustScrollForCursorVisibility(text_pos, text, editor_state, line_height, size.y, size.x, current_scroll_y, current_scroll_x, ensure_cursor_visible);

    gEditorScroll.updateScrollAnimation(editor_state, current_scroll_x, current_scroll_y, ImGui::GetIO().DeltaTime);

    // Apply the calculated scroll positions.
    ImGui::SetScrollY(current_scroll_y);
    ImGui::SetScrollX(current_scroll_x);
    // Render the main editor content (text and cursor).
    renderEditorContent(text, colors, editor_state, line_height, text_pos);

    // Render file finder if active
    gFileFinder.renderWindow();

    // Update final scroll values and render the line numbers.
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + total_height + editor_top_margin);

    // Get scroll positions from ImGui
    float scrollY = ImGui::GetScrollY();
    float scrollX = ImGui::GetScrollX();

    // Update EditorScroll with the final ImGui scroll values
    gEditorScroll.setScrollPosition(ImVec2(scrollX, scrollY));
    gEditorScroll.setScrollX(scrollX);

    ImGui::EndChild();
    ImGui::PopStyleColor(4);
    ImGui::PopStyleVar(4);
    ImGui::PushClipRect(line_numbers_pos, ImVec2(line_numbers_pos.x + line_number_width, line_numbers_pos.y + size.y - editor_top_margin), true);

    // Use the new EditorLineNumbers class instead
    gEditorLineNumbers.renderLineNumbers(line_numbers_pos, line_number_width, line_height, editor_state.line_starts.size(), gEditorScroll.getScrollPosition().y, size.y - editor_top_margin, editor_state, editor_state.cursor_blink_time);

    ImGui::PopClipRect();
    ImGui::EndGroup();
    ImGui::PopID();
    return text_changed;
}

bool Editor::validateAndResizeColors(std::string &text, std::vector<ImVec4> &colors)
{
    if (colors.size() != text.size()) {
        std::cout << "Warning: colors vector size (" << colors.size() << ") does not match text size (" << text.size() << "). Resizing." << std::endl;
        colors.resize(text.size(), ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
        return true;
    }
    return false;
}

void Editor::setupEditorWindow(const char *label, ImVec2 &size, float &line_number_width, float &line_height, float &editor_top_margin, float &text_left_margin)
{
    size = ImGui::GetContentRegionAvail();
    line_number_width = ImGui::CalcTextSize("0").x * 4 + 8.0f;
    line_height = ImGui::GetTextLineHeight();
    editor_top_margin = 2.0f;
    text_left_margin = 7.0f;
    ImGui::PushID(label);
}

ImVec2 Editor::renderLineNumbersPanel(float line_number_width, float editor_top_margin) { return gEditorLineNumbers.createLineNumbersPanel(line_number_width, editor_top_margin); }

void Editor::beginTextEditorChild(const char *label, float remaining_width, float content_width, float content_height, float &current_scroll_y, float &current_scroll_x, ImVec2 &text_pos, float editor_top_margin, float text_left_margin, EditorState &editor_state)
{
    ImGui::PushStyleVar(ImGuiStyleVar_ChildBorderSize, 1.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.5f, 0.5f, 0.5f, 0.5f));
    ImGui::PushStyleColor(ImGuiCol_ScrollbarBg, ImVec4(0.05f, 0.05f, 0.05f, 0.0f));
    ImGui::PushStyleColor(ImGuiCol_ScrollbarGrabHovered, ImVec4(0.6f, 0.6f, 0.6f, 0.7f));
    ImGui::PushStyleColor(ImGuiCol_ScrollbarGrabActive, ImVec4(0.8f, 0.8f, 0.8f, 0.9f));
    ImGui::PushStyleVar(ImGuiStyleVar_ScrollbarSize, 12.0f);
    ImGui::SetNextWindowContentSize(ImVec2(content_width, content_height));
    ImGui::BeginChild(label, ImVec2(remaining_width, ImGui::GetContentRegionAvail().y), false, ImGuiWindowFlags_HorizontalScrollbar);

    // Set keyboard focus to this child window if appropriate.
    if (!gBookmarks.isWindowOpen() && !editor_state.blockInput) {
        if (ImGui::IsWindowAppearing() || (ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows) && !ImGui::IsAnyItemActive())) {
            ImGui::SetKeyboardFocusHere();
        }
    }

    // Get current scroll positions from ImGui
    current_scroll_y = ImGui::GetScrollY();
    current_scroll_x = ImGui::GetScrollX();

    // Update EditorScroll with the latest scroll values
    gEditorScroll.setScrollPosition(ImVec2(current_scroll_x, current_scroll_y));
    gEditorScroll.setScrollX(current_scroll_x);

    text_pos = ImGui::GetCursorScreenPos();
    text_pos.y += editor_top_margin;
    text_pos.x += text_left_margin;
}

void Editor::processTextEditorInput(std::string &text, EditorState &editor_state, const ImVec2 &text_start_pos, float line_height, bool &text_changed, std::vector<ImVec4> &colors, CursorVisibility &ensure_cursor_visible, int initial_cursor_pos)
{
    if (!editor_state.blockInput) {
        handleEditorInput(text, editor_state, text_start_pos, line_height, text_changed, colors, ensure_cursor_visible);
    } else {
        ensure_cursor_visible.vertical = true;
        ensure_cursor_visible.horizontal = true;
    }

    if (gEditorScroll.getEnsureCursorVisibleFrames() > 0) {
        ensure_cursor_visible.vertical = true;
        ensure_cursor_visible.horizontal = true;
        gEditorScroll.decrementEnsureCursorVisibleFrames();
    }

    if (editor_state.cursor_pos != initial_cursor_pos) {
        ensure_cursor_visible.vertical = true;
        ensure_cursor_visible.horizontal = true;
    }
}
void Editor::renderEditorContent(const std::string &text, const std::vector<ImVec4> &colors, EditorState &editor_state, float line_height, const ImVec2 &text_pos)
{
    gLineJump.handleLineJumpInput(editor_state);
    gLineJump.renderLineJumpWindow(editor_state);

    // Render the text (with selection) using our character-by-character function
    renderTextWithSelection(ImGui::GetWindowDrawList(), text_pos, text, colors, editor_state, line_height);

    // Compute the cursor's line by finding which line the cursor is on
    int cursor_line = getLineFromPos(editor_state.line_starts, editor_state.cursor_pos);

    // Calculate cursor x position character-by-character using EditorCursor
    float cursor_x = gEditorCursor.calculateCursorXPosition(text_pos, text, editor_state.cursor_pos);

    ImVec2 cursor_screen_pos = text_pos;
    cursor_screen_pos.x = cursor_x;
    cursor_screen_pos.y = text_pos.y + cursor_line * line_height;

    // Draw the cursor using EditorCursor
    gEditorCursor.renderCursor(ImGui::GetWindowDrawList(), cursor_screen_pos, line_height, editor_state.cursor_blink_time);
}

void Editor::updateLineStarts(const std::string &text, std::vector<int> &line_starts)
{
    // Only update if the text has changed.
    if (text == editor_state.cached_text) {
        return;
    }

    // Update cached text and clear old caches.
    editor_state.cached_text = text;
    line_starts.clear();
    editor_state.line_widths.clear();

    // Remove the cached cumulative widths completely
    editor_state.cachedLineCumulativeWidths.clear();

    // Compute line_starts.
    line_starts.reserve(text.size() / 40);
    line_starts.push_back(0);
    size_t pos = 0;
    while ((pos = text.find('\n', pos)) != std::string::npos) {
        line_starts.push_back(pos + 1);
        ++pos;
    }

    // Just compute the total width for each line (for layout calculations)
    for (size_t i = 0; i < line_starts.size(); ++i) {
        int start = line_starts[i];
        int end = (i + 1 < line_starts.size()) ? line_starts[i + 1] - 1 : text.size();
        float width = ImGui::CalcTextSize(text.c_str() + start, text.c_str() + end).x;
        editor_state.line_widths.push_back(width);
    }
}

int Editor::getLineFromPos(const std::vector<int> &line_starts, int pos)
{
    auto it = std::upper_bound(line_starts.begin(), line_starts.end(), pos);
    return std::distance(line_starts.begin(), it) - 1;
}

// Selection and cursor movement functions
void Editor::startSelection(EditorState &state)
{
    state.is_selecting = true;
    state.selection_start = state.cursor_pos;
    state.selection_end = state.cursor_pos;
}

void Editor::updateSelection(EditorState &state)
{
    if (state.is_selecting) {
        state.selection_end = state.cursor_pos;
    }
}

void Editor::endSelection(EditorState &state) { state.is_selecting = false; }

int Editor::getSelectionStart(const EditorState &state) { return std::min(state.selection_start, state.selection_end); }

int Editor::getSelectionEnd(const EditorState &state) { return std::max(state.selection_start, state.selection_end); }

void Editor::selectAllText(EditorState &state, const std::string &text)
{
    const size_t MAX_SELECTION_SIZE = 100000; // Adjust this value as needed
    state.is_selecting = true;
    state.selection_start = 0;
    state.cursor_pos = std::min(text.size(), MAX_SELECTION_SIZE);
    state.selection_end = state.cursor_pos;
}

void Editor::handleMouseInput(const std::string &text, EditorState &state, const ImVec2 &text_start_pos, float line_height)
{
    static bool is_dragging = false;
    static int anchor_pos = -1; // anchor for shift-click selection

    ImVec2 mouse_pos = ImGui::GetMousePos();
    int char_index = getCharIndexFromCoords(text, mouse_pos, text_start_pos, state.line_starts, line_height);

    // When the left mouse button is clicked:
    if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
        if (ImGui::GetIO().KeyShift) {
            // If shift is held and we're not already selecting, set the anchor to the current
            // cursor position.
            if (!state.is_selecting) {
                anchor_pos = state.cursor_pos;
                state.selection_start = anchor_pos;
                state.is_selecting = true;
            }
            // Update the selection end based on the new click.
            state.selection_end = char_index;
            state.cursor_pos = char_index;
        } else {
            // On a regular click (without shift), reset the selection and update the anchor.
            state.cursor_pos = char_index;
            anchor_pos = char_index;
            state.selection_start = char_index;
            state.selection_end = char_index;
            state.is_selecting = false;
            int current_line = gEditor.getLineFromPos(state.line_starts, state.cursor_pos);
            state.preferred_column = state.cursor_pos - state.line_starts[current_line];
        }
        is_dragging = true;
    }
    // If the mouse is being dragged:
    else if (ImGui::IsMouseDragging(ImGuiMouseButton_Left) && is_dragging) {
        if (ImGui::GetIO().KeyShift) {
            // If shift is held, use the existing anchor for updating selection.
            if (anchor_pos == -1) {
                anchor_pos = state.cursor_pos;
                state.selection_start = anchor_pos;
            }
            state.selection_end = char_index;
            state.cursor_pos = char_index;
        } else {
            // Normal drag selection without shift: use the initial click as the anchor.
            state.is_selecting = true;
            if (anchor_pos == -1) {
                anchor_pos = state.cursor_pos;
            }
            state.selection_start = anchor_pos;
            state.selection_end = char_index;
            state.cursor_pos = char_index;
        }
    }
    // On mouse release, reset dragging state and clear the anchor.
    else if (ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
        is_dragging = false;
        anchor_pos = -1;
    }
}

// Text input handling
void Editor::handleCharacterInput(std::string &text, std::vector<ImVec4> &colors, EditorState &state, bool &text_changed, int &input_start, int &input_end)
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

void Editor::handleEnterKey(std::string &text, std::vector<ImVec4> &colors, EditorState &state, bool &text_changed, int &input_end)
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

void Editor::handleDeleteKey(std::string &text, std::vector<ImVec4> &colors, EditorState &state, bool &text_changed, int &input_end)
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

void Editor::handleBackspaceKey(std::string &text, std::vector<ImVec4> &colors, EditorState &state, bool &text_changed, int &input_start)
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
            // No selection, delete the character before cursor
            // position
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

void Editor::handleTextInput(std::string &text, std::vector<ImVec4> &colors, EditorState &state, bool &text_changed)
{
    int input_start = state.cursor_pos;
    int input_end = state.cursor_pos;

    // Handle selection deletion only for Enter key
    if (state.selection_start != state.selection_end && ImGui::IsKeyPressed(ImGuiKey_Enter)) {
        int start = gEditor.getSelectionStart(state);
        int end = gEditor.getSelectionEnd(state);
        text.erase(start, end - start);
        colors.erase(colors.begin() + start, colors.begin() + end);
        state.cursor_pos = start;
        state.selection_start = state.selection_end = start;
        text_changed = true;
        input_start = input_end = start;
    }

    gEditor.handleCharacterInput(text, colors, state, text_changed, input_start, input_end);
    gEditor.handleEnterKey(text, colors, state, text_changed, input_end);
    gEditor.handleBackspaceKey(text, colors, state, text_changed, input_start);
    gEditor.handleDeleteKey(text, colors, state, text_changed, input_end);
    gEditorIndentation.handleTabKey(text, colors, state, text_changed, input_end);

    if (text_changed) {
        // Get the start of the line where the change began
        int line_start = state.line_starts[gEditor.getLineFromPos(state.line_starts, input_start)];

        // Get the end of the line where the change ended (or the end of
        // the text if it's the last line)
        int line_end = input_end < text.size() ? state.line_starts[gEditor.getLineFromPos(state.line_starts, input_end)] : text.size();

        // Update syntax highlighting only for the affected lines
        gEditor.highlightContent(text, colors, line_start, line_end);

        // Update line starts
        gEditor.updateLineStarts(text, state.line_starts);

        // Add undo state with change range
        gFileExplorer.addUndoState(line_start, line_end);
    }
}

void Editor::renderTextWithSelection(ImDrawList *drawList, const ImVec2 &pos, const std::string &text, const std::vector<ImVec4> &colors, const EditorState &state, float line_height)
{
    ImVec2 text_pos = pos;
    int sel_start = getSelectionStart(state);
    int sel_end = getSelectionEnd(state);

    // Calculate visible range - use gEditorScroll instead of direct access
    float scroll_y = gEditorScroll.getScrollPosition().y;
    float window_height = ImGui::GetWindowHeight();
    int start_line = static_cast<int>(scroll_y / line_height);
    int end_line = start_line + static_cast<int>(window_height / line_height) + 1;

    int current_line = 0;
    for (size_t i = 0; i < text.size(); i++) {
        // Safety check
        if (i >= colors.size()) {
            std::cerr << "Error: Color index out of bounds in renderTextWithSelection" << std::endl;
            break;
        }

        // Handle newline: update line count and reset x
        if (text[i] == '\n') {
            current_line++;
            if (current_line > end_line)
                break;
            text_pos.x = pos.x;
            text_pos.y += line_height;
            continue;
        }

        // Skip lines above visible area
        if (current_line < start_line) {
            while (i < text.size() && text[i] != '\n')
                i++;
            i--; // Adjust for the outer loop's increment
            continue;
        }
        if (current_line > end_line)
            break;

        // Check if this character is selected
        bool is_selected = (i >= sel_start && i < sel_end);
        if (is_selected) {
            // Draw selection highlight
            float char_width = ImGui::CalcTextSize(&text[i], &text[i + 1]).x;
            ImVec2 sel_start_pos = text_pos;
            ImVec2 sel_end_pos = ImVec2(sel_start_pos.x + char_width, sel_start_pos.y + line_height);
            drawList->AddRectFilled(sel_start_pos, sel_end_pos, ImGui::ColorConvertFloat4ToU32(ImVec4(1.0f, 0.1f, 0.7f, 0.3f)));
        }

        // Draw individual character
        char buf[2] = {text[i], '\0'};
        drawList->AddText(text_pos, ImGui::ColorConvertFloat4ToU32(colors[i]), buf);
        text_pos.x += ImGui::CalcTextSize(buf).x;
    }
}

int Editor::getCharIndexFromCoords(const std::string &text, const ImVec2 &click_pos, const ImVec2 &text_start_pos, const std::vector<int> &line_starts, float line_height)
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

float Editor::calculateTextWidth(const std::string &text, const std::vector<int> &line_starts)
{
    // Assume updateLineStarts has been called so that editor_state.line_widths is up to date.
    float max_width = 0.0f;
    for (float width : editor_state.line_widths) {
        max_width = std::max(max_width, width);
    }
    return max_width;
}

void Editor::processFontSizeAdjustment(CursorVisibility &ensure_cursor_visible)
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

void Editor::processSelectAll(std::string &text, EditorState &state, CursorVisibility &ensure_cursor_visible)
{
    if (ImGui::IsKeyPressed(ImGuiKey_A)) {
        selectAllText(state, text);
        ensure_cursor_visible.vertical = true;
        ensure_cursor_visible.horizontal = true;
        std::cout << "Ctrl+A: Selected all text" << std::endl;
    }
}

void Editor::processUndoRedo(std::string &text, std::vector<ImVec4> &colors, EditorState &state, bool &text_changed, CursorVisibility &ensure_cursor_visible, bool shift_pressed)
{
    if (ImGui::IsKeyPressed(ImGuiKey_Z)) {
        std::cout << "Z key pressed. Ctrl: " << ImGui::GetIO().KeyCtrl << ", Shift: " << shift_pressed << std::endl;
        int oldCursorPos = state.cursor_pos;
        int oldLine = getLineFromPos(state.line_starts, oldCursorPos);
        int oldColumn = oldCursorPos - state.line_starts[oldLine];
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
        updateLineStarts(text, state.line_starts);
        int newLine = std::min(oldLine, static_cast<int>(state.line_starts.size()) - 1);
        int lineStart = state.line_starts[newLine];
        int lineEnd = (newLine + 1 < state.line_starts.size()) ? state.line_starts[newLine + 1] - 1 : text.size();
        int lineLength = lineEnd - lineStart;
        state.cursor_pos = lineStart + std::min(oldColumn, lineLength);
        state.selection_start = state.selection_end = state.cursor_pos;
        text_changed = true;
        ensure_cursor_visible.vertical = true;
        ensure_cursor_visible.horizontal = true;
        gFileExplorer.currentUndoManager->printStacks();
    }
}
void Editor::handleEditorInput(std::string &text, EditorState &state, const ImVec2 &text_start_pos, float line_height, bool &text_changed, std::vector<ImVec4> &colors, CursorVisibility &ensure_cursor_visible)
{
    bool ctrl_pressed = ImGui::GetIO().KeyCtrl;
    bool shift_pressed = ImGui::GetIO().KeyShift;

    // block input if searching for file...
    if (gFileFinder.isWindowOpen()) {
        return;
    }
    // Process bookmarks first
    gBookmarks.handleBookmarkInput(gFileExplorer, state);

    if (ImGui::IsWindowFocused() && !state.blockInput) {
        // Process Shift+Tab for indentation removal. If handled, exit
        // early.
        if (gEditorIndentation.processIndentRemoval(text, state, text_changed, ensure_cursor_visible))
            return;

        if (ctrl_pressed) {
            processFontSizeAdjustment(ensure_cursor_visible);
            processSelectAll(text, state, ensure_cursor_visible);
            processUndoRedo(text, colors, state, text_changed, ensure_cursor_visible, shift_pressed);
            gEditorCursor.processWordMovement(text, state, ensure_cursor_visible, shift_pressed);
            gEditorCursor.processCursorJump(text, state, ensure_cursor_visible);
        }
    }

    if (ImGui::IsWindowHovered()) {
        handleMouseInput(text, state, text_start_pos, line_height);
        gEditorScroll.processMouseWheelScrolling(line_height, state);
    }

    // Additional arrow key presses outside the ctrl block
    if (ImGui::IsKeyPressed(ImGuiKey_UpArrow) || ImGui::IsKeyPressed(ImGuiKey_DownArrow))
        ensure_cursor_visible.vertical = true;
    if (ImGui::IsKeyPressed(ImGuiKey_LeftArrow) || ImGui::IsKeyPressed(ImGuiKey_RightArrow))
        ensure_cursor_visible.horizontal = true;

    // Pass the correct variables to handleCursorMovement
    float window_height = ImGui::GetWindowHeight();
    float window_width = ImGui::GetWindowWidth();
    gEditorCursor.handleCursorMovement(text, state, text_start_pos, line_height, window_height, window_width);

    handleTextInput(text, colors, state, text_changed);

    if (ImGui::IsWindowFocused() && ctrl_pressed)
        gEditorCopyPaste.processClipboardShortcuts(text, colors, state, text_changed, ensure_cursor_visible);

    // Ensure cursor is visible if text has changed
    if (text_changed) {
        ensure_cursor_visible.vertical = true;
        ensure_cursor_visible.horizontal = true;
    }
}

void Editor::cancelHighlighting()
{
    cancelHighlightFlag = true;
    if (highlightFuture.valid()) {
        highlightFuture.wait();
    }
    cancelHighlightFlag = false;
}

void Editor::forceColorUpdate()
{
    pythonLexer.forceColorUpdate();
    cppLexer.forceColorUpdate();
    htmlLexer.forceColorUpdate();
    jsxLexer.forceColorUpdate();
}

bool Editor::validateHighlightContentParams(const std::string &content, const std::vector<ImVec4> &colors, int start_pos, int end_pos)
{
    if (content.empty()) {
        std::cerr << "Error: Empty content in highlightContent" << std::endl;
        return false;
    }
    if (colors.empty()) {
        std::cerr << "Error: Empty colors vector in highlightContent" << std::endl;
        return false;
    }
    if (content.size() != colors.size()) {
        std::cerr << "Error: Mismatch between content and colors size "
                     "in highlightContent"
                  << std::endl;
        return false;
    }
    if (start_pos < 0 || start_pos >= static_cast<int>(content.size())) {
        std::cerr << "Error: Invalid start_pos in highlightContent" << std::endl;
        return false;
    }
    if (end_pos < start_pos || end_pos > static_cast<int>(content.size())) {
        std::cerr << "Error: Invalid end_pos in highlightContent" << std::endl;
        return false;
    }
    return true;
}

void Editor::highlightContent(const std::string &content, std::vector<ImVec4> &colors, int start_pos, int end_pos)
{
    std::lock_guard<std::mutex> lock(highlight_mutex);
    std::cout << "\033[36mEditor:\033[0m   Highlight Content. content size: " << content.size() << std::endl;

    // Cancel any ongoing highlighting first
    cancelHighlighting();

    // For large files (>100KB), just use default color
    const size_t LARGE_FILE_THRESHOLD = 100 * 1024;
    if (content.size() > LARGE_FILE_THRESHOLD) {
        // Pre-allocate colors vector to match content size
        colors.resize(content.size(), ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
        return;
    }

    // Validate inputs
    if (!validateHighlightContentParams(content, colors, start_pos, end_pos))
        return;

    // Pre-allocate vectors to avoid reallocation during async operation
    std::string content_copy = content;
    std::vector<ImVec4> colors_copy;
    colors_copy.reserve(content.size()); // Reserve space before copying
    colors_copy = colors;

    std::string currentFile = gFileExplorer.getCurrentFile();

    highlightingInProgress = true;
    cancelHighlightFlag = false;

    std::string extension = fs::path(currentFile).extension().string();

    // Launch highlighting task - properly capture colors by reference
    highlightFuture = std::async(std::launch::async, [this, content_copy, &colors, colors_copy = std::move(colors_copy), currentFile, start_pos, end_pos, extension]() mutable {
        try {
            if (cancelHighlightFlag) {
                highlightingInProgress = false;
                return;
            }

            // Apply highlighting
            if (extension == ".cpp" || extension == ".h" || extension == ".hpp") {
                cppLexer.applyHighlighting(content_copy, colors_copy, 0);
            } else if (extension == ".py") {
                pythonLexer.applyHighlighting(content_copy, colors_copy, 0);
            } else if (extension == ".html") {
                htmlLexer.applyHighlighting(content_copy, colors_copy, 0);
            } else if (extension == ".js" || extension == ".jsx") {
                jsxLexer.applyHighlighting(content_copy, colors_copy, 0);
            } else {
                std::fill(colors_copy.begin() + start_pos, colors_copy.begin() + end_pos, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
            }

            if (!cancelHighlightFlag && currentFile == gFileExplorer.getCurrentFile()) {
                std::lock_guard<std::mutex> colorsLock(colorsMutex);
                // Note: colors is captured by reference now
                if (colors.size() == colors_copy.size()) {
                    colors = std::move(colors_copy);
                }
            }
        } catch (const std::exception &e) {
            std::cerr << "Error in highlighting: " << e.what() << std::endl;
        }
        highlightingInProgress = false;
    });
}

void Editor::setTheme(const std::string &themeName) { loadTheme(themeName); }

void Editor::loadTheme(const std::string &themeName)
{
    auto &settings = gSettings.getSettings();
    if (settings.contains("themes") && settings["themes"].contains(themeName)) {
        auto &theme = settings["themes"][themeName];
        for (const auto &[key, value] : theme.items()) {
            themeColors[key] = ImVec4(value[0], value[1], value[2], value[3]);
        }
    } else {
        // Set default colors if theme not found
        themeColors["keyword"] = ImVec4(0.0f, 0.4f, 1.0f, 1.0f);
        themeColors["function"] = ImVec4(0.0f, 0.4f, 1.0f, 1.0f);
        themeColors["string"] = ImVec4(0.87f, 0.87f, 0.0f, 1.0f);
        themeColors["number"] = ImVec4(0.0f, 0.8f, 0.8f, 1.0f);
        themeColors["comment"] = ImVec4(0.5f, 0.5f, 0.5f, 1.0f);
    }
}
