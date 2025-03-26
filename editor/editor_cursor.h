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

    void cursorLeft();

    void cursorRight();

    void cursorUp();

    void cursorDown();

    void moveCursorVertically(std::string &text, int line_delta);

    void moveWordForward(const std::string &text);

    void moveWordBackward(const std::string &text);

    void processWordMovement(std::string &text, CursorVisibility &ensure_cursor_visible, bool shift_pressed);

    void processCursorJump(std::string &text, CursorVisibility &ensure_cursor_visible);

    void handleCursorMovement(const std::string &text, const ImVec2 &text_pos, float line_height, float window_height, float window_width);

    float getCursorYPosition(float line_height);

    float getCursorXPosition(const ImVec2 &text_pos, const std::string &text, int cursor_pos);

    void updateBlinkTime();

    void renderCursor();

  private:
    void calculateVisualColumn();

    void findPositionFromVisualColumn(int line_start, int line_end);
};

extern EditorCursor gEditorCursor;