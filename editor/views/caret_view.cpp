#include "caret_view.h"
#include "../editor_state.h"
#include "../editor_view_state.h"
#include "../util/editor_utils.h"
#include "view_layout.h"
#include "wrap_layout.h"

#include <algorithm>
#include <cmath>
#include <string>

float CaretView::caretScreenX(const ImVec2 &textPos) const
{
	return std::floor(EditorUtils::LineColumnX(
		viewState->document().line(viewState->row), viewState->column, textPos.x));
}

void CaretView::draw() const
{
	if (!viewState || !layout)
		return;
	if (viewState->isInputBlocked())
		return;

	ImDrawList *draw_list = ImGui::GetWindowDrawList();
	const float thickness = 2.0f;
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
		const std::string line = (sel.headRow >= 0 && sel.headRow < doc.lineCount())
									 ? doc.line(sel.headRow)
									 : std::string{};
		float x;
		int visualLine = sel.headRow;
		if (layout->wrap && sel.headRow < doc.lineCount())
		{
			const int seg = layout->wrap->segmentOf(sel.headRow, sel.headColumn);
			x = std::floor(layout->textPos.x +
						   layout->wrap->columnX(line, sel.headRow, sel.headColumn));
			visualLine = layout->wrap->rowStartVisualLine(sel.headRow) + seg;
		} else
			x = std::floor(
				EditorUtils::LineColumnX(line, sel.headColumn, layout->textPos.x));
		const float y0 = std::floor(layout->textPos.y +
									static_cast<float>(visualLine) * layout->lineHeight);
		const float half = thickness * 0.5f;
		draw_list->AddRectFilled(ImVec2(x - half, y0),
								 ImVec2(x + half, y0 + layout->lineHeight - 1.0f),
								 (i == primary) ? primaryColor : secondaryColor);
	}
}
