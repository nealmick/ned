#pragma once
#include "imgui.h"
#include <string>
#include <unordered_map>
#include <vector>

// Scroll-related structures
struct ScrollChange
{
    bool vertical;
    bool horizontal;
};

struct CursorVisibility
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

struct EditorState
{
    int cursor_pos;
    int current_line;

    int selection_start;
    int selection_end;

    bool is_selecting;

    // Display and scrolling
    std::vector<int> line_starts;
    std::vector<float> line_widths;
    ImVec2 scroll_pos;

    float scroll_x;
    float cursor_blink_time;
    int ensure_cursor_visible_frames;

    // Caching for expensive measurements
    std::string cached_text;
    std::unordered_map<int, std::vector<float>> cachedLineCumulativeWidths;

    // Scroll animation state
    ScrollAnimationState scroll_animation;

    // Visual settings, input state, etc.
    bool rainbow_cursor;
    bool activateFindBox;
    bool blockInput;
    bool full_text_selected;
    bool pendingBookmarkScroll;
    float pendingScrollX;
    float pendingScrollY;

    int preferred_column; // Tracks intended horizontal position during vertical movement

    EditorState() : preferred_column(0), ensure_cursor_visible_frames(0), cursor_pos(0), selection_start(0), selection_end(0), is_selecting(false), line_starts({0}), line_widths(), scroll_pos(0, 0), scroll_x(0.0f), rainbow_cursor(true), cursor_blink_time(0.0f), activateFindBox(false), blockInput(false), current_line(0) {}
};