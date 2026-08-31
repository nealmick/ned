#include "text_view.h"
#include "../editor_state.h"
#include "../editor_view_state.h"
#include "../services/diagnostics/diagnostics_store.h"
#include "../services/highlight/highlight_service.h"
#include "../util/editor_utils.h"
#include "../util/utf8.h"
#include "diagnostic_style.h"
#include "hover_tooltip.h"
#include "view_layout.h"

#include <algorithm>
#include <cmath>
#include <vector>

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
	renderCurrentLineHighlight();
	renderVisibleLines();
	renderDiagnosticMarks();
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

void TextView::renderVisibleLines() const
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
	const ImU32 guideCol = ImGui::ColorConvertFloat4ToU32(WHITESPACE_GUIDE_COLOR);
	const ImVec4 defaultColor = highlight->defaultTextColor();
	const float originX = layout->textPos.x;
	const float lineH = layout->lineHeight;
	const float spaceWidth = EditorUtils::SpaceWidth();
	const ImVec2 winPos = ImGui::GetWindowPos();
	const float clipL = winPos.x;
	const float clipR = winPos.x + ImGui::GetWindowWidth();

	static thread_local std::string line;

	for (int line_num = start_line; line_num <= end_line; ++line_num)
	{
		state->lineInto(line_num, line);
		const float y0 = layout->textPos.y + static_cast<float>(line_num) * lineH;

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
		const float gy0 = y0 - WHITESPACE_GUIDE_Y_OFFSET;
		const float gy1 = gy0 + lineH;
		for (int level = 1; level * TAB_SIZE < whitespace_units; ++level)
		{
			const float gx = originX + static_cast<float>(level * TAB_SIZE) * spaceWidth;
			if (gx >= clipR)
				break;
			if (gx >= clipL)
				draw->AddLine(ImVec2(gx, gy0), ImVec2(gx, gy1), guideCol, 1.0f);
		}

		ImVec2 draw_pos(originX, y0);
		const LineColorSpans &spans = highlight->spansForLine(line_num);
		size_t spanIdx = 0;

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

			if (draw_pos.x > clipR)
			{
				flushRun();
				break;
			}

			const bool visible = draw_pos.x + width >= clipL;
			if (visible && isSelected(line_num, static_cast<int>(i)))
				draw->AddRectFilled(
					draw_pos, ImVec2(draw_pos.x + width, draw_pos.y + lineH), selCol);

			if (!visible || isTab)
			{
				flushRun();
			} else if (!runStart || runColor.x != color.x || runColor.y != color.y ||
					   runColor.z != color.z || runColor.w != color.w ||
					   runEnd != char_start)
			{
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

void TextView::renderDiagnosticMarks() const
{
	if (!diagnostics || !state || state->path.empty() || layout->lineHeight <= 0.0f)
		return;

	int start_line = 0;
	int end_line = -1;
	getVisibleLineRange(start_line, end_line);
	if (start_line > end_line)
		return;

	const auto marks = diagnostics->forDocument(state->path);
	if (marks.empty())
		return;

	ImDrawList *draw = ImGui::GetWindowDrawList();
	const float originX = layout->textPos.x;
	const float lineH = layout->lineHeight;
	const ImVec2 winPos = ImGui::GetWindowPos();
	const float clipL = winPos.x;
	const float clipR = winPos.x + ImGui::GetWindowWidth();

	// Squiggle geometry (pixels): wave amplitude / wavelength step / thickness.
	constexpr float kSquiggleAmp = 1.25f;
	constexpr float kSquiggleStep = 2.0f;
	constexpr float kSquiggleThickness = 1.4f;
	constexpr float kSquiggleFreq = 1.2f; // radians per pixel
	// Degenerate ranges still get a visible wave.
	constexpr float kSquiggleMinWidth = 6.0f;
	constexpr float kSquiggleFallbackWidth = 8.0f;
	constexpr float kSquiggleBaselineLift = 3.0f; // above the line bottom

	auto squiggle = [&](float x0, float x1, float y, ImU32 col) {
		if (x1 <= x0)
			x1 = x0 + kSquiggleMinWidth;
		x0 = std::max(x0, clipL);
		x1 = std::min(x1, clipR);
		if (x1 <= x0)
			return;
		ImVec2 prev(x0, y);
		for (float x = x0 + kSquiggleStep; x <= x1 + 0.01f; x += kSquiggleStep)
		{
			const float yy = y + std::sin((x - x0) * kSquiggleFreq) * kSquiggleAmp;
			const ImVec2 next(std::min(x, x1), yy);
			draw->AddLine(prev, next, col, kSquiggleThickness);
			prev = next;
		}
	};

	for (const auto &d : marks)
	{
		if (d.endLine < start_line || d.startLine > end_line)
			continue;

		const ImU32 col = DiagnosticSeverityMark(d.severity);
		const int from = std::max(d.startLine, start_line);
		const int to = std::min(d.endLine, end_line);
		for (int line = from; line <= to; ++line)
		{
			const std::string text = state->line(line);
			int startCol = 0;
			int endCol = static_cast<int>(text.size());
			if (line == d.startLine)
				startCol = EditorUtils::Utf16ToUtf8ByteOffset(text, d.startCharacter);
			if (line == d.endLine)
				endCol = EditorUtils::Utf16ToUtf8ByteOffset(text, d.endCharacter);
			if (endCol < startCol)
				std::swap(endCol, startCol);

			const float x0 = EditorUtils::LineColumnX(text, startCol, originX);
			float x1 = EditorUtils::LineColumnX(text, endCol, originX);
			if (x1 - x0 < 4.0f)
				x1 = x0 + kSquiggleFallbackWidth;
			const float y = layout->textPos.y + static_cast<float>(line) * lineH + lineH -
							kSquiggleBaselineLift;
			squiggle(x0, x1, y, col);
		}
	}

	// Tooltip is trigger-driven (same machine as symbol hover): show only for
	// the frozen hover target when a diagnostic actually covers it.
	if (tooltipArbiter && hoverInfo && hoverInfo->active &&
		hoverInfo->zone == HoverTrigger::Zone::Text)
	{
		const std::string text = state->line(hoverInfo->row);
		const int utf16 = EditorUtils::Utf8ByteOffsetToUtf16(text, hoverInfo->column);
		const auto items = diagnostics->forLine(state->path, hoverInfo->row);
		for (const auto &d : items)
			if (DiagnosticContains(d, hoverInfo->row, utf16))
			{
				RenderDiagnosticTooltip(items, *tooltipArbiter);
				break;
			}
	}
}
