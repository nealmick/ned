#pragma once
#include "imgui.h"
#include "editor_types.h"
#include <string>


struct ScrollChange
{
    bool vertical;
    bool horizontal;
};

struct ScrollAnimationState
{
    bool active_x = false;
    bool active_y = false;
    float target_x = 0.0f;
    float target_y = 0.0f;
    float current_velocity_x = 0.0f;
    float current_velocity_y = 0.0f;
};

struct CursorVisibility
{
    bool vertical;
    bool horizontal;
};


// Forward declarations
class Editor;
extern Editor gEditor;

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

    // New scroll position accessors (to replace direct access to EditorState)
    ImVec2 getScrollPosition() const { return scrollPos; }
    float getScrollX() const { return scrollX; }
    void setScrollPosition(const ImVec2 &pos) { scrollPos = pos; }
    void setScrollX(float x) { scrollX = x; }

    // Access methods for cursor visibility frames
    int getEnsureCursorVisibleFrames() const { return ensureCursorVisibleFrames; }
    void setEnsureCursorVisibleFrames(int frames) { ensureCursorVisibleFrames = frames; }
    void decrementEnsureCursorVisibleFrames()
    {
        if (ensureCursorVisibleFrames > 0)
            ensureCursorVisibleFrames--;
    }

    // Scroll animation state accessors
    const ScrollAnimationState &getAnimationState() const { return scrollAnimation; }
    ScrollAnimationState &getAnimationState() { return scrollAnimation; }
    void setAnimationState(const ScrollAnimationState &state) { scrollAnimation = state; }

    // Higher-level scrolling API methods
    void scrollToLine(EditorState &state, int lineNumber, float line_height);
    void scrollToCharacter(EditorState &state, int charIndex, const ImVec2 &text_pos, const std::string &text, float line_height);
    void centerOnCursor(EditorState &state, const ImVec2 &text_pos, const std::string &text, float line_height, float window_height, float window_width);

    // Cursor movement scroll handling
    void handleCursorMovementScroll(const ImVec2 &text_pos, const std::string &text, EditorState &state, float line_height, float window_height, float window_width);

  private:
    // Helpers
    float calculateCursorXPosition(const ImVec2 &text_pos, const std::string &text, int cursor_pos);

    // Helper that doesn't need direct access to Editor (to avoid circular dependency)
    int getLineFromPosition(const std::vector<int> &line_starts, int pos);

    // Scroll state (moved from EditorState)
    ImVec2 scrollPos = ImVec2(0, 0);      // Replaces state.scroll_pos
    float scrollX = 0.0f;                 // Replaces state.scroll_x
    int ensureCursorVisibleFrames = 0;    // Replaces state.ensure_cursor_visible_frames
    ScrollAnimationState scrollAnimation; // Replaces state.scroll_animation

    // Direct scroll request state
    float requestedScrollX = 0;
    float requestedScrollY = 0;
    bool hasScrollRequest = false;

    // Extra state variables for bookmark/specific scrolling
    bool pendingBookmarkScroll = false;
    float pendingScrollX = 0.0f;
    float pendingScrollY = 0.0f;
};

// Global instance
extern EditorScroll gEditorScroll;