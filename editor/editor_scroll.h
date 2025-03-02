#pragma once
#include "editor_types.h"
#include "imgui.h"
#include <string>

class EditorScroll
{
  public:
    EditorScroll();
    ~EditorScroll() = default;

    // Main scroll animation update function
    void updateScrollAnimation(EditorState &state, float &current_scroll_x, float &current_scroll_y, float dt);

    // Mouse wheel handling
    void processMouseWheelForEditor(float line_height, float &current_scroll_y, float &current_scroll_x, EditorState &editor_state);
    void processMouseWheelScrolling(float line_height, EditorState &state);

    // Cursor visibility functions
    ScrollChange ensureCursorVisible(const ImVec2 &text_pos, const std::string &text, EditorState &state, float line_height, float window_height, float window_width);

    void adjustScrollForCursorVisibility(const ImVec2 &text_pos, const std::string &text, EditorState &state, float line_height, float window_height, float window_width, float &current_scroll_y, float &current_scroll_x, CursorVisibility &ensure_cursor_visible);

    // Direct scroll request handling
    void requestScroll(float x, float y)
    {
        requestedScrollX = x;
        requestedScrollY = y;
        hasScrollRequest = true;
    }

    bool handleScrollRequest(float &outScrollX, float &outScrollY)
    {
        if (hasScrollRequest) {
            outScrollX = requestedScrollX;
            outScrollY = requestedScrollY;
            hasScrollRequest = false;
            return true;
        }
        return false;
    }

  private:
    // Helpers
    float calculateCursorXPosition(const ImVec2 &text_pos, const std::string &text, int cursor_pos);

    // Scroll state
    float requestedScrollX = 0;
    float requestedScrollY = 0;
    bool hasScrollRequest = false;
};

// Global instance
extern EditorScroll gEditorScroll;