#include "caret_view.h"
#include "../editor_state.h"
#include "../editor_view_state.h"
#include "../util/editor_utils.h"
#include "view_layout.h"

#include <algorithm>
#include <cmath>

float CaretView::caretScreenX(const ImVec2 &textPos) const
{
	const std::string line = viewState->document().line(viewState->row);
	float x = textPos.x;
	const int end = std::min(viewState->column, static_cast<int>(line.size()));
	for (int i = 0; i < end;)
	{
		if (line[i] == '\t')
		{
			x += EditorUtils::MeasureGlyphWidth(
				&line[i], &line[i] + 1, x, textPos.x, EditorUtils::kTabSize);
			++i;
		} else
		{
			const char *char_start = &line[i];
			const char *char_end = char_start + 1;
			while (char_end < line.data() + line.size() &&
				   (static_cast<unsigned char>(*char_end) & 0xC0) == 0x80)
				++char_end;
			x += EditorUtils::MeasureGlyphWidth(
				char_start, char_end, x, textPos.x, EditorUtils::kTabSize);
			i = static_cast<int>(char_end - line.data());
		}
	}
	return x;
}

static float
caretXOnLine(const EditorState &doc, int row, int column, const ImVec2 &textPos)
{
	if (row < 0 || row >= doc.lineCount())
		return textPos.x;
	const std::string line = doc.line(row);
	float x = textPos.x;
	const int end = std::min(column, static_cast<int>(line.size()));
	for (int i = 0; i < end;)
	{
		if (line[i] == '\t')
		{
			x += EditorUtils::MeasureGlyphWidth(
				&line[i], &line[i] + 1, x, textPos.x, EditorUtils::kTabSize);
			++i;
		} else
		{
			const char *char_start = &line[i];
			const char *char_end = char_start + 1;
			while (char_end < line.data() + line.size() &&
				   (static_cast<unsigned char>(*char_end) & 0xC0) == 0x80)
				++char_end;
			x += EditorUtils::MeasureGlyphWidth(
				char_start, char_end, x, textPos.x, EditorUtils::kTabSize);
			i = static_cast<int>(char_end - line.data());
		}
	}
	return x;
}

void CaretView::draw() const
{
	if (!viewState || !layout)
		return;
	if (viewState->isInputBlocked())
		return;

	ImDrawList *draw_list = ImGui::GetWindowDrawList();
	const float cursor_thickness = 2.0f;
	const bool rainbowMode = layout->rainbowMode;
	const float blink_alpha = (sinf(viewState->cursorBlinkTime * 4.0f) + 1.0f) * 0.5f;
	const ImU32 primaryColor =
		rainbowMode ? ImGui::ColorConvertFloat4ToU32(EditorUtils::GetRainbowColor())
					: IM_COL32(255, 255, 255, (int)(blink_alpha * 255));
	const ImU32 secondaryColor =
		rainbowMode ? ImGui::ColorConvertFloat4ToU32(EditorUtils::GetRainbowColor())
					: IM_COL32(255, 255, 255, 160);

	const EditorState &doc = viewState->document();
	const int primary = viewState->primaryIndex;

	for (int i = 0; i < viewState->selectionCount(); ++i)
	{
		const Selection &sel = viewState->selections[static_cast<size_t>(i)];
		const float x = caretXOnLine(doc, sel.headRow, sel.headColumn, layout->textPos);
		ImVec2 start = layout->textPos;
		start.x = x;
		start.y += static_cast<float>(sel.headRow) * layout->lineHeight;
		ImVec2 end(start.x, start.y + layout->lineHeight - 1.0f);
		const bool isPrimary = (i == primary);
		draw_list->AddLine(
			start, end, isPrimary ? primaryColor : secondaryColor, cursor_thickness);
	}
}
