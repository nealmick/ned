/*
    File: editor.h
    Description: Main editor coordinator that manages the text editing interface.

    The Editor class serves as the central coordinator for the text editing experience.
    It orchestrates various specialized components (keyboard input, cursor handling,
    text rendering, etc.) to provide a cohesive editing interface.

    The main processing flow in the textEditor method:
    1. Initialization & Setup
    2. Content Analysis
    3. Input Processing
    4. Scroll Management
    5. Rendering
    6. Cleanup & Finalization
*/

#pragma once
#include "imgui.h"

#include "editor_copy_paste.h"
#include "editor_cursor.h"
#include "editor_highlight.h"
#include "editor_indentation.h"
#include "editor_keyboard.h"
#include "editor_line_numbers.h"
#include "editor_mouse.h"
#include "editor_render.h"
#include "editor_scroll.h"
#include "editor_types.h"

#include <string>
#include <vector>

// Forward declarations
class Editor;
extern Editor gEditor;
extern EditorState editor_state;

class Editor
{
  public:
    //--------------------------------------------------------------------------
    // Main Editor Function
    //--------------------------------------------------------------------------

    /**
     * Main editor function - core rendering and interaction loop.
     *
     * @param label        Unique identifier for the editor instance
     * @param text         Text content to edit (modified by user input)
     * @param colors       Color information for syntax highlighting
     * @param editor_state State information for cursor position, selection, etc.
     * @return             True if text was modified during this frame
     */
    bool textEditor(const char *label, std::string &text, std::vector<ImVec4> &colors, EditorState &editor_state);

    //--------------------------------------------------------------------------
    // Text Editor Helper Functions
    //--------------------------------------------------------------------------

    /**
     * Sets up the editor display and initializes layout parameters
     */
    void setupEditorDisplay(const char *label, std::string &text, std::vector<ImVec4> &colors, EditorState &editor_state, ImVec2 &size, float &line_height, ImVec2 &line_numbers_pos, ImVec2 &text_pos, float &line_number_width, float &total_height, float &editor_top_margin, float &text_left_margin, float &current_scroll_x, float &current_scroll_y);

    /**
     * Handles user input processing for the editor
     */
    bool processEditorInput(std::string &text, std::vector<ImVec4> &colors, EditorState &editor_state, ImVec2 &text_pos, float line_height, ImVec2 &size, float &current_scroll_x, float &current_scroll_y, CursorVisibility &ensure_cursor_visible);

    //--------------------------------------------------------------------------
    // Text Content Analysis
    //--------------------------------------------------------------------------

    /**
     * Updates the line_starts vector with positions where each line begins.
     * Also computes and caches the width of each line for layout calculations.
     *
     * @param text         The text content to analyze
     * @param line_starts  Vector to populate with line start positions
     */
    void updateLineStarts(const std::string &text, std::vector<int> &line_starts);

    /**
     * Gets the line number for a given character position in the text.
     *
     * @param line_starts  Vector of character positions where lines start
     * @param pos          Character position to find the line for
     * @return             The line number (0-based) containing the character position
     */
    int getLineFromPos(const std::vector<int> &line_starts, int pos);

    /**
     * Calculates the maximum width of all lines in the text.
     * Used to determine the horizontal scroll range.
     *
     * @param text         The text content
     * @param line_starts  Vector of character positions where lines start
     * @return             The maximum line width in pixels
     */
    float calculateTextWidth(const std::string &text, const std::vector<int> &line_starts);
};