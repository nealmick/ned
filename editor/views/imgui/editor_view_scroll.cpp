/*
	File: views/imgui/editor_view_scroll.cpp
	Description: EditorViewState viewport methods that touch ImGui scroll
	(wheel, reveal, animation). Cursor/selection logic stays in the core
	editor_view_state.cpp; this file is the backend part.
*/

#include "../../editor_state.h"
#include "../../editor_view_state.h"
#include "../../util/editor_utils.h"
#include "../../views/view_layout.h"
#include "../wrap_layout.h"

#include "imgui.h"

#include <algorithm>

void EditorViewState::updateBlinkTime(double deltaTime)
{
	cursorBlinkTime += static_cast<float>(deltaTime);
}

// ---------------------------------------------------------------------------
// Viewport scroll (reveal / wheel / animation)
// ---------------------------------------------------------------------------

void EditorViewState::requestCursorCenter(int line, int character)
{
	pendingCursorCenter =
		NedVec2(static_cast<float>(line), static_cast<float>(character));
}

void EditorViewState::updateScroll(const ViewLayout &layout)
{
	// Baseline from ImGui (scrollbar). requestScroll / center / reveal override.
	scrollPosition = NedVec2(ImGui::GetScrollX(), ImGui::GetScrollY());

	// Document child focus (not RootAndChildWindows — dock siblings share hierarchy).
	// Find / line-jump put keyboard focus on their InputText, so the document is
	// often "unfocused" while still needing scroll-to-match. One-shot ensure/center
	// flags are intentional navigation and must always run. Free-scroll on an
	// unfocused dock pane is protected by: (1) no longer spamming ensure every
	// frame when blockInput, (2) killing leftover multi-frame scroll animation.
	const bool windowFocused = ImGui::IsWindowFocused(ImGuiFocusedFlags_ChildWindows) ||
							   ImGui::IsWindowFocused(0);

	if (pendingCursorCenter)
	{
		const int line = static_cast<int>(pendingCursorCenter->x);
		const int character = static_cast<int>(pendingCursorCenter->y);

		if (state && state->lineCount() > 0)
		{
			Selection &p = primary();
			p.headRow = std::clamp(line, 0, state->lineCount() - 1);
			p.headColumn = std::clamp(character, 0, state->lineLength(p.headRow));
			p.collapseToHead();
			collapseToPrimary();
			syncPrimaryMirrors();
			centerCursorVertical = true;
		}
		pendingCursorCenter.reset();
	}

	if (!windowFocused)
		scrollAnimation.active = false;

	if (requestedScroll)
	{
		scrollPosition = clampToScrollRange(*requestedScroll, layout);
		scrollAnimation.active = false;
		requestedScroll.reset();
	} else if (centerCursorVertical)
	{
		centerCursorVertically(layout);
		centerCursorVertical = false;
	} else if (ensureCursorVisible.horizontal || ensureCursorVisible.vertical)
	{
		revealCursor(layout, ensureCursorVisible.horizontal, ensureCursorVisible.vertical);
		ensureCursorVisible.horizontal = false;
		ensureCursorVisible.vertical = false;
		// Find/line-jump focus an InputText, so multi-frame anim would be killed
		// while !windowFocused — snap the reveal target immediately instead.
		if (!windowFocused && scrollAnimation.active)
		{
			scrollPosition = scrollAnimation.target;
			scrollAnimation.active = false;
		}
	}

	// Multi-frame anim only while this document owns focus (wheel free-scroll on
	// a sibling must not keep getting pulled toward a stale animation target).
	if (windowFocused)
		updateScrollAnimation();
	ImGui::SetScrollX(scrollPosition.x);
	ImGui::SetScrollY(scrollPosition.y);
}

void EditorViewState::processMouseWheelScrolling(const ViewLayout &layout)
{
	// AllowWhenBlockedByActiveItem: focused sibling can hold ActiveId (keyboard
	// focus) which makes plain IsWindowHovered() false on the pane under the mouse.
	if (!ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem))
		return;

	const ImGuiIO &io = ImGui::GetIO();
	if (io.MouseWheel == 0.0f && io.MouseWheelH == 0.0f)
		return;

	NedVec2 next(ImGui::GetScrollX(), ImGui::GetScrollY());
	if (io.KeyShift && !layout.wrap)
		next.x -= io.MouseWheel * ImGui::GetFontSize();
	else
		next.y -= io.MouseWheel * layout.lineHeight * 3.0f;
	if (!layout.wrap)
		next.x -= io.MouseWheelH * ImGui::GetFontSize();

	scrollPosition = clampToScrollRange(next);
	scrollAnimation.active = false;
	ImGui::SetScrollX(scrollPosition.x);
	ImGui::SetScrollY(scrollPosition.y);
}

void EditorViewState::centerCursorVertically(const ViewLayout &layout)
{
	const float cursorY = static_cast<float>(caretVisualLine(layout)) * layout.lineHeight;
	const float viewH = ImGui::GetWindowHeight();
	float targetY = cursorY - (viewH - layout.lineHeight) * 0.5f;

	// Prefer ImGui's scroll max; if it's still 0 (content size not realized yet),
	// fall back to layout metrics so goto-line can still reveal the caret.
	float maxY = ImGui::GetScrollMaxY();
	if (maxY < 1.0f && layout.totalHeight > viewH)
		maxY = layout.totalHeight + layout.editorTopMargin - viewH;

	scrollPosition.y = std::clamp(targetY, 0.0f, std::max(0.0f, maxY));
	scrollAnimation.active = false; // snap — multi-frame anim was easy to lose
}

int EditorViewState::caretVisualLine(const ViewLayout &layout) const
{
	if (layout.wrap && state && row >= 0 && row < state->lineCount())
		return layout.wrap->rowStartVisualLine(row) + layout.wrap->segmentOf(row, column);
	return row;
}

float EditorViewState::cursorScreenX() const
{
	if (!state)
		return 0.0f;
	if (row < 0 || row >= state->lineCount())
		return 0.0f;

	return EditorUtils::LineColumnX(state->line(row), column);
}

void EditorViewState::revealCursor(const ViewLayout &layout,
								   bool horizontal,
								   bool vertical)
{
	NedVec2 target = scrollPosition;
	const float viewportWidth = ImGui::GetWindowWidth() - ImGui::GetStyle().ScrollbarSize;
	const float viewportHeight = ImGui::GetWindowHeight();
	const float marginX = ImGui::GetFontSize() * 2.0f;
	const float marginY = layout.lineHeight;

	// No horizontal extent in wrap mode — vertical only.
	if (horizontal && !layout.wrap)
	{
		const float x = cursorScreenX();
		if (x < target.x + marginX)
			target.x = x - marginX;
		else if (x + ImGui::GetFontSize() > target.x + viewportWidth - marginX)
			target.x = x + ImGui::GetFontSize() - viewportWidth + marginX;
	}
	if (horizontal && layout.wrap)
		ImGui::SetScrollX(0.0f);

	if (vertical)
	{
		const float y = static_cast<float>(caretVisualLine(layout)) * layout.lineHeight;
		if (y < target.y + marginY)
			target.y = y - marginY;
		else if (y + layout.lineHeight > target.y + viewportHeight - marginY)
			target.y = y + layout.lineHeight - viewportHeight + marginY;
	}

	if (target.x != scrollPosition.x || target.y != scrollPosition.y)
		animateScrollTo(target);
}

void EditorViewState::animateScrollTo(const NedVec2 &target)
{
	scrollAnimation.target = clampToScrollRange(target);
	scrollAnimation.active = true;
}

void EditorViewState::updateScrollAnimation()
{
	if (!scrollAnimation.active)
		return;

	auto advance = [](float current, float target) {
		const float distance = target - current;
		const float step =
			std::max(1.0f, std::abs(distance) * 15.0f * ImGui::GetIO().DeltaTime);
		return std::abs(distance) <= step ? target
										  : current + std::copysign(step, distance);
	};

	scrollPosition.x = advance(scrollPosition.x, scrollAnimation.target.x);
	scrollPosition.y = advance(scrollPosition.y, scrollAnimation.target.y);
	scrollAnimation.active = scrollPosition.x != scrollAnimation.target.x ||
							 scrollPosition.y != scrollAnimation.target.y;
}

NedVec2 EditorViewState::clampToScrollRange(const NedVec2 &position) const
{
	return NedVec2(std::clamp(position.x, 0.0f, std::max(0.0f, ImGui::GetScrollMaxX())),
				   std::clamp(position.y, 0.0f, std::max(0.0f, ImGui::GetScrollMaxY())));
}

NedVec2 EditorViewState::clampToScrollRange(const NedVec2 &position,
											const ViewLayout &layout) const
{
	float maxX = ImGui::GetScrollMaxX();
	float maxY = ImGui::GetScrollMaxY();
	if (maxY < 1.0f && layout.totalHeight > layout.size.y)
		maxY = layout.totalHeight + layout.editorTopMargin - layout.size.y;
	return NedVec2(std::clamp(position.x, 0.0f, std::max(0.0f, maxX)),
				   std::clamp(position.y, 0.0f, std::max(0.0f, maxY)));
}
