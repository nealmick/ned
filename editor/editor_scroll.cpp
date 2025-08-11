#include "editor_scroll.h"
#include "editor.h"
#include "editor_types.h"
#include <algorithm>
#include <cmath>
#include <iostream>

// Global instance
EditorScroll gEditorScroll;

EditorScroll::EditorScroll()
	: scrollPos(0, 0), scrollX(0.0f), ensureCursorVisibleFrames(0), requestedScrollX(0),
	  requestedScrollY(0), hasScrollRequest(false), pendingBookmarkScroll(false),
	  pendingScrollX(0.0f), pendingScrollY(0.0f)
{
}

// Private helper that doesn't reference Editor directly to avoid circular
// dependency
int EditorScroll::getLineFromPosition(const std::vector<int> &line_starts, int pos)
{
	auto it = std::upper_bound(line_starts.begin(), line_starts.end(), pos);
	return std::distance(line_starts.begin(), it) - 1;
}

void EditorScroll::updateScrollAnimation()
{
	// Handle pending cursor centering
	if (pending_cursor_centering && pending_cursor_line >= 0)
	{
		int index = 0;
		int currentLine = 0;
		while (currentLine < pending_cursor_line &&
			   index < editor_state.fileContent.length())
		{
			if (editor_state.fileContent[index] == '\n')
			{
				currentLine++;
			}
			index++;
		}
		index += pending_cursor_char;
		index = std::min(index, (int)editor_state.fileContent.length());
		editor_state.cursor_index = index;
		editor_state.center_cursor_vertical = true;
		gEditorScroll.centerCursorVertically();

		// Reset pending state
		pending_cursor_centering = false;
		pending_cursor_line = -1;
		pending_cursor_char = -1;
	}

	// Animation should complete in ~0.1 seconds
	const float animation_speed = 15.0f; // Move 15% of remaining distance per frame
	const float min_step = 1.0f;		 // Minimum pixels to move per frame
	const float threshold = 0.5f;		 // Snap threshold

	// Handle horizontal animation
	if (scrollAnimation.active_x)
	{
		float target_x = scrollAnimation.target_x;
		float delta_x = target_x - editor_state.current_scroll_x;

		// If we're close enough, snap to target
		if (std::abs(delta_x) < threshold)
		{
			editor_state.current_scroll_x = target_x;
			scrollAnimation.active_x = false;
		} else
		{
			// Move a percentage of the remaining distance
			float step = delta_x * animation_speed * ImGui::GetIO().DeltaTime;

			// Ensure minimum step size for consistent motion
			if (std::abs(step) < min_step)
			{
				step = min_step * (delta_x > 0 ? 1.0f : -1.0f);
			}

			// If we would overshoot, just snap to target
			if ((delta_x > 0 && step > delta_x) || (delta_x < 0 && step < delta_x))
			{
				editor_state.current_scroll_x = target_x;
				scrollAnimation.active_x = false;
			} else
			{
				editor_state.current_scroll_x += step;
			}
		}
	}

	// Handle vertical animation
	if (scrollAnimation.active_y)
	{
		float target_y = scrollAnimation.target_y;
		float delta_y = target_y - editor_state.current_scroll_y;

		// If we're close enough, snap to target
		if (std::abs(delta_y) < threshold)
		{
			editor_state.current_scroll_y = target_y;
			scrollAnimation.active_y = false;
		} else
		{
			// Move a percentage of the remaining distance
			float step = delta_y * animation_speed * ImGui::GetIO().DeltaTime;

			// Ensure minimum step size for consistent motion
			if (std::abs(step) < min_step)
			{
				step = min_step * (delta_y > 0 ? 1.0f : -1.0f);
			}
			// If we would overshoot, just snap to target
			if ((delta_y > 0 && step > delta_y) || (delta_y < 0 && step < delta_y))
			{
				editor_state.current_scroll_y = target_y;
				scrollAnimation.active_y = false;
			} else
			{
				editor_state.current_scroll_y += step;
			}
		}
	}

	// Store updated scroll positions
	scrollX = editor_state.current_scroll_x;
	scrollPos.y = editor_state.current_scroll_y;
}

void EditorScroll::processMouseWheelForEditor()
{
	if (ImGui::IsWindowHovered() && !editor_state.block_input)
	{
		float wheel_y = ImGui::GetIO().MouseWheel;
		float wheel_x = ImGui::GetIO().MouseWheelH;

		// Reduce the multiplier from 3 to 1.5 for a slower vertical scroll
		if (wheel_y != 0)
		{
			editor_state.current_scroll_y -= wheel_y * editor_state.line_height * 1.0f;
			editor_state.current_scroll_y =
				std::max(0.0f,
						 std::min(editor_state.current_scroll_y, ImGui::GetScrollMaxY()));
			scrollPos.y = editor_state.current_scroll_y;
		}

		// Also reduce the horizontal scroll speed multiplier
		if (wheel_x != 0)
		{
			editor_state.current_scroll_x -= wheel_x * ImGui::GetFontSize() * 1.0f;
			editor_state.current_scroll_x =
				std::max(0.0f,
						 std::min(editor_state.current_scroll_x, ImGui::GetScrollMaxX()));
			scrollX = editor_state.current_scroll_x;
		}
	}
}

void EditorScroll::processMouseWheelScrolling()
{
	float wheel_y = ImGui::GetIO().MouseWheel;
	float wheel_x = ImGui::GetIO().MouseWheelH;
	if (wheel_y != 0 || wheel_x != 0)
	{
		if (ImGui::GetIO().KeyShift)
		{ // Horizontal scrolling with Shift+Scroll
			float scroll_amount = wheel_y * ImGui::GetFontSize() * 1;
			float new_scroll_x = ImGui::GetScrollX() - scroll_amount;
			new_scroll_x = std::max(0.0f, std::min(new_scroll_x, ImGui::GetScrollMaxX()));
			ImGui::SetScrollX(new_scroll_x);
			scrollX = new_scroll_x;
		} else
		{ // Vertical scrolling
			float new_scroll_y =
				ImGui::GetScrollY() - wheel_y * editor_state.line_height * 3;
			new_scroll_y = std::max(0.0f, std::min(new_scroll_y, ImGui::GetScrollMaxY()));
			ImGui::SetScrollY(new_scroll_y);
			scrollPos.y = new_scroll_y;
		}
	}
}

float EditorScroll::calculateCursorXPosition()
{
	// Get the line number for the current cursor_index
	// You can use the helper getLineFromPosition from this class if it exists,
	// or EditorUtils::GetLineFromPosition, or gEditor.getLineFromPos.
	// Let's use EditorUtils as it seems to be a common pattern.
	int current_cursor_line =
		EditorUtils::GetLineFromPosition(editor_state.editor_content_lines,
										 editor_state.cursor_index);

	// Basic validation
	if (current_cursor_line < 0 || editor_state.editor_content_lines.empty() ||
		static_cast<size_t>(current_cursor_line) >=
			editor_state.editor_content_lines.size())
	{
		// This case should ideally not be hit if cursor_index and line_starts
		// are consistent. Return base X position as a fallback.
		return editor_state.text_pos.x;
	}

	size_t line_start_char_index = editor_state.editor_content_lines[current_cursor_line];

	// Ensure cursor_index is not before the calculated line_start_char_index
	// (could happen if there's an issue with line_starts or cursor_index update
	// logic elsewhere)
	if (editor_state.cursor_index < static_cast<int>(line_start_char_index))
	{
		// Fallback for inconsistent state.
		return editor_state.text_pos.x;
	}
	const char *line_start_ptr = editor_state.fileContent.c_str() + line_start_char_index;
	const char *cursor_ptr = editor_state.fileContent.c_str() + editor_state.cursor_index;
	const size_t segment_length = cursor_ptr - line_start_ptr;

	float relative_x_offset_on_line = 0.0f;
	if (segment_length > 0)
	{
		ImFont *font = ImGui::GetFont();

		// Calculate width character by character for better accuracy
		const char *current = line_start_ptr;
		while (current < cursor_ptr)
		{
			// Measure each character individually
			char c = *current;
			float char_width =
				font->CalcTextSizeA(font->LegacySize, FLT_MAX, 0.0f, &c, &c + 1).x;

			// Add compensation for this character
			const float base_compensation = 0.3f;
			const float size_factor = 12.0f / font->LegacySize;
			const float compensation_factor =
				base_compensation * size_factor * size_factor;

			relative_x_offset_on_line += char_width + compensation_factor;
			current++;
		}

		// Add extra safety margin proportional to the text width
		relative_x_offset_on_line *= 1.01f;
	}

	return editor_state.text_pos.x + relative_x_offset_on_line;
}

ScrollChange EditorScroll::ensureCursorVisible()
{
	// Removed block_input statement for file content search - cursor visibility should
	// work during search

	// Get current scroll offsets
	float scroll_x = ImGui::GetScrollX();
	float scroll_y = ImGui::GetScrollY();

	// Calculate viewport dimensions
	float scrollbar_width = ImGui::GetStyle().ScrollbarSize;
	float additional_padding = 80.0f;
	float viewport_width = editor_state.size.x - scrollbar_width - additional_padding;
	float viewport_height = editor_state.size.y;

	// Calculate cursor position
	float abs_cursor_x = calculateCursorXPosition();
	// Use our own getLineFromPosition to avoid circular dependency
	int cursor_line =
		getLineFromPosition(editor_state.editor_content_lines, editor_state.cursor_index);
	float abs_cursor_y = editor_state.text_pos.y + cursor_line * editor_state.line_height;

	// Calculate cursor position relative to viewport
	float visible_cursor_x = abs_cursor_x - editor_state.text_pos.x - scroll_x;
	float visible_cursor_y = abs_cursor_y - editor_state.text_pos.y - scroll_y;

	// Margins - space to keep between cursor and edges
	float margin_x = ImGui::GetFontSize() * 4.0f;
	float margin_y = editor_state.line_height * 1.5f;

	// Calculate distances from each edge (negative means cursor is outside)
	float dist_left = visible_cursor_x;
	float dist_right = viewport_width - visible_cursor_x;
	float dist_top = visible_cursor_y;
	float dist_bottom = viewport_height - visible_cursor_y - editor_state.line_height;

	bool scroll_x_changed = false;
	bool scroll_y_changed = false;
	float new_scroll_x = scroll_x;
	float new_scroll_y = scroll_y;

	const float cursor_width = ImGui::GetFontSize() * 1.2f;

	// Calculate the actual visible area considering padding
	const float actual_viewport_width = viewport_width - margin_x * 2;

	// Check if cursor needs horizontal scrolling
	if (visible_cursor_x < margin_x)
	{
		// Cursor too close to left edge
		new_scroll_x = abs_cursor_x - editor_state.text_pos.x - margin_x;
		new_scroll_x = std::max(0.0f, new_scroll_x);
		scroll_x_changed = true;
	} else if (visible_cursor_x > viewport_width - margin_x)
	{
		// Cursor too close to right edge - scroll more aggressively
		float overflow = visible_cursor_x - (viewport_width - margin_x);

		// Add extra buffer proportional to font size
		float extra_buffer = std::max(10.0f, cursor_width * 1.5f);

		new_scroll_x = scroll_x + overflow + extra_buffer;
		new_scroll_x = std::min(new_scroll_x, ImGui::GetScrollMaxX());
		scroll_x_changed = true;
	}

	// Check if cursor needs vertical scrolling
	if (dist_top < margin_y)
	{
		// Cursor too close to or beyond top edge
		new_scroll_y = scroll_y - (margin_y - dist_top);
		new_scroll_y = std::max(0.0f, new_scroll_y);
		scroll_y_changed = true;
	} else if (dist_bottom < margin_y)
	{
		// Cursor too close to or beyond bottom edge
		new_scroll_y = scroll_y + (margin_y - dist_bottom);
		new_scroll_y = std::min(new_scroll_y, ImGui::GetScrollMaxY());
		scroll_y_changed = true;
	}

	// Store target positions in our scroll state variables
	if (scroll_x_changed)
	{
		scrollX = new_scroll_x;
	}

	if (scroll_y_changed)
	{
		scrollPos.y = new_scroll_y;
	}

	return {scroll_y_changed, scroll_x_changed};
}

void EditorScroll::adjustScrollForCursorVisibility()
{
	// IMPORTANT: Update scroll positions from ImGui to capture manual scrolling
	editor_state.current_scroll_y = ImGui::GetScrollY();
	editor_state.current_scroll_x = ImGui::GetScrollX();
	if (editor_state.center_cursor_vertical)
	{
		centerCursorVertically();
		editor_state.center_cursor_vertical = false; // Reset the flag
		return;
	}
	// Update our internal state
	scrollPos.y = editor_state.current_scroll_y;
	scrollX = editor_state.current_scroll_x;

	// First check if there's a direct scroll request
	float requested_x, requested_y;
	if (handleScrollRequest(requested_x, requested_y))
	{
		// Set animation targets for direct requests
		scrollAnimation.active_x = true;
		scrollAnimation.target_x = requested_x;
		scrollAnimation.active_y = true;
		scrollAnimation.target_y = requested_y;

		// Store the targets in state variables too
		scrollX = requested_x;
		scrollPos.y = requested_y;

		// Reset visibility flags
		editor_state.ensure_cursor_visible.vertical = false;
		editor_state.ensure_cursor_visible.horizontal = false;

		return;
	}

	// Only try to ensure cursor visibility if not manually scrolled too far
	if (editor_state.ensure_cursor_visible.vertical ||
		editor_state.ensure_cursor_visible.horizontal)
	{
		// Call ensureCursorVisible to calculate scroll adjustments
		ScrollChange scroll_change = ensureCursorVisible();
		// If scroll changes are needed, set animation targets
		if (scroll_change.horizontal)
		{
			scrollAnimation.active_x = true;
			scrollAnimation.target_x = scrollX;
		}

		if (scroll_change.vertical)
		{
			scrollAnimation.active_y = true;
			scrollAnimation.target_y = scrollPos.y;
		}

		// Reset the visibility flags
		editor_state.ensure_cursor_visible.vertical = false;
		editor_state.ensure_cursor_visible.horizontal = false;
	}
}

void EditorScroll::handleCursorMovementScroll()
{
	float window_height = ImGui::GetWindowHeight();
	float window_width = ImGui::GetWindowWidth();
	// Get current viewport bounds
	float visible_start_y = ImGui::GetScrollY();
	float visible_end_y = visible_start_y + window_height;
	float visible_start_x = ImGui::GetScrollX();
	float visible_end_x = visible_start_x + window_width;

	// Get cursor position
	// Use our own method for getting line from position
	int cursor_line =
		getLineFromPosition(editor_state.editor_content_lines, editor_state.cursor_index);
	float cursor_y = editor_state.text_pos.y + (cursor_line * editor_state.line_height);
	float cursor_x = calculateCursorXPosition();

	// Handle vertical scrolling
	if (cursor_y < visible_start_y)
	{
		scrollPos.y = cursor_y - editor_state.text_pos.y;
	} else if (cursor_y + editor_state.line_height > visible_end_y)
	{
		scrollPos.y = cursor_y + editor_state.line_height - window_height;
	}

	// Handle horizontal scrolling
	if (cursor_x < visible_start_x)
	{
		scrollX = cursor_x - editor_state.text_pos.x;
	} else if (cursor_x > visible_end_x)
	{
		scrollX = cursor_x - window_width + ImGui::GetFontSize();
	}
}

void EditorScroll::centerCursorVertically()
{
	// Calculate cursor line position
	int cursor_line =
		getLineFromPosition(editor_state.editor_content_lines, editor_state.cursor_index);
	float cursor_y = cursor_line * editor_state.line_height;

	// Calculate center position
	float viewport_height = editor_state.size.y;
	float target_scroll_y =
		cursor_y - (viewport_height / 2.0f) + editor_state.line_height;

	// Clamp to valid scroll range
	target_scroll_y = std::clamp(target_scroll_y, 0.0f, ImGui::GetScrollMaxY());

	// Set animation target
	scrollAnimation.active_y = true;
	scrollAnimation.target_y = target_scroll_y;
	scrollPos.y = target_scroll_y;

	// Reset horizontal animation if needed
	scrollAnimation.active_x = false;
}

bool EditorScroll::isScrollAnimationActive() const
{
	return scrollAnimation.active_x || scrollAnimation.active_y;
}
