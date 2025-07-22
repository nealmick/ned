// editor_types.h global state, include editor.h for external access
#pragma once
#include "imgui.h"
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

struct CursorVisibility
{
	bool vertical;
	bool horizontal;
};
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
struct MultiSelectionRange
{
	int start_index;
	int end_index;

	MultiSelectionRange(int start = 0, int end = 0) : start_index(start), end_index(end)
	{
	}
};
struct EditorState
{
	// Content of file being edited as string
	std::string fileContent;

	// syntax colors for every char
	std::vector<ImVec4> fileColors;
	std::mutex colorsMutex;

	// Size of editor window
	ImVec2 size;

	// Height of window as float
	float total_height;

	// height of single line of content
	float line_height;

	// Cursor State
	int cursor_index; // content index of curent cursor
	std::vector<int> multi_cursor_indices;

	/*
	 * cursor_column_prefered
	 * Remembers horizontal position if line is shorter than preferred.
	 * It will go to end while returning to preferred column for longer lines.
	 */
	int cursor_column_prefered;
	std::vector<int> multi_cursor_prefered_columns;

	// Selection State
	int selection_start; // Character index of selection start
	int selection_end;	 // Character index of selection end
	bool selection_active;
	bool full_text_selected;
	std::vector<MultiSelectionRange> multi_selections;
	/*
	 * Editor Content Lines
	 * The first entry (index 0) is always 0, representing the start of the
	 * first line. Each subsequent entry represents the character position where
	 * a new line begins. These positions correspond to the character
	 * immediately after a newline character.
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
	bool rainbow_mode;		 // Visual setting for cursor mode, line numbers, and file
	bool active_find_box;	 // Cmd+F search file dialog open
	bool block_input;		 // Turn off all editor inputs
	float cursor_blink_time; // Used for cursor timing and rainbow mode state

	// leaves room for file path and icon above editor window
	float editor_top_margin;
	// leave room for line numbers on left side of editor window
	float text_left_margin;
	// line number width changes based off how long file is,
	// if less then 1000, it will only leave room for 3 chars...
	float line_number_width;
	// position where line numbers start
	ImVec2 line_numbers_pos;

	// text postion offset, used for scrolling.
	ImVec2 text_pos;

	// used for snapping scroll to cursor making cursor visibile
	CursorVisibility ensure_cursor_visible = {false, false};
	bool center_cursor_vertical = false;
	// did text change or was edit made...
	bool text_changed = false;
	bool ghost_text_changed = false; // New flag for ghost text changes

	bool get_autocomplete = false;
	EditorState()
		: cursor_column_prefered(0), cursor_index(0), selection_start(0),
		  selection_end(0), selection_active(false), full_text_selected(false),
		  editor_content_lines({0}), line_widths(), rainbow_mode(true),
		  cursor_blink_time(0.0f), active_find_box(false), block_input(false)
	{
	}
};
