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

    void cursorLeft(EditorState &state);

    void cursorRight(const std::string &text, EditorState &state);

    void cursorUp(const std::string &text, EditorState &state, float line_height, float window_height);

    void cursorDown(const std::string &text, EditorState &state, float line_height, float window_height);

    void moveCursorVertically(std::string &text, EditorState &state, int line_delta);

    void moveWordForward(const std::string &text, EditorState &state);

    void moveWordBackward(const std::string &text, EditorState &state);

    void processWordMovement(std::string &text, EditorState &state, CursorVisibility &ensure_cursor_visible, bool shift_pressed);

    void processCursorJump(std::string &text, EditorState &state, CursorVisibility &ensure_cursor_visible);

    void handleCursorMovement(const std::string &text, EditorState &state, const ImVec2 &text_pos, float line_height, float window_height, float window_width);

    float getCursorYPosition(const EditorState &state, float line_height);

    float getCursorXPosition(const ImVec2 &text_pos, const std::string &text, int cursor_pos);

    void updateBlinkTime(EditorState &state, float deltaTime);

    void renderCursor(ImDrawList *draw_list, const ImVec2 &cursor_screen_pos, float line_height, float blink_time);

  private:
    int calculateVisualColumn(const std::string &text, int line_start, int cursor_pos);

    int findPositionFromVisualColumn(const std::string &text, int line_start, int line_end, int visual_column);
};

extern EditorCursor gEditorCursor;