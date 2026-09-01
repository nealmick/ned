/*
	File: editor_view_state.cpp
	Description: Interactive editor state — carets, selections, scroll intent.
*/

#include "editor_view_state.h"
#include "editor_state.h"
#include "util/text_columns.h"
#include "views/wrap_layout.h"
#include "views/view_layout.h"

#include <algorithm>
#include <cmath>
#include <string>

EditorViewState::EditorViewState(EditorState &document) : state(&document)
{
	selections.push_back(Selection{});
	primaryIndex = 0;
	syncPrimaryMirrors();
}

// ---------------------------------------------------------------------------
// Selection set
// ---------------------------------------------------------------------------

Selection &EditorViewState::primary()
{
	ensureSelections();
	primaryIndex = std::clamp(primaryIndex, 0, static_cast<int>(selections.size()) - 1);
	return selections[static_cast<size_t>(primaryIndex)];
}

const Selection &EditorViewState::primary() const
{
	const int n = static_cast<int>(selections.size());
	const int i = n <= 0 ? 0 : std::clamp(primaryIndex, 0, n - 1);
	// ensureSelections is non-const; const path assumes size >= 1 after ctor.
	return selections[static_cast<size_t>(i)];
}

bool EditorViewState::hasSelection() const
{
	for (const Selection &s : selections)
	{
		if (!s.empty())
			return true;
	}
	return false;
}

void EditorViewState::getOrdered(int &sr, int &sc, int &er, int &ec) const
{
	primary().getOrdered(sr, sc, er, ec);
}

void EditorViewState::setBoth(int r, int c)
{
	ensureSelections();
	primary().setBoth(r, c);
	collapseToPrimary();
	syncPrimaryMirrors();
}

void EditorViewState::collapseSelection()
{
	// Collapse every head and keep only primary (Escape / clear multi).
	for (Selection &s : selections)
		s.collapseToHead();
	collapseToPrimary();
}

void EditorViewState::collapseToPrimary()
{
	ensureSelections();
	primaryIndex = std::clamp(primaryIndex, 0, static_cast<int>(selections.size()) - 1);
	Selection keep = selections[static_cast<size_t>(primaryIndex)];
	selections.clear();
	selections.push_back(keep);
	primaryIndex = 0;
	syncPrimaryMirrors();
}

void EditorViewState::selectAll()
{
	if (!state)
		return;
	ensureSelections();
	Selection s;
	s.anchorRow = 0;
	s.anchorColumn = 0;
	const int last = state->lineCount() - 1;
	s.headRow = last;
	s.headColumn = state->lineLength(last);
	s.preferredColumn = 0;
	selections.clear();
	selections.push_back(s);
	primaryIndex = 0;
	syncPrimaryMirrors();
}

void EditorViewState::syncPrimaryMirrors()
{
	ensureSelections();
	const Selection &p = primary();
	row = p.headRow;
	column = p.headColumn;
	cursorColumnPreferred = p.preferredColumn;
}

void EditorViewState::applyMirrorsToPrimary()
{
	ensureSelections();
	Selection &p = primary();
	const bool wasEmpty = p.empty();
	p.headRow = row;
	p.headColumn = column;
	p.preferredColumn = cursorColumnPreferred;
	if (wasEmpty)
		p.collapseToHead();
}

void EditorViewState::ensureSelections()
{
	if (selections.empty())
	{
		selections.push_back(Selection{});
		primaryIndex = 0;
	}
	if (primaryIndex < 0 || primaryIndex >= static_cast<int>(selections.size()))
		primaryIndex = 0;
}

void EditorViewState::clampSelection(Selection &sel)
{
	if (!state)
		return;
	sel.headRow = std::clamp(sel.headRow, 0, state->lineCount() - 1);
	sel.anchorRow = std::clamp(sel.anchorRow, 0, state->lineCount() - 1);
	const std::string headLine = state->line(sel.headRow);
	const std::string anchorLine = state->line(sel.anchorRow);
	sel.headColumn = std::clamp(sel.headColumn, 0, static_cast<int>(headLine.size()));
	sel.anchorColumn =
		std::clamp(sel.anchorColumn, 0, static_cast<int>(anchorLine.size()));
	sel.headColumn = EditorUtils::SnapToUtf8CharBoundary(headLine, sel.headColumn);
	sel.anchorColumn = EditorUtils::SnapToUtf8CharBoundary(anchorLine, sel.anchorColumn);
}

void EditorViewState::clampAll()
{
	ensureSelections();
	for (Selection &s : selections)
		clampSelection(s);
	mergeSelections();
	syncPrimaryMirrors();
}

void EditorViewState::mergeSelections()
{
	ensureSelections();
	if (selections.size() <= 1)
		return;

	// Sort by head position; remember primary head so we can restore index.
	const Selection primaryCopy = primary();

	std::sort(
		selections.begin(), selections.end(), [](const Selection &a, const Selection &b) {
			if (a.headRow != b.headRow)
				return a.headRow < b.headRow;
			return a.headColumn < b.headColumn;
		});

	std::vector<Selection> merged;
	merged.reserve(selections.size());
	for (const Selection &s : selections)
	{
		if (merged.empty())
		{
			merged.push_back(s);
			continue;
		}
		Selection &last = merged.back();
		// Same head → keep one (prefer non-empty range).
		if (last.headRow == s.headRow && last.headColumn == s.headColumn)
		{
			if (last.empty() && !s.empty())
				last = s;
			continue;
		}
		// Overlapping ordered ranges → union into one selection (head at end).
		int asr, asc, aer, aec, bsr, bsc, ber, bec;
		last.getOrdered(asr, asc, aer, aec);
		s.getOrdered(bsr, bsc, ber, bec);
		const bool overlapOrTouch = !(aer < bsr || (aer == bsr && aec < bsc) ||
									  ber < asr || (ber == asr && bec < asc));
		if (overlapOrTouch && (!last.empty() || !s.empty()))
		{
			// Union range; place head at document-max end, anchor at min.
			int usr = asr, usc = asc, uer = aer, uec = aec;
			if (bsr < usr || (bsr == usr && bsc < usc))
			{
				usr = bsr;
				usc = bsc;
			}
			if (ber > uer || (ber == uer && bec > uec))
			{
				uer = ber;
				uec = bec;
			}
			last.anchorRow = usr;
			last.anchorColumn = usc;
			last.headRow = uer;
			last.headColumn = uec;
			continue;
		}
		merged.push_back(s);
	}
	selections = std::move(merged);

	// Restore primaryIndex by nearest head to previous primary.
	primaryIndex = 0;
	for (size_t i = 0; i < selections.size(); ++i)
	{
		if (selections[i].headRow == primaryCopy.headRow &&
			selections[i].headColumn == primaryCopy.headColumn)
		{
			primaryIndex = static_cast<int>(i);
			break;
		}
	}
}

void EditorViewState::setSelections(std::vector<Selection> next, int primary)
{
	if (next.empty())
		next.push_back(Selection{});
	selections = std::move(next);
	primaryIndex = std::clamp(primary, 0, static_cast<int>(selections.size()) - 1);
	clampAll();
}

bool EditorViewState::isPositionSelected(int r, int col) const
{
	for (const Selection &sel : selections)
	{
		if (sel.empty())
			continue;
		int sr, sc, er, ec;
		sel.getOrdered(sr, sc, er, ec);
		if (r < sr || r > er)
			continue;
		if (r == sr && r == er)
		{
			if (col >= sc && col < ec)
				return true;
			continue;
		}
		if (r == sr)
		{
			if (col >= sc)
				return true;
			continue;
		}
		if (r == er)
		{
			if (col < ec)
				return true;
			continue;
		}
		return true;
	}
	return false;
}

void EditorViewState::selectionLineSpan(int &startLine, int &endLineExclusive) const
{
	startLine = 0;
	endLineExclusive = 0;
	bool any = false;
	for (const Selection &sel : selections)
	{
		if (sel.empty())
			continue;
		int sr, sc, er, ec;
		sel.getOrdered(sr, sc, er, ec);
		const int endEx = (ec > 0 || er == sr) ? er + 1 : er;
		if (!any)
		{
			startLine = sr;
			endLineExclusive = endEx;
			any = true;
		} else
		{
			startLine = std::min(startLine, sr);
			endLineExclusive = std::max(endLineExclusive, endEx);
		}
	}
}

// ---------------------------------------------------------------------------
// Clamp / visual column
// ---------------------------------------------------------------------------

void EditorViewState::clamp() { clampAll(); }

void EditorViewState::calculateVisualColumn()
{
	calculateVisualColumn(primary());
	syncPrimaryMirrors();
}

void EditorViewState::calculateVisualColumn(Selection &sel)
{
	if (!state)
		return;
	constexpr int TAB_WIDTH = 4;
	int visual_column = 0;
	const std::string line = state->line(sel.headRow);
	const int end = std::min(sel.headColumn, static_cast<int>(line.size()));
	for (int i = 0; i < end; ++i)
	{
		if (line[i] == '\t')
			visual_column = ((visual_column / TAB_WIDTH) + 1) * TAB_WIDTH;
		else
			++visual_column;
	}
	sel.preferredColumn = visual_column;
}

void EditorViewState::findColumnFromVisualColumn(Selection &sel, int line)
{
	constexpr int TAB_WIDTH = 4;
	const std::string text = state->line(line);
	int current_visual = 0;
	int pos = 0;
	const int len = static_cast<int>(text.size());

	while (pos < len && current_visual < sel.preferredColumn)
	{
		if (text[pos] == '\t')
		{
			const int next_tab = ((current_visual / TAB_WIDTH) + 1) * TAB_WIDTH;
			if (next_tab > sel.preferredColumn)
				break;
			current_visual = next_tab;
		} else
		{
			++current_visual;
		}
		++pos;
	}
	sel.headColumn = pos;
	sel.headRow = line;
}

// ---------------------------------------------------------------------------
// Movement
// ---------------------------------------------------------------------------

void EditorViewState::cursorLeft(Selection &sel)
{
	if (sel.headColumn > 0)
	{
		std::string line = state->line(sel.headRow);
		auto it = line.begin() + sel.headColumn;
		EditorUtils::MoveToPrevUtf8Char(it);
		sel.headColumn = static_cast<int>(std::distance(line.begin(), it));
	} else if (sel.headRow > 0)
	{
		--sel.headRow;
		sel.headColumn = state->lineLength(sel.headRow);
	}
	calculateVisualColumn(sel);
}

void EditorViewState::cursorRight(Selection &sel)
{
	std::string line = state->line(sel.headRow);
	const int len = static_cast<int>(line.size());
	if (sel.headColumn < len)
	{
		auto it = line.begin() + sel.headColumn;
		if (it != line.end())
			EditorUtils::MoveToNextUtf8Char(it);
		sel.headColumn = static_cast<int>(std::distance(line.begin(), it));
	} else if (sel.headRow + 1 < state->lineCount())
	{
		++sel.headRow;
		sel.headColumn = 0;
	}
	calculateVisualColumn(sel);
}

bool EditorViewState::moveCursorVertically(Selection &sel, int line_delta)
{
	if (!state)
		return false;
	const int line_count = state->lineCount();
	if (line_count <= 0)
		return false;

	const int target = std::clamp(sel.headRow + line_delta, 0, line_count - 1);
	if (target == sel.headRow)
		return false;

	if (sel.preferredColumn == 0 && sel.headColumn != 0)
		calculateVisualColumn(sel);

	findColumnFromVisualColumn(sel, target);
	return true;
}

void EditorViewState::cursorUp(Selection &sel)
{
	moveCursorVertically(sel, -1);
	clampSelection(sel);
}

void EditorViewState::cursorDown(Selection &sel)
{
	moveCursorVertically(sel, 1);
	clampSelection(sel);
}

void EditorViewState::moveWordForward(Selection &sel)
{
	const std::string line = state->line(sel.headRow);
	const int len = static_cast<int>(line.size());
	int pos = sel.headColumn;

	if (pos < len)
	{
		while (pos < len && !EditorUtils::IsWordChar(line[pos]))
			++pos;
		while (pos < len && EditorUtils::IsWordChar(line[pos]))
			++pos;
	} else if (sel.headRow + 1 < state->lineCount())
	{
		++sel.headRow;
		sel.headColumn = 0;
		calculateVisualColumn(sel);
		return;
	}

	if (sel.headColumn != pos)
	{
		sel.headColumn = pos;
		sel.preferredColumn = sel.headColumn;
	}
}

void EditorViewState::moveWordBackward(Selection &sel)
{
	const std::string line = state->line(sel.headRow);
	int pos = sel.headColumn;

	if (pos > 0)
	{
		while (pos > 0 && !EditorUtils::IsWordChar(line[pos - 1]))
			--pos;
		while (pos > 0 && EditorUtils::IsWordChar(line[pos - 1]))
			--pos;
	} else if (sel.headRow > 0)
	{
		--sel.headRow;
		sel.headColumn = state->lineLength(sel.headRow);
		calculateVisualColumn(sel);
		return;
	}

	if (sel.headColumn != pos)
	{
		sel.headColumn = pos;
		sel.preferredColumn = sel.headColumn;
	}
}

void EditorViewState::cursorLeft()
{
	cursorLeft(primary());
	syncPrimaryMirrors();
}
void EditorViewState::cursorRight()
{
	cursorRight(primary());
	syncPrimaryMirrors();
}
void EditorViewState::cursorUp()
{
	cursorUp(primary());
	syncPrimaryMirrors();
}
void EditorViewState::cursorDown()
{
	cursorDown(primary());
	syncPrimaryMirrors();
}
void EditorViewState::moveWordForward()
{
	moveWordForward(primary());
	syncPrimaryMirrors();
}
void EditorViewState::moveWordBackward()
{
	moveWordBackward(primary());
	syncPrimaryMirrors();
}
