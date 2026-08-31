#include "gutter_view.h"
#include "../editor_state.h"
#include "../editor_view_state.h"
#include "../services/diagnostics/diagnostics_store.h"
#include "../services/git/git_service.h"
#include "../util/editor_utils.h"
#include "diagnostic_style.h"
#include "hover_tooltip.h"
#include "view_layout.h"
#include "wrap_layout.h"

#include <algorithm>
#include <cstdio>
#include <string>

void GutterView::renderLineNumbers() const
{
	if (!state || !viewState || !git || !layout)
		return;

	static char line_number_buffer[LINE_NUMBER_BUFFER_SIZE];
	ImDrawList *draw_list = ImGui::GetWindowDrawList();

	const int lineCount = state->lineCount();
	const float scrollY = viewState->getScrollPosition().y;
	int start_line =
		layout->wrap ? std::max(0, layout->wrap->yToRow(scrollY / layout->lineHeight).row)
					 : static_cast<int>(scrollY / layout->lineHeight);
	// In wrap mode rows are >1 visual line tall — end is "first row past the
	// pane bottom"; loop below stops on y instead of a row count.
	int end_line =
		std::min(lineCount,
				 static_cast<int>((scrollY + (layout->size.y - layout->editorTopMargin)) /
								  layout->lineHeight) +
					 1);
	if (layout->wrap)
		end_line = lineCount;

	const bool rainbowMode = layout->rainbowMode;
	ImU32 rainbow_color = CURRENT_LINE_COLOR;
	if (rainbowMode)
		rainbow_color = ImGui::ColorConvertFloat4ToU32(EditorUtils::GetRainbowColor());

	int selectionStartLine = 0;
	int selectionEndLine = 0;
	calculateSelectionLines(selectionStartLine, selectionEndLine);
	const bool anySelection = viewState->hasSelection();

	const int current_line = viewState->row;

	// One snapshot for the whole loop — per-line queries copy the full set.
	const std::vector<int> severityByLine =
		(diagnostics && !state->path.empty())
			? diagnostics->maxSeverityByLine(state->path, lineCount)
			: std::vector<int>{};

	const float paneBottom = lineNumbersPos.y + layout->size.y - layout->editorTopMargin;
	for (int i = start_line; i < end_line; i++)
	{
		const int visualStart = layout->wrap ? layout->wrap->rowStartVisualLine(i) : i;
		float yPos = lineNumbersPos.y + (visualStart * layout->lineHeight) - scrollY;
		if (yPos >= paneBottom)
			break;

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

		const float colW = diagnosticColumnWidth();
		const int sev = i < static_cast<int>(severityByLine.size())
							? severityByLine[static_cast<size_t>(i)]
							: 0;
		if (colW > 0.0f && sev > 0)
		{
			// Mark spans the row's first visual line only — intentional
			// (VSCode-style): continuation lines carry no gutter decoration.
			const ImU32 mark = DiagnosticSeverityMark(sev);
			const float markW = std::max(3.0f, colW * 0.45f);
			const float mx = lineNumbersPos.x + (colW - markW) * 0.5f;
			const float gy0 = yPos + 2.0f;
			const float gy1 = yPos + layout->lineHeight - 2.0f;
			draw_list->AddRectFilled(ImVec2(mx, gy0), ImVec2(mx + markW, gy1), mark);
		}

		draw_list->AddText(ImVec2(xPos, yPos), line_number_color, line_number_buffer);
	}

	// Tooltip is trigger-driven (same machine as symbol hover): the frame's
	// Gutter-zone target picks the row; TextView handles squiggle hover.
	if (diagnostics && !state->path.empty() && tooltipArbiter && hoverInfo &&
		hoverInfo->active && hoverInfo->zone == HoverTrigger::Zone::Gutter)
	{
		RenderDiagnosticTooltip(diagnostics->forLine(state->path, hoverInfo->row),
								*tooltipArbiter);
	}
}

float GutterView::diagnosticColumnWidth() const
{
	if (!diagnostics)
		return 0.0f;
	return std::max(6.0f, ImGui::GetFontSize() * 0.55f);
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
	return diagnosticColumnWidth() + text_width + 2.0f + 10.0f;
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
	return lineNumbersPos.x + width - text_width - 10.0f;
}

void GutterView::calculateSelectionLines(int &selectionStartLine,
										 int &selectionEndLine) const
{
	viewState->selectionLineSpan(selectionStartLine, selectionEndLine);
}
