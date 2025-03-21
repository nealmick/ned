/*
    File: editor.cpp
    Description: Main editor coordinator that manages the text editing interface.

    The editor follows a clear execution flow:
    1. Initialization & Setup - Prepare the editor window, initialize data structures
    2. Content Analysis - Parse text to identify lines and compute layout metrics
    3. Input Processing - Handle keyboard and mouse input to modify text
    4. Scroll Management - Handle scrolling and cursor visibility
    5. Rendering - Draw the text, cursor, selection, and UI elements
    6. Cleanup - Finalize the frame and return control
*/

#include "editor.h"
#include "editor_bookmarks.h"
#include "editor_copy_paste.h"
#include "editor_cursor.h"
#include "editor_highlight.h"
#include "editor_keyboard.h"
#include "editor_line_jump.h"
#include "editor_line_numbers.h"
#include "editor_mouse.h"
#include "editor_render.h"
#include "editor_selection.h"
#include "editor_utils.h"

#include "../files/file_finder.h"
#include "../files/files.h"
#include "../util/settings.h"

#include <algorithm>
#include <cmath>
#include <iostream>

Editor gEditor;
EditorState editor_state;

//==============================================================================
// Main Editor Function
//==============================================================================

bool Editor::textEditor(const char *label, std::string &text, std::vector<ImVec4> &colors, EditorState &editor_state)
{
    // Initialize variables
    bool text_changed = false;
    CursorVisibility ensure_cursor_visible = {false, false};
    ImVec2 size, text_pos, line_numbers_pos;
    float line_height, line_number_width, total_height, editor_top_margin, text_left_margin;
    float current_scroll_x, current_scroll_y;

    // PHASE 1: Setup the editor display and windows
    setupEditorDisplay(label, text, colors, editor_state, size, line_height, line_numbers_pos, text_pos, line_number_width, total_height, editor_top_margin, text_left_margin, current_scroll_x, current_scroll_y);

    // PHASE 2: Process user input and handle scrolling
    text_changed = processEditorInput(text, colors, editor_state, text_pos, line_height, size, current_scroll_x, current_scroll_y, ensure_cursor_visible);

    // PHASE 3: Render editor content and finalize frame
    gEditorRender.renderEditorFrame(text, colors, editor_state, text_pos, line_height, line_numbers_pos, line_number_width, size, total_height, editor_top_margin);

    return text_changed;
}

//==============================================================================
// Text Editor Helper Functions
//==============================================================================

void Editor::setupEditorDisplay(const char *label, std::string &text, std::vector<ImVec4> &colors, EditorState &editor_state, ImVec2 &size, float &line_height, ImVec2 &line_numbers_pos, ImVec2 &text_pos, float &line_number_width, float &total_height, float &editor_top_margin, float &text_left_margin, float &current_scroll_x, float &current_scroll_y)
{
    // Validate input data and prepare state
    gEditorRender.validateAndResizeColors(text, colors);

    gEditorCursor.updateBlinkTime(editor_state, ImGui::GetIO().DeltaTime);

    // Setup editor layout parameters
    gEditorRender.setupEditorWindow(label, size, line_number_width, line_height, editor_top_margin, text_left_margin);

    // Setup line numbers panel
    line_numbers_pos = gEditorRender.renderLineNumbersPanel(line_number_width, editor_top_margin);

    // Parse text and compute line information
    updateLineStarts(text, editor_state.editor_content_lines);
    total_height = line_height * editor_state.editor_content_lines.size();

    // Calculate content dimensions for layout
    float remaining_width = size.x - line_number_width;
    float content_width = calculateTextWidth(text, editor_state.editor_content_lines) + ImGui::GetFontSize() * 10.0f;
    float content_height = editor_state.editor_content_lines.size() * line_height;

    // Setup the main editor child window
    gEditorRender.beginTextEditorChild(label, remaining_width, content_width, content_height, current_scroll_y, current_scroll_x, text_pos, editor_top_margin, text_left_margin, editor_state);
}

bool Editor::processEditorInput(std::string &text, std::vector<ImVec4> &colors, EditorState &editor_state, ImVec2 &text_pos, float line_height, ImVec2 &size, float &current_scroll_x, float &current_scroll_y, CursorVisibility &ensure_cursor_visible)
{
    bool text_changed = false;
    ImVec2 text_start_pos = text_pos;
    int initial_cursor_pos = editor_state.cursor_column;

    // Process keyboard input (text editing, cursor movement, etc.)
    gEditorKeyboard.processTextEditorInput(text, editor_state, text_start_pos, line_height, text_changed, colors, ensure_cursor_visible, initial_cursor_pos);

    // Handle context menu (right-click menu)
    gEditorMouse.handleContextMenu(text, colors, editor_state, text_changed);

    // Handle mouse wheel scrolling
    gEditorScroll.processMouseWheelForEditor(line_height, current_scroll_y, current_scroll_x, editor_state);

    // Ensure cursor visibility by adjusting scroll if needed
    gEditorScroll.adjustScrollForCursorVisibility(text_pos, text, editor_state, line_height, size.y, size.x, current_scroll_y, current_scroll_x, ensure_cursor_visible);

    // Update scroll animation (smooth scrolling)
    gEditorScroll.updateScrollAnimation(editor_state, current_scroll_x, current_scroll_y, ImGui::GetIO().DeltaTime);

    // Apply calculated scroll positions to ImGui
    ImGui::SetScrollY(current_scroll_y);
    ImGui::SetScrollX(current_scroll_x);

    return text_changed;
}

//==============================================================================
// Text Content Analysis
//==============================================================================

/*
 * Updates the line_starts vector with the character positions where each line begins.
 * Also computes and caches the width of each line for layout calculations.
 *
 * @param text         The text content to analyze
 * @param line_starts  Vector to populate with line start positions
 */
void Editor::updateLineStarts(const std::string &text, std::vector<int> &line_starts)
{
    // Skip update if text hasn't changed since last analysis
    if (text == editor_state.cached_text) {
        return;
    }

    // Update cached text and clear old data
    editor_state.cached_text = text;
    line_starts.clear();
    editor_state.line_widths.clear();

    // Find all line breaks and record starting positions
    // The first entry (index 0) is always 0, representing the start of the first line
    line_starts.reserve(text.size() / 40); // Heuristic: assume average line length of 40 chars
    line_starts.push_back(0);

    size_t pos = 0;
    while ((pos = text.find('\n', pos)) != std::string::npos) {
        line_starts.push_back(pos + 1); // Position after the newline character
        ++pos;
    }

    // Calculate and cache the pixel width of each line for layout calculations
    for (size_t i = 0; i < line_starts.size(); ++i) {
        int start = line_starts[i];
        int end = (i + 1 < line_starts.size()) ? line_starts[i + 1] - 1 : text.size();
        float width = ImGui::CalcTextSize(text.c_str() + start, text.c_str() + end).x;
        editor_state.line_widths.push_back(width);
    }
}

/*
 * Gets the line number for a given character position in the text.
 *
 * @param line_starts  Vector of character positions where lines start
 * @param pos          Character position to find the line for
 * @return             The line number (0-based) containing the character position
 */
int Editor::getLineFromPos(const std::vector<int> &line_starts, int pos)
{
    auto it = std::upper_bound(line_starts.begin(), line_starts.end(), pos);
    return std::distance(line_starts.begin(), it) - 1;
}

/*
 * Calculates the maximum width of all lines in the text.
 * Used to determine the horizontal scroll range.
 *
 * @param text         The text content
 * @param line_starts  Vector of character positions where lines start
 * @return             The maximum line width in pixels
 */
float Editor::calculateTextWidth(const std::string &text, const std::vector<int> &line_starts)
{
    // Find the maximum line width from the cached line widths
    float max_width = 0.0f;
    for (float width : editor_state.line_widths) {
        max_width = std::max(max_width, width);
    }
    return max_width;
}
