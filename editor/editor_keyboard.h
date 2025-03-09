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

    // Text input handling
    void handleCharacterInput(std::string &text, std::vector<ImVec4> &colors, EditorState &state, bool &text_changed, int &input_start, int &input_end);
    void handleEnterKey(std::string &text, std::vector<ImVec4> &colors, EditorState &state, bool &text_changed, int &input_end);
    void handleDeleteKey(std::string &text, std::vector<ImVec4> &colors, EditorState &state, bool &text_changed, int &input_end);
    void handleBackspaceKey(std::string &text, std::vector<ImVec4> &colors, EditorState &state, bool &text_changed, int &input_start);

    void handleTextInput(std::string &text, std::vector<ImVec4> &colors, EditorState &state, bool &text_changed);
    void processFontSizeAdjustment(CursorVisibility &ensure_cursor_visible);
    void processSelectAll(std::string &text, EditorState &state, CursorVisibility &ensure_cursor_visible);
};

// Global instance
extern EditorKeyboard gEditorKeyboard;
