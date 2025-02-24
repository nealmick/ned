/*
    File: editor.cpp
    Description: Main editor logic for displaying file content, handling keybinds and more...
*/

#include "editor.h"
#include "../files/file_finder.h"
#include "../files/files.h"
#include "../util/settings.h"
#include "bookmarks.h"
#include "line_jump.h"

#include <algorithm>
#include <cmath>
#include <iostream>

Editor gEditor;
EditorState editor_state;

bool Editor::textEditor(const char *label, std::string &text, std::vector<ImVec4> &colors, EditorState &editor_state)
{
    // Validate and resize colors if neededw.
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
    processMouseWheelForEditor(line_height, current_scroll_y, current_scroll_x, editor_state);

    // Adjust scrolling to ensure the cursor is visible.
    adjustScrollForCursorVisibility(text_pos, text, editor_state, line_height, size.y, size.x, current_scroll_y, current_scroll_x, ensure_cursor_visible);

    // Apply the calculated scroll positions.
    ImGui::SetScrollY(current_scroll_y);
    ImGui::SetScrollX(current_scroll_x);

    // Render the main editor content (text and cursor).
    renderEditorContent(text, colors, editor_state, line_height, text_pos);

    // Render file finder if active
    gFileFinder.renderWindow();

    // Update final scroll values and render the line numbers.
    updateFinalScrollAndRenderLineNumbers(line_numbers_pos, line_number_width, editor_top_margin, size, editor_state, line_height, total_height);

    return text_changed;
}

bool Editor::validateAndResizeColors(std::string &text, std::vector<ImVec4> &colors)
{
    if (colors.size() != text.size())
    {
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

ImVec2 Editor::renderLineNumbersPanel(float line_number_width, float editor_top_margin)
{
    ImGui::BeginGroup();
    ImGui::PushStyleVar(ImGuiStyleVar_ChildBorderSize, 0.0f);
    ImGui::BeginChild("LineNumbers", ImVec2(line_number_width, ImGui::GetContentRegionAvail().y), false, ImGuiWindowFlags_NoScrollbar);
    ImVec2 line_numbers_pos = ImGui::GetCursorScreenPos();
    line_numbers_pos.y += editor_top_margin;
    ImGui::EndChild();
    ImGui::PopStyleVar();
    ImGui::SameLine();
    return line_numbers_pos;
}

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
    if (!gBookmarks.isWindowOpen() && !editor_state.blockInput)
    {
        if (ImGui::IsWindowAppearing() || (ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows) && !ImGui::IsAnyItemActive()))
        {
            ImGui::SetKeyboardFocusHere();
        }
    }
    current_scroll_y = ImGui::GetScrollY();
    current_scroll_x = ImGui::GetScrollX();
    text_pos = ImGui::GetCursorScreenPos();
    text_pos.y += editor_top_margin;
    text_pos.x += text_left_margin;
}

void Editor::processTextEditorInput(std::string &text, EditorState &editor_state, const ImVec2 &text_start_pos, float line_height, bool &text_changed, std::vector<ImVec4> &colors, CursorVisibility &ensure_cursor_visible, int initial_cursor_pos)
{
    if (!editor_state.blockInput)
    {
        handleEditorInput(text, editor_state, text_start_pos, line_height, text_changed, colors, ensure_cursor_visible);
    }
    else
    {
        ensure_cursor_visible.vertical = true;
        ensure_cursor_visible.horizontal = true;
    }

    // Handle frame counter for ensuring cursor visibility
    if (editor_state.ensure_cursor_visible_frames > 0)
    {
        ensure_cursor_visible.vertical = true;
        ensure_cursor_visible.horizontal = true;
        editor_state.ensure_cursor_visible_frames--;
    }

    if (editor_state.cursor_pos != initial_cursor_pos)
    {
        ensure_cursor_visible.vertical = true;
        ensure_cursor_visible.horizontal = true;
    }
}

void Editor::processMouseWheelForEditor(float line_height, float &current_scroll_y, float &current_scroll_x, EditorState &editor_state)
{
    if (ImGui::IsWindowHovered() && !editor_state.blockInput)
    {
        float wheel_y = ImGui::GetIO().MouseWheel;
        float wheel_x = ImGui::GetIO().MouseWheelH;

        // Reduce the multiplier from 3 to 1.5 for a slower vertical scroll
        if (wheel_y != 0)
        {
            current_scroll_y -= wheel_y * line_height * 1.0f;
            current_scroll_y = std::max(0.0f, std::min(current_scroll_y, ImGui::GetScrollMaxY()));
        }

        // Also reduce the horizontal scroll speed multiplier
        if (wheel_x != 0)
        {
            current_scroll_x -= wheel_x * ImGui::GetFontSize() * 1.0f;
            current_scroll_x = std::max(0.0f, std::min(current_scroll_x, ImGui::GetScrollMaxX()));
        }
    }
}

void Editor::adjustScrollForCursorVisibility(const ImVec2 &text_pos, const std::string &text, EditorState &state, float line_height, float window_height, float window_width, float &current_scroll_y, float &current_scroll_x, CursorVisibility &ensure_cursor_visible)
{
    // First check if there's a direct scroll request
    float requested_x, requested_y;
    if (handleScrollRequest(requested_x, requested_y))
    {
        current_scroll_x = requested_x;
        current_scroll_y = requested_y;
        state.scroll_x = requested_x;
        state.scroll_pos.y = requested_y;

        // Directly apply the scroll in ImGui too
        ImGui::SetScrollX(requested_x);
        ImGui::SetScrollY(requested_y);

        // Reset visibility flags - we've just explicitly scrolled
        ensure_cursor_visible.vertical = false;
        ensure_cursor_visible.horizontal = false;

        return;
    }

    // Original logic...
    if (ensure_cursor_visible.vertical || ensure_cursor_visible.horizontal)
    {
        // Call ensureCursorVisible to calculate and apply scroll adjustments
        ScrollChange scroll_change = ensureCursorVisible(text_pos, text, state, line_height, window_height, window_width);

        // Update the current scroll position variables with the new values
        if (scroll_change.horizontal)
        {
            current_scroll_x = state.scroll_x; // Use the value from state that was updated in ensureCursorVisible
        }

        if (scroll_change.vertical)
        {
            current_scroll_y = state.scroll_pos.y; // Use the value from state that was updated in ensureCursorVisible
        }

        // Reset the visibility flags since we've handled them
        ensure_cursor_visible.vertical = false;
        ensure_cursor_visible.horizontal = false;
    }
}

void Editor::renderEditorContent(const std::string &text, const std::vector<ImVec4> &colors, EditorState &editor_state, float line_height, const ImVec2 &text_pos)
{
    // Render the text (with selection) using our character-by-character function
    renderTextWithSelection(ImGui::GetWindowDrawList(), text_pos, text, colors, editor_state, line_height);

    // Compute the cursor's line by finding which line the cursor is on
    int cursor_line = getLineFromPos(editor_state.line_starts, editor_state.cursor_pos);

    // Calculate cursor x position character-by-character
    float cursor_x = text_pos.x;
    int line_start = editor_state.line_starts[cursor_line];
    for (int i = line_start; i < editor_state.cursor_pos; i++)
    {
        cursor_x += ImGui::CalcTextSize(&text[i], &text[i + 1]).x;
    }

    ImVec2 cursor_screen_pos = text_pos;
    cursor_screen_pos.x = cursor_x;
    cursor_screen_pos.y = text_pos.y + cursor_line * line_height;

    // Draw the cursor
    renderCursor(ImGui::GetWindowDrawList(), cursor_screen_pos, line_height, editor_state.cursor_blink_time);
}

void Editor::updateFinalScrollAndRenderLineNumbers(const ImVec2 &line_numbers_pos, float line_number_width, float editor_top_margin, const ImVec2 &size, EditorState &editor_state, float line_height, float total_height)
{
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + total_height + editor_top_margin);
    editor_state.scroll_pos.y = ImGui::GetScrollY();
    editor_state.scroll_x = ImGui::GetScrollX();
    ImGui::EndChild();
    ImGui::PopStyleColor(4);
    ImGui::PopStyleVar(4);
    ImGui::PushClipRect(line_numbers_pos, ImVec2(line_numbers_pos.x + line_number_width, line_numbers_pos.y + size.y - editor_top_margin), true);
    renderLineNumbers(line_numbers_pos, line_number_width, line_height, editor_state.line_starts.size(), editor_state.scroll_pos.y, size.y - editor_top_margin, editor_state, editor_state.cursor_blink_time);
    ImGui::PopClipRect();
    ImGui::EndGroup();
    ImGui::PopID();
}
void Editor::updateLineStarts(const std::string &text, std::vector<int> &line_starts)
{
    // Only update if the text has changed.
    if (text == editor_state.cached_text)
    {
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
    while ((pos = text.find('\n', pos)) != std::string::npos)
    {
        line_starts.push_back(pos + 1);
        ++pos;
    }

    // Just compute the total width for each line (for layout calculations)
    for (size_t i = 0; i < line_starts.size(); ++i)
    {
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
    if (state.is_selecting)
    {
        state.selection_end = state.cursor_pos;
    }
}

void Editor::endSelection(EditorState &state) { state.is_selecting = false; }

int Editor::getSelectionStart(const EditorState &state) { return std::min(state.selection_start, state.selection_end); }

int Editor::getSelectionEnd(const EditorState &state) { return std::max(state.selection_start, state.selection_end); }

float Editor::getCursorYPosition(const EditorState &state, float line_height)
{
    int cursor_line = gEditor.getLineFromPos(state.line_starts, state.cursor_pos);
    return cursor_line * line_height;
}

// Copy, cut, and paste functions
void Editor::copySelectedText(const std::string &text, const EditorState &state)
{
    if (state.selection_start != state.selection_end)
    {
        int start = gEditor.getSelectionStart(state);
        int end = gEditor.getSelectionEnd(state);
        std::string selected_text = text.substr(start, end - start);
        ImGui::SetClipboardText(selected_text.c_str());
    }
}

float Editor::calculateCursorXPosition(const ImVec2 &text_pos, const std::string &text, int cursor_pos)
{
    float x = text_pos.x;
    for (int i = 0; i < cursor_pos; i++)
    {
        if (text[i] == '\n')
        {
            x = text_pos.x;
        }
        else
        {
            x += ImGui::CalcTextSize(&text[i], &text[i + 1]).x;
        }
    }
    return x;
}

ScrollChange Editor::ensureCursorVisible(const ImVec2 &text_pos, const std::string &text, EditorState &state, float line_height, float window_height, float window_width)
{
    // Get current scroll offsets
    float scroll_x = ImGui::GetScrollX();
    float scroll_y = ImGui::GetScrollY();

    // Calculate viewport dimensions
    float scrollbar_width = ImGui::GetStyle().ScrollbarSize;
    float additional_padding = 80.0f;
    float viewport_width = window_width - scrollbar_width - additional_padding;
    float viewport_height = window_height;

    // Calculate cursor position
    float abs_cursor_x = calculateCursorXPosition(text_pos, text, state.cursor_pos);
    int cursor_line = getLineFromPos(state.line_starts, state.cursor_pos);
    float abs_cursor_y = text_pos.y + cursor_line * line_height;

    // Calculate cursor position relative to viewport
    float visible_cursor_x = abs_cursor_x - text_pos.x - scroll_x;
    float visible_cursor_y = abs_cursor_y - text_pos.y - scroll_y;

    // Margins - space to keep between cursor and edges
    float margin_x = ImGui::GetFontSize() * 3.0f;
    float margin_y = line_height * 1.5f;

    // Calculate distances from each edge (negative means cursor is outside)
    float dist_left = visible_cursor_x;
    float dist_right = viewport_width - visible_cursor_x;
    float dist_top = visible_cursor_y;
    float dist_bottom = viewport_height - visible_cursor_y - line_height;

    bool scroll_x_changed = false;
    bool scroll_y_changed = false;
    float new_scroll_x = scroll_x;
    float new_scroll_y = scroll_y;

    // Check if cursor needs horizontal scrolling
    if (dist_left < margin_x)
    {
        // Cursor too close to or beyond left edge
        new_scroll_x = scroll_x - (margin_x - dist_left);
        new_scroll_x = std::max(0.0f, new_scroll_x);
        scroll_x_changed = true;
    }
    else if (dist_right < margin_x)
    {
        // Cursor too close to or beyond right edge
        new_scroll_x = scroll_x + (margin_x - dist_right);
        new_scroll_x = std::min(new_scroll_x, ImGui::GetScrollMaxX());
        scroll_x_changed = true;
    }

    // Check if cursor needs vertical scrolling
    if (dist_top < margin_y)
    {
        // Cursor too close to or beyond top edge
        new_scroll_y = scroll_y - (margin_y - dist_top);
        new_scroll_y = std::max(0.0f, new_scroll_y);
        scroll_y_changed = true;
    }
    else if (dist_bottom < margin_y)
    {
        // Cursor too close to or beyond bottom edge
        new_scroll_y = scroll_y + (margin_y - dist_bottom);
        new_scroll_y = std::min(new_scroll_y, ImGui::GetScrollMaxY());
        scroll_y_changed = true;
    }

    // Apply scroll changes directly
    if (scroll_x_changed)
    {
        ImGui::SetScrollX(new_scroll_x);
        state.scroll_x = new_scroll_x;
    }

    if (scroll_y_changed)
    {
        ImGui::SetScrollY(new_scroll_y);
        state.scroll_pos.y = new_scroll_y;
    }
    /*
    // Debug output
    std::cout << "=== ensureCursorVisible DEBUG ===" << std::endl;
    std::cout << "Distance from left: " << dist_left << ", right: " << dist_right
              << ", top: " << dist_top << ", bottom: " << dist_bottom << std::endl;
    std::cout << "Scroll changes - X: " << (scroll_x_changed ? "YES" : "NO")
              << ", Y: " << (scroll_y_changed ? "YES" : "NO") << std::endl;
    std::cout << "=================================" << std::endl;
    */
    return {scroll_y_changed, scroll_x_changed};
}

void Editor::selectAllText(EditorState &state, const std::string &text)
{
    const size_t MAX_SELECTION_SIZE = 100000; // Adjust this value as needed
    state.is_selecting = true;
    state.selection_start = 0;
    state.cursor_pos = std::min(text.size(), MAX_SELECTION_SIZE);
    state.selection_end = state.cursor_pos;
}

void Editor::cutSelectedText(std::string &text, std::vector<ImVec4> &colors, EditorState &state, bool &text_changed)
{
    if (state.selection_start != state.selection_end)
    {
        int start = gEditor.getSelectionStart(state);
        int end = gEditor.getSelectionEnd(state);
        std::string selected_text = text.substr(start, end - start);
        ImGui::SetClipboardText(selected_text.c_str());
        text.erase(start, end - start);
        colors.erase(colors.begin() + start, colors.begin() + end);
        state.cursor_pos = start;
        state.selection_start = state.selection_end = start;
        text_changed = true;
    }
}

void Editor::cutWholeLine(std::string &text, std::vector<ImVec4> &colors, EditorState &state, bool &text_changed)
{

    int line = gEditor.getLineFromPos(state.line_starts, state.cursor_pos);
    int line_start = state.line_starts[line];
    int line_end = (line + 1 < state.line_starts.size()) ? state.line_starts[line + 1] : text.size();

    std::string line_text = text.substr(line_start, line_end - line_start);
    ImGui::SetClipboardText(line_text.c_str());

    text.erase(line_start, line_end - line_start);
    colors.erase(colors.begin() + line_start, colors.begin() + line_end);

    state.cursor_pos = line > 0 ? state.line_starts[line] : 0;
    text_changed = true;
    gEditor.updateLineStarts(text, state.line_starts);
}

void Editor::pasteText(std::string &text, std::vector<ImVec4> &colors, EditorState &state, bool &text_changed)
{
    const char *clipboard_text = ImGui::GetClipboardText();
    if (clipboard_text != nullptr)
    {
        std::string paste_content = clipboard_text;
        if (!paste_content.empty())
        {
            int paste_start = state.cursor_pos;
            int paste_end = paste_start + paste_content.size();
            if (state.selection_start != state.selection_end)
            {
                int start = gEditor.getSelectionStart(state);
                int end = gEditor.getSelectionEnd(state);
                text.replace(start, end - start, paste_content);
                colors.erase(colors.begin() + start, colors.begin() + end);
                colors.insert(colors.begin() + start, paste_content.size(), ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
                paste_start = start;
                paste_end = start + paste_content.size();
            }
            else
            {
                text.insert(state.cursor_pos, paste_content);
                colors.insert(colors.begin() + state.cursor_pos, paste_content.size(), ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
            }
            state.cursor_pos = paste_end;
            state.selection_start = state.selection_end = state.cursor_pos;
            text_changed = true;

            // Trigger syntax highlighting for the pasted content
            gEditor.highlightContent(text, colors, paste_start, paste_end);
        }
    }
}
void Editor::handleMouseInput(const std::string &text, EditorState &state, const ImVec2 &text_start_pos, float line_height)
{
    static bool is_dragging = false;
    static int anchor_pos = -1; // anchor for shift-click selection

    ImVec2 mouse_pos = ImGui::GetMousePos();
    int char_index = getCharIndexFromCoords(text, mouse_pos, text_start_pos, state.line_starts, line_height);

    // When the left mouse button is clicked:
    if (ImGui::IsMouseClicked(ImGuiMouseButton_Left))
    {
        if (ImGui::GetIO().KeyShift)
        {
            // If shift is held and we're not already selecting, set the anchor to the current
            // cursor position.
            if (!state.is_selecting)
            {
                anchor_pos = state.cursor_pos;
                state.selection_start = anchor_pos;
                state.is_selecting = true;
            }
            // Update the selection end based on the new click.
            state.selection_end = char_index;
            state.cursor_pos = char_index;
        }
        else
        {
            // On a regular click (without shift), reset the selection and update the anchor.
            state.cursor_pos = char_index;
            anchor_pos = char_index;
            state.selection_start = char_index;
            state.selection_end = char_index;
            state.is_selecting = false;
        }
        is_dragging = true;
    }
    // If the mouse is being dragged:
    else if (ImGui::IsMouseDragging(ImGuiMouseButton_Left) && is_dragging)
    {
        if (ImGui::GetIO().KeyShift)
        {
            // If shift is held, use the existing anchor for updating selection.
            if (anchor_pos == -1)
            {
                anchor_pos = state.cursor_pos;
                state.selection_start = anchor_pos;
            }
            state.selection_end = char_index;
            state.cursor_pos = char_index;
        }
        else
        {
            // Normal drag selection without shift: use the initial click as the anchor.
            state.is_selecting = true;
            if (anchor_pos == -1)
            {
                anchor_pos = state.cursor_pos;
            }
            state.selection_start = anchor_pos;
            state.selection_end = char_index;
            state.cursor_pos = char_index;
        }
    }
    // On mouse release, reset dragging state and clear the anchor.
    else if (ImGui::IsMouseReleased(ImGuiMouseButton_Left))
    {
        is_dragging = false;
        anchor_pos = -1;
    }
}
void Editor::cursorLeft(EditorState &state)
{
    if (state.cursor_pos > 0)
    {
        state.cursor_pos--;
    }
}

void Editor::cursorRight(const std::string &text, EditorState &state)
{
    if (state.cursor_pos < text.size())
    {
        state.cursor_pos++;
    }
}

void Editor::cursorUp(const std::string &text, EditorState &state, float line_height, float window_height)
{
    int current_line = gEditor.getLineFromPos(state.line_starts, state.cursor_pos);
    if (current_line > 0)
    {
        int current_column = state.cursor_pos - state.line_starts[current_line];
        state.cursor_pos = std::min(state.line_starts[current_line - 1] + current_column, state.line_starts[current_line] - 1);
        state.scroll_pos.y = std::max(0.0f, state.scroll_pos.y - line_height);
    }
}

void Editor::cursorDown(const std::string &text, EditorState &state, float line_height, float window_height)
{
    int current_line = gEditor.getLineFromPos(state.line_starts, state.cursor_pos);
    if (current_line < state.line_starts.size() - 1)
    {
        int current_column = state.cursor_pos - state.line_starts[current_line];
        state.cursor_pos = std::min(state.line_starts[current_line + 1] + current_column, static_cast<int>(text.size()));
        state.scroll_pos.y = state.scroll_pos.y + line_height;
    }
}

void Editor::moveCursorVertically(std::string &text, EditorState &state, int line_delta)
{
    int current_line = gEditor.getLineFromPos(state.line_starts, state.cursor_pos);
    int target_line = std::max(0, std::min(static_cast<int>(state.line_starts.size()) - 1, current_line + line_delta));

    // Calculate the current column (horizontal position)
    int current_column = state.cursor_pos - state.line_starts[current_line];

    // Set the new cursor position
    int new_line_start = state.line_starts[target_line];
    int new_line_end = (target_line + 1 < state.line_starts.size()) ? state.line_starts[target_line + 1] - 1 : text.size();

    // Try to maintain the same column, but don't go past the end of the new
    // line
    state.cursor_pos = std::min(new_line_start + current_column, new_line_end);
}

void Editor::handleCursorMovement(const std::string &text, EditorState &state, const ImVec2 &text_pos, float line_height, float window_height, float window_width)
{
    float visible_start_y = ImGui::GetScrollY();
    float visible_end_y = visible_start_y + window_height;
    float visible_start_x = ImGui::GetScrollX();
    float visible_end_x = visible_start_x + window_width;

    bool shift_pressed = ImGui::GetIO().KeyShift;
    state.current_line = gEditor.getLineFromPos(state.line_starts, state.cursor_pos);
    // Start a new selection only if Shift is pressed and we're not already
    // selecting
    if (shift_pressed && !state.is_selecting)
    {
        state.is_selecting = true;
        state.selection_start = state.cursor_pos;
    }

    // Clear selection only if a movement key is pressed without Shift
    if (!shift_pressed && (ImGui::IsKeyPressed(ImGuiKey_UpArrow) || ImGui::IsKeyPressed(ImGuiKey_DownArrow) || ImGui::IsKeyPressed(ImGuiKey_LeftArrow) || ImGui::IsKeyPressed(ImGuiKey_RightArrow)))
    {
        state.is_selecting = false;
        state.selection_start = state.selection_end = state.cursor_pos;
    }

    if (ImGui::IsKeyPressed(ImGuiKey_UpArrow))
    {
        gEditor.cursorUp(text, state, line_height, window_height);
    }
    if (ImGui::IsKeyPressed(ImGuiKey_DownArrow))
    {
        gEditor.cursorDown(text, state, line_height, window_height);
    }
    if (ImGui::IsKeyPressed(ImGuiKey_LeftArrow))
    {
        gEditor.cursorLeft(state);
    }
    if (ImGui::IsKeyPressed(ImGuiKey_RightArrow))
    {
        gEditor.cursorRight(text, state);
    }

    if (state.is_selecting)
    {
        state.selection_end = state.cursor_pos;
    }

    // Ensure cursor stays in view after movement
    float cursor_y = text_pos.y + (gEditor.getLineFromPos(state.line_starts, state.cursor_pos) * line_height);
    float cursor_x = gEditor.calculateCursorXPosition(text_pos, text, state.cursor_pos);

    if (cursor_y < visible_start_y)
    {
        state.scroll_pos.y = cursor_y - text_pos.y;
    }
    else if (cursor_y + line_height > visible_end_y)
    {
        state.scroll_pos.y = cursor_y + line_height - window_height;
    }

    if (cursor_x < visible_start_x)
    {
        state.scroll_x = cursor_x - text_pos.x;
    }
    else if (cursor_x > visible_end_x)
    {
        state.scroll_x = cursor_x - window_width + ImGui::GetFontSize();
    }
}

// Text input handling
void Editor::handleCharacterInput(std::string &text, std::vector<ImVec4> &colors, EditorState &state, bool &text_changed, int &input_start, int &input_end)
{
    ImGuiIO &io = ImGui::GetIO();
    std::string input;
    input.reserve(io.InputQueueCharacters.Size);
    for (int n = 0; n < io.InputQueueCharacters.Size; n++)
    {
        char c = static_cast<char>(io.InputQueueCharacters[n]);
        if (c != 0 && c >= 32)
        {
            input += c;
        }
    }
    if (!input.empty())
    {
        // Clear any existing selection
        if (state.selection_start != state.selection_end)
        {
            int start = gEditor.getSelectionStart(state);
            int end = gEditor.getSelectionEnd(state);
            if (start < 0 || end > static_cast<int>(text.size()) || start > end)
            {
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
        if (state.cursor_pos < 0 || state.cursor_pos > static_cast<int>(text.size()))
        {
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

    if (gLineJump.hasJustJumped())
    {
        return;
    }

    if (ImGui::IsKeyPressed(ImGuiKey_Enter))
    {
        // Insert the newline character
        text.insert(state.cursor_pos, 1, '\n');

        // Safely insert the color
        if (state.cursor_pos <= colors.size())
        {
            colors.insert(colors.begin() + state.cursor_pos, 1, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
        }
        else
        {
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
    if (ImGui::IsKeyPressed(ImGuiKey_Delete))
    {
        if (state.selection_start != state.selection_end)
        {
            // There's a selection, delete it
            int start = gEditor.getSelectionStart(state);
            int end = gEditor.getSelectionEnd(state);
            text.erase(start, end - start);
            colors.erase(colors.begin() + start, colors.begin() + end - 1);
            state.cursor_pos = start;
            text_changed = true;
            input_end = start;
        }
        else if (state.cursor_pos < text.size())
        {
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

    if (ImGui::IsKeyPressed(ImGuiKey_Backspace))
    {
        if (state.selection_start != state.selection_end)
        {
            // There's a selection, delete it
            int start = gEditor.getSelectionStart(state);
            int end = gEditor.getSelectionEnd(state);
            text.erase(start, end - start);
            colors.erase(colors.begin() + start, colors.begin() + end);
            state.cursor_pos = start;
            text_changed = true;
            input_start = start;
        }
        else if (state.cursor_pos > 0)
        {
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
void Editor::handleTabKey(std::string &text, std::vector<ImVec4> &colors, EditorState &state, bool &text_changed, int &input_end)
{

    if (ImGui::IsKeyPressed(ImGuiKey_Tab))
    {
        if (state.is_selecting)
        {
            // Handle multi-line indentation
            int start = std::min(state.selection_start, state.selection_end);
            int end = std::max(state.selection_start, state.selection_end);

            // Find the start of the first line
            int firstLineStart = start;
            while (firstLineStart > 0 && text[firstLineStart - 1] != '\n')
            {
                firstLineStart--;
            }

            // Find the end of the last line
            int lastLineEnd = end;
            while (lastLineEnd < text.length() && text[lastLineEnd] != '\n')
            {
                lastLineEnd++;
            }

            int tabsInserted = 0;
            int lineStart = firstLineStart;
            while (lineStart < lastLineEnd)
            {
                // Insert tab at the beginning of the line
                text.insert(lineStart, 1, '\t');
                colors.insert(colors.begin() + lineStart, 1, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
                tabsInserted++;

                // Move to the next line
                lineStart = text.find('\n', lineStart) + 1;
                if (lineStart == 0)
                    break; // If we've reached the end of
                           // the text
            }

            // Adjust cursor and selection positions
            state.cursor_pos += (state.cursor_pos >= start) ? tabsInserted : 0;
            state.selection_start += (state.selection_start > start) ? tabsInserted : 0;
            state.selection_end += tabsInserted;

            input_end = lastLineEnd + tabsInserted;
        }
        else
        {
            // Insert a single tab character at cursor position
            text.insert(state.cursor_pos, 1, '\t');
            colors.insert(colors.begin() + state.cursor_pos, 1, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
            state.cursor_pos++;
            state.selection_start = state.selection_end = state.cursor_pos;
            input_end = state.cursor_pos;
        }

        // Mark text as changed and update
        text_changed = true;
        gEditor.updateLineStarts(text, state.line_starts);
        gFileExplorer.setUnsavedChanges(true);

        // Trigger syntax highlighting for the affected area
        gEditor.highlightContent(text, colors, std::min(state.selection_start, state.selection_end), std::max(state.selection_end, input_end));
    }
}

void Editor::moveWordForward(const std::string &text, EditorState &state)
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

void Editor::moveWordBackward(const std::string &text, EditorState &state)
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

void Editor::removeIndentation(std::string &text, EditorState &state)
{
    int start, end;
    if (state.is_selecting)
    {
        start = std::min(state.selection_start, state.selection_end);
        end = std::max(state.selection_start, state.selection_end);
    }
    else
    {
        // If no selection, work on the current line
        start = end = state.cursor_pos;
    }

    // Find the start of the first line
    int firstLineStart = start;
    while (firstLineStart > 0 && text[firstLineStart - 1] != '\n')
    {
        firstLineStart--;
    }

    // Find the end of the last line
    int lastLineEnd = end;
    while (lastLineEnd < text.length() && text[lastLineEnd] != '\n')
    {
        lastLineEnd++;
    }

    int totalSpacesRemoved = 0;
    std::string newText;
    newText.reserve(text.length());

    // Copy text before the affected lines
    newText.append(text.substr(0, firstLineStart));

    // Process each line
    size_t lineStart = firstLineStart;
    while (lineStart <= lastLineEnd)
    {
        // Check for spaces or tab at the beginning of the line
        int spacesToRemove = 0;
        if (lineStart + 4 <= text.length() && text.substr(lineStart, 4) == "    ")
        {
            spacesToRemove = 4;
        }
        else if (lineStart < text.length() && text[lineStart] == '\t')
        {
            spacesToRemove = 1;
        }

        // Append the line without leading indentation
        size_t lineEnd = text.find('\n', lineStart);
        if (lineEnd == std::string::npos || lineEnd > lastLineEnd)
            lineEnd = lastLineEnd;
        newText.append(text.substr(lineStart + spacesToRemove, lineEnd - lineStart - spacesToRemove));
        if (lineEnd < lastLineEnd)
            newText.push_back('\n');

        totalSpacesRemoved += spacesToRemove;
        lineStart = lineEnd + 1;
    }

    // Copy text after the affected lines
    newText.append(text.substr(lastLineEnd));

    // Update text and adjust cursor and selection
    text = std::move(newText);
    state.cursor_pos = std::max(state.cursor_pos - totalSpacesRemoved, firstLineStart);
    if (state.is_selecting)
    {
        state.selection_start = std::max(state.selection_start - totalSpacesRemoved, firstLineStart);
        state.selection_end = std::max(state.selection_end - totalSpacesRemoved, firstLineStart);
    }
    else
    {
        state.selection_start = state.selection_end = state.cursor_pos;
    }

    // Update colors vector
    auto &colors = gFileExplorer.getFileColors();
    colors.erase(colors.begin() + firstLineStart, colors.begin() + lastLineEnd);
    colors.insert(colors.begin() + firstLineStart, lastLineEnd - firstLineStart - totalSpacesRemoved, ImVec4(1.0f, 1.0f, 1.0f, 1.0f)); // Insert default color
    // Update line starts
    gEditor.updateLineStarts(text, state.line_starts);

    // Mark text as changed
    gFileExplorer.setUnsavedChanges(true);

    // Trigger syntax highlighting for the affected area
    gEditor.highlightContent(text, colors, firstLineStart, lastLineEnd - totalSpacesRemoved);
}

void Editor::handleTextInput(std::string &text, std::vector<ImVec4> &colors, EditorState &state, bool &text_changed)
{
    int input_start = state.cursor_pos;
    int input_end = state.cursor_pos;

    // Handle selection deletion only for Enter key
    if (state.selection_start != state.selection_end && ImGui::IsKeyPressed(ImGuiKey_Enter))
    {
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
    gEditor.handleTabKey(text, colors, state, text_changed, input_end);

    if (text_changed)
    {
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

    // Calculate visible range
    float scroll_y = ImGui::GetScrollY();
    float window_height = ImGui::GetWindowHeight();
    int start_line = static_cast<int>(scroll_y / line_height);
    int end_line = start_line + static_cast<int>(window_height / line_height) + 1;

    int current_line = 0;
    for (size_t i = 0; i < text.size(); i++)
    {
        // Safety check
        if (i >= colors.size())
        {
            std::cerr << "Error: Color index out of bounds in renderTextWithSelection" << std::endl;
            break;
        }

        // Handle newline: update line count and reset x
        if (text[i] == '\n')
        {
            current_line++;
            if (current_line > end_line)
                break;
            text_pos.x = pos.x;
            text_pos.y += line_height;
            continue;
        }

        // Skip lines above visible area
        if (current_line < start_line)
        {
            while (i < text.size() && text[i] != '\n')
                i++;
            i--; // Adjust for the outer loop's increment
            continue;
        }
        if (current_line > end_line)
            break;

        // Check if this character is selected
        bool is_selected = (i >= sel_start && i < sel_end);
        if (is_selected)
        {
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

void Editor::renderCursor(ImDrawList *draw_list, const ImVec2 &cursor_screen_pos, float line_height, float blink_time)
{
    float blink_alpha = (sinf(blink_time * 4.0f) + 1.0f) * 0.5f; // Blink frequency
    ImU32 cursor_color;
    bool rainbow_mode = gSettings.getRainbowMode(); // Get setting here

    if (rainbow_mode)
    {
        ImVec4 rainbow = GetRainbowColor(blink_time * 2.0f); // Rainbow frequency
        cursor_color = ImGui::ColorConvertFloat4ToU32(rainbow);
    }
    else
    {
        cursor_color = IM_COL32(255, 255, 255, (int)(blink_alpha * 255));
    }

    draw_list->AddLine(cursor_screen_pos, ImVec2(cursor_screen_pos.x, cursor_screen_pos.y + line_height - 1), cursor_color);
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
    for (int i = 1; i <= n; i++)
    {
        insertionPositions[i] = ImGui::CalcTextSize(&text[line_start], &text[line_start + i]).x;
    }

    // Compute the click's x-coordinate relative to the beginning of the text.
    float click_x = click_pos.x - text_start_pos.x;

    // Find the insertion index (0...n) whose position is closest to click_x.
    int bestIndex = 0;
    float bestDist = std::abs(click_x - insertionPositions[0]);
    for (int i = 1; i <= n; i++)
    {
        float d = std::abs(click_x - insertionPositions[i]);
        if (d < bestDist)
        {
            bestDist = d;
            bestIndex = i;
        }
    }

    // Return the character index in the full text corresponding to that insertion point.
    return line_start + bestIndex;
}

void Editor::renderLineNumbers(const ImVec2 &pos, float line_number_width, float line_height, int num_lines, float scroll_y, float window_height, const EditorState &editor_state, float blink_time)
{
    static char line_number_buffer[16];
    ImDrawList *draw_list = ImGui::GetWindowDrawList();
    const ImU32 default_line_number_color = IM_COL32(128, 128, 128, 255);
    const ImU32 current_line_color = IM_COL32(255, 255, 255, 255);
    const ImU32 selected_line_color = IM_COL32(0, 40, 255, 200); // Neon pink color
    int start_line = static_cast<int>(scroll_y / line_height);
    int end_line = std::min(num_lines, static_cast<int>((scroll_y + window_height) / line_height) + 1);
    // Pre-calculate rainbow color
    ImU32 rainbow_color = current_line_color;
    bool rainbow_mode = gSettings.getRainbowMode(); // Get setting here
    if (rainbow_mode)
    {
        // Update color less frequently
        static float last_update_time = 0.0f;
        static ImU32 last_rainbow_color = current_line_color;
        if (blink_time - last_update_time > 0.05f)
        { // Update every 50ms
            ImVec4 rainbow = GetRainbowColor(blink_time * 2.0f);
            last_rainbow_color = ImGui::ColorConvertFloat4ToU32(rainbow);
            last_update_time = blink_time;
        }
        rainbow_color = last_rainbow_color;
    }
    // Determine the selected lines, accounting for selection direction
    int selection_start = std::min(editor_state.selection_start, editor_state.selection_end);
    int selection_end = std::max(editor_state.selection_start, editor_state.selection_end);
    int selection_start_line = std::lower_bound(editor_state.line_starts.begin(), editor_state.line_starts.end(), selection_start) - editor_state.line_starts.begin();
    int selection_end_line = std::lower_bound(editor_state.line_starts.begin(), editor_state.line_starts.end(), selection_end) - editor_state.line_starts.begin();
    for (int i = start_line; i < end_line; i++)
    {
        float y_pos = pos.y + (i * line_height) - scroll_y;
        snprintf(line_number_buffer, sizeof(line_number_buffer), "%d", i + 1);
        ImU32 line_number_color;
        if (i >= selection_start_line && i < selection_end_line && editor_state.is_selecting)
        {
            line_number_color = selected_line_color; // Use neon pink for selected
                                                     // lines
        }
        else if (i == editor_state.current_line)
        {
            line_number_color = rainbow_mode ? rainbow_color : current_line_color;
        }
        else
        {
            line_number_color = default_line_number_color;
        }
        // Calculate the position for right-aligned text
        float text_width = ImGui::CalcTextSize(line_number_buffer).x;
        float x_pos = pos.x + line_number_width - text_width - 8.0f; // 4.0f is a small right margin
        draw_list->AddText(ImVec2(x_pos, y_pos), line_number_color, line_number_buffer);
    }
}
float Editor::calculateTextWidth(const std::string &text, const std::vector<int> &line_starts)
{
    // Assume updateLineStarts has been called so that editor_state.line_widths is up to date.
    float max_width = 0.0f;
    for (float width : editor_state.line_widths)
    {
        max_width = std::max(max_width, width);
    }
    return max_width;
}
bool Editor::processIndentRemoval(std::string &text, EditorState &state, bool &text_changed, CursorVisibility &ensure_cursor_visible)
{
    // If Shift+Tab is pressed, remove indentation and exit early.
    if (ImGui::GetIO().KeyShift && ImGui::IsKeyPressed(ImGuiKey_Tab, false))
    {
        removeIndentation(text, state);
        text_changed = true;
        ensure_cursor_visible.horizontal = true;
        ensure_cursor_visible.vertical = true;
        ImGui::SetKeyboardFocusHere(-1); // Prevent default tab behavior
        return true;
    }
    return false;
}

void Editor::processFontSizeAdjustment(CursorVisibility &ensure_cursor_visible)
{
    if (ImGui::IsKeyPressed(ImGuiKey_Equal))
    { // '+' key
        float currentSize = gSettings.getFontSize();
        gSettings.setFontSize(currentSize + 2.0f);
        ensure_cursor_visible.vertical = true;
        ensure_cursor_visible.horizontal = true;
        std::cout << "Cmd++: Font size increased to " << gSettings.getFontSize() << std::endl;
    }
    else if (ImGui::IsKeyPressed(ImGuiKey_Minus))
    { // '-' key
        float currentSize = gSettings.getFontSize();
        gSettings.setFontSize(std::max(currentSize - 2.0f, 8.0f));
        ensure_cursor_visible.vertical = true;
        ensure_cursor_visible.horizontal = true;
        std::cout << "Cmd+-: Font size decreased to " << gSettings.getFontSize() << std::endl;
    }
}

void Editor::processSelectAll(std::string &text, EditorState &state, CursorVisibility &ensure_cursor_visible)
{
    if (ImGui::IsKeyPressed(ImGuiKey_A))
    {
        selectAllText(state, text);
        ensure_cursor_visible.vertical = true;
        ensure_cursor_visible.horizontal = true;
        std::cout << "Ctrl+A: Selected all text" << std::endl;
    }
}

void Editor::processUndoRedo(std::string &text, std::vector<ImVec4> &colors, EditorState &state, bool &text_changed, CursorVisibility &ensure_cursor_visible, bool shift_pressed)
{
    if (ImGui::IsKeyPressed(ImGuiKey_Z))
    {
        std::cout << "Z key pressed. Ctrl: " << ImGui::GetIO().KeyCtrl << ", Shift: " << shift_pressed << std::endl;
        int oldCursorPos = state.cursor_pos;
        int oldLine = getLineFromPos(state.line_starts, oldCursorPos);
        int oldColumn = oldCursorPos - state.line_starts[oldLine];
        if (shift_pressed)
        {
            std::cout << "Attempting Redo" << std::endl;
            gFileExplorer.handleRedo();
        }
        else
        {
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

void Editor::processWordMovement(std::string &text, EditorState &state, CursorVisibility &ensure_cursor_visible, bool shift_pressed)
{
    if (ImGui::IsKeyPressed(ImGuiKey_W))
    {
        if (shift_pressed)
        {
            moveWordBackward(text, state);
        }
        else
        {
            moveWordForward(text, state);
        }
        ensure_cursor_visible.horizontal = true;
        ensure_cursor_visible.vertical = true;
    }
}

void Editor::processCursorJump(std::string &text, EditorState &state, CursorVisibility &ensure_cursor_visible)
{
    if (ImGui::IsKeyPressed(ImGuiKey_LeftArrow))
    {
        int current_line = getLineFromPos(state.line_starts, state.cursor_pos);
        state.cursor_pos = state.line_starts[current_line] + 1;
        ensure_cursor_visible.horizontal = true;
        std::cout << "Ctrl/Cmd+Left: Cursor moved to " << state.cursor_pos << std::endl;
    }
    else if (ImGui::IsKeyPressed(ImGuiKey_RightArrow))
    {
        int current_line = getLineFromPos(state.line_starts, state.cursor_pos);
        int next_line = current_line + 1;
        if (next_line < state.line_starts.size())
        {
            state.cursor_pos = state.line_starts[next_line] - 2; // Position before the newline
        }
        else
        {
            state.cursor_pos = text.size();
        }
        ensure_cursor_visible.horizontal = true;
        std::cout << "Ctrl+Right: Cursor moved to " << state.cursor_pos << std::endl;
    }
    else if (ImGui::IsKeyPressed(ImGuiKey_UpArrow))
    {
        moveCursorVertically(text, state, -5);
        ensure_cursor_visible.vertical = true;
        ensure_cursor_visible.horizontal = true;
        std::cout << "Ctrl+Up: Cursor moved 5 lines up to " << state.cursor_pos << std::endl;
    }
    else if (ImGui::IsKeyPressed(ImGuiKey_DownArrow))
    {
        moveCursorVertically(text, state, 5);
        ensure_cursor_visible.vertical = true;
        ensure_cursor_visible.horizontal = true;
        std::cout << "Ctrl+Down: Cursor moved 5 lines down to " << state.cursor_pos << std::endl;
    }
}

void Editor::processMouseWheelScrolling(float line_height, EditorState &state)
{
    float wheel_y = ImGui::GetIO().MouseWheel;
    float wheel_x = ImGui::GetIO().MouseWheelH;
    if (wheel_y != 0 || wheel_x != 0)
    {
        if (ImGui::GetIO().KeyShift)
        { // Horizontal scrolling with Shift+Scroll
            float scroll_amount = wheel_y * ImGui::GetFontSize() * 3;
            float new_scroll_x = ImGui::GetScrollX() - scroll_amount;
            new_scroll_x = std::max(0.0f, std::min(new_scroll_x, ImGui::GetScrollMaxX()));
            ImGui::SetScrollX(new_scroll_x);
            state.scroll_pos.x = new_scroll_x;
        }
        else
        { // Vertical scrolling
            float new_scroll_y = ImGui::GetScrollY() - wheel_y * line_height * 3;
            new_scroll_y = std::max(0.0f, std::min(new_scroll_y, ImGui::GetScrollMaxY()));
            ImGui::SetScrollY(new_scroll_y);
            state.scroll_pos.y = new_scroll_y;
        }
    }
}

void Editor::processClipboardShortcuts(std::string &text, std::vector<ImVec4> &colors, EditorState &state, bool &text_changed, CursorVisibility &ensure_cursor_visible)
{
    if (ImGui::IsKeyPressed(ImGuiKey_C, false))
    {
        copySelectedText(text, state);
    }
    if (ImGui::IsKeyPressed(ImGuiKey_X, false))
    {
        if (state.selection_start != state.selection_end)
            cutSelectedText(text, colors, state, text_changed);
        else
            cutWholeLine(text, colors, state, text_changed);
        ensure_cursor_visible.vertical = true;
        ensure_cursor_visible.horizontal = true;
    }
    if (ImGui::IsKeyPressed(ImGuiKey_V, false))
    {
        pasteText(text, colors, state, text_changed);
        ensure_cursor_visible.vertical = true;
        ensure_cursor_visible.horizontal = true;
    }
}

void Editor::handleEditorInput(std::string &text, EditorState &state, const ImVec2 &text_start_pos, float line_height, bool &text_changed, std::vector<ImVec4> &colors, CursorVisibility &ensure_cursor_visible)
{
    bool ctrl_pressed = ImGui::GetIO().KeyCtrl;
    bool shift_pressed = ImGui::GetIO().KeyShift;

    // block input if searching for file...
    if (gFileFinder.isWindowOpen())
    {
        return;
    }
    // Process bookmarks first
    gBookmarks.handleBookmarkInput(gFileExplorer, state);

    if (ImGui::IsWindowFocused() && !state.blockInput)
    {
        // Process Shift+Tab for indentation removal. If handled, exit
        // early.
        if (processIndentRemoval(text, state, text_changed, ensure_cursor_visible))
            return;

        if (ctrl_pressed)
        {
            processFontSizeAdjustment(ensure_cursor_visible);
            processSelectAll(text, state, ensure_cursor_visible);
            processUndoRedo(text, colors, state, text_changed, ensure_cursor_visible, shift_pressed);
            processWordMovement(text, state, ensure_cursor_visible, shift_pressed);
            processCursorJump(text, state, ensure_cursor_visible);
        }
    }

    if (ImGui::IsWindowHovered())
    {
        handleMouseInput(text, state, text_start_pos, line_height);
        processMouseWheelScrolling(line_height, state);
    }

    // Additional arrow key presses outside the ctrl block
    if (ImGui::IsKeyPressed(ImGuiKey_UpArrow) || ImGui::IsKeyPressed(ImGuiKey_DownArrow))
        ensure_cursor_visible.vertical = true;
    if (ImGui::IsKeyPressed(ImGuiKey_LeftArrow) || ImGui::IsKeyPressed(ImGuiKey_RightArrow))
        ensure_cursor_visible.horizontal = true;

    handleCursorMovement(text, state, text_start_pos, line_height, ImGui::GetWindowHeight(), ImGui::GetWindowWidth());
    handleTextInput(text, colors, state, text_changed);

    if (ImGui::IsWindowFocused() && ctrl_pressed)
        processClipboardShortcuts(text, colors, state, text_changed, ensure_cursor_visible);

    // Ensure cursor is visible if text has changed
    if (text_changed)
    {
        ensure_cursor_visible.vertical = true;
        ensure_cursor_visible.horizontal = true;
    }
}

void Editor::cancelHighlighting()
{
    cancelHighlightFlag = true;
    if (highlightFuture.valid())
    {
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
    if (content.empty())
    {
        std::cerr << "Error: Empty content in highlightContent" << std::endl;
        return false;
    }
    if (colors.empty())
    {
        std::cerr << "Error: Empty colors vector in highlightContent" << std::endl;
        return false;
    }
    if (content.size() != colors.size())
    {
        std::cerr << "Error: Mismatch between content and colors size "
                     "in highlightContent"
                  << std::endl;
        return false;
    }
    if (start_pos < 0 || start_pos >= static_cast<int>(content.size()))
    {
        std::cerr << "Error: Invalid start_pos in highlightContent" << std::endl;
        return false;
    }
    if (end_pos < start_pos || end_pos > static_cast<int>(content.size()))
    {
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
    if (content.size() > LARGE_FILE_THRESHOLD)
    {
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
    highlightFuture = std::async(std::launch::async,
                                 [this, content_copy, &colors, colors_copy = std::move(colors_copy), currentFile, start_pos, end_pos, extension]() mutable
                                 {
                                     try
                                     {
                                         if (cancelHighlightFlag)
                                         {
                                             highlightingInProgress = false;
                                             return;
                                         }

                                         // Apply highlighting
                                         if (extension == ".cpp" || extension == ".h" || extension == ".hpp")
                                         {
                                             cppLexer.applyHighlighting(content_copy, colors_copy, 0);
                                         }
                                         else if (extension == ".py")
                                         {
                                             pythonLexer.applyHighlighting(content_copy, colors_copy, 0);
                                         }
                                         else if (extension == ".html")
                                         {
                                             htmlLexer.applyHighlighting(content_copy, colors_copy, 0);
                                         }
                                         else if (extension == ".js" || extension == ".jsx")
                                         {
                                             jsxLexer.applyHighlighting(content_copy, colors_copy, 0);
                                         }
                                         else
                                         {
                                             std::fill(colors_copy.begin() + start_pos, colors_copy.begin() + end_pos, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
                                         }

                                         if (!cancelHighlightFlag && currentFile == gFileExplorer.getCurrentFile())
                                         {
                                             std::lock_guard<std::mutex> colorsLock(colorsMutex);
                                             // Note: colors is captured by reference now
                                             if (colors.size() == colors_copy.size())
                                             {
                                                 colors = std::move(colors_copy);
                                             }
                                         }
                                     }
                                     catch (const std::exception &e)
                                     {
                                         std::cerr << "Error in highlighting: " << e.what() << std::endl;
                                     }
                                     highlightingInProgress = false;
                                 });
}

void Editor::setTheme(const std::string &themeName) { loadTheme(themeName); }

void Editor::loadTheme(const std::string &themeName)
{
    auto &settings = gSettings.getSettings();
    if (settings.contains("themes") && settings["themes"].contains(themeName))
    {
        auto &theme = settings["themes"][themeName];
        for (const auto &[key, value] : theme.items())
        {
            themeColors[key] = ImVec4(value[0], value[1], value[2], value[3]);
        }
    }
    else
    {
        // Set default colors if theme not found
        themeColors["keyword"] = ImVec4(0.0f, 0.4f, 1.0f, 1.0f);
        themeColors["string"] = ImVec4(0.87f, 0.87f, 0.0f, 1.0f);
        themeColors["number"] = ImVec4(0.0f, 0.8f, 0.8f, 1.0f);
        themeColors["comment"] = ImVec4(0.5f, 0.5f, 0.5f, 1.0f);
    }
}
