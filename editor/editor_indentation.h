#pragma once
#include "editor_scroll.h"
#include "editor_types.h"
#include "imgui.h"
#include <string>
#include <vector>

// Forward declarations
class Editor;
extern Editor gEditor;

class EditorIndentation
{
  public:
    EditorIndentation() = default;
    ~EditorIndentation() = default;

    // Main API functions
    void handleTabKey(std::string &text, std::vector<ImVec4> &colors, bool &text_changed, int &input_end);
    void removeIndentation(std::string &text);
    bool processIndentRemoval(std::string &text, bool &text_changed, CursorVisibility &ensure_cursor_visible);

  private:
    // Selection helper methods
    int getSelectionStart(const EditorState &state) const;
    int getSelectionEnd(const EditorState &state) const;

    // Tab indentation helpers
    void handleMultiLineIndentation(std::string &text, std::vector<ImVec4> &colors, EditorState &state, int &input_end);
    void handleSingleLineIndentation(std::string &text, std::vector<ImVec4> &colors, EditorState &state, int &input_end);
    void finishIndentationChange(std::string &text, std::vector<ImVec4> &colors, EditorState &state, int &input_end, bool &text_changed);

    // Line finding helpers
    int findLineStart(const std::string &text, int position);
    int findLineEnd(const std::string &text, int position, size_t textLength);

    // Indentation removal helpers
    void processLineIndentRemoval(const std::string &text, std::string &newText, size_t lineStart, size_t lineEnd, size_t lastLineEnd, int &totalSpacesRemoved);
    void updateStateAfterIndentRemoval(std::string &text, EditorState &state, std::string &newText, int firstLineStart, int lastLineEnd, int totalSpacesRemoved);
    void updateColorsAfterIndentRemoval(std::string &text, EditorState &state, int firstLineStart, int lastLineEnd, int totalSpacesRemoved);
};

// Global instance
extern EditorIndentation gEditorIndentation;