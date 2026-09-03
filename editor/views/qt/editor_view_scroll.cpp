/*
	File: views/qt/editor_view_scroll.cpp
	Description: EditorFrame viewport methods that touch Qt scroll state
	(pixel scroll sync, reveal, h-scroll range, caret blink). Counterpart
	of views/imgui/editor_view_scroll.cpp (the ImGui-scroll part of the
	shared EditorViewState).
*/

#include "editor_frame.h"

#include <QApplication>
#include <QScrollBar>

#include <algorithm>

bool EditorFrame::caretActive() const
{
	const QWidget *fw = QApplication::focusWidget();
	return fw == nullptr || fw == this || isAncestorOf(fw);
}
void EditorFrame::scheduleBlink()
{
	// Activity: caret solid, blink phase restarts (single-clock blink).
	blinkClock.restart();
	caretVisible = true;
	update();
}

int EditorFrame::visibleLines() const
{
	return std::max(1, (height() - topInset()) / lineHeightPx);
}

int EditorFrame::maxScrollLine() const
{
	// Last line fully visible at bottom: allow scrolling past it a little
	// (ImGui scrolls to keep the caret line plus context visible).
	return std::max(0, totalLines() - visibleLines() + 2);
}

int EditorFrame::maxScrollPx() const { return maxScrollLine() * lineHeightPx; }

int EditorFrame::firstVisualLine() const
{
	return std::max(0, static_cast<int>(scrollPx / lineHeightPx));
}

// Screen y of the first painted visual line. With fractional scroll the
// top row slides up under the top inset (paint clips there).
qreal EditorFrame::rowYBase() const
{
	return static_cast<qreal>(topInset()) - (scrollPx - firstVisualLine() * lineHeightPx);
}

void EditorFrame::setScrollPixels(qreal px)
{
	scrollPx = std::clamp(px, 0.0, static_cast<qreal>(maxScrollPx()));
	syncingScroll = true;
	scrollBar->setValue(
		std::clamp(static_cast<int>(scrollPx / lineHeightPx + 0.5), 0, maxScrollLine()));
	syncingScroll = false;
	update();
}

qreal EditorFrame::maxScrollPxX()
{
	ensureLongestLine();
	// Two cells of overshoot past the longest line (ImGui scroll-range pad).
	return std::max<qreal>(0.0, widthMaxPx - textAreaWidth() + charWidthF() * 2.0);
}

void EditorFrame::setScrollXPixels(qreal px)
{
	scrollPxX = std::clamp(px, 0.0, maxScrollPxX());
	syncHScrollBar();
	update();
}

void EditorFrame::syncHScrollBar()
{
	// Overlay strip: only when wrap is off AND lines overflow. Range in
	// pixels so dragging matches the wheel's sub-cell precision.
	const int maxPx = static_cast<int>(maxScrollPxX());
	hScrollBar->setVisible(!wordWrapEnabled() && maxPx > 0);
	syncingScrollX = true;
	hScrollBar->setRange(0, maxPx);
	hScrollBar->setPageStep(std::max(1, textAreaWidth()));
	hScrollBar->setValue(static_cast<int>(scrollPxX));
	syncingScrollX = false;
}

void EditorFrame::ensureLongestLine()
{
	if (!widthDirty)
		return;
	widthDirty = false;
	refreshLongestLine(0, state.lineCount() - 1);
}

// Incremental longest-line cache (editor_frame.cpp scheme): scan the dirty
// span; keep the old max as a safe overestimate when the longest row
// shrank (avoids an O(n) rescan per keystroke).
void EditorFrame::refreshLongestLine(int lo, int hi)
{
	if (hi < lo)
		return;
	qreal localMax = 0.0;
	int localLongest = -1;
	for (int r = lo; r <= hi && r < state.lineCount(); ++r)
	{
		// Painted width = tab-expanded CELLS * cell width (the monospace
		// grid paintTextRow draws on) — visualCount counts glyphs, so an
		// astral-plane char is one cell, not two UTF-16 units.
		const qreal w = expandRow(r).visualCount() * charWidthF();
		if (w > localMax)
		{
			localMax = w;
			localLongest = r;
		}
	}
	const bool longestInDirty =
		widthLongestRow >= lo && widthLongestRow <= hi && widthLongestRow >= 0;
	if (localMax > widthMaxPx)
	{
		widthMaxPx = localMax;
		widthLongestRow = localLongest;
	} else if (longestInDirty && localMax >= widthMaxPx)
	{
		widthMaxPx = localMax;
		widthLongestRow = localLongest;
	} else if (longestInDirty)
		widthLongestRow = -1; // overestimate stays; range only shrinks lazily
}

// Keep the caret inside the viewport after edits/navigation (ImGui:
// EditorViewState::revealCursor). Wrap-aware via visual lines; wrap-off
// also reveals horizontally (ImGui revealCursor's x axis).
void EditorFrame::revealCaret()
{
	ensureWrapFresh(); // callers can arrive between an edit and the paint
	const Selection &caret = viewState.selections[viewState.primaryIndex];
	const int v = visualLineOf(caret.headRow, caret.headColumn);
	const int first = firstVisualLine();
	const int visible = visibleLines();
	if (v < first)
		setScrollPixels(v * lineHeightPx);
	else if (v >= first + visible - 1)
		setScrollPixels((v - visible + 2) * lineHeightPx);

	if (!wordWrapEnabled())
	{
		const qreal caretX = xAtByteColumn(caret.headRow, caret.headColumn);
		const qreal pad = charWidthF() * 2.0;
		const qreal right = scrollPxX + textAreaWidth();
		if (caretX > right - pad)
			setScrollXPixels(caretX - textAreaWidth() + pad);
		else if (caretX < scrollPxX + pad)
			setScrollXPixels(std::max<qreal>(0.0, caretX - pad));
	}
}
