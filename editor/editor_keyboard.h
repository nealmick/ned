#pragma once
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
};

// Global instance
extern EditorKeyboard gEditorKeyboard;