/*
    File: editor.h
    Description: Main editor logic for displaying file content, handling keybinds and more...
*/

#pragma once
#include "imgui.h"

#include "editor_indentation.h"
#include "editor_line_numbers.h"
#include "editor_mouse.h"
#include "editor_scroll.h"
#include "editor_types.h"
#include "editor_utils.h"

#include "../lexers/cpp.h"
#include "../lexers/html.h"
#include "../lexers/jsx.h"
#include "../lexers/python.h"

#include <atomic>
#include <cmath>
#include <future>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

// Forward declarations
class Editor;
extern Editor gEditor;
extern EditorState editor_state;

class Editor
{
  public:
    // main editor function
    bool textEditor(const char *label, std::string &text, std::vector<ImVec4> &colors, EditorState &editor_state);

    void handleEditorInput(std::string &text, EditorState &state, const ImVec2 &text_start_pos, float line_height, bool &text_changed, std::vector<ImVec4> &colors, CursorVisibility &ensure_cursor_visible);

    void updateLineStarts(const std::string &text, std::vector<int> &line_starts);
    int getLineFromPos(const std::vector<int> &line_starts, int pos);

    // Selection management
    void startSelection(EditorState &state);
    void updateSelection(EditorState &state);
    void endSelection(EditorState &state);
    int getSelectionStart(const EditorState &state);
    int getSelectionEnd(const EditorState &state);

    // Clipboard operations
    void copySelectedText(const std::string &text, const EditorState &state);
    void cutSelectedText(std::string &text, std::vector<ImVec4> &colors, EditorState &state, bool &text_changed);
    void cutWholeLine(std::string &text, std::vector<ImVec4> &colors, EditorState &state, bool &text_changed);
    void pasteText(std::string &text, std::vector<ImVec4> &colors, EditorState &state, bool &text_changed);

    // Text selection
    void selectAllText(EditorState &state, const std::string &text);

    // Input handling
    void handleTextInput(std::string &text, std::vector<ImVec4> &colors, EditorState &state, bool &text_changed);

    // Rendering
    void renderTextWithSelection(ImDrawList *drawList, const ImVec2 &pos, const std::string &text, const std::vector<ImVec4> &colors, const EditorState &state, float line_height);

    // Utility functions
    float calculateTextWidth(const std::string &text, const std::vector<int> &line_starts);

  private:
    // Theme management

    bool showFileFinder = false;

    // handling editor input
    void processFontSizeAdjustment(CursorVisibility &ensure_cursor_visible);
    void processSelectAll(std::string &text, EditorState &state, CursorVisibility &ensure_cursor_visible);
    void processUndoRedo(std::string &text, std::vector<ImVec4> &colors, EditorState &state, bool &text_changed, CursorVisibility &ensure_cursor_visible, bool shift_pressed);

    bool validateAndResizeColors(std::string &text, std::vector<ImVec4> &colors);
    void setupEditorWindow(const char *label, ImVec2 &size, float &line_number_width, float &line_height, float &editor_top_margin, float &text_left_margin);
    ImVec2 renderLineNumbersPanel(float line_number_width, float editor_top_margin);
    void beginTextEditorChild(const char *label, float remaining_width, float content_width, float content_height, float &current_scroll_y, float &current_scroll_x, ImVec2 &text_pos, float editor_top_margin, float text_left_margin, EditorState &editor_state);
    void processTextEditorInput(std::string &text, EditorState &editor_state, const ImVec2 &text_start_pos, float line_height, bool &text_changed, std::vector<ImVec4> &colors, CursorVisibility &ensure_cursor_visible, int initial_cursor_pos);
    void renderEditorContent(const std::string &text, const std::vector<ImVec4> &colors, EditorState &editor_state, float line_height, const ImVec2 &text_pos);
};