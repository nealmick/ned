/*
	File: editor_cursor.h
	Description: Manages cursor positioning, movement, and rendering in the
   text editor.

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

	void processWordMovement(std::string &text, CursorVisibility &ensure_cursor_visible);

	void processCursorJump(std::string &text, CursorVisibility &ensure_cursor_visible);

	void handleCursorMovement(const std::string &text,
							  const ImVec2 &text_pos,
							  float line_height,
							  float window_height,
							  float window_width);

	float getCursorYPosition(float line_height);

	float
	getCursorXPosition(const ImVec2 &text_pos, const std::string &text, int cursor_pos);

	void updateBlinkTime();

	void renderCursor();
	void swapLines(int direction);
	void spawnCursorBelow();
	void spawnCursorAbove();
	static int CalculateVisualColumnForPosition(int position,
												const std::string &content,
												const std::vector<int> &content_lines);
	void calculateVisualColumn();

  private:
	void findPositionFromVisualColumn(int line_start, int line_end);

	static bool isWordChar(char c)
	{
		// Standard definition: Alphanumeric characters plus underscore
		return std::isalnum(static_cast<unsigned char>(c)) || c == '_';
	}
};

extern EditorCursor gEditorCursor;
