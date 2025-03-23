#pragma once
#include "editor_cursor.h"
#include "editor_types.h"
#include "imgui.h"
#include <string>
#include <vector>

// Forward declarations
class Editor;
extern Editor gEditor;

class EditorKeyboard
{
  public:
    EditorKeyboard();
    ~EditorKeyboard() = default;

    void handleCharacterInput(std::string &text, std::vector<ImVec4> &colors, bool &text_changed, int &input_start, int &input_end);
    void handleEnterKey(std::string &text, std::vector<ImVec4> &colors, bool &text_changed, int &input_end);
    void handleDeleteKey(std::string &text, std::vector<ImVec4> &colors, bool &text_changed, int &input_end);
    void handleBackspaceKey(std::string &text, std::vector<ImVec4> &colors, bool &text_changed, int &input_start);
    void handleTextInput(std::string &text, std::vector<ImVec4> &colors, bool &text_changed);

    void processFontSizeAdjustment(CursorVisibility &ensure_cursor_visible);
    void processSelectAll(std::string &text, CursorVisibility &ensure_cursor_visible);

    void processTextEditorInput(std::string &text, const ImVec2 &text_start_pos, float line_height, bool &text_changed, std::vector<ImVec4> &colors, CursorVisibility &ensure_cursor_visible, int initial_cursor_pos);
    void handleEditorKeyboardInput(std::string &text, const ImVec2 &text_start_pos, float line_height, bool &text_changed, std::vector<ImVec4> &colors, CursorVisibility &ensure_cursor_visible);

    void handleArrowKeyVisibility(CursorVisibility &ensure_cursor_visible);

    void updateCursorVisibilityOnTextChange(bool text_changed, CursorVisibility &ensure_cursor_visible);

    void processUndoRedo(std::string &text, std::vector<ImVec4> &colors, bool &text_changed, CursorVisibility &ensure_cursor_visible, bool shift_pressed);
};

// Global instance
extern EditorKeyboard gEditorKeyboard;