#include "editor_mouse.h"
#include "../files/files.h"
#ifdef _WIN32
// Fix for Windows UTF-8 library assert macro conflict
#include <cassert>
#ifdef assert
#undef assert
#endif
#include "../lib/utfcpp/source/utf8.h"
#ifdef _WIN32
#define assert(expr) ((void)0)
#endif
#else
#include "../lib/utfcpp/source/utf8.h"
#endif

#include "editor.h"
#include "editor/utf8_utils.h"
#include "editor_copy_paste.h"
#include <algorithm>
#include <iostream>

// Global instance
EditorMouse gEditorMouse;

EditorMouse::EditorMouse() : is_dragging(false), anchor_pos(-1), show_context_menu(false)
{
}

// Helper function to check if a character is a word boundary
bool isWordBoundary(char c)
{
	return c == ' ' || c == '.' || c == '(' || c == ')' || c == '{' || c == '}' ||
		   c == '[' || c == ']' || c == ',' || c == ';' || c == ':' || c == '\n' ||
		   c == '\t' || c == '\0' || c == '+' || c == '-' || c == '*' || c == '/' ||
		   c == '%' || c == '=' || c == '!' || c == '<' || c == '>' || c == '&' ||
		   c == '|' || c == '^' || c == '~' || c == '?' || c == '@' || c == '#' ||
		   c == '$' || c == '\\' || c == '\'' || c == '"' || c == '`';
}

// Helper function to find word boundaries
void findWordBoundaries(const std::string &text, int cursor_pos, int &start, int &end)
{
	// Find start boundary (move left until we hit a boundary)
	start = cursor_pos;
	while (start > 0 && !isWordBoundary(text[start - 1]))
	{
		start--;
	}

	// Find end boundary (move right until we hit a boundary)
	end = cursor_pos;
	while (end < text.size() && !isWordBoundary(text[end]))
	{
		end++;
	}
}

void EditorMouse::handleMouseInput()
{
	int char_index = getCharIndexFromCoords();

	// Handle double click
	if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
	{
		// Find word boundaries
		int start, end;
		findWordBoundaries(editor_state.fileContent, char_index, start, end);

		// Set selection
		editor_state.selection_active = true;
		editor_state.selection_start = start;
		editor_state.selection_end = end;
		editor_state.cursor_index = end;

		// Update cursor column preference
		int current_line = gEditor.getLineFromPos(editor_state.cursor_index);
		editor_state.cursor_column_prefered =
			editor_state.cursor_index - editor_state.editor_content_lines[current_line];
		gAITab.cancel_request();
		gAITab.dismiss_completion();
		return;
	}

	// Handle left click
	if (ImGui::IsMouseClicked(ImGuiMouseButton_Left))
	{
		gAITab.cancel_request();
		gAITab.dismiss_completion();
		handleMouseClick(char_index);
	}
	// Handle drag
	else if (ImGui::IsMouseDragging(ImGuiMouseButton_Left) && is_dragging)
	{
		gAITab.cancel_request();
		gAITab.dismiss_completion();
		handleMouseDrag(char_index);
	}
	// Handle release
	else if (ImGui::IsMouseReleased(ImGuiMouseButton_Left))
	{
		gAITab.cancel_request();
		gAITab.dismiss_completion();
		handleMouseRelease();
	}

	// Handle right click for context menu
	if (ImGui::IsMouseClicked(ImGuiMouseButton_Right))
	{
		// Check if the mouse is over the AI agent area
		ImVec2 mousePos = ImGui::GetMousePos();
		float windowWidth = ImGui::GetIO().DisplaySize.x;
		float padding = ImGui::GetStyle().WindowPadding.x;
		float availableWidth = windowWidth - padding * 3;

		// Simple calculation: AI agent pane is the right portion based on split position
		float leftSplit = gSettings.getSplitPos();
		float agentPaneWidth = availableWidth * (1.0f - leftSplit);
		float agentStartX = padding + availableWidth * leftSplit;
		float agentEndX = agentStartX + agentPaneWidth;

		// If mouse is over AI agent area, don't show editor context menu
		if (mousePos.x >= agentStartX && mousePos.x <= agentEndX)
		{
			// Let the AI agent handle the right-click for text selection
			return;
		}

		context_menu_pos = ImGui::GetMousePos();
		show_context_menu = true;
		ImGui::OpenPopup("##EditorContextMenu");
	}
}

void EditorMouse::handleMouseClick(int char_index)
{
	bool cmd_pressed = ImGui::GetIO().KeyCtrl;
	if (cmd_pressed)
	{
		// Add to multi-cursor (ensure no duplicates)
		auto &cursors = editor_state.multi_cursor_indices;
		auto &pref_cols = editor_state.multi_cursor_prefered_columns;
		int current_line = gEditor.getLineFromPos(char_index);

		int new_preferred_col =
			char_index - editor_state.editor_content_lines[current_line];

		if (std::find(cursors.begin(), cursors.end(), char_index) == cursors.end())
		{
			cursors.push_back(char_index);
			pref_cols.push_back(new_preferred_col);
			editor_state.multi_selections.emplace_back(char_index, char_index);
		}
		// Snap all multi-cursors to UTF-8 boundary
		for (size_t i = 0; i < cursors.size(); ++i)
			cursors[i] = snapToUtf8CharBoundary(editor_state.fileContent, cursors[i]);
		std::cout << "Multi-cursors: ";
		for (int idx : cursors)
			std::cout << idx << " ";
		std::cout << "\n";
		editor_state.selection_start = 0;
		editor_state.selection_end = 0;
		editor_state.selection_active = false;
		return;
	} else
	{
		editor_state.multi_cursor_indices.clear();
		editor_state.multi_cursor_prefered_columns.clear();
		editor_state.multi_selections.clear();
	}
	if (ImGui::GetIO().KeyShift)
	{
		// If shift is held and we're not already selecting, set the anchor to
		// the current cursor position.
		if (!editor_state.selection_active)
		{
			anchor_pos = editor_state.cursor_index;
			editor_state.selection_start = anchor_pos;
			editor_state.selection_active = true;
		}
		// Update the selection end based on the new click.
		editor_state.selection_end = char_index;
		editor_state.cursor_index = char_index;
		// Snap to UTF-8 boundary
		editor_state.cursor_index =
			snapToUtf8CharBoundary(editor_state.fileContent, editor_state.cursor_index);

		// Update cursor column preference for shift-click (visual column)
		int current_line = gEditor.getLineFromPos(editor_state.cursor_index);
		int line_start = editor_state.editor_content_lines[current_line];
		int visual_column = 0;
		const int TAB_WIDTH = 4;
		for (int i = line_start; i < editor_state.cursor_index; i++)
		{
			if (editor_state.fileContent[i] == '\t')
			{
				visual_column = ((visual_column / TAB_WIDTH) + 1) * TAB_WIDTH;
			} else
			{
				visual_column++;
			}
		}
		editor_state.cursor_column_prefered = visual_column;
	} else
	{
		// On a regular click (without shift), reset the selection and update
		// the anchor.
		editor_state.cursor_index = char_index;
		anchor_pos = char_index;
		editor_state.selection_start = char_index;
		editor_state.selection_end = char_index;
		editor_state.selection_active = false;
		// Calculate visual column for preferred column (accounts for tabs)
		int current_line = gEditor.getLineFromPos(editor_state.cursor_index);
		int line_start = editor_state.editor_content_lines[current_line];
		int visual_column = 0;
		const int TAB_WIDTH = 4;
		for (int i = line_start; i < editor_state.cursor_index; i++)
		{
			if (editor_state.fileContent[i] == '\t')
			{
				visual_column = ((visual_column / TAB_WIDTH) + 1) * TAB_WIDTH;
			} else
			{
				visual_column++;
			}
		}
		editor_state.cursor_column_prefered = visual_column;
		editor_state.ensure_cursor_visible.horizontal = true;
		// Snap to UTF-8 boundary
		editor_state.cursor_index =
			snapToUtf8CharBoundary(editor_state.fileContent, editor_state.cursor_index);
	}
	is_dragging = true;

	// Update undo manager's pendingFinalCursor for first edit logic
	if (gFileExplorer.currentUndoManager)
	{
		gFileExplorer.currentUndoManager->updatePendingFinalCursor(
			editor_state.cursor_index);
	}
}

void EditorMouse::handleMouseDrag(int char_index)
{
	if (ImGui::GetIO().KeyShift)
	{
		// If shift is held, use the existing anchor for updating selection.
		if (anchor_pos == -1)
		{
			anchor_pos = editor_state.cursor_index;
			editor_state.selection_start = anchor_pos;
		}
		editor_state.selection_end = char_index;
		editor_state.cursor_index = char_index;
		// Snap to UTF-8 boundary
		editor_state.cursor_index =
			snapToUtf8CharBoundary(editor_state.fileContent, editor_state.cursor_index);
	} else
	{
		// Normal drag selection without shift: use the initial click as the
		// anchor.
		editor_state.selection_active = true;

		if (anchor_pos == -1)
		{
			anchor_pos = editor_state.cursor_index;
		}
		editor_state.selection_start = anchor_pos;
		editor_state.selection_end = char_index;
		editor_state.cursor_index = char_index;
		// Snap to UTF-8 boundary
		editor_state.cursor_index =
			snapToUtf8CharBoundary(editor_state.fileContent, editor_state.cursor_index);
	}

	// Update cursor column preference for any drag operation (visual column)
	int current_line = gEditor.getLineFromPos(editor_state.cursor_index);
	int line_start = editor_state.editor_content_lines[current_line];
	int visual_column = 0;
	const int TAB_WIDTH = 4;
	for (int i = line_start; i < editor_state.cursor_index; i++)
	{
		if (editor_state.fileContent[i] == '\t')
		{
			visual_column = ((visual_column / TAB_WIDTH) + 1) * TAB_WIDTH;
		} else
		{
			visual_column++;
		}
	}
	editor_state.cursor_column_prefered = visual_column;
}

void EditorMouse::handleMouseRelease()
{
	is_dragging = false;
	anchor_pos = -1;
}
void EditorMouse::handleContextMenu()
{
	// For debugging
	static bool popupWasOpen = false;
	bool popupIsOpen = ImGui::IsPopupOpen("##EditorContextMenu");

	// Detect state changes for tracking
	if (popupIsOpen && !popupWasOpen)
	{
		std::cout << "Popup was just opened" << std::endl;
	}
	if (!popupIsOpen && popupWasOpen)
	{
		std::cout << "Popup was just closed" << std::endl;
		show_context_menu = false; // Sync our state when ImGui closes the popup
	}
	popupWasOpen = popupIsOpen;

	// Check if right click just happened to open menu
	if (ImGui::IsMouseClicked(ImGuiMouseButton_Right))
	{
		// Check if the mouse is over the AI agent area
		ImVec2 mousePos = ImGui::GetMousePos();
		float windowWidth = ImGui::GetIO().DisplaySize.x;
		float padding = ImGui::GetStyle().WindowPadding.x;
		float availableWidth = windowWidth - padding * 3;

		// Simple calculation: AI agent pane is the right portion based on split position
		float leftSplit = gSettings.getSplitPos();
		float agentPaneWidth = availableWidth * (1.0f - leftSplit);
		float agentStartX = padding + availableWidth * leftSplit;
		float agentEndX = agentStartX + agentPaneWidth;

		// If mouse is over AI agent area, don't show editor context menu
		if (mousePos.x >= agentStartX && mousePos.x <= agentEndX)
		{
			// Let the AI agent handle the right-click for text selection
			return;
		}

		context_menu_pos = ImGui::GetMousePos();
		show_context_menu = true;
		ImGui::OpenPopup("##EditorContextMenu");
	}

	// Set position with edge detection
	if (show_context_menu)
	{
		ImGuiViewport *viewport = ImGui::GetMainViewport();
		ImVec2 safe_pos = context_menu_pos;

		// Get expected menu size (width is fixed at 220, estimate height)
		const float menu_width = 200.0f;
		const float estimated_menu_height = 250.0f; // Adjust based on your item count

		// Check bottom edge collision
		if ((safe_pos.y + estimated_menu_height) > (viewport->Pos.y + viewport->Size.y))
		{
			safe_pos.y -= estimated_menu_height; // Move up by estimated height
		}

		// Check right edge collision
		if ((safe_pos.x + menu_width) > (viewport->Pos.x + viewport->Size.x))
		{
			safe_pos.x -= menu_width; // Move left by menu width
		}

		ImGui::SetNextWindowPos(safe_pos, ImGuiCond_Always);
		ImGui::SetNextWindowSize(ImVec2(menu_width, 0), ImGuiCond_Always);
	}
	// Apply styling - improved with rounded corners and better padding
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(12, 12));
	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 10.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding,
						6.0f); // Round the menu items
	ImGui::PushStyleVar(ImGuiStyleVar_PopupRounding,
						10.0f); // Specific rounding for popups
	ImGui::PushStyleVar(ImGuiStyleVar_PopupBorderSize,
						1.0f); // Thin border for the popup
	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing,
						ImVec2(8, 6)); // Reduced vertical spacing

	ImGui::PushStyleColor(
		ImGuiCol_PopupBg,
		ImVec4(gSettings.getSettings()["backgroundColor"][0].get<float>() * .8,
			   gSettings.getSettings()["backgroundColor"][1].get<float>() * .8,
			   gSettings.getSettings()["backgroundColor"][2].get<float>() * .8,
			   1.0f));
	ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.9f, 0.9f, 0.9f, 1.0f));
	ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.3f, 0.3f, 0.3f, 1.0f));

	// Set hover color to match your selection highlight
	ImGui::PushStyleColor(ImGuiCol_HeaderHovered,
						  ImVec4(1.0f, 0.1f, 0.7f, 0.3f)); // Pink hover
	ImGui::PushStyleColor(ImGuiCol_Header,
						  ImVec4(1.0f, 0.1f, 0.7f, 0.5f)); // Pink selection

	// Begin the popup with flags to prevent it from closing on outside clicks
	if (ImGui::BeginPopup("##EditorContextMenu",
						  ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize |
							  ImGuiWindowFlags_NoTitleBar))
	{
		// We're inside the popup so we must be showing it
		show_context_menu = true;

		// Manual click outside detection
		if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) &&
			!ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByPopup))
		{
			show_context_menu = false;
			ImGui::CloseCurrentPopup();
		}

		// Check for escape key to close
		if (ImGui::IsKeyPressed(ImGuiKey_Escape))
		{
			show_context_menu = false;
			ImGui::CloseCurrentPopup();
		}

		// Menu title - without extra spacing after
		ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Menu");
		ImGui::Separator();

		// Cut action
		if (ImGui::MenuItem("Cut", nullptr, nullptr, editor_state.selection_active))
		{
			gEditorCopyPaste.cutSelectedText();
			show_context_menu = false;
			ImGui::CloseCurrentPopup();
		}

		// Copy action
		if (ImGui::MenuItem("Copy", nullptr, nullptr, editor_state.selection_active))
		{
			gEditorCopyPaste.copySelectedText(editor_state.fileContent);
			show_context_menu = false;
			ImGui::CloseCurrentPopup();
		}

		// Paste action
		if (ImGui::MenuItem("Paste", nullptr, nullptr, true))
		{
			gEditorCopyPaste.pasteText();
			show_context_menu = false;
			ImGui::CloseCurrentPopup();
		}

		// Add a subtle separator for better organization
		ImGui::Separator();

		// Save file action
		if (ImGui::MenuItem("Save", nullptr, nullptr, true))
		{
			gFileExplorer.saveCurrentFile();
			show_context_menu = false;
			ImGui::CloseCurrentPopup();
		}

		// Another subtle separator
		ImGui::Separator();

		// Select All
		if (ImGui::MenuItem("Select All", nullptr, nullptr, true))
		{
			editor_state.selection_active = true;
			editor_state.selection_start = 0;
			editor_state.selection_end = editor_state.fileContent.size();
			editor_state.cursor_index = editor_state.fileContent.size();
			show_context_menu = false;
			ImGui::CloseCurrentPopup();
		}
		ImGui::Separator();

		ImGui::EndPopup();
	} else
	{
		// If popup is not open (closed by ImGui itself)
		if (show_context_menu)
		{
			show_context_menu = false;
			std::cout << "Popup closed by ImGui" << std::endl;
		}
	}

	// Pop styling
	ImGui::PopStyleColor(5);
	ImGui::PopStyleVar(6);
}
int EditorMouse::getCharIndexFromCoords()
{
	ImVec2 mouse_pos = ImGui::GetMousePos();

	// Determine which line was clicked (clamped to valid indices)
	int clicked_line =
		std::clamp(static_cast<int>((mouse_pos.y - editor_state.text_pos.y) /
									editor_state.line_height),
				   0,
				   static_cast<int>(editor_state.editor_content_lines.size()) - 1);

	// Get start/end indices for that line in the text.
	int line_start = editor_state.editor_content_lines[clicked_line];
	int line_end = (clicked_line + 1 < editor_state.editor_content_lines.size())
					   ? editor_state.editor_content_lines[clicked_line + 1]
					   : editor_state.fileContent.size();

	// Adjust line_end to exclude newline character if present
	if (line_end > line_start && line_end <= editor_state.fileContent.size() &&
		editor_state.fileContent[line_end - 1] == '\n')
	{
		line_end--;
	}

	// If the line is empty, return its start.
	if (line_end <= line_start)
		return line_start;

	// Compute the click's x-coordinate relative to the beginning of the text.
	float click_x = mouse_pos.x - editor_state.text_pos.x;

	// UTF-8 aware: iterate by codepoints, not bytes
	std::vector<float> charWidths;
	std::vector<size_t> codepointIndices; // maps codepoint index to byte index
	std::vector<float> insertionPositions;

	size_t char_idx = line_start;
	float cumulative_x = 0.0f;
	insertionPositions.push_back(0.0f);

	while (char_idx < (size_t)line_end)
	{
		const char *char_start = &editor_state.fileContent[char_idx];
		const char *char_end = char_start + 1;
		float width;

		// Handle tab characters specially to match rendering logic
		if (*char_start == '\t')
		{
			// Calculate tab width based on current position
			float space_width = ImGui::CalcTextSize(" ").x;
			const int TAB_SIZE = 4;
			int current_column = static_cast<int>(cumulative_x / space_width);
			int next_tab_stop = ((current_column / TAB_SIZE) + 1) * TAB_SIZE;
			width = (next_tab_stop - current_column) * space_width;
		} else
		{
			if ((*char_start & 0x80) != 0)
			{
				while (char_end < &editor_state.fileContent[line_end] &&
					   (*char_end & 0xC0) == 0x80)
					++char_end;
			}
			width = ImGui::CalcTextSize(char_start, char_end).x;
		}

		charWidths.push_back(width);
		codepointIndices.push_back(char_idx);
		cumulative_x += width;
		insertionPositions.push_back(cumulative_x);
		char_idx += (char_end - char_start);
	}

	// Find the codepoint whose insertion position is closest to click_x
	size_t best_cp = 0;
	float best_dist = std::abs(click_x - insertionPositions[0]);
	for (size_t i = 1; i < insertionPositions.size(); ++i)
	{
		float dist = std::abs(click_x - insertionPositions[i]);
		if (dist < best_dist)
		{
			best_cp = i;
			best_dist = dist;
		}
	}
	// Clamp to valid range
	if (best_cp >= codepointIndices.size())
		return line_end;
	return static_cast<int>(codepointIndices[best_cp]);
}