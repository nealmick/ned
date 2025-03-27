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

    void handleCharacterInput();
    void handleEnterKey();
    void handleDeleteKey();
    void handleBackspaceKey();
    void handleTextInput();

    void processFontSizeAdjustment();
    void processSelectAll();

    void processTextEditorInput();

    void handleEditorKeyboardInput();

    void handleArrowKeyVisibility(CursorVisibility &ensure_cursor_visible);

    void updateCursorVisibilityOnTextChange(bool text_changed, CursorVisibility &ensure_cursor_visible);

    void processUndoRedo(std::string &text, std::vector<ImVec4> &colors, bool &text_changed, CursorVisibility &ensure_cursor_visible, bool shift_pressed);
};

// Global instance
extern EditorKeyboard gEditorKeyboard;