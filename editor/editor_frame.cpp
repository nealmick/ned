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
#include "services/diagnostics/diagnostics_store.h"
#include "services/git/git_service.h"
#include "services/highlight/highlight_service.h"
#include "util/editor_utils.h"

#include <algorithm>
#include <cfloat>

EditorFrame::EditorFrame(EditorState &document,
						 EditorViewState &view,
						 EditorInput &editorInput,
						 Settings &appSettings,
						 EditorGit &gitService,
						 EditorHighlight &hl,
						 Icons &iconSet)
	: viewState(&view),
	  input(&editorInput),
	  state(&document),
	  settings(&appSettings),
	  titleBar(gitService, iconSet, appSettings),
	  textView(document, view, hl, layout),
	  gutter(document, view, gitService, layout),
	  minimap(document, hl, layout),
	  caret(view, layout)
{
	// Frame owns layout; input needs it for hit-testing.
	editorInput.setLayout(layout);
	textView.setTooltipArbiter(&tooltipArbiter);
	gutter.setTooltipArbiter(&tooltipArbiter);
	textView.setHoverInfo(&hoverTrigger.info());
	gutter.setHoverInfo(&hoverTrigger.info());
}

void EditorFrame::setDiagnostics(const LSPDiagnostics *store)
{
	textView.setDiagnostics(store);
	gutter.setDiagnostics(store);
}

void EditorFrame::drawTitleBar(ImFont *font)
{
	// No document path → no chrome ("Editor - No file selected" was noise).
	if (!state || state->path.empty())
		return;
	const bool showGitChanges = layout.paneSize.x >= ImGui::GetFontSize() * 12.5f;
	titleBar.render(font, state->path, showGitChanges);
}

void EditorFrame::recomputeWidthPad()
{
	widthPad = widthMax + std::max(ImGui::GetFontSize() * 7.5f, widthMax * 0.15f);
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
	if (!state)
		return;
	// Wrap cache dirties unconditionally; the width cache below early-returns
	// when full-invalidated (invalidateContentWidth invalidates both anyway).
	wrapLayout.noteEdit(lo, hi);

	if (widthFull)
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
	const float fs = ImGui::GetFontSize();
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
	const float fs = ImGui::GetFontSize();
	gutter.lineNumberWidth = ImGui::CalcTextSize("0").x * LINE_NUMBER_DIGITS + fs * 0.4f;
	layout.lineHeight = ImGui::GetTextLineHeight();
	layout.editorTopMargin = fs * 0.1f;
	layout.textLeftMargin = fs * 0.35f;

	const int lineCount = state->lineCount();
	layout.totalHeight = layout.lineHeight * static_cast<float>(lineCount);
	layout.rainbowMode = settings->settings.value("rainbow", true);
	layout.wordWrap = settings->settings.value("word_wrap", false);
	layout.wrap = nullptr; // assigned in beginDocumentChild once the wrap width is known

	// Minimap width is layout policy (not a paint-leaf constant leak).
	// settings["minimap"] gates visibility; pane width still hides on narrow splits.
	const bool minimapOn = !settings || settings->settings.value("minimap", true);
	layout.minimapWidth =
		(minimapOn && layout.size.x >= fs * MinimapView::kMinPaneFontMul)
			? fs * MinimapView::kWidthFontMul
			: 0.0f;
}

void EditorFrame::beginDocumentChild()
{
	ImGui::PushID(EDITOR_CHILD_ID);

	// This-frame strip AABB (screen space) before gutter/document widgets move the cursor.
	const ImVec2 origin = ImGui::GetCursorScreenPos();
	const ImVec2 avail = ImGui::GetContentRegionAvail();
	if (layout.minimapWidth > 0.5f)
	{
		layout.minimapMin = ImVec2(origin.x + avail.x - layout.minimapWidth, origin.y);
		layout.minimapMax = ImVec2(origin.x + avail.x, origin.y + avail.y);
	} else
	{
		layout.minimapMin = layout.minimapMax = ImVec2(0, 0);
	}

	gutter.lineNumbersPos = gutter.createLineNumbersPanel();

	const int lineCount = state->lineCount();
	const float remaining_width =
		std::max(1.0f, layout.size.x - gutter.lineNumberWidth - layout.minimapWidth);

	// Wrap width: child inner width minus the vertical scrollbar and margins.
	// Computed before BeginChild so the wrapped height drives the content size
	// (scrollbar extent); re-ensured after the child opens against the real
	// inner width to catch same-frame resizes.
	float content_width;
	float content_height;
	if (layout.wordWrap)
	{
		const float wrapWidth =
			std::max(50.0f,
					 remaining_width - ImGui::GetStyle().ScrollbarSize -
						 layout.textLeftMargin * 2.0f);
		wrapLayout.ensure(*state, wrapWidth);
		layout.wrap = &wrapLayout;
		content_width = 0.0f; // no horizontal extent in wrap mode
		content_height =
			static_cast<float>(wrapLayout.totalVisualLines()) * layout.lineHeight;
	} else
	{
		content_width = contentWidth() + ImGui::GetFontSize() * SCROLL_WIDTH_FONT_MUL;
		content_height = static_cast<float>(lineCount) * layout.lineHeight;
	}

	ImGui::PushStyleVar(ImGuiStyleVar_ChildBorderSize, 1.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 0.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
	ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.5f, 0.5f, 0.5f, 0.5f));
	ImGui::PushStyleColor(ImGuiCol_ScrollbarBg, ImVec4(0.05f, 0.05f, 0.05f, 0.0f));
	ImGui::PushStyleColor(ImGuiCol_ScrollbarGrabHovered, ImVec4(0.6f, 0.6f, 0.6f, 0.7f));
	ImGui::PushStyleColor(ImGuiCol_ScrollbarGrabActive, ImVec4(0.8f, 0.8f, 0.8f, 0.9f));
	ImGui::PushStyleVar(ImGuiStyleVar_ScrollbarSize, ImGui::GetFontSize() * 0.6f);
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

	// Re-ensure against the child's realized inner width (resize same-frame).
	if (layout.wrap)
	{
		const float inner =
			std::max(50.0f,
					 ImGui::GetWindowWidth() - ImGui::GetStyle().ScrollbarSize -
						 layout.textLeftMargin * 2.0f);
		wrapLayout.ensure(*state, inner);
		layout.totalHeight =
			static_cast<float>(wrapLayout.totalVisualLines()) * layout.lineHeight;
	}
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

	if (layout.minimapVisible())
	{
		ImGui::SameLine(0.0f, 0.0f);
		minimap.draw(*viewState);
	}

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
	// Interact uses this-frame layout.minimap* rect; requestScroll applied below.
	if (layout.minimapVisible())
		minimap.interact(*viewState);
	viewState->updateScroll(layout);

	updateHoverTrigger();
	drawDocument();

	ImGui::PopStyleVar();
}

HoverTrigger::Target EditorFrame::hoverHitTest() const
{
	HoverTrigger::Target hit;
	const ViewLayout &l = layout;
	if (l.lineHeight <= 0.0f)
		return hit;

	const ImVec2 mouse = ImGui::GetMousePos();
	if (!ImGui::IsMousePosValid(&mouse))
		return hit;

	// Gutter zone: the line-number column left of the text pane. The gutter
	// child does not scroll; rows are drawn at lineNumbersPos.y + row*lineH -
	// scrollY (GutterView::renderLineNumbers), so the inverse mapping uses the
	// gutter's own origin — NOT layout.textPos, which is already
	// scroll-adjusted (mixing the two double-counts scrollY).
	if (mouse.x >= gutter.lineNumbersPos.x &&
		mouse.x < gutter.lineNumbersPos.x + gutter.lineNumberWidth &&
		mouse.y >= gutter.lineNumbersPos.y && mouse.y < l.panePos.y + l.paneSize.y)
	{
		const float scrollY = viewState->getScrollPosition().y;
		const int row =
			l.wrap ? l.wrap
						 ->yToRow((mouse.y - gutter.lineNumbersPos.y + scrollY) /
								  l.lineHeight)
						 .row
				   : static_cast<int>((mouse.y - gutter.lineNumbersPos.y + scrollY) /
									  l.lineHeight);
		if (row >= 0 && row < state->lineCount())
		{
			hit.zone = HoverTrigger::Zone::Gutter;
			hit.row = row;
		}
		return hit;
	}

	// Text zone: pane minus minimap.
	if (l.minimapVisible() && mouse.x >= l.minimapMin.x && mouse.x <= l.minimapMax.x &&
		mouse.y >= l.minimapMin.y && mouse.y <= l.minimapMax.y)
		return hit;
	const float right =
		l.minimapVisible() ? l.minimapMin.x : (l.panePos.x + l.paneSize.x);
	const float bottom = l.panePos.y + l.paneSize.y;
	if (mouse.x < l.textPos.x || mouse.x >= right || mouse.y < l.textPos.y ||
		mouse.y >= bottom)
		return hit;

	const int n = state->lineCount();
	if (l.wrap)
	{
		const WrapLayout::Hit hitPos =
			l.wrap->yToRow((mouse.y - l.textPos.y) / l.lineHeight);
		if (hitPos.row < 0 || hitPos.row >= n)
			return hit;
		const std::string line = state->line(hitPos.row);
		int column =
			l.wrap->columnAt(line, hitPos.row, hitPos.segment, mouse.x - l.textPos.x);
		column = EditorUtils::SnapToUtf8CharBoundary(line, column);
		hit.zone = HoverTrigger::Zone::Text;
		hit.row = hitPos.row;
		hit.column = column;
		return hit;
	}

	const int row = static_cast<int>((mouse.y - l.textPos.y) / l.lineHeight);
	if (row < 0 || row >= n)
		return hit;

	const std::string line = state->line(row);
	int column = EditorUtils::ColumnAtX(line, mouse.x - l.textPos.x);
	column = EditorUtils::SnapToUtf8CharBoundary(line, column);
	hit.zone = HoverTrigger::Zone::Text;
	hit.row = row;
	hit.column = column;
	return hit;
}

void EditorFrame::updateHoverTrigger()
{
	// Dismissal: keystrokes, click/drag, blocked input, or scrolled content.
	frameDismissed = viewState->blockInput || ImGui::IsMouseDown(ImGuiMouseButton_Left);
	{
		const ImGuiIO &io = ImGui::GetIO();
		for (int key = ImGuiKey_NamedKey_BEGIN; key < ImGuiKey_NamedKey_END; ++key)
		{
			const ImGuiKeyData &kd = io.KeysData[key - ImGuiKey_NamedKey_BEGIN];
			if (kd.Down && kd.DownDuration == 0.0f)
			{
				frameDismissed = true;
				break;
			}
		}
	}
	const ImVec2 scrollNow = viewState->getScrollPosition();
	if (scrollNow.x != lastScroll.x || scrollNow.y != lastScroll.y)
		frameDismissed = true;
	lastScroll = scrollNow;

	const ImVec2 mouse = ImGui::GetMousePos();
	const bool mouseMoved = ImGui::IsMousePosValid(&mouse) &&
							(mouse.x != lastMousePos.x || mouse.y != lastMousePos.y);
	lastMousePos = mouse;

	// Occlusion check: the document child is the current window here. If the
	// mouse is over anything else (dock tab bar, another window, a popup),
	// there is no hover zone — rect math alone can't see stacked windows.
	const HoverTrigger::Target hit =
		ImGui::IsWindowHovered() ? hoverHitTest() : HoverTrigger::Target{};
	hoverTrigger.update(mouseMoved, frameDismissed, hit);
}
