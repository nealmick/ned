/*
    File: editor_cursor.h
    Description: Manages cursor positioning, movement, and rendering in the text editor.

    This class is responsible for all cursor-related functionality, including:
    - Cursor movement (arrow keys, word navigation, etc.)
    - Visual cursor rendering
    - Cursor position calculation
    - Cursor animation and timing
*/

#pragma once
#include "editor_scroll.h"
#include "editor_types.h"
#include "imgui.h"
#include <string>

// Forward declarations
class EditorCursor;
extern EditorCursor gEditorCursor;

class EditorCursor
{
  public:
    EditorCursor();
    ~EditorCursor() = default;

    //--------------------------------------------------------------------------
    // Basic Cursor Movement Operations
    //--------------------------------------------------------------------------

    /**
     * Moves the cursor one position to the left
     */
    void cursorLeft(EditorState &state);

    /**
     * Moves the cursor one position to the right
     */
    void cursorRight(const std::string &text, EditorState &state);

    /**
     * Moves the cursor up one line
     */
    void cursorUp(const std::string &text, EditorState &state, float line_height, float window_height);

    /**
     * Moves the cursor down one line
     */
    void cursorDown(const std::string &text, EditorState &state, float line_height, float window_height);

    /**
     * Moves the cursor vertically by a specified number of lines
     * @param line_delta Number of lines to move (positive = down, negative = up)
     */
    void moveCursorVertically(std::string &text, EditorState &state, int line_delta);

    //--------------------------------------------------------------------------
    // Word Navigation
    //--------------------------------------------------------------------------

    /**
     * Moves the cursor forward to the beginning of the next word
     */
    void moveWordForward(const std::string &text, EditorState &state);

    /**
     * Moves the cursor backward to the beginning of the previous word
     */
    void moveWordBackward(const std::string &text, EditorState &state);

    /**
     * Processes word-based cursor movement (Ctrl+Left/Right)
     * @param shift_pressed Whether shift is pressed (for selection)
     */
    void processWordMovement(std::string &text, EditorState &state, CursorVisibility &ensure_cursor_visible, bool shift_pressed);

    /**
     * Handles cursor jumps to beginning/end of line or document
     */
    void processCursorJump(std::string &text, EditorState &state, CursorVisibility &ensure_cursor_visible);

    //--------------------------------------------------------------------------
    // High-level Cursor Control
    //--------------------------------------------------------------------------

    /**
     * Main handler for cursor movement operations
     */
    void handleCursorMovement(const std::string &text, EditorState &state, const ImVec2 &text_pos, float line_height, float window_height, float window_width);

    //--------------------------------------------------------------------------
    // Cursor Position and Rendering
    //--------------------------------------------------------------------------

    /**
     * Gets the cursor's Y position in pixels
     * @return Y position in pixels
     */
    float getCursorYPosition(const EditorState &state, float line_height);

    /**
     * Calculates the cursor's X position in pixels
     * @return X position in pixels
     */
    float calculateCursorXPosition(const ImVec2 &text_pos, const std::string &text, int cursor_pos);

    /**
     * Updates the cursor blink timer
     * @param deltaTime Time elapsed since last frame
     */
    void updateBlinkTime(EditorState &state, float deltaTime);

    /**
     * Renders the cursor at the specified position
     */
    void renderCursor(ImDrawList *draw_list, const ImVec2 &cursor_screen_pos, float line_height, float blink_time);

  private:
    // Calculate visual column position considering tabs
    int calculateVisualColumn(const std::string &text, int line_start, int cursor_pos);

    // Find character position from visual column
    int findPositionFromVisualColumn(const std::string &text, int line_start, int line_end, int visual_column);
};

// Global instance
extern EditorCursor gEditorCursor;