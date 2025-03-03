#pragma once
#include "editor_scroll.h"
#include "editor_types.h"
#include "imgui.h"
#include <string>
#include <vector>

// Forward declarations
class Editor;
extern Editor gEditor;

class EditorCopyPaste
{
  public:
    EditorCopyPaste() = default;
    ~EditorCopyPaste() = default;

    // Copy operation
    void copySelectedText(const std::string &text, const EditorState &state);

    // Cut operations
    void cutSelectedText(std::string &text, std::vector<ImVec4> &colors, EditorState &state, bool &text_changed);
    void cutWholeLine(std::string &text, std::vector<ImVec4> &colors, EditorState &state, bool &text_changed);

    // Paste operation
    void pasteText(std::string &text, std::vector<ImVec4> &colors, EditorState &state, bool &text_changed);
    void processClipboardShortcuts(std::string &text, std::vector<ImVec4> &colors, EditorState &state, bool &text_changed, CursorVisibility &ensure_cursor_visible);

  private:
    // Helper methods
    // Get selection bounds (helper methods)
    int getSelectionStart(const EditorState &state) const;
    int getSelectionEnd(const EditorState &state) const;
};

// Global instance
extern EditorCopyPaste gEditorCopyPaste;