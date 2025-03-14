/*
    File: editor.h
    Description: Main editor coordinator that manages the text editing interface.
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
    bool textEditor(const char *label, std::string &text, std::vector<ImVec4> &colors, EditorState &editor_state);

    void setupEditorDisplay(const char *label, std::string &text, std::vector<ImVec4> &colors, EditorState &editor_state, ImVec2 &size, float &line_height, ImVec2 &line_numbers_pos, ImVec2 &text_pos, float &line_number_width, float &total_height, float &editor_top_margin, float &text_left_margin, float &current_scroll_x, float &current_scroll_y);

    bool processEditorInput(std::string &text, std::vector<ImVec4> &colors, EditorState &editor_state, ImVec2 &text_pos, float line_height, ImVec2 &size, float &current_scroll_x, float &current_scroll_y, CursorVisibility &ensure_cursor_visible);

    void updateLineStarts(const std::string &text, std::vector<int> &line_starts);

    int getLineFromPos(const std::vector<int> &line_starts, int pos);

    float calculateTextWidth(const std::string &text, const std::vector<int> &line_starts);
};