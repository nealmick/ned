#pragma once
#include "editor_types.h"
#include "imgui.h"
#include <string>
#include <vector>

// Forward declarations
class Editor;
extern Editor gEditor;

class EditorMouse
{
  public:
    EditorMouse();
    ~EditorMouse() = default;

    // Main mouse handling function
    void handleMouseInput(const std::string &text, const ImVec2 &text_start_pos, float line_height);

    // Character position calculation from mouse coordinates
    int getCharIndexFromCoords(const std::string &text, const ImVec2 &click_pos, const ImVec2 &text_start_pos, const std::vector<int> &line_starts, float line_height);

    // Context menu handler
    void handleContextMenu(std::string &text, std::vector<ImVec4> &colors, bool &text_changed);

  private:
    // Selection state
    bool is_dragging;
    int anchor_pos;
    bool show_context_menu;
    ImVec2 context_menu_pos;

    // Handle different mouse actions
    void handleMouseClick(int char_index, const std::vector<int> &line_starts);
    void handleMouseDrag(int char_index);
    void handleMouseRelease();
};

// Global instance
extern EditorMouse gEditorMouse;