#include "editor_line_numbers.h"
#include "editor_git.h"
#include "../util/settings.h"
#include "editor.h"
#include <algorithm>
#include <cmath>

// Global instance
EditorLineNumbers gEditorLineNumbers;

void EditorLineNumbers::setCurrentFilePath(const std::string& filepath) {
	current_filepath = filepath;
	EditorGit::getInstance().setCurrentFile(filepath);
	EditorGit::getInstance().initializeFileTracking(filepath);
}

void EditorLineNumbers::renderLineNumbers()
{
	static char line_number_buffer[LINE_NUMBER_BUFFER_SIZE];
	ImDrawList *draw_list = ImGui::GetWindowDrawList();

	// Calculate visible line range
	int start_line =
		static_cast<int>(gEditorScroll.getScrollPosition().y / editor_state.line_height);
	int end_line =
		std::min(static_cast<int>(editor_state.editor_content_lines.size()),
				 static_cast<int>((gEditorScroll.getScrollPosition().y +
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

	// Render each visible line number
	for (int i = start_line; i < end_line; i++)
	{
		// Calculate vertical position
		float y_pos = editor_state.line_numbers_pos.y + (i * editor_state.line_height) -
					  gEditorScroll.getScrollPosition().y;

		// Format line number text with asterisk if edited
		bool is_edited = EditorGit::getInstance().isLineEdited(i + 1);
		snprintf(line_number_buffer, sizeof(line_number_buffer), "%s%d", 
				is_edited ? "*" : "", i + 1);

		// Determine color based on selection and current line
		ImU32 line_number_color = determineLineNumberColor(i,
														   current_line,
														   selection_start_line,
														   selection_end_line,
														   editor_state.selection_active,
														   rainbow_mode,
														   rainbow_color);

		// Calculate position for right-aligned text
		float text_width = ImGui::CalcTextSize(line_number_buffer).x;
		float x_pos =
			calculateTextRightAlignedPosition(line_number_buffer, editor_state.line_number_width);

		// Draw the line number
		draw_list->AddText(ImVec2(x_pos, y_pos), line_number_color, line_number_buffer);
	}
}

ImVec2 EditorLineNumbers::createLineNumbersPanel()
{
	ImGui::BeginGroup();
	ImGui::PushStyleVar(ImGuiStyleVar_ChildBorderSize, 0.0f);
	ImGui::BeginChild("LineNumbers",
					  ImVec2(editor_state.line_number_width, ImGui::GetContentRegionAvail().y),
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
	return ImGui::GetCursorScreenPos().x + line_number_width - text_width - right_margin;
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
	if (line_index >= selection_start_line && line_index < selection_end_line && is_selecting)
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

void EditorLineNumbers::calculateSelectionLines(int &selection_start_line, int &selection_end_line)
{
	// Get selection range accounting for selection direction
	int selection_start = std::min(editor_state.selection_start, editor_state.selection_end);
	int selection_end = std::max(editor_state.selection_start, editor_state.selection_end);

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