/*
	File: editor_frame.cpp
	Description: Document presentation frame (no overlays).
*/

#include "editor_frame.h"
#include "../util/icons.h"
#include "../util/settings.h"
#include "editor_input.h"
#include "editor_state.h"
#include "editor_view_state.h"
#include "services/git/git_service.h"
#include "services/highlight/highlight_service.h"

#include <algorithm>
#include <cfloat>

EditorFrame::EditorFrame(EditorState &document,
						 EditorViewState &view,
						 EditorInput &editorInput,
						 Settings &appSettings,
						 EditorGit &gitService,
						 EditorHighlight &hl,
						 Icons &iconSet,
						 EditorApi &api)
	: viewState(&view),
	  input(&editorInput),
	  state(&document),
	  settings(&appSettings),
	  titleBar(gitService, iconSet, appSettings, api),
	  textView(document, view, hl, layout),
	  gutter(document, view, gitService, layout),
	  caret(view, layout)
{
	// Frame owns layout; input needs it for hit-testing.
	editorInput.setLayout(layout);
}

void EditorFrame::drawTitleBar(ImFont *font)
{
	// Top chrome (path / git / settings) — same presentation layer as gutter.
	const bool showGitChanges = layout.paneSize.x >= GIT_CHANGES_MIN_WIDTH;
	titleBar.render(font, state->path, showGitChanges);
}

void EditorFrame::recomputeWidthPad()
{
	widthPad = widthMax + std::max(150.0f, widthMax * 0.15f);
}

void EditorFrame::shiftLongestForLineDelta(int dirtyLo, int lineDelta)
{
	// widthLongest is still in pre-edit row indices when noteContentEdit runs.
	if (widthLongest < 0 || lineDelta == 0)
		return;

	if (lineDelta > 0)
	{
		// Insert: rows after the first dirty row shift down by lineDelta.
		if (widthLongest > dirtyLo)
			widthLongest += lineDelta;
		else if (widthLongest == dirtyLo)
			widthLongest = -1; // that line's content changed
	} else
	{
		// Delete: |lineDelta| rows removed starting at dirtyLo (pre-edit).
		const int removed = -lineDelta;
		if (widthLongest >= dirtyLo && widthLongest < dirtyLo + removed)
			widthLongest = -1; // longest row was deleted
		else if (widthLongest >= dirtyLo + removed)
			widthLongest += lineDelta;
	}
}

void EditorFrame::noteContentEdit(int lo, int hi)
{
	// DidEdit is once per user/history action (batches already unioned).
	// Never force a full width rescan on line-count change — that is O(lines)
	// and freezes large files on every Enter. Shift the longest-line index and
	// remeasure only the dirty row span on the next contentWidth() call.
	if (widthFull || !state)
		return;

	if (lo > hi)
		std::swap(lo, hi);

	const int newLines = state->lineCount();
	if (widthLines > 0 && newLines != widthLines)
	{
		const int lineDelta = newLines - widthLines;
		shiftLongestForLineDelta(lo, lineDelta);
		if (widthLongest >= newLines)
			widthLongest = -1;
		widthLines = newLines;
	}

	if (widthDirtyHi < widthDirtyLo)
	{
		widthDirtyLo = lo;
		widthDirtyHi = hi;
	} else
	{
		widthDirtyLo = std::min(widthDirtyLo, lo);
		widthDirtyHi = std::max(widthDirtyHi, hi);
	}
}

float EditorFrame::contentWidth()
{
	// Longest-line horizontal scroll width. Prefer dirty-row updates; full scan
	// only on invalidate / font change / first use.
	ImFont *font = ImGui::GetFont();
	const float fs = font->LegacySize;
	auto measure = [&](const std::string &line) {
		if (line.empty())
			return 0.0f;
		const float w =
			font->CalcTextSizeA(
					fs, FLT_MAX, 0.0f, line.c_str(), line.c_str() + line.size())
				.x;
		return (w + static_cast<float>(line.size()) * 0.1f * (24.0f / fs) * 10.0f) *
			   1.01f;
	};
	auto fullScan = [&] {
		widthMax = 0.0f;
		widthLongest = -1;
		widthLines = state ? state->lineCount() : 0;
		for (int i = 0; i < widthLines; ++i)
		{
			const float w = measure(state->line(i));
			if (w > widthMax)
			{
				widthMax = w;
				widthLongest = i;
			}
		}
		widthFont = fs;
		recomputeWidthPad();
		widthFull = false;
		widthDirtyHi = -1;
		widthDirtyLo = 0;
	};

	if (!state)
		return widthPad;

	const int n = state->lineCount();
	const bool dirty = widthDirtyHi >= widthDirtyLo;

	if (widthFull || widthFont != fs || widthLines <= 0)
	{
		fullScan();
		return widthPad;
	}

	// Line count can change without noteContentEdit (e.g. external setContent
	// already invalidates). If it diverged without a dirty span, full scan once.
	if (n != widthLines)
	{
		if (!dirty)
		{
			fullScan();
			return widthPad;
		}
		// Dirty path: noteContentEdit should have updated widthLines; heal.
		widthLines = n;
		if (widthLongest >= n)
			widthLongest = -1;
	}

	if (dirty && n > 0)
	{
		int lo = std::clamp(widthDirtyLo, 0, n - 1);
		int hi = std::clamp(widthDirtyHi, 0, n - 1);
		if (lo > hi)
			std::swap(lo, hi);

		float localMax = 0.0f;
		int localLongest = -1;
		for (int r = lo; r <= hi; ++r)
		{
			const float w = measure(state->line(r));
			if (w > localMax)
			{
				localMax = w;
				localLongest = r;
			}
		}

		const bool longestInDirty =
			widthLongest >= lo && widthLongest <= hi && widthLongest >= 0;

		if (localMax > widthMax)
		{
			widthMax = localMax;
			widthLongest = localLongest;
		} else if (longestInDirty)
		{
			// Remeasured the previous longest row. Prefer a new local max if it
			// matches; otherwise keep widthMax as a safe overestimate (avoids
			// O(n) full scan when a long line shrinks).
			if (localMax >= widthMax)
			{
				widthMax = localMax;
				widthLongest = localLongest;
			} else
			{
				widthLongest = -1;
			}
		}

		recomputeWidthPad();
		widthDirtyHi = -1;
		widthDirtyLo = 0;
		widthLines = n;
	}

	return widthPad;
}

void EditorFrame::updateLayoutMetrics()
{
	viewState->updateBlinkTime();

	layout.size = ImGui::GetContentRegionAvail();
	gutter.lineNumberWidth =
		ImGui::CalcTextSize("0").x * LINE_NUMBER_DIGITS + LINE_NUMBER_PAD;
	layout.lineHeight = ImGui::GetTextLineHeight();
	layout.editorTopMargin = EDITOR_TOP_MARGIN;
	layout.textLeftMargin = TEXT_LEFT_MARGIN;

	const int lineCount = state->lineCount();
	layout.totalHeight = layout.lineHeight * static_cast<float>(lineCount);
	layout.rainbowMode = settings->settings.value("rainbow", true);
}

void EditorFrame::beginDocumentChild()
{
	ImGui::PushID(EDITOR_CHILD_ID);

	gutter.lineNumbersPos = gutter.createLineNumbersPanel();

	const int lineCount = state->lineCount();
	const float remaining_width = layout.size.x - gutter.lineNumberWidth;
	const float content_width =
		contentWidth() + ImGui::GetFontSize() * SCROLL_WIDTH_FONT_MUL;
	const float content_height = static_cast<float>(lineCount) * layout.lineHeight;

	ImGui::PushStyleVar(ImGuiStyleVar_ChildBorderSize, 1.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 0.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
	ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.5f, 0.5f, 0.5f, 0.5f));
	ImGui::PushStyleColor(ImGuiCol_ScrollbarBg, ImVec4(0.05f, 0.05f, 0.05f, 0.0f));
	ImGui::PushStyleColor(ImGuiCol_ScrollbarGrabHovered, ImVec4(0.6f, 0.6f, 0.6f, 0.7f));
	ImGui::PushStyleColor(ImGuiCol_ScrollbarGrabActive, ImVec4(0.8f, 0.8f, 0.8f, 0.9f));
	ImGui::PushStyleVar(ImGuiStyleVar_ScrollbarSize, 12.0f);
	ImGui::SetNextWindowContentSize(ImVec2(content_width, content_height));
	ImGui::BeginChild(EDITOR_CHILD_ID,
					  ImVec2(remaining_width, ImGui::GetContentRegionAvail().y),
					  false,
					  ImGuiWindowFlags_HorizontalScrollbar);

	updateFocusPolicy();

	viewState->setScrollPosition(ImVec2(ImGui::GetScrollX(), ImGui::GetScrollY()));

	layout.textPos = ImGui::GetCursorScreenPos();
	layout.textPos.y += layout.editorTopMargin;
	layout.textPos.x += layout.textLeftMargin;
}

void EditorFrame::updateFocusPolicy()
{
	// Open-from-tree/finder: focus left on the explorer after the click. Steal
	// ImGui + keyboard focus into this document child so typing works immediately.
	if (viewState->requestFocus)
	{
		ImGui::SetWindowFocus();
		ImGui::SetKeyboardFocusHere();
		viewState->blockInput = false;
		wasEditorFocused = true;
		viewState->requestFocus = false;
		return;
	}

	// Current document child only — not RootAndChildWindows (dock siblings share
	// a root hierarchy and would all look "focused").
	const bool isEditorFocused = ImGui::IsWindowFocused(ImGuiFocusedFlags_ChildWindows) ||
								 ImGui::IsWindowFocused(0);

	if (!isEditorFocused)
	{
		// Always block when not focused. Multi-tab: unfocused editors used to
		// keep blockInput==false and still handled global arrow keys.
		viewState->blockInput = true;
		wasEditorFocused = false;
		return;
	}

	// Focused: only clear block on the gain-focus edge so overlays
	// (file finder, line jump) can keep blockInput true while open.
	if (!wasEditorFocused)
	{
		viewState->blockInput = false;
		wasEditorFocused = true;
		ImGui::SetKeyboardFocusHere();
	} else if (!viewState->blockInput && ImGui::IsWindowAppearing())
	{
		ImGui::SetKeyboardFocusHere();
	}
}

void EditorFrame::drawDocument()
{
	textView.draw();
	caret.draw();

	ImGui::SetCursorPosY(ImGui::GetCursorPosY() + layout.totalHeight +
						 layout.editorTopMargin);
	ImGui::Dummy(ImVec2(0, 0));

	viewState->setScrollPosition(ImVec2(ImGui::GetScrollX(), ImGui::GetScrollY()));

	ImGui::EndChild();
	ImGui::PopStyleColor(4);
	ImGui::PopStyleVar(4);

	ImGui::PushClipRect(
		gutter.lineNumbersPos,
		ImVec2(gutter.lineNumbersPos.x + gutter.lineNumberWidth,
			   gutter.lineNumbersPos.y + layout.size.y - layout.editorTopMargin),
		true);
	gutter.renderLineNumbers();
	ImGui::PopClipRect();

	ImGui::EndGroup();
	ImGui::PopID();
}

void EditorFrame::run(ImFont *font)
{
	// Outer "Editor" child is already open (host). Capture pane for overlays.
	layout.panePos = ImGui::GetWindowPos();
	layout.paneSize = ImGui::GetWindowSize();

	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));

	drawTitleBar(font);
	updateLayoutMetrics();
	beginDocumentChild();
	input->process();
	viewState->updateScroll(layout);
	drawDocument();

	ImGui::PopStyleVar();
}
