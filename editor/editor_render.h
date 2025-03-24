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
    void renderEditorFrame();

    void renderEditorContent(const std::string &text, const std::vector<ImVec4> &colors, float line_height, const ImVec2 &text_pos);

    void renderText(ImDrawList *drawList, const ImVec2 &pos, const std::string &text, const std::vector<ImVec4> &colors, float line_height);

    bool validateAndResizeColors();

    void setupEditorWindow(const char *label);

    void beginTextEditorChild(const char *label, float remaining_width, float content_width, float content_height);
};