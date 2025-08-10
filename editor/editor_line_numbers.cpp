#include "editor_line_numbers.h"
#include "../files/files.h"
#include "../util/settings.h"
#include "editor.h"
#include "editor_git.h"
#include <algorithm>
#include <cmath>

// Global instance
EditorLineNumbers gEditorLineNumbers;

void EditorLineNumbers::setCurrentFilePath(const std::string &filepath)
{
	current_filepath = filepath;
}

void EditorLineNumbers::renderLineNumbers()
{
	static char line_number_buffer[LINE_NUMBER_BUFFER_SIZE];
	ImDrawList *draw_list = ImGui::GetWindowDrawList();

	// Calculate visible line range
	int start_line =
		static_cast<int>(gEditorScroll.getScrollPosition().y / editor_state.line_height);
	int end_line = std::min(static_cast<int>(editor_state.editor_content_lines.size()),
							static_cast<int>(
								(gEditorScroll.getScrollPosition().y +
								 (editor_state.size.y - editor_state.editor_top_margin)) /
								editor_state.line_height) +
								1);

	// Pre-calculate rainbow color if needed
	bool rainbow_mode = gSettings.getRainbowMode();
	ImU32 rainbow_color = CURRENT_LINE_COLOR;
	if (rainbow_mode)
	{
		rainbow_color = calculateRainbowColor(editor_state.cursor_blink_time);
	}

	// Calculate selection lines
	int selection_start_line = 0;
	int selection_end_line = 0;
	calculateSelectionLines(selection_start_line, selection_end_line);

	// Get current line from cursor position
	int current_line = EditorUtils::GetLineFromPosition(editor_state.editor_content_lines,
														editor_state.cursor_index);

	// Get text color from theme
	auto &theme = gSettings.getSettings()["themes"][gSettings.getCurrentTheme()];
	ImVec4 text_color =
		ImVec4(theme["text"][0], theme["text"][1], theme["text"][2], theme["text"][3]);
	ImU32 text_color_u32 = ImGui::ColorConvertFloat4ToU32(text_color);

	// Render each visible line number
	for (int i = start_line; i < end_line; i++)
	{
		// Calculate vertical position
		float y_pos = editor_state.line_numbers_pos.y + (i * editor_state.line_height) -
					  gEditorScroll.getScrollPosition().y;

		// Format line number text
		snprintf(line_number_buffer, sizeof(line_number_buffer), "%d", i + 1);

		// Determine color based on selection, current line, and edited status
		ImU32 line_number_color;
		bool is_edited = gEditorGit.isLineEdited(gFileExplorer.currentFile, i + 1);
		float edit_alpha =
			gEditorGit.getLineAnimationAlpha(gFileExplorer.currentFile, i + 1);

		if (i >= selection_start_line && i < selection_end_line &&
			editor_state.selection_active)
		{
			line_number_color = rainbow_color;
		} else if (i == current_line)
		{
			line_number_color = rainbow_mode ? rainbow_color : CURRENT_LINE_COLOR;
		} else if (edit_alpha > 0.0f) // Show animation for both fade in and fade out
		{
			// Interpolate between default color and edited color based on animation alpha
			ImU32 default_color = DEFAULT_LINE_NUMBER_COLOR;
			ImU32 edited_color = IM_COL32(255, 255, 255, 255); // White for edited lines

			// Extract RGBA components
			float default_r = ((default_color >> 24) & 0xFF) / 255.0f;
			float default_g = ((default_color >> 16) & 0xFF) / 255.0f;
			float default_b = ((default_color >> 8) & 0xFF) / 255.0f;
			float default_a = (default_color & 0xFF) / 255.0f;

			float edited_r = ((edited_color >> 24) & 0xFF) / 255.0f;
			float edited_g = ((edited_color >> 16) & 0xFF) / 255.0f;
			float edited_b = ((edited_color >> 8) & 0xFF) / 255.0f;
			float edited_a = (edited_color & 0xFF) / 255.0f;

			// Interpolate
			float r = default_r + (edited_r - default_r) * edit_alpha;
			float g = default_g + (edited_g - default_g) * edit_alpha;
			float b = default_b + (edited_b - default_b) * edit_alpha;
			float a = default_a + (edited_a - default_a) * edit_alpha;

			line_number_color =
				IM_COL32((int)(r * 255), (int)(g * 255), (int)(b * 255), (int)(a * 255));
		} else
		{
			line_number_color = DEFAULT_LINE_NUMBER_COLOR;
		}

		// Calculate position for right-aligned text
		float text_width = ImGui::CalcTextSize(line_number_buffer).x;
		float x_pos = calculateTextRightAlignedPosition(line_number_buffer,
														editor_state.line_number_width);

		// Draw the line number
		draw_list->AddText(ImVec2(x_pos, y_pos), line_number_color, line_number_buffer);
	}
}

float EditorLineNumbers::calculateRequiredLineNumberWidth() const
{
	// Get the maximum line number
	int max_line_number = static_cast<int>(editor_state.editor_content_lines.size());

	// Always reserve space for at least 4 digits (9999)
	int min_digits_reference = 999;
	int width_reference = (max_line_number + 1 > min_digits_reference)
							  ? (max_line_number + 1)
							  : min_digits_reference;

	char test_buffer[32];
	snprintf(test_buffer, sizeof(test_buffer), "%d", width_reference);
	float text_width = ImGui::CalcTextSize(test_buffer).x;

	// Add padding on both sides (2.0f on left, 10.0f on right)
	return text_width + 2.0f + 10.0f;
}

ImVec2 EditorLineNumbers::createLineNumbersPanel()
{
	ImGui::BeginGroup();
	ImGui::PushStyleVar(ImGuiStyleVar_ChildBorderSize, 0.0f);

	// Calculate dynamic width based on max line number
	float dynamic_width = calculateRequiredLineNumberWidth();
	editor_state.line_number_width = dynamic_width;

	ImGui::BeginChild("LineNumbers",
					  ImVec2(dynamic_width, ImGui::GetContentRegionAvail().y),
					  false,
					  ImGuiWindowFlags_NoScrollbar);
	ImVec2 line_numbers_pos = ImGui::GetCursorScreenPos();
	line_numbers_pos.y += editor_state.editor_top_margin;
	ImGui::EndChild();
	ImGui::PopStyleVar();
	ImGui::SameLine();
	return line_numbers_pos;
}

float EditorLineNumbers::calculateTextRightAlignedPosition(const char *text,
														   float line_number_width,
														   float right_margin) const
{
	float text_width = ImGui::CalcTextSize(text).x;
	// Position text with 10.0f padding from the right edge
	return ImGui::GetCursorScreenPos().x + line_number_width - text_width - 10.0f;
}

ImU32 EditorLineNumbers::calculateRainbowColor(float blink_time) const
{
	// Update color less frequently for better performance
	static float last_update_time = 0.0f;
	static ImU32 last_rainbow_color = CURRENT_LINE_COLOR;

	if (blink_time - last_update_time > 0.05f)
	{													 // Update every 50ms
		ImVec4 rainbow = EditorUtils::GetRainbowColor(); // Use the synchronized version
		last_rainbow_color = ImGui::ColorConvertFloat4ToU32(rainbow);
		last_update_time = blink_time;
	}

	return last_rainbow_color;
}

ImU32 EditorLineNumbers::determineLineNumberColor(int line_index,
												  int current_line,
												  int selection_start_line,
												  int selection_end_line,
												  bool is_selecting,
												  bool rainbow_mode,
												  ImU32 rainbow_color) const
{
	// Selected line takes precedence
	if (line_index >= selection_start_line && line_index < selection_end_line &&
		is_selecting)
	{
		return rainbow_color;
	}
	// Current line comes next
	else if (line_index == current_line)
	{
		return rainbow_mode ? rainbow_color : CURRENT_LINE_COLOR;
	}
	// Default line color
	else
	{
		return DEFAULT_LINE_NUMBER_COLOR;
	}
}

void EditorLineNumbers::calculateSelectionLines(int &selection_start_line,
												int &selection_end_line)
{
	// Get selection range accounting for selection direction
	int selection_start =
		std::min(editor_state.selection_start, editor_state.selection_end);
	int selection_end =
		std::max(editor_state.selection_start, editor_state.selection_end);

	// Convert character positions to line indices
	selection_start_line = std::lower_bound(editor_state.editor_content_lines.begin(),
											editor_state.editor_content_lines.end(),
											selection_start) -
						   editor_state.editor_content_lines.begin();

	selection_end_line = std::lower_bound(editor_state.editor_content_lines.begin(),
										  editor_state.editor_content_lines.end(),
										  selection_end) -
						 editor_state.editor_content_lines.begin();
}