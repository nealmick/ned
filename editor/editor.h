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
    // Main editor function - core rendering and interaction loop
    bool textEditor(const char *label, std::string &text, std::vector<ImVec4> &colors, EditorState &editor_state);

    // Text content analysis
    void updateLineStarts(const std::string &text, std::vector<int> &line_starts);
    int getLineFromPos(const std::vector<int> &line_starts, int pos);
    float calculateTextWidth(const std::string &text, const std::vector<int> &line_starts);

    // Clipboard operations (could potentially be moved to EditorCopyPaste)
    void copySelectedText(const std::string &text, const EditorState &state);
    void cutSelectedText(std::string &text, std::vector<ImVec4> &colors, EditorState &state, bool &text_changed);
    void cutWholeLine(std::string &text, std::vector<ImVec4> &colors, EditorState &state, bool &text_changed);
    void pasteText(std::string &text, std::vector<ImVec4> &colors, EditorState &state, bool &text_changed);

  private:
    // Main render pipeline
    bool validateAndResizeColors(std::string &text, std::vector<ImVec4> &colors);
    void setupEditorWindow(const char *label, ImVec2 &size, float &line_number_width, float &line_height, float &editor_top_margin, float &text_left_margin);
    ImVec2 renderLineNumbersPanel(float line_number_width, float editor_top_margin);
    void beginTextEditorChild(const char *label, float remaining_width, float content_width, float content_height, float &current_scroll_y, float &current_scroll_x, ImVec2 &text_pos, float editor_top_margin, float text_left_margin, EditorState &editor_state);
    void renderEditorContent(const std::string &text, const std::vector<ImVec4> &colors, EditorState &editor_state, float line_height, const ImVec2 &text_pos);
    void renderTextWithSelection(ImDrawList *drawList, const ImVec2 &pos, const std::string &text, const std::vector<ImVec4> &colors, const EditorState &state, float line_height);

    // Input handling
    void handleEditorInput(std::string &text, EditorState &state, const ImVec2 &text_start_pos, float line_height, bool &text_changed, std::vector<ImVec4> &colors, CursorVisibility &ensure_cursor_visible);
    void processTextEditorInput(std::string &text, EditorState &editor_state, const ImVec2 &text_start_pos, float line_height, bool &text_changed, std::vector<ImVec4> &colors, CursorVisibility &ensure_cursor_visible, int initial_cursor_pos);

    // Editor commands
    void processSelectAll(std::string &text, EditorState &state, CursorVisibility &ensure_cursor_visible);
    void processUndoRedo(std::string &text, std::vector<ImVec4> &colors, EditorState &state, bool &text_changed, CursorVisibility &ensure_cursor_visible, bool shift_pressed);
};
