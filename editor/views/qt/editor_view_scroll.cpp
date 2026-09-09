/*
	File: views/qt/editor_view_scroll.cpp
	Description: Pixel-scroll half of the shared EditorViewState (clamp,
	reveal, wheel-remainder state — same class the ImGui editor_view_scroll
	.cpp extends) plus the EditorFrame widget glue that mirrors the state
	into the QScrollBars and triggers repaints.
*/

#include "editor_frame.h"

#include <QApplication>
#include <QScrollBar>

#include <algorithm>

// --- EditorViewState pixel-scroll operations (Qt backend) -------------------

void EditorViewState::setScrollPx(double px, double maxPx)
{
	scrollPx = std::clamp(px, 0.0, maxPx);
}

void EditorViewState::setScrollXPx(double px, double maxPx)
{
	scrollPxX = std::clamp(px, 0.0, maxPx);
}

void EditorViewState::revealCaretPixels(int v,
										int visible,
										double lineHeight,
										double maxPx)
{
	const int first = firstScrollVisualLine(lineHeight);
	if (v < first)
		setScrollPx(static_cast<double>(v) * lineHeight, maxPx);
	else if (v >= first + visible - 1)
		setScrollPx(static_cast<double>(v - visible + 2) * lineHeight, maxPx);
}

void EditorViewState::revealCaretXPixels(double caretX,
										 double charWidth,
										 double areaWidth,
										 double maxPx)
{
	const double pad = charWidth * 2.0;
	const double right = scrollPxX + areaWidth;
	if (caretX > right - pad)
		setScrollXPx(caretX - areaWidth + pad, maxPx);
	else if (caretX < scrollPxX + pad)
		setScrollXPx(std::max(0.0, caretX - pad), maxPx);
}

// --- EditorFrame widget glue (bar sync + repaint) ----------------------------

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
	return viewState.firstScrollVisualLine(static_cast<double>(lineHeightPx));
}

// Screen y of the first painted visual line. With fractional scroll the
// top row slides up under the top inset (paint clips there).
qreal EditorFrame::rowYBase() const
{
	return static_cast<qreal>(topInset()) -
		   (viewState.scrollPx - firstVisualLine() * lineHeightPx);
}

void EditorFrame::setScrollPixels(qreal px)
{
	// Clamp lives in EditorViewState; the bar mirror is widget glue.
	viewState.setScrollPx(px, static_cast<qreal>(maxScrollPx()));
	syncingScroll = true;
	scrollBar->setValue(std::clamp(
		static_cast<int>(viewState.scrollPx / lineHeightPx + 0.5), 0, maxScrollLine()));
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
	viewState.setScrollXPx(px, maxScrollPxX());
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
	hScrollBar->setValue(static_cast<int>(viewState.scrollPxX));
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
		// grid paintTextRow draws on). countVisualCells counts glyphs
		// without building an expansion — this scan covers the whole
		// document on open, so it must stay allocation-free.
		const qreal w = static_cast<qreal>(countVisualCells(lineRef(r))) * charWidthF();
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
// EditorViewState::revealCaret). Wrap-aware via visual lines; wrap-off
// also reveals horizontally. The clamp/reveal math is EditorViewState's
// (revealCaretPixels/revealCaretXPixels); this computes the widget
// metrics (wrap visual line, caret x) and re-syncs the scrollbars.
void EditorFrame::revealCaret()
{
	ensureWrapFresh(); // callers can arrive between an edit and the paint
	const Selection &caret = viewState.selections[viewState.primaryIndex];
	viewState.revealCaretPixels(visualLineOf(caret.headRow, caret.headColumn),
								visibleLines(),
								static_cast<double>(lineHeightPx),
								static_cast<double>(maxScrollPx()));

	if (!wordWrapEnabled())
	{
		// Exact x even when the caret sits far outside the viewport window
		// (a jump to a distant column must scroll TO it, not a viewport
		// over) — the counting walk is O(column), no expansion built.
		const qreal caretX = static_cast<qreal>(countVisualCells(lineRef(caret.headRow),
																 caret.headColumn)) *
							 charWidthF();
		viewState.revealCaretXPixels(static_cast<double>(caretX),
									 static_cast<double>(charWidthF()),
									 static_cast<double>(textAreaWidth()),
									 static_cast<double>(maxScrollPxX()));
	}
	// The view state moved without the bar-syncing wrappers — re-sync.
	setScrollPixels(viewState.scrollPx);
	if (!wordWrapEnabled())
		setScrollXPixels(viewState.scrollPxX);
}
