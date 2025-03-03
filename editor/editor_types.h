#pragma once
#include "imgui.h"
#include <string>
#include <unordered_map>
#include <vector>

struct EditorState
{
    int cursor_pos;
    int current_line;
    int selection_start;
    int selection_end;
    bool is_selecting;

    // Display and content
    std::vector<int> line_starts;
    std::vector<float> line_widths;

    // Caching for expensive measurements
    std::string cached_text;
    std::unordered_map<int, std::vector<float>> cachedLineCumulativeWidths;

    // Visual settings, input state, etc.
    bool rainbow_cursor;
    bool activateFindBox;
    bool blockInput;
    bool full_text_selected;

    int preferred_column; // Tracks intended horizontal position during vertical movement

    float cursor_blink_time;

    EditorState() : preferred_column(0), cursor_pos(0), selection_start(0), selection_end(0), is_selecting(false), line_starts({0}), line_widths(), rainbow_cursor(true), cursor_blink_time(0.0f), activateFindBox(false), blockInput(false), current_line(0) {}
};