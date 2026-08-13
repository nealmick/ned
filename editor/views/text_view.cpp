#include "text_view.h"
#include "../editor_state.h"
#include "../editor_view_state.h"
#include "../services/highlight/highlight_service.h"
#include "../util/editor_utils.h"
#include "view_layout.h"

#include <algorithm>

const ImVec4 TextView::SELECTION_COLOR(1.0f, 0.1f, 0.7f, 0.3f);
const ImVec4 TextView::CURRENT_LINE_COLOR(0.5f, 0.5f, 0.5f, 0.08f);
const ImVec4 TextView::WHITESPACE_GUIDE_COLOR(0.3f, 0.3f, 0.3f, 0.4f);

size_t TextView::advanceUtf8(const std::string &text, size_t index, size_t end)
{
	if (index >= end)
		return end;
	++index;
	while (index < end && (static_cast<unsigned char>(text[index]) & 0xC0) == 0x80)
		++index;
	return index;
}

float TextView::measureGlyphWidth(const char *start,
								  const char *end,
								  float draw_x,
								  float text_origin_x)
{
	return EditorUtils::MeasureGlyphWidth(start, end, draw_x, text_origin_x, TAB_SIZE);
}

void TextView::getVisibleLineRange(int &start_line, int &end_line) const
{
	const int lineCount = state->lineCount();
	if (lineCount <= 0 || layout->lineHeight <= 0.0f)
	{
		start_line = 0;
		end_line = -1;
		return;
	}

	const float scroll_y = ImGui::GetScrollY();
	const float window_height = ImGui::GetWindowHeight();
	const int last = lineCount - 1;

	start_line = std::max(
		0, static_cast<int>(scroll_y / layout->lineHeight) - VISIBLE_LINE_BUFFER);
	end_line =
		std::min(last,
				 static_cast<int>((scroll_y + window_height) / layout->lineHeight) +
					 VISIBLE_LINE_BUFFER);
}

bool TextView::isSelected(int row, int col) const
{
	return viewState->isPositionSelected(row, col);
}

void TextView::draw() const
{
	if (!state || !viewState || !highlight || !layout)
		return;
	renderWhitespaceGuides();
	renderCurrentLineHighlight();
	renderText();
}

void TextView::renderWhitespaceGuides() const
{
	if (layout->lineHeight <= 0.0f || state->lineCount() <= 0)
		return;

	int start_line = 0;
	int end_line = -1;
	getVisibleLineRange(start_line, end_line);
	if (start_line > end_line)
		return;

	const float space_width = EditorUtils::SpaceWidth();
	const ImU32 guide_color = ImGui::ColorConvertFloat4ToU32(WHITESPACE_GUIDE_COLOR);
	ImDrawList *draw = ImGui::GetWindowDrawList();
	for (int line_num = start_line; line_num <= end_line; ++line_num)
	{
		const std::string line = state->line(line_num);
		int whitespace_units = 0;
		for (char c : line)
		{
			if (c == ' ')
				++whitespace_units;
			else if (c == '\t')
				whitespace_units += TAB_SIZE;
			else
				break;
		}

		const float y0 = layout->textPos.y +
						 static_cast<float>(line_num) * layout->lineHeight -
						 WHITESPACE_GUIDE_Y_OFFSET;
		const float y1 = y0 + layout->lineHeight;

		for (int level = 1; level * TAB_SIZE < whitespace_units; ++level)
		{
			const float guide_x =
				layout->textPos.x + static_cast<float>(level * TAB_SIZE) * space_width;
			draw->AddLine(ImVec2(guide_x, y0), ImVec2(guide_x, y1), guide_color, 1.0f);
		}
	}
}

void TextView::renderCurrentLineHighlight() const
{
	if (layout->lineHeight <= 0.0f || state->lineCount() <= 0)
		return;

	int start_line = 0;
	int end_line = -1;
	getVisibleLineRange(start_line, end_line);
	if (viewState->row < start_line || viewState->row > end_line)
		return;

	const float y0 =
		layout->textPos.y + static_cast<float>(viewState->row) * layout->lineHeight;
	const float y1 = y0 + layout->lineHeight;

	ImVec2 window_pos = ImGui::GetWindowPos();
	const float hl_x0 = window_pos.x + CURRENT_LINE_X_OFFSET;
	const float hl_x1 = window_pos.x + ImGui::GetWindowWidth();

	ImGui::GetWindowDrawList()->AddRectFilled(
		ImVec2(hl_x0, y0),
		ImVec2(hl_x1, y1),
		ImGui::ColorConvertFloat4ToU32(CURRENT_LINE_COLOR));
}

void TextView::renderText() const
{
	if (layout->lineHeight <= 0.0f || state->lineCount() <= 0)
		return;

	int start_line = 0;
	int end_line = -1;
	getVisibleLineRange(start_line, end_line);
	if (start_line > end_line)
		return;

	ImDrawList *draw = ImGui::GetWindowDrawList();
	const ImU32 selCol = ImGui::ColorConvertFloat4ToU32(SELECTION_COLOR);
	const ImVec4 defaultColor = highlight->defaultTextColor();
	const float originX = layout->textPos.x;
	const float lineH = layout->lineHeight;

	for (int line_num = start_line; line_num <= end_line; ++line_num)
	{
		const std::string line = state->line(line_num);
		ImVec2 draw_pos(originX, layout->textPos.y + static_cast<float>(line_num) * lineH);

		const LineColorSpans &spans = highlight->spansForLine(line_num);
		size_t spanIdx = 0;

		// Batch consecutive same-color glyphs into one AddText (tabs break runs).
		const char *runStart = nullptr;
		const char *runEnd = nullptr;
		ImVec2 runPos{};
		ImVec4 runColor{};

		auto flushRun = [&]() {
			if (!runStart)
				return;
			draw->AddText(
				runPos, ImGui::ColorConvertFloat4ToU32(runColor), runStart, runEnd);
			runStart = nullptr;
		};

		for (size_t i = 0; i < line.size();)
		{
			if ((static_cast<unsigned char>(line[i]) & 0xC0) == 0x80)
			{
				++i;
				continue;
			}

			while (spanIdx < spans.size() && spans[spanIdx].end <= static_cast<int>(i))
				++spanIdx;

			ImVec4 color = defaultColor;
			if (spanIdx < spans.size() && spans[spanIdx].start <= static_cast<int>(i))
				color = highlight->colorForSlot(spans[spanIdx].slot);

			const char *char_start = &line[i];
			const char *char_end = char_start + 1;
			const bool isTab = (*char_start == '\t');
			if (!isTab && (static_cast<unsigned char>(*char_start) & 0x80) != 0)
			{
				while (char_end < line.data() + line.size() &&
					   (static_cast<unsigned char>(*char_end) & 0xC0) == 0x80)
					++char_end;
			}

			const float width =
				measureGlyphWidth(char_start, char_end, draw_pos.x, originX);

			if (isSelected(line_num, static_cast<int>(i)))
				draw->AddRectFilled(
					draw_pos, ImVec2(draw_pos.x + width, draw_pos.y + lineH), selCol);

			if (isTab)
			{
				flushRun();
			} else if (!runStart || runColor.x != color.x || runColor.y != color.y ||
					   runColor.z != color.z || runColor.w != color.w ||
					   runEnd != char_start)
			{
				// Color change or non-contiguous bytes → new run.
				flushRun();
				runStart = char_start;
				runEnd = char_end;
				runPos = draw_pos;
				runColor = color;
			} else
			{
				runEnd = char_end;
			}

			draw_pos.x += width;
			i = advanceUtf8(line, i, line.size());
		}
		flushRun();
	}
}
