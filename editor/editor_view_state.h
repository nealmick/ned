/*
	File: editor_view_state.h
	Description: Carets, selections, and viewport scroll for one editor session.
	Document text lives on EditorState; this is not the document model.

	One or more selections; primary drives scroll/API mirrors (row/column).
	Reveal/scroll lives with caret intent (view layer), not a free-floating service.
*/
#pragma once

#include "platform/ned_types.h"
#include <optional>
#include <vector>

class EditorState;
struct ViewLayout;

struct CursorVisibility
{
	bool vertical = false;
	bool horizontal = false;
};

// One caret + optional range. head is the active end; anchor is the other.
// Columns are UTF-8 byte offsets in the line.
struct Selection
{
	int headRow = 0;
	int headColumn = 0;
	int anchorRow = 0;
	int anchorColumn = 0;
	// Preferred visual column for vertical movement (tabs expand to width 4).
	int preferredColumn = 0;

	bool empty() const { return headRow == anchorRow && headColumn == anchorColumn; }

	void collapseToHead()
	{
		anchorRow = headRow;
		anchorColumn = headColumn;
	}

	void setBoth(int r, int c)
	{
		headRow = anchorRow = r;
		headColumn = anchorColumn = c;
	}

	// Ordered endpoints: (sr, sc) <= (er, ec) lexicographically.
	void getOrdered(int &sr, int &sc, int &er, int &ec) const
	{
		if (anchorRow < headRow || (anchorRow == headRow && anchorColumn <= headColumn))
		{
			sr = anchorRow;
			sc = anchorColumn;
			er = headRow;
			ec = headColumn;
		} else
		{
			sr = headRow;
			sc = headColumn;
			er = anchorRow;
			ec = anchorColumn;
		}
	}
};

// Carets + selections + viewport for one editor session.
// Drawing lives in views/ (CaretView, TextView, GutterView).
class EditorViewState
{
  public:
	explicit EditorViewState(EditorState &document);

	const EditorState &document() const { return *state; }

	// True while an overlay or unfocused host owns the keyboard.
	// Input suppresses keys/text; caret draw stays off (see isInputBlocked).
	bool blockInput = false;
	bool isInputBlocked() const { return blockInput; }

	// One-shot: steal ImGui + keyboard focus into the document child next frame.
	// Set when opening a file from the tree/finder so typing works without a click.
	bool requestFocus = false;

	// Selection set. Always size >= 1. primaryIndex picks scroll/API caret.
	std::vector<Selection> selections;
	int primaryIndex = 0;

	// Primary head mirrors (API, scroll helpers, existing call sites).
	// Keep equal to selections[primaryIndex] head after every mutation.
	int row = 0;
	int column = 0;
	int cursorColumnPreferred = 0;

	// Scroll intent (set by commands/overlays; applied in updateScroll).
	CursorVisibility ensureCursorVisible{};
	bool centerCursorVertical = false;
	float cursorBlinkTime = 0.0f;

	// --- Selection set ---

	Selection &primary();
	const Selection &primary() const;

	int selectionCount() const { return static_cast<int>(selections.size()); }

	// True if any selection has a non-empty range.
	bool hasSelection() const;
	// Primary range empty (clipboard / single-range helpers).
	bool selectionEmpty() const { return primary().empty(); }

	// Ordered endpoints of the primary selection.
	void getOrdered(int &sr, int &sc, int &er, int &ec) const;

	void setBoth(int r, int c);
	// Collapse every selection to its head, then keep only primary.
	void collapseSelection();
	// Drop secondaries; keep primary (collapsed or not).
	void collapseToPrimary();
	void selectAll();

	// Sync row/column/preferred from primary head.
	void syncPrimaryMirrors();
	// Push row/column/preferred into primary head (and anchor if empty).

	void clampAll();
	void mergeSelections();

	// Replace set (clamps + merge). primaryIndex clamped.
	void setSelections(std::vector<Selection> next, int primary = 0);
	// Ensure at least one selection exists.
	void ensureSelections();

	// --- Caret movement (document-aware, per selection) ---

	void calculateVisualColumn(Selection &sel);

	void cursorLeft(Selection &sel);
	void cursorRight(Selection &sel);
	void cursorUp(Selection &sel);
	void cursorDown(Selection &sel);
	void moveWordForward(Selection &sel);
	void moveWordBackward(Selection &sel);

	void updateBlinkTime(double deltaTime);

	// --- Viewport scroll ---
	// Needs current ImGui editor child window active when applying.

	NedVec2 getScrollPosition() const { return scrollPosition; }
	void setScrollPosition(const NedVec2 &position) { scrollPosition = position; }

	// Immediate scroll intent (no animation). Applied in updateScroll same frame.
	// Prefer this over SetScrollY from outside so clamp is centralized.
	void requestScroll(float x, float y) { requestedScroll = NedVec2(x, y); }
	void requestCursorCenter(int line, int character);

	// Apply pending reveal/center/request + animation; write ImGui scroll.
	// Document ImGui child must be current.
	void updateScroll(const ViewLayout &layout);
	// Wheel while hovered over the editor child.
	void processMouseWheelScrolling(const ViewLayout &layout);

	// --- Pixel scroll (Qt backend) ---
	// Vertical/horizontal scroll in pixels with sub-line / sub-cell
	// precision; the Qt QScrollBars mirror these for display. The ImGui
	// backend uses the float scrollPosition above instead.
	double scrollPx = 0.0;
	double scrollPxX = 0.0;
	// Fractional wheel pixels not yet applied (trackpad micro-deltas).
	double wheelCarry = 0.0;
	double wheelCarryX = 0.0;

	// Clamp + store (max comes from the widget's current scroll range).
	void setScrollPx(double px, double maxPx);
	void setScrollXPx(double px, double maxPx);
	// First visual line under fractional pixel scroll.
	int firstScrollVisualLine(double lineHeight) const
	{
		return scrollPx <= 0.0 ? 0 : static_cast<int>(scrollPx / lineHeight);
	}
	// Pixel form of revealCaret (Qt): clamp the caret's visual line / x
	// position into the pixel viewport. Metrics come from the widget.
	void revealCaretPixels(int caretVisualLine,
						   int visibleLines,
						   double lineHeight,
						   double maxPx);
	void revealCaretXPixels(double caretX,
							double charWidth,
							double textAreaWidth,
							double maxPx);

	// True if any selection covers byte col on row (for paint).
	bool isPositionSelected(int row, int col) const;
	// Union of selection line spans for gutter highlight [start, end).
	void selectionLineSpan(int &startLine, int &endLineExclusive) const;

  private:
	EditorState *state;

	struct ScrollAnimation
	{
		bool active = false;
		NedVec2 target = NedVec2(0.0f, 0.0f);
	};

	NedVec2 scrollPosition = NedVec2(0.0f, 0.0f);
	ScrollAnimation scrollAnimation;
	std::optional<NedVec2> requestedScroll;
	std::optional<NedVec2> pendingCursorCenter;

	bool moveCursorVertically(Selection &sel, int line_delta);
	void findColumnFromVisualColumn(Selection &sel, int line);

	float cursorScreenX() const;
	// Visual line of the primary caret (wrap-aware; plain row when not wrapping).
	int caretVisualLine(const ViewLayout &layout) const;
	void centerCursorVertically(const ViewLayout &layout);
	void revealCaret(const ViewLayout &layout, bool horizontal, bool vertical);
	void animateScrollTo(const NedVec2 &target);
	void updateScrollAnimation();
	// Clamp against ImGui scroll max; falls back to layout when max is unset.
	NedVec2 clampToScrollRange(const NedVec2 &position) const;
	NedVec2 clampToScrollRange(const NedVec2 &position, const ViewLayout &layout) const;

	void clampSelection(Selection &sel);
};
