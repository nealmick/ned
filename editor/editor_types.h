#pragma once
#include "imgui.h"
#include <string>
#include <unordered_map>
#include <vector>

struct EditorState
{
    // Content of file being edited as string
    std::string fileContent;

    // syntax colors for every char
    std::vector<ImVec4> fileColors;

    // Size of editor window
    ImVec2 size;

    // height of single line of content
    float line_height;

    // Cursor State
    int cursor_index; // content index of curent cursor

    /*
     * cursor_column_prefered
     * Remembers horizontal position if line is shorter than preferred.
     * It will go to end while returning to preferred column for longer lines.
     */
    int cursor_column_prefered;

    // Selection State
    int selection_start; // Character index of selection start
    int selection_end;   // Character index of selection end
    bool selection_active;
    bool full_text_selected;

    /*
     * Editor Content Lines
     * The first entry (index 0) is always 0, representing the start of the first line.
     * Each subsequent entry represents the character position where a new line begins.
     * These positions correspond to the character immediately after a newline character.
     */
    std::vector<int> editor_content_lines;

    /*
     * Line Widths
     * Stores the pixel width of each line in the editor.
     * Each entry corresponds to the rendered width of a line in pixels.
     * Used for layout calculations, particularly for horizontal scrolling.
     */
    std::vector<float> line_widths;

    // scalling values
    float current_scroll_x, current_scroll_y;

    // Caching for expensive measurements
    std::string cached_text;

    // Miscellaneous state variables
    bool rainbow_mode;       // Visual setting for cursor mode, line numbers, and file
    bool active_find_box;    // Cmd+F search file dialog open
    bool block_input;        // Turn off all editor inputs
    float cursor_blink_time; // Used for cursor timing and rainbow mode state

    // leaves room for file path and icon above editor window
    float editor_top_margin;
    // leave room for line numbers on left side of editor window
    float text_left_margin;

    EditorState() : cursor_column_prefered(0), cursor_index(0), selection_start(0), selection_end(0), selection_active(false), full_text_selected(false), editor_content_lines({0}), line_widths(), rainbow_mode(true), cursor_blink_time(0.0f), active_find_box(false), block_input(false) {}
};
