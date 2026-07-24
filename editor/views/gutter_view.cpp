#include "gutter_view.h"
#include "../editor_state.h"
#include "../editor_view_state.h"
#include "../services/git/git_service.h"
#include "../util/editor_utils.h"
#include "view_layout.h"

#include <algorithm>
#include <cstdio>

void GutterView::renderLineNumbers() const
{
	if (!state || !viewState || !git || !layout)
		return;

	static char line_number_buffer[LINE_NUMBER_BUFFER_SIZE];
	ImDrawList *draw_list = ImGui::GetWindowDrawList();

	const int lineCount = state->lineCount();
	const float scrollY = viewState->getScrollPosition().y;
	int start_line = static_cast<int>(scrollY / layout->lineHeight);
	int end_line =
		std::min(lineCount,
				 static_cast<int>((scrollY + (layout->size.y - layout->editorTopMargin)) /
								  layout->lineHeight) +
					 1);

	const bool rainbowMode = layout->rainbowMode;
	ImU32 rainbow_color = CURRENT_LINE_COLOR;
	if (rainbowMode)
		rainbow_color = ImGui::ColorConvertFloat4ToU32(EditorUtils::GetRainbowColor());

	int selectionStartLine = 0;
	int selectionEndLine = 0;
	calculateSelectionLines(selectionStartLine, selectionEndLine);
	const bool anySelection = viewState->hasSelection();

	const int current_line = viewState->row;

	for (int i = start_line; i < end_line; i++)
	{
		float yPos = lineNumbersPos.y + (i * layout->lineHeight) - scrollY;

		snprintf(line_number_buffer, sizeof(line_number_buffer), "%d", i + 1);

		ImU32 line_number_color;
		const bool is_edited = git->isLineEdited(state->path, i + 1);

		if (anySelection && i >= selectionStartLine && i < selectionEndLine)
			line_number_color = rainbow_color;
		else if (i == current_line)
			line_number_color = rainbowMode ? rainbow_color : CURRENT_LINE_COLOR;
		else if (is_edited)
			line_number_color = IM_COL32(255, 255, 255, 255);
		else
			line_number_color = DEFAULT_LINE_NUMBER_COLOR;

		float xPos =
			calculateTextRightAlignedPosition(line_number_buffer, lineNumberWidth);

		draw_list->AddText(ImVec2(xPos, yPos), line_number_color, line_number_buffer);
	}
}

float GutterView::calculateRequiredLineNumberWidth() const
{
	int max_line_number = state->lineCount();
	int min_digits_reference = 999;
	int width_reference = (max_line_number + 1 > min_digits_reference)
							  ? (max_line_number + 1)
							  : min_digits_reference;

	char test_buffer[32];
	snprintf(test_buffer, sizeof(test_buffer), "%d", width_reference);
	float text_width = ImGui::CalcTextSize(test_buffer).x;
	return text_width + 2.0f + 10.0f;
}

ImVec2 GutterView::createLineNumbersPanel()
{
	ImGui::BeginGroup();
	ImGui::PushStyleVar(ImGuiStyleVar_ChildBorderSize, 0.0f);

	float dynamic_width = calculateRequiredLineNumberWidth();
	lineNumberWidth = dynamic_width;

	ImGui::BeginChild("LineNumbers",
					  ImVec2(dynamic_width, ImGui::GetContentRegionAvail().y),
					  false,
					  ImGuiWindowFlags_NoScrollbar);
	ImVec2 pos = ImGui::GetCursorScreenPos();
	pos.y += layout->editorTopMargin;
	lineNumbersPos = pos;
	ImGui::EndChild();
	ImGui::PopStyleVar();
	ImGui::SameLine();
	return lineNumbersPos;
}

float GutterView::calculateTextRightAlignedPosition(const char *text, float width) const
{
	float text_width = ImGui::CalcTextSize(text).x;
	return ImGui::GetCursorScreenPos().x + width - text_width - 10.0f;
}

void GutterView::calculateSelectionLines(int &selectionStartLine,
										 int &selectionEndLine) const
{
	viewState->selectionLineSpan(selectionStartLine, selectionEndLine);
}
