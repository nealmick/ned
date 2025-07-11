#include "editor_keyboard.h"
#include "../ai/ai_tab.h"
#include "../files/file_finder.h"
#include "../files/files.h"
#include "../lsp/lsp_autocomplete.h"
#include "../lsp/lsp_goto_def.h"
#include "../lsp/lsp_goto_ref.h"
#include "../lsp/lsp_symbol_info.h"
#include "editor.h"
#include "editor_bookmarks.h"
#include "editor_copy_paste.h"
#include "editor_highlight.h"
#include "editor_indentation.h"
#include "editor_line_jump.h"
#include "editor_mouse.h"
#include "editor_render.h"
#include "editor_scroll.h"
#include "editor_selection.h"
#include "editor_utils.h"
#include "imgui.h"
#include <algorithm>
#include <iostream>
#include <set>
#include "../editor/editor_git.h"

// Global instance
EditorKeyboard gEditorKeyboard;

EditorKeyboard::EditorKeyboard() {}

void EditorKeyboard::handleBackspaceKey()
{
	if (!ImGui::IsKeyPressed(ImGuiKey_Backspace))
	{
		return;
	}

	std::vector<MultiSelectionRange> ranges_to_delete;
	bool selections_were_active = false;

	if (editor_state.selection_start != editor_state.selection_end)
	{
		selections_were_active = true;
		ranges_to_delete.emplace_back(std::min(editor_state.selection_start,
											   editor_state.selection_end),
									  std::max(editor_state.selection_start,
											   editor_state.selection_end));
	}

	for (const auto &ms_range : editor_state.multi_selections)
	{
		if (ms_range.start_index != ms_range.end_index)
		{
			selections_were_active = true;
			ranges_to_delete.emplace_back(std::min(ms_range.start_index, ms_range.end_index),
										  std::max(ms_range.start_index, ms_range.end_index));
		}
	}

	if (!selections_were_active)
	{
		std::set<int> unique_cursor_positions_for_backspace;
		if (editor_state.cursor_index > 0)
		{
			unique_cursor_positions_for_backspace.insert(editor_state.cursor_index);
		}
		for (int mc_idx : editor_state.multi_cursor_indices)
		{
			if (mc_idx > 0)
			{
				unique_cursor_positions_for_backspace.insert(mc_idx);
			}
		}
		for (int pos : unique_cursor_positions_for_backspace)
		{
			if (pos > 0)
			{
				ranges_to_delete.emplace_back(pos - 1, pos); // Range is [pos-1, pos)
			}
		}
	}
	std::sort(ranges_to_delete.begin(),
			  ranges_to_delete.end(),
			  [](const MultiSelectionRange &a, const MultiSelectionRange &b) {
				  return a.start_index < b.start_index;
			  });
	std::vector<MultiSelectionRange> merged_ranges;
	if (!ranges_to_delete.empty())
	{
		merged_ranges.push_back(ranges_to_delete[0]);
		for (size_t i = 1; i < ranges_to_delete.size(); ++i)
		{
			MultiSelectionRange &last_merged = merged_ranges.back();
			const MultiSelectionRange &current_range = ranges_to_delete[i];

			if (current_range.start_index <= last_merged.end_index)
			{
				last_merged.end_index = std::max(last_merged.end_index, current_range.end_index);
			} else
			{
				merged_ranges.push_back(current_range);
			}
		}
	}

	int total_chars_deleted_this_op = 0;
	std::set<int> new_caret_positions;

	if (!merged_ranges.empty())
	{
		for (const auto &range : merged_ranges)
		{
			int effective_start = range.start_index - total_chars_deleted_this_op;
			int effective_end = range.end_index - total_chars_deleted_this_op;

			effective_start = std::max(0,
									   std::min(effective_start,
												static_cast<int>(editor_state.fileContent.size())));
			effective_end = std::max(effective_start,
									 std::min(effective_end,
											  static_cast<int>(editor_state.fileContent.size())));

			int length_to_delete = effective_end - effective_start;

			if (length_to_delete > 0)
			{
				editor_state.fileContent.erase(effective_start, length_to_delete);

				if (static_cast<size_t>(effective_start) < editor_state.fileColors.size())
				{
					editor_state.fileColors.erase(editor_state.fileColors.begin() + effective_start,
												  editor_state.fileColors.begin() +
													  std::min(static_cast<size_t>(effective_end),
															   editor_state.fileColors.size()));
				} else if (editor_state.fileContent.empty() && !editor_state.fileColors.empty())
				{
					editor_state.fileColors.clear();
				}
				total_chars_deleted_this_op += length_to_delete;
			}
			new_caret_positions.insert(effective_start);
		}
	}

	if (total_chars_deleted_this_op > 0)
	{
		editor_state.text_changed = true;

		editor_state.multi_cursor_indices.clear();

		if (!new_caret_positions.empty())
		{
			editor_state.cursor_index = *new_caret_positions.begin();
			auto it = new_caret_positions.begin();
			++it;
			for (; it != new_caret_positions.end(); ++it)
			{
				editor_state.multi_cursor_indices.push_back(*it);
			}
		} else
		{
			editor_state.cursor_index = std::min(editor_state.cursor_index,
												 static_cast<int>(editor_state.fileContent.size()));
		}

		editor_state.cursor_column_prefered = 0;
		editor_state.multi_cursor_prefered_columns.assign(editor_state.multi_cursor_indices.size(),
														  0);
	} else if (selections_were_active && !new_caret_positions.empty())
	{
		editor_state.multi_cursor_indices.clear();
		editor_state.cursor_index = *new_caret_positions.begin();
		auto it = new_caret_positions.begin();
		++it;
		for (; it != new_caret_positions.end(); ++it)
		{
			editor_state.multi_cursor_indices.push_back(*it);
		}
		editor_state.cursor_column_prefered = 0;
		editor_state.multi_cursor_prefered_columns.assign(editor_state.multi_cursor_indices.size(),
														  0);
	}

	editor_state.selection_start = editor_state.selection_end = editor_state.cursor_index;
	editor_state.selection_active = false;
	editor_state.multi_selections.clear();
}

void EditorKeyboard::handleCharacterInput()
{
	ImGuiIO &io = ImGui::GetIO();
	std::string inputText; // Renamed from 'input' to avoid conflict if 'input' is a member
	inputText.reserve(io.InputQueueCharacters.Size);
	for (int n = 0; n < io.InputQueueCharacters.Size; n++)
	{
		char c = static_cast<char>(io.InputQueueCharacters[n]);
		if (c != 0 && c >= 32)
		{
			inputText += c;
		}
	}
	io.InputQueueCharacters.clear(); // Clear input queue after processing

	if (inputText.empty())
	{
		return;
	}

	// Save initial state before first edit if needed
	if (gFileExplorer._firstEditPending) {
		gFileExplorer.addUndoState();
		gFileExplorer._firstEditPending = false;
	}

	// Only close LSP completion menu for specific characters
	bool shouldCloseCompletion = false;
	for (char c : inputText) {
		// Close for space, dot (for method chaining), and other special characters
		if (c == ' ' || c == ' ' || c == '(' || c == ')' || c == '[' || c == ']' || 
			c == '{' || c == '}' || c == ',' || c == ';' || c == ':' || c == '+' || 
			c == '-' || c == '*' || c == '/' || c == '=' || c == '!' || c == '&' || 
			c == '|' || c == '^' || c == '%' || c == '<' || c == '>') {
			shouldCloseCompletion = true;
			break;
		}
	}
	
	if (shouldCloseCompletion) {
		editor_state.block_input = false;
	}

	std::set<int> caret_positions_for_insertion; // Stores unique, sorted positions for new text
	bool had_any_selections = false;

	std::vector<MultiSelectionRange> all_active_selections;

	if (editor_state.selection_start != editor_state.selection_end)
	{
		had_any_selections = true;
		all_active_selections.emplace_back(std::min(editor_state.selection_start,
													editor_state.selection_end),
										   std::max(editor_state.selection_start,
													editor_state.selection_end));
	}

	for (const auto &ms_range : editor_state.multi_selections)
	{
		if (ms_range.start_index != ms_range.end_index)
		{
			had_any_selections = true;
			all_active_selections.emplace_back(std::min(ms_range.start_index, ms_range.end_index),
											   std::max(ms_range.start_index, ms_range.end_index));
		}
	}

	if (had_any_selections)
	{
		std::sort(all_active_selections.begin(),
				  all_active_selections.end(),
				  [](const MultiSelectionRange &a, const MultiSelectionRange &b) {
					  return a.start_index < b.start_index;
				  });

		std::vector<MultiSelectionRange> merged_selections;
		if (!all_active_selections.empty())
		{
			merged_selections.push_back(all_active_selections[0]);
			for (size_t i = 1; i < all_active_selections.size(); ++i)
			{
				MultiSelectionRange &last_merged = merged_selections.back();
				const MultiSelectionRange &current_sel = all_active_selections[i];

				if (current_sel.start_index <= last_merged.end_index) // Overlap or adjacent
				{
					last_merged.end_index = std::max(last_merged.end_index, current_sel.end_index);
				} else
				{
					merged_selections.push_back(current_sel);
				}
			}
		}
		all_active_selections = merged_selections; // Use the merged list

		int total_chars_deleted_this_op = 0;
		for (const auto &sel_to_delete : all_active_selections)
		{
			int current_start = sel_to_delete.start_index - total_chars_deleted_this_op;
			int current_end = sel_to_delete.end_index - total_chars_deleted_this_op;

			current_start = std::max(0,
									 std::min(current_start,
											  static_cast<int>(editor_state.fileContent.size())));
			current_end =
				std::max(current_start,
						 std::min(current_end, static_cast<int>(editor_state.fileContent.size())));

			int length_to_delete = current_end - current_start;

			if (length_to_delete > 0)
			{
				editor_state.fileContent.erase(current_start, length_to_delete);
				if (static_cast<size_t>(current_start) < editor_state.fileColors.size())
				{
					editor_state.fileColors.erase(
						editor_state.fileColors.begin() + current_start,
						editor_state.fileColors.begin() +
							std::min(current_end,
									 static_cast<int>(editor_state.fileColors.size())));
				} else if (!editor_state.fileColors.empty() && current_start == 0 &&
						   length_to_delete == editor_state.fileColors.size())
				{
					editor_state.fileColors.clear();
				}

				total_chars_deleted_this_op += length_to_delete;
			}
			caret_positions_for_insertion.insert(
				current_start); // Add the start of the (adjusted) deleted region
		}
	} else
	{
		caret_positions_for_insertion.insert(editor_state.cursor_index);
		for (int mc_idx : editor_state.multi_cursor_indices)
		{
			int valid_mc_idx =
				std::max(0, std::min(mc_idx, static_cast<int>(editor_state.fileContent.size())));
			caret_positions_for_insertion.insert(valid_mc_idx);
		}
		if (editor_state.fileContent.empty() && caret_positions_for_insertion.empty())
		{
			caret_positions_for_insertion.insert(0);
		}
	}

	std::vector<int> final_new_cursor_positions;
	int cumulative_insertion_offset = 0;

	for (int caret_pos : caret_positions_for_insertion)
	{
		int actual_insert_pos = caret_pos + cumulative_insertion_offset;

		actual_insert_pos = std::max(0,
									 std::min(actual_insert_pos,
											  static_cast<int>(editor_state.fileContent.size())));

		editor_state.fileContent.insert(actual_insert_pos, inputText);

		if (static_cast<size_t>(actual_insert_pos) <= editor_state.fileColors.size())
		{
			editor_state.fileColors.insert(editor_state.fileColors.begin() + actual_insert_pos,
										   inputText.size(),
										   ImVec4(1.0f, 1.0f, 1.0f, 1.0f) // Default color
			);
		} else
		{
			for (size_t k = 0; k < inputText.size(); ++k)
			{
				editor_state.fileColors.push_back(ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
			}
		}

		final_new_cursor_positions.push_back(actual_insert_pos + inputText.size());
		cumulative_insertion_offset += inputText.size();
	}

	if (!final_new_cursor_positions.empty())
	{
		editor_state.cursor_index = final_new_cursor_positions[0]; // Primary cursor
		editor_state.multi_cursor_indices.assign(				   // Multi-cursors
			final_new_cursor_positions.begin() + 1,
			final_new_cursor_positions.end());
	} else if (!caret_positions_for_insertion.empty())
	{
		auto it = caret_positions_for_insertion.begin();
		editor_state.cursor_index = *it;
		editor_state.multi_cursor_indices.clear();
		++it;
		for (; it != caret_positions_for_insertion.end(); ++it)
		{
			editor_state.multi_cursor_indices.push_back(*it);
		}
	} else
	{
	}

	// Reset selection state
	editor_state.selection_start = editor_state.selection_end = editor_state.cursor_index;
	editor_state.selection_active = false;
	editor_state.multi_selections.clear(); // Important: clear multi-selections after typing

	editor_state.text_changed = true;
	//gEditor.updateLineStarts();

	// After processing the input, trigger LSP completion only if there was no space
	if (!inputText.empty() && !shouldCloseCompletion) {
		if (gSettings.getSettings()["lsp_autocomplete"]) {
			editor_state.get_autocomplete = true;
		}
		// Trigger AI completion if enabled
		if (gSettings.getSettings()["ai_autocomplete"]) {
			gAITab.cancel_request();
			gAITab.tab_complete();
		}
	}
}

std::string CalculateIndentForPosition(const std::string &content,
									   int char_pos_for_newline_insertion)
{
	if (content.empty() && char_pos_for_newline_insertion == 0)
	{
		return ""; // Handles inserting into an empty file
	}

	// Ensure char_pos is within the current content bounds (or at the end for insertion)
	int effective_pos =
		std::max(0, std::min(char_pos_for_newline_insertion, static_cast<int>(content.length())));

	// Find the start of the line where 'effective_pos' currently resides.
	// This is the line from which we'll copy indentation.
	int current_line_start = 0;
	if (effective_pos > 0)
	{
		// Search backwards for the last newline before or at effective_pos - 1
		size_t last_newline_pos = content.rfind('\n', effective_pos - 1);
		if (last_newline_pos != std::string::npos)
		{
			current_line_start = last_newline_pos + 1;
		}
	}
	// Make sure current_line_start is not past effective_pos itself.
	current_line_start = std::min(current_line_start, effective_pos);

	// Find the end of the leading whitespace on this current_line_start
	size_t current_line_indent_end = current_line_start;
	while (current_line_indent_end < content.length() && content[current_line_indent_end] != '\n' &&
		   (content[current_line_indent_end] == ' ' || content[current_line_indent_end] == '\t'))
	{
		current_line_indent_end++;
	}
	size_t leading_whitespace_on_current_line = current_line_indent_end - current_line_start;

	// The "cursor's position within the line" for determining how much indent to copy.
	// This is the offset from the start of the line to where the newline will be inserted.
	size_t conceptual_cursor_pos_in_line = effective_pos - current_line_start;

	// Determine indent length to copy: min(cursor's position in line, actual leading whitespace)
	size_t indent_length_to_copy =
		std::min(conceptual_cursor_pos_in_line, leading_whitespace_on_current_line);

	// Substring to get the actual indent characters
	if (current_line_start + indent_length_to_copy > content.length())
	{ // Boundary check
		indent_length_to_copy = std::max(0, (int)content.length() - current_line_start);
	}

	return content.substr(current_line_start, indent_length_to_copy);
}
void EditorKeyboard::handleEnterKey()
{
	if (gLineJump.hasJustJumped())
	{
		return;
	}

	if (!ImGui::IsKeyPressed(ImGuiKey_Enter))
	{
		return;
	}
	if (gSettings.getSettings()["ai_autocomplete"]) {
		gAITab.cancel_request();
		gAITab.tab_complete();
	}

	std::vector<MultiSelectionRange> selections_to_delete;
	bool selections_were_active = false;

	// --- Phase 1: Collect and Delete Selections (if any) ---
	if (editor_state.selection_start != editor_state.selection_end)
	{
		selections_were_active = true;
		selections_to_delete.emplace_back(std::min(editor_state.selection_start,
												   editor_state.selection_end),
										  std::max(editor_state.selection_start,
												   editor_state.selection_end));
	}

	for (const auto &ms_range : editor_state.multi_selections)
	{
		if (ms_range.start_index != ms_range.end_index)
		{
			selections_were_active = true;
			selections_to_delete.emplace_back(std::min(ms_range.start_index, ms_range.end_index),
											  std::max(ms_range.start_index, ms_range.end_index));
		}
	}

	std::set<int> target_positions_for_newline; // Positions where newlines will be inserted,
												// relative to content *after* deletions
	int total_chars_deleted_this_op = 0;
	bool text_changed_by_deletion = false;

	if (selections_were_active)
	{
		std::sort(selections_to_delete.begin(),
				  selections_to_delete.end(),
				  [](const MultiSelectionRange &a, const MultiSelectionRange &b) {
					  return a.start_index < b.start_index;
				  });

		std::vector<MultiSelectionRange> merged_selections;
		if (!selections_to_delete.empty())
		{
			merged_selections.push_back(selections_to_delete[0]);
			for (size_t i = 1; i < selections_to_delete.size(); ++i)
			{
				MultiSelectionRange &last_merged = merged_selections.back();
				const MultiSelectionRange current_sel = selections_to_delete[i];
				if (current_sel.start_index <= last_merged.end_index)
				{
					last_merged.end_index = std::max(last_merged.end_index, current_sel.end_index);
				} else
				{
					merged_selections.push_back(current_sel);
				}
			}
		}

		for (const auto &sel_to_delete : merged_selections)
		{
			int effective_start = sel_to_delete.start_index - total_chars_deleted_this_op;
			int effective_end = sel_to_delete.end_index - total_chars_deleted_this_op;

			effective_start = std::max(0,
									   std::min(effective_start,
												static_cast<int>(editor_state.fileContent.size())));
			effective_end = std::max(effective_start,
									 std::min(effective_end,
											  static_cast<int>(editor_state.fileContent.size())));

			int length_to_delete = effective_end - effective_start;

			if (length_to_delete > 0)
			{
				text_changed_by_deletion = true;
				editor_state.fileContent.erase(effective_start, length_to_delete);
				if (static_cast<size_t>(effective_start) < editor_state.fileColors.size())
				{
					editor_state.fileColors.erase(editor_state.fileColors.begin() + effective_start,
												  editor_state.fileColors.begin() +
													  std::min(static_cast<size_t>(effective_end),
															   editor_state.fileColors.size()));
				} else if (editor_state.fileContent.empty() && !editor_state.fileColors.empty())
				{
					editor_state.fileColors.clear();
				}
				total_chars_deleted_this_op += length_to_delete;
			}
			target_positions_for_newline.insert(effective_start);
		}
	} else // No selections were active, use current cursor positions
	{
		target_positions_for_newline.insert(editor_state.cursor_index);
		for (int mc_idx : editor_state.multi_cursor_indices)
		{
			target_positions_for_newline.insert(mc_idx);
		}
	}

	// If after potential deletions, target_positions_for_newline is empty, and fileContent is also
	// empty, ensure we have a target position 0 to insert the first newline.
	if (target_positions_for_newline.empty() && editor_state.fileContent.empty())
	{
		target_positions_for_newline.insert(0);
	}

	// --- Phase 2: Insert Newlines with Indentation ---
	std::vector<int> final_new_cursor_positions;
	int cumulative_insertion_offset = 0;
	bool text_changed_by_insertion = false;

	if (!target_positions_for_newline.empty())
	{
		text_changed_by_insertion = true;				  // Will insert at least one newline
		for (int base_pos : target_positions_for_newline) // Iterates in sorted order
		{
			int actual_insert_pos = base_pos + cumulative_insertion_offset;
			// Clamp insert position, crucial if base_pos was from multi_cursor_indices which might
			// be out of sync after deletions
			actual_insert_pos =
				std::max(0,
						 std::min(actual_insert_pos,
								  static_cast<int>(editor_state.fileContent.length())));

			// Calculate indent based on the line at 'actual_insert_pos' in the *current* state of
			// fileContent
			std::string indent_str =
				CalculateIndentForPosition(editor_state.fileContent, actual_insert_pos);
			std::string to_insert = "\n" + indent_str;
			size_t insert_length = to_insert.length();

			editor_state.fileContent.insert(actual_insert_pos, to_insert);

			ImVec4 default_color = ImVec4(1.0f, 1.0f, 1.0f, 1.0f); // Or your editor's default
			if (static_cast<size_t>(actual_insert_pos) <= editor_state.fileColors.size())
			{
				editor_state.fileColors.insert(editor_state.fileColors.begin() + actual_insert_pos,
											   insert_length,
											   default_color);
			} else // Should ideally not happen if actual_insert_pos is clamped correctly
			{
				for (size_t k = 0; k < insert_length; ++k)
					editor_state.fileColors.push_back(default_color);
			}

			final_new_cursor_positions.push_back(actual_insert_pos + insert_length);
			cumulative_insertion_offset += insert_length;
		}
	}

	// --- Phase 3: Update Cursors and Editor State ---
	if (!final_new_cursor_positions.empty())
	{
		editor_state.cursor_index = final_new_cursor_positions[0];
		editor_state.multi_cursor_indices.assign(final_new_cursor_positions.begin() + 1,
												 final_new_cursor_positions.end());
	} else if (!target_positions_for_newline
					.empty()) // Selections were deleted, but somehow no newlines inserted (e.g., if
							  // CalculateIndent was empty and newline char itself was omitted)
	{
		// Cursors should be at the points where deletions happened (or where they were if no
		// deletions) target_positions_for_newline are already adjusted for deletions.
		auto it = target_positions_for_newline.begin();
		editor_state.cursor_index = *it;
		editor_state.multi_cursor_indices.clear();
		++it;
		for (; it != target_positions_for_newline.end(); ++it)
		{
			editor_state.multi_cursor_indices.push_back(*it);
		}
	} else if (editor_state.fileContent.empty()) // Completely empty, cursor at 0
	{
		editor_state.cursor_index = 0;
		editor_state.multi_cursor_indices.clear();
	}
	// else: cursors remain as they were if no operations happened.

	// Reset selection state
	editor_state.selection_start = editor_state.selection_end = editor_state.cursor_index;
	editor_state.selection_active = false;
	editor_state.multi_selections.clear();

	// Reset preferred columns
	editor_state.cursor_column_prefered = 0; // Newline usually means new preferred col calculation
	editor_state.multi_cursor_prefered_columns.assign(editor_state.multi_cursor_indices.size(), 0);

	if (text_changed_by_deletion || text_changed_by_insertion)
	{
		editor_state.text_changed = true;
	}
}
void EditorKeyboard::handleDeleteKey()
{
	if (!ImGui::IsKeyPressed(ImGuiKey_Delete))
	{
		return;
	}

	std::vector<MultiSelectionRange> ranges_to_delete;
	bool selections_were_active = false;

	if (editor_state.selection_start != editor_state.selection_end)
	{
		selections_were_active = true;
		ranges_to_delete.emplace_back(std::min(editor_state.selection_start,
											   editor_state.selection_end),
									  std::max(editor_state.selection_start,
											   editor_state.selection_end));
	}

	for (const auto &ms_range : editor_state.multi_selections)
	{
		if (ms_range.start_index != ms_range.end_index)
		{
			selections_were_active = true;
			ranges_to_delete.emplace_back(std::min(ms_range.start_index, ms_range.end_index),
										  std::max(ms_range.start_index, ms_range.end_index));
		}
	}

	if (!selections_were_active)
	{
		std::set<int> unique_cursor_positions_for_delete;
		// Primary cursor: can delete if not at the end of the content
		if (editor_state.cursor_index < static_cast<int>(editor_state.fileContent.size()))
		{
			unique_cursor_positions_for_delete.insert(editor_state.cursor_index);
		}
		// Multi-cursors: can delete if not at the end of the content
		for (int mc_idx : editor_state.multi_cursor_indices)
		{
			if (mc_idx < static_cast<int>(editor_state.fileContent.size()))
			{
				unique_cursor_positions_for_delete.insert(mc_idx);
			}
		}
		for (int pos : unique_cursor_positions_for_delete)
		{
			ranges_to_delete.emplace_back(pos, pos + 1);
		}
	}

	std::sort(ranges_to_delete.begin(),
			  ranges_to_delete.end(),
			  [](const MultiSelectionRange &a, const MultiSelectionRange &b) {
				  return a.start_index < b.start_index;
			  });

	std::vector<MultiSelectionRange> merged_ranges;
	if (!ranges_to_delete.empty())
	{
		merged_ranges.push_back(ranges_to_delete[0]);
		for (size_t i = 1; i < ranges_to_delete.size(); ++i)
		{
			MultiSelectionRange &last_merged = merged_ranges.back();
			const MultiSelectionRange current_range = ranges_to_delete[i];

			if (current_range.start_index <= last_merged.end_index)
			{
				last_merged.end_index = std::max(last_merged.end_index, current_range.end_index);
			} else
			{
				merged_ranges.push_back(current_range);
			}
		}
	}

	int total_chars_deleted_this_op = 0;
	std::set<int> new_caret_positions;

	if (!merged_ranges.empty())
	{
		for (const auto &range : merged_ranges)
		{
			int effective_start = range.start_index - total_chars_deleted_this_op;
			int effective_end = range.end_index - total_chars_deleted_this_op;

			effective_start = std::max(0,
									   std::min(effective_start,
												static_cast<int>(editor_state.fileContent.size())));
			effective_end = std::max(effective_start,
									 std::min(effective_end,
											  static_cast<int>(editor_state.fileContent.size())));

			int length_to_delete = effective_end - effective_start;

			if (length_to_delete > 0)
			{
				editor_state.fileContent.erase(effective_start, length_to_delete);

				if (static_cast<size_t>(effective_start) < editor_state.fileColors.size())
				{
					editor_state.fileColors.erase(editor_state.fileColors.begin() + effective_start,
												  editor_state.fileColors.begin() +
													  std::min(static_cast<size_t>(effective_end),
															   editor_state.fileColors.size()));
				} else if (editor_state.fileContent.empty() && !editor_state.fileColors.empty())
				{
					editor_state.fileColors.clear();
				}
				total_chars_deleted_this_op += length_to_delete;
			}
			new_caret_positions.insert(effective_start);
		}
	}

	if (total_chars_deleted_this_op > 0)
	{
		editor_state.text_changed = true;

		editor_state.multi_cursor_indices.clear();

		if (!new_caret_positions.empty())
		{
			editor_state.cursor_index = *new_caret_positions.begin();
			auto it = new_caret_positions.begin();
			++it;
			for (; it != new_caret_positions.end(); ++it)
			{
				editor_state.multi_cursor_indices.push_back(*it);
			}
		} else
		{
			editor_state.cursor_index = std::min(editor_state.cursor_index,
												 static_cast<int>(editor_state.fileContent.size()));
		}

		editor_state.cursor_column_prefered = 0;
		editor_state.multi_cursor_prefered_columns.assign(editor_state.multi_cursor_indices.size(),
														  0);
	} else if (selections_were_active && !new_caret_positions.empty())
	{
		editor_state.multi_cursor_indices.clear();
		editor_state.cursor_index = *new_caret_positions.begin();
		auto it = new_caret_positions.begin();
		++it;
		for (; it != new_caret_positions.end(); ++it)
		{
			editor_state.multi_cursor_indices.push_back(*it);
		}
		editor_state.cursor_column_prefered = 0;
		editor_state.multi_cursor_prefered_columns.assign(editor_state.multi_cursor_indices.size(),
														  0);
	}

	editor_state.selection_start = editor_state.selection_end = editor_state.cursor_index;
	editor_state.selection_active = false;
	editor_state.multi_selections.clear();
}
void EditorKeyboard::handleTextInput()
{
	if (gLSPAutocomplete.blockEnter || editor_state.block_input)
	{
		gLSPAutocomplete.blockEnter = false;
		return;
	}
	int input_start = editor_state.cursor_index;
	int input_end = editor_state.cursor_index;

	// Handle selection deletion only for Enter key
	if (editor_state.selection_start != editor_state.selection_end &&
		ImGui::IsKeyPressed(ImGuiKey_Enter))
	{
		int start = editor_state.selection_start;
		int end = editor_state.selection_end;
		editor_state.fileContent.erase(start, end - start);
		editor_state.fileColors.erase(editor_state.fileColors.begin() + start,
									  editor_state.fileColors.begin() + end);
		editor_state.cursor_index = start;
		editor_state.selection_start = editor_state.selection_end = start;
		editor_state.text_changed = true;
		input_start = input_end = start;
	}

	handleCharacterInput();
	handleEnterKey();
	handleBackspaceKey();
	handleDeleteKey();

	if (editor_state.text_changed)
	{
		int line_start = editor_state.editor_content_lines[gEditor.getLineFromPos(input_start)];

		int line_end = input_end < editor_state.fileContent.size()
						   ? editor_state.editor_content_lines[gEditor.getLineFromPos(input_end)]
						   : editor_state.fileContent.size();

		gEditorHighlight.highlightContent();

		// Update line starts
		gEditor.updateLineStarts();

		// Add undo state with change range (only if not already handled manually)
		if (!editor_state.manual_undo_handled) {
			gFileExplorer.addUndoState();
			gFileExplorer._unsavedChanges = true;
			gFileExplorer.saveCurrentFile();
		}
		
		// Reset the manual undo handled flag
		editor_state.manual_undo_handled = false;

		editor_state.text_changed = false;
		editor_state.ensure_cursor_visible.horizontal = true;
		editor_state.ensure_cursor_visible.vertical = true;
	}
	else if (editor_state.ghost_text_changed)
	{
		// For ghost text changes, just update line starts
		gEditor.updateLineStarts();
		editor_state.ghost_text_changed = false;
		editor_state.ensure_cursor_visible.horizontal = true;
		editor_state.ensure_cursor_visible.vertical = true;
	}
}

void EditorKeyboard::processFontSizeAdjustment() {}
void EditorKeyboard::processSelectAll()
{
	if (ImGui::IsKeyPressed(ImGuiKey_A))
	{
		gEditorSelection.selectAllText(editor_state.fileContent);
		editor_state.ensure_cursor_visible.vertical = true;
		editor_state.ensure_cursor_visible.horizontal = true;
		std::cout << "Ctrl+A: Selected all text" << std::endl;
	}
}

// New method implementations for the refactored code

void EditorKeyboard::processTextEditorInput()
{
	if (ImGui::IsWindowHovered())
	{
		gEditorMouse.handleMouseInput();
		gEditorScroll.processMouseWheelScrolling();
	}
	if (!editor_state.block_input)
	{
		handleEditorKeyboardInput();
	} else
	{
		editor_state.ensure_cursor_visible.vertical = true;
		editor_state.ensure_cursor_visible.horizontal = true;
	}

	if (gEditorScroll.getEnsureCursorVisibleFrames() > 0)
	{
		editor_state.ensure_cursor_visible.vertical = true;
		editor_state.ensure_cursor_visible.horizontal = true;
		gEditorScroll.decrementEnsureCursorVisibleFrames();
	}
}

void EditorKeyboard::handleEditorKeyboardInput()
{
	bool ctrl_pressed = ImGui::GetIO().KeyCtrl;
	bool shift_pressed = ImGui::GetIO().KeyShift;

	// block input if searching for file...
	if (gFileFinder.showFFWindow || gLineJump.showLineJumpWindow || gLSPAutocomplete.blockTab)
	{
		gLSPAutocomplete.blockTab = false;
		return;
	}

	// Handle AI completion first
	if (gAITab.has_ghost_text)
	{
		if (ImGui::IsKeyPressed(ImGuiKey_Tab))
		{
			gAITab.accept_completion();
			return;
		}
		// Dismiss completion on any character input or arrow key
		if (ImGui::GetIO().InputQueueCharacters.Size > 0 ||
			ImGui::IsKeyPressed(ImGuiKey_LeftArrow) ||
			ImGui::IsKeyPressed(ImGuiKey_RightArrow) ||
			ImGui::IsKeyPressed(ImGuiKey_UpArrow) ||
			ImGui::IsKeyPressed(ImGuiKey_DownArrow) ||
			ImGui::IsKeyPressed(ImGuiKey_Delete) ||
			ImGui::IsKeyPressed(ImGuiKey_Backspace)||
			ImGui::IsKeyPressed(ImGuiKey_Enter)||
			ImGui::IsKeyPressed(ImGuiKey_Escape) ||
			(ImGui::GetIO().KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_X)))
		{
			gAITab.dismiss_completion();
		}

		
	}
	// Cancel any ongoing requests when arrow keys are pressed
	if (ImGui::IsKeyPressed(ImGuiKey_LeftArrow) ||
		ImGui::IsKeyPressed(ImGuiKey_RightArrow) ||
		ImGui::IsKeyPressed(ImGuiKey_UpArrow) ||
		ImGui::IsKeyPressed(ImGuiKey_DownArrow)||			
		ImGui::IsKeyPressed(ImGuiKey_Delete) ||
		ImGui::IsKeyPressed(ImGuiKey_Backspace)||
		ImGui::IsKeyPressed(ImGuiKey_Enter)||
		ImGui::IsKeyPressed(ImGuiKey_Escape))
	{
		gAITab.cancel_request();
	}

	// Process bookmarks first
	if (ImGui::GetIO().KeyAlt && !ImGui::GetIO().KeyCtrl)
	{
		if (ImGui::IsKeyPressed(ImGuiKey_UpArrow))
		{
			gEditorCursor.swapLines(-1);
			return; // Prevent other Alt+Up handling
		}
		if (ImGui::IsKeyPressed(ImGuiKey_DownArrow))
		{
			gEditorCursor.swapLines(1);
			return; // Prevent other Alt+Down handling
		}
		gEditorCursor.processWordMovement(editor_state.fileContent,
										  editor_state.ensure_cursor_visible);
	}
	if (ImGui::IsKeyPressed(ImGuiKey_Escape))
	{
		editor_state.multi_selections.clear();
		editor_state.multi_cursor_indices.clear();
		editor_state.selection_active = false;
		editor_state.selection_end = 0;
		editor_state.selection_start = 0;
	}
	// Process bookmarks first
	if (ImGui::GetIO().KeyAlt && ImGui::GetIO().KeyCtrl)
	{
		if (ImGui::IsKeyPressed(ImGuiKey_UpArrow))
		{
			gEditorCursor.spawnCursorAbove();
			return;
		}
		if (ImGui::IsKeyPressed(ImGuiKey_DownArrow))
		{
			gEditorCursor.spawnCursorBelow();
			return;
		}
		gEditorCursor.processWordMovement(editor_state.fileContent,
										  editor_state.ensure_cursor_visible);
	}

	if (ImGui::IsWindowFocused() && !editor_state.block_input)
	{
		// Process Shift+Tab for indentation removal. If handled, exit early.
		if (gEditorIndentation.processIndentRemoval())
			return;
		gEditorIndentation.handleTabKey();


		if (ctrl_pressed)
		{
			// Handle Ctrl+R for reloading files with external changes
			if (ImGui::IsKeyPressed(ImGuiKey_R, false))
			{
				gFileExplorer.reloadCurrentFile();
			}

			ImGuiKey ai_completions = gKeybinds.getActionKey("ai_completion");
		 
			if (ImGui::IsKeyPressed(ai_completions, false))
			{
				gAITab.tab_complete();
			}
			processFontSizeAdjustment();
			processSelectAll();
			gEditorKeyboard.processUndoRedo();
			gBookmarks.handleBookmarkInput(gFileExplorer);
			gEditorCursor.processCursorJump(editor_state.fileContent,
											editor_state.ensure_cursor_visible);
			ImGuiKey lsp_symbol_info = gKeybinds.getActionKey("lsp_symbol_info");
		 
		
			if (ImGui::IsKeyPressed(lsp_symbol_info, false))
			{
				gLSPSymbolInfo.fetchSymbolInfo(gFileExplorer.currentFile);
			}
			ImGuiKey lsp_find_ref = gKeybinds.getActionKey("lsp_find_ref");
		 
			if (ImGui::IsKeyPressed(lsp_find_ref, false))
			{
				int current_line = gEditor.getLineFromPos(editor_state.cursor_index);
				// Get character offset in current line (same as above)
				int line_start = 0; // Default to 0
				if (current_line >= 0 && current_line < editor_state.editor_content_lines.size())
				{
					line_start = editor_state.editor_content_lines[current_line];
				}
				int char_offset = editor_state.cursor_index - line_start;
				char_offset = std::max(0, char_offset); // Ensure non-negative

				// Call LSP find references using the new global instance
				gLSPGotoRef.findReferences(gFileExplorer.currentFile, current_line, char_offset);
			}
			ImGuiKey lsp_find_def = gKeybinds.getActionKey("lsp_find_def");
		 
			if (ImGui::IsKeyPressed(lsp_find_def, false))
			{
				// Get current line number from editor_state
				int current_line = gEditor.getLineFromPos(editor_state.cursor_index);

				// Get character offset in current line
				int line_start = editor_state.editor_content_lines[current_line];
				int char_offset = editor_state.cursor_index - line_start;

				// Call LSP goto definition
				gLSPGotoDef.gotoDefinition(gFileExplorer.currentFile, current_line, char_offset);
			}
			ImGuiKey lsp_completion = gKeybinds.getActionKey("lsp_completion");
		 
			if (ImGui::IsKeyPressed(lsp_completion, false))
			{
				editor_state.get_autocomplete = false;
				// Get current line number from editor_state
				int current_line = gEditor.getLineFromPos(editor_state.cursor_index);

				// Get character offset in current line
				int line_start = editor_state.editor_content_lines[current_line];
				int char_offset = editor_state.cursor_index - line_start;

				gLSPAutocomplete.requestCompletion(gFileExplorer.currentFile,
												   current_line,
												   char_offset);
			}
		}
	}
	handleTextInput();
	
	if(editor_state.get_autocomplete){
		editor_state.get_autocomplete = false;
				// Get current line number from editor_state
				int current_line = gEditor.getLineFromPos(editor_state.cursor_index);

				// Get character offset in current line
				int line_start = editor_state.editor_content_lines[current_line];
				int char_offset = editor_state.cursor_index - line_start;

				gLSPAutocomplete.requestCompletion(gFileExplorer.currentFile,
												   current_line,
												   char_offset);
	}


	// Handle arrow key visibility
	handleArrowKeyVisibility();

	// Pass the correct variables to handleCursorMovement
	float window_height = ImGui::GetWindowHeight();
	float window_width = ImGui::GetWindowWidth();
	gEditorCursor.handleCursorMovement(editor_state.fileContent,
									   editor_state.text_pos,
									   editor_state.line_height,
									   window_height,
									   window_width);


	if (ImGui::IsWindowFocused() && ctrl_pressed)
		gEditorCopyPaste.processClipboardShortcuts();

	// Update cursor visibility if text has changed
	updateCursorVisibilityOnTextChange();
}

void EditorKeyboard::handleArrowKeyVisibility()
{
	// Additional arrow key presses outside the ctrl block
	if (ImGui::IsKeyPressed(ImGuiKey_UpArrow) || ImGui::IsKeyPressed(ImGuiKey_DownArrow))
		editor_state.ensure_cursor_visible.vertical = true;
	if (ImGui::IsKeyPressed(ImGuiKey_LeftArrow) || ImGui::IsKeyPressed(ImGuiKey_RightArrow))
		editor_state.ensure_cursor_visible.horizontal = true;
}

void EditorKeyboard::updateCursorVisibilityOnTextChange()
{
	// Ensure cursor is visible if text has changed
	if (editor_state.text_changed)
	{
		editor_state.ensure_cursor_visible.vertical = true;
		editor_state.ensure_cursor_visible.horizontal = true;
	}
}

void EditorKeyboard::processUndoRedo()
{
	bool shift_pressed = ImGui::GetIO().KeyShift;
	if (ImGui::IsKeyPressed(ImGuiKey_Z))
	{
		std::cout << "Z key pressed. Ctrl: " << ImGui::GetIO().KeyCtrl
				  << ", Shift: " << shift_pressed << std::endl;

		if (shift_pressed)
		{
			std::cout << "Attempting Redo" << std::endl;
			gFileExplorer.handleRedo();
		} else
		{
			std::cout << "Attempting Undo" << std::endl;
			gFileExplorer.handleUndo();
		}

		gEditor.updateLineStarts();

		editor_state.ensure_cursor_visible.vertical = true;
		editor_state.ensure_cursor_visible.horizontal = true;

		gFileExplorer.currentUndoManager->printStacks();
	}
}