/*
    File: editor_render.h
    Description: Handles rendering for the editor, separating rendering logic from the main editor
   class.
*/

#pragma once
#include "editor_types.h"
#include "imgui.h"

#include <string>
#include <vector>

// Forward declarations
class EditorRender;
extern EditorRender gEditorRender;

class EditorRender
{
  public:
    void renderEditorFrame(std::string &text, std::vector<ImVec4> &colors, ImVec2 &text_pos, float line_height, ImVec2 &line_numbers_pos, float line_number_width, ImVec2 &size, float total_height, float editor_top_margin);

    void renderEditorContent(const std::string &text, const std::vector<ImVec4> &colors, EditorState &editor_state, float line_height, const ImVec2 &text_pos);

    void renderText(ImDrawList *drawList, const ImVec2 &pos, const std::string &text, const std::vector<ImVec4> &colors, const EditorState &state, float line_height);

    bool validateAndResizeColors(std::string &text, std::vector<ImVec4> &colors);

    void setupEditorWindow(const char *label, ImVec2 &size, float &line_number_width, float &line_height, float &editor_top_margin, float &text_left_margin);

    ImVec2 renderLineNumbersPanel(float line_number_width, float editor_top_margin);

    void beginTextEditorChild(const char *label, float remaining_width, float content_width, float content_height, float &current_scroll_y, float &current_scroll_x, ImVec2 &text_pos, float editor_top_margin, float text_left_margin, EditorState &editor_state);
};