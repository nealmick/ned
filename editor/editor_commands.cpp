/*
	File: editor_commands.cpp
	Description: User-facing editor actions.
*/

#include "editor_commands.h"
#include "editor_events.h"
#include "editor_state.h"
#include "editor_view_state.h"
#include "services/save_service.h"
#include "util/editor_utils.h"
#include "util/project_undo.h"

#include "imgui.h"

#include <algorithm>
#include <string>
#include <vector>

namespace {

std::vector<EditorEvents::DocumentChange>
documentChangesFromPending(const EditorOperations *ops)
{
	std::vector<EditorEvents::DocumentChange> out;
	if (!ops)
		return out;
	const auto &pending = ops->pendingEdits();
	out.reserve(pending.size());
	for (const PendingEdit &edit : pending)
		out.push_back(edit.toDocumentChange());
	return out;
}

} // namespace

// ---------------------------------------------------------------------------
// Undo group + user edit path
// ---------------------------------------------------------------------------

std::vector<SelectionSnapshot> EditorCommands::snapshotSelections() const
{
	std::vector<SelectionSnapshot> out;
	if (!view)
		return out;
	out.reserve(view->selections.size());
	for (const Selection &s : view->selections)
	{
		SelectionSnapshot snap;
		snap.headRow = s.headRow;
		snap.headColumn = s.headColumn;
		snap.anchorRow = s.anchorRow;
		snap.anchorColumn = s.anchorColumn;
		snap.preferredColumn = s.preferredColumn;
		out.push_back(snap);
	}
	return out;
}

void EditorCommands::restoreSelections(const std::vector<SelectionSnapshot> &snaps,
									   int primary)
{
	if (!view)
		return;
	std::vector<Selection> next;
	next.reserve(snaps.size());
	for (const SelectionSnapshot &snap : snaps)
	{
		Selection s;
		s.headRow = snap.headRow;
		s.headColumn = snap.headColumn;
		s.anchorRow = snap.anchorRow;
		s.anchorColumn = snap.anchorColumn;
		s.preferredColumn = snap.preferredColumn;
		next.push_back(s);
	}
	if (next.empty())
		next.push_back(Selection{});
	view->setSelections(std::move(next), primary);
}

void EditorCommands::captureSelectionsBeforeIfNeeded()
{
	if (undoHasBefore)
		return;
	undoSelectionsBefore = snapshotSelections();
	undoPrimaryBefore = view ? view->primaryIndex : 0;
	undoHasBefore = true;
}

void EditorCommands::commitUndoGroup()
{
	if (!ready() || !undoHasBefore || undoSteps.empty())
	{
		undoSteps.clear();
		undoSelectionsBefore.clear();
		undoHasBefore = false;
		return;
	}

	HistoryEdit edit;
	edit.steps = std::move(undoSteps);
	edit.selectionsBefore = std::move(undoSelectionsBefore);
	edit.primaryBefore = undoPrimaryBefore;
	edit.selectionsAfter = snapshotSelections();
	edit.primaryAfter = view->primaryIndex;
	projectUndo->record(state->path, std::move(edit));

	undoSteps.clear();
	undoSelectionsBefore.clear();
	undoHasBefore = false;
}

bool EditorCommands::applyUserEdit(const TextOp &op)
{
	return applyUserEdit(op, ApplyOptions{});
}

bool EditorCommands::applyUserEdit(const TextOp &op, ApplyOptions opt)
{
	if (!ready())
		return false;

	captureSelectionsBeforeIfNeeded();

	const ApplyResult result = ops->apply(op);
	if (!result.ok)
		return false;

	view->ensureSelections();
	const int selIdx = opt.selectionIndex >= 0
						   ? std::clamp(opt.selectionIndex, 0, view->selectionCount() - 1)
						   : view->primaryIndex;

	// Shift other carets first (pre-edit coordinates on their heads), then update
	// the edited selection. Reverse apply order + this keeps multi inserts correct.
	adjustSelectionsAfterEdit(selIdx, op, result);

	Selection &sel = view->selections[static_cast<size_t>(selIdx)];

	switch (opt.caret)
	{
	case CaretAfter::toResult:
		sel.headRow = result.endRow;
		sel.headColumn = result.endColumn;
		break;
	case CaretAfter::leave:
		break;
	case CaretAfter::atPosition:
		sel.headRow = opt.explicitRow;
		sel.headColumn = opt.explicitColumn;
		break;
	}

	if (opt.collapseSelection)
		sel.collapseToHead();

	if (selIdx == view->primaryIndex)
		view->syncPrimaryMirrors();

	undoSteps.push_back({op, result.deletedText});

	const int lo = std::min(op.row, result.endRow);
	const int hi = std::max(op.row, result.endRow);
	if (editDirtyHi < editDirtyLo)
	{
		editDirtyLo = lo;
		editDirtyHi = hi;
	} else
	{
		editDirtyLo = std::min(editDirtyLo, lo);
		editDirtyHi = std::max(editDirtyHi, hi);
	}
	finishUserEdit();
	return true;
}

void EditorCommands::beginBatch() { ++editBatchDepth; }

void EditorCommands::endBatch()
{
	if (editBatchDepth > 0)
		--editBatchDepth;
	if (editBatchDepth == 0 && editBatchPending)
	{
		editBatchPending = false;
		finishUserEdit();
	}
}

void EditorCommands::finishUserEdit()
{
	if (!ready())
		return;
	if (editBatchDepth > 0)
	{
		editBatchPending = true;
		return;
	}

	commitUndoGroup();

	const bool hasDirty = editDirtyHi >= editDirtyLo;
	const int lo = hasDirty ? editDirtyLo : 0;
	const int hi = hasDirty ? editDirtyHi : std::max(0, state->lineCount() - 1);
	editDirtyHi = -1;
	editDirtyLo = 0;
	// The changes list is COPIED into the payload before emit: subscribers
	// (highlight, registered first) move-consume ops->takePending(), which
	// would leave nothing for later readers. Don't reorder.
	events->emitDidEdit({state->version, lo, hi, documentChangesFromPending(ops)});
	view->ensureCursorVisible.vertical = true;
	view->ensureCursorVisible.horizontal = true;
}

std::vector<int> EditorCommands::selectionOrderReverse() const
{
	std::vector<int> order;
	if (!view)
		return order;
	const int n = view->selectionCount();
	order.resize(static_cast<size_t>(n));
	for (int i = 0; i < n; ++i)
		order[static_cast<size_t>(i)] = i;
	std::sort(order.begin(), order.end(), [&](int a, int b) {
		const Selection &sa = view->selections[static_cast<size_t>(a)];
		const Selection &sb = view->selections[static_cast<size_t>(b)];
		if (sa.headRow != sb.headRow)
			return sa.headRow > sb.headRow;
		return sa.headColumn > sb.headColumn;
	});
	return order;
}

static bool posAfter(int row, int col, int atRow, int atCol)
{
	return row > atRow || (row == atRow && col >= atCol);
}

static void shiftPosAfterInsert(int &row,
								int &col,
								int atRow,
								int atCol,
								const std::string &text,
								const std::string &lineEnding)
{
	if (!posAfter(row, col, atRow, atCol))
		return;

	int newlines = 0;
	size_t lastBreak = std::string::npos;
	const size_t le = lineEnding.size();
	if (le > 0)
	{
		for (size_t i = 0; i + le <= text.size();)
		{
			if (text.compare(i, le, lineEnding) == 0)
			{
				++newlines;
				lastBreak = i;
				i += le;
			} else
			{
				++i;
			}
		}
	}
	if (newlines == 0)
	{
		if (row == atRow)
			col += static_cast<int>(text.size());
		return;
	}
	const int lastLineLen = static_cast<int>(text.size() - (lastBreak + le));
	if (row == atRow)
	{
		col = (col - atCol) + lastLineLen;
		row = atRow + newlines;
	} else
	{
		row += newlines;
	}
}

static void shiftPosAfterDelete(int &row,
								int &col,
								int atRow,
								int atCol,
								const std::string &deleted,
								const std::string &lineEnding)
{
	// End of deleted span in pre-delete coordinates.
	int endRow = atRow;
	int endCol = atCol;
	const size_t le = lineEnding.size();
	for (size_t i = 0; i < deleted.size();)
	{
		if (le > 0 && i + le <= deleted.size() && deleted.compare(i, le, lineEnding) == 0)
		{
			++endRow;
			endCol = 0;
			i += le;
		} else
		{
			++endCol;
			++i;
		}
	}

	// Strictly before delete start — unchanged.
	if (row < atRow || (row == atRow && col < atCol))
		return;

	// Inside [start, end) — clamp to start.
	if (row < endRow || (row == endRow && col < endCol))
	{
		row = atRow;
		col = atCol;
		return;
	}

	// At/after end — pull back by the deleted span.
	if (endRow == atRow)
	{
		if (row == atRow)
			col -= (endCol - atCol);
		return;
	}
	if (row == endRow)
	{
		row = atRow;
		col = atCol + (col - endCol);
	} else
	{
		row -= (endRow - atRow);
	}
}

void EditorCommands::adjustSelectionsAfterEdit(int editedIndex,
											   const TextOp &op,
											   const ApplyResult &result)
{
	if (!view || !state)
		return;
	const std::string &le = state->lineEnding;
	for (int i = 0; i < view->selectionCount(); ++i)
	{
		if (i == editedIndex)
			continue;
		Selection &s = view->selections[static_cast<size_t>(i)];
		if (op.kind == OpKind::Insert)
		{
			shiftPosAfterInsert(s.headRow, s.headColumn, op.row, op.column, op.text, le);
			shiftPosAfterInsert(
				s.anchorRow, s.anchorColumn, op.row, op.column, op.text, le);
		} else
		{
			const std::string &del =
				result.deletedText.empty() ? std::string() : result.deletedText;
			// Prefer deletedText from result; length-only deletes still shift via it.
			shiftPosAfterDelete(s.headRow, s.headColumn, op.row, op.column, del, le);
			shiftPosAfterDelete(s.anchorRow, s.anchorColumn, op.row, op.column, del, le);
		}
	}
}

// ---------------------------------------------------------------------------
// Selection gesture helpers
// ---------------------------------------------------------------------------

void EditorCommands::beginSelectGesture(bool select)
{
	if (!ready())
		return;
	if (!select)
	{
		// Drop any range; caret is the head. Movement only updates head, so
		// endSelectGesture must collapse again after the move.
		for (Selection &s : view->selections)
			s.collapseToHead();
	}
	// Selecting: leave anchors; empty carets already have anchor == head.
}

void EditorCommands::endSelectGesture(bool select)
{
	if (!ready())
		return;
	// cursorLeft/Up/etc. only move head. Without shift, snap anchor to head
	// so we do not paint a range from the previous position.
	if (!select)
	{
		for (Selection &s : view->selections)
			s.collapseToHead();
	}
	view->mergeSelections();
	view->clampAll();
	requestEnsureVisible();
}

void EditorCommands::requestEnsureVisible()
{
	if (!view)
		return;
	view->ensureCursorVisible.vertical = true;
	view->ensureCursorVisible.horizontal = true;
}

void EditorCommands::applyReveal(CursorReveal reveal)
{
	if (!view)
		return;
	if (reveal == CursorReveal::center)
	{
		view->centerCursorVertical = true;
		view->ensureCursorVisible.horizontal = true;
		return;
	}
	requestEnsureVisible();
}

// ---------------------------------------------------------------------------
// Navigation
// ---------------------------------------------------------------------------

void EditorCommands::navMove(bool select, void (EditorViewState::*fn)(Selection &))
{
	if (!ready())
		return;
	beginSelectGesture(select);
	for (Selection &s : view->selections)
		(view->*fn)(s);
	endSelectGesture(select);
}

void EditorCommands::moveLeft(bool select)
{
	navMove(select, &EditorViewState::cursorLeft);
}

void EditorCommands::moveRight(bool select)
{
	navMove(select, &EditorViewState::cursorRight);
}

void EditorCommands::moveUp(bool select) { navMove(select, &EditorViewState::cursorUp); }

void EditorCommands::moveDown(bool select)
{
	navMove(select, &EditorViewState::cursorDown);
}

void EditorCommands::moveWordLeft(bool select)
{
	navMove(select, &EditorViewState::moveWordBackward);
}

void EditorCommands::moveWordRight(bool select)
{
	navMove(select, &EditorViewState::moveWordForward);
}

void EditorCommands::moveLineStart(bool select)
{
	if (!ready())
		return;
	beginSelectGesture(select);
	for (Selection &s : view->selections)
	{
		const std::string line = state->line(s.headRow);
		int indent = 0;
		while (indent < static_cast<int>(line.size()) &&
			   (line[indent] == ' ' || line[indent] == '\t'))
			++indent;
		s.headColumn = (s.headColumn > indent) ? indent : 0;
		s.preferredColumn = s.headColumn;
	}
	endSelectGesture(select);
}

void EditorCommands::moveLineEnd(bool select)
{
	if (!ready())
		return;
	beginSelectGesture(select);
	for (Selection &s : view->selections)
	{
		s.headColumn = state->lineLength(s.headRow);
		s.preferredColumn = s.headColumn;
	}
	endSelectGesture(select);
}

void EditorCommands::moveDocStart(bool select)
{
	if (!ready())
		return;
	beginSelectGesture(select);
	// Multi: all carets collapse to doc start → merge to one.
	for (Selection &s : view->selections)
	{
		s.headRow = 0;
		s.headColumn = 0;
		s.preferredColumn = 0;
	}
	endSelectGesture(select);
}

void EditorCommands::moveDocEnd(bool select)
{
	if (!ready())
		return;
	beginSelectGesture(select);
	const int last = state->lineCount() - 1;
	const int col = state->lineLength(last);
	for (Selection &s : view->selections)
	{
		s.headRow = last;
		s.headColumn = col;
		view->calculateVisualColumn(s);
	}
	endSelectGesture(select);
}

void EditorCommands::moveLines(int delta, bool select)
{
	if (!ready() || delta == 0)
		return;
	beginSelectGesture(select);
	const int steps = delta > 0 ? delta : -delta;
	for (int i = 0; i < steps; ++i)
	{
		for (Selection &s : view->selections)
		{
			if (delta > 0)
				view->cursorDown(s);
			else
				view->cursorUp(s);
		}
	}
	endSelectGesture(select);
}

void EditorCommands::setCursor(int row, int column, bool select, CursorReveal reveal)
{
	if (!ready())
		return;
	if (state->lineCount() <= 0)
		return;
	row = std::clamp(row, 0, state->lineCount() - 1);
	const std::string rowLine = state->line(row);
	column = std::clamp(column, 0, static_cast<int>(rowLine.size()));
	column = EditorUtils::SnapToUtf8CharBoundary(rowLine, column);

	if (select)
	{
		// Extend primary only; drop secondaries (mouse / single-range APIs).
		Selection p = view->primary();
		if (p.empty())
		{
			p.anchorRow = p.headRow;
			p.anchorColumn = p.headColumn;
		}
		p.headRow = row;
		p.headColumn = column;
		view->selections.clear();
		view->selections.push_back(p);
		view->primaryIndex = 0;
		view->calculateVisualColumn(view->primary());
		view->syncPrimaryMirrors();
	} else
	{
		Selection s;
		s.setBoth(row, column);
		view->selections.clear();
		view->selections.push_back(s);
		view->primaryIndex = 0;
		view->calculateVisualColumn(view->primary());
		view->syncPrimaryMirrors();
	}
	applyReveal(reveal);
}

void EditorCommands::setSelection(
	int anchorRow, int anchorCol, int activeRow, int activeCol, CursorReveal reveal)
{
	if (!ready())
		return;
	if (state->lineCount() <= 0)
		return;

	anchorRow = std::clamp(anchorRow, 0, state->lineCount() - 1);
	activeRow = std::clamp(activeRow, 0, state->lineCount() - 1);
	anchorCol = std::clamp(anchorCol, 0, state->lineLength(anchorRow));
	activeCol = std::clamp(activeCol, 0, state->lineLength(activeRow));
	anchorCol = EditorUtils::SnapToUtf8CharBoundary(state->line(anchorRow), anchorCol);
	activeCol = EditorUtils::SnapToUtf8CharBoundary(state->line(activeRow), activeCol);

	Selection s;
	s.anchorRow = anchorRow;
	s.anchorColumn = anchorCol;
	s.headRow = activeRow;
	s.headColumn = activeCol;
	view->selections.clear();
	view->selections.push_back(s);
	view->primaryIndex = 0;
	view->calculateVisualColumn(view->primary());
	view->syncPrimaryMirrors();
	applyReveal(reveal);
}

void EditorCommands::setSelections(std::vector<Selection> selections,
								   int primaryIndex,
								   CursorReveal reveal)
{
	if (!ready())
		return;
	view->setSelections(std::move(selections), primaryIndex);
	for (Selection &s : view->selections)
		view->calculateVisualColumn(s);
	view->syncPrimaryMirrors();
	applyReveal(reveal);
}

void EditorCommands::goToLine(int line)
{
	setCursor(line, 0, false, CursorReveal::center);
}

void EditorCommands::selectAll()
{
	if (!ready())
		return;
	view->selectAll();
	requestEnsureVisible();
}

void EditorCommands::collapseSelection()
{
	if (!ready())
		return;
	view->collapseSelection();
}

void EditorCommands::addCursorVertical(int lineDelta)
{
	if (!ready() || state->lineCount() <= 0 || lineDelta == 0)
		return;

	Selection seed = view->primary();
	if (seed.preferredColumn == 0 && seed.headColumn != 0)
		view->calculateVisualColumn(seed);

	// New caret is collapsed at the moved head (vertical move only updates head).
	Selection neu;
	neu.headRow = seed.headRow;
	neu.headColumn = seed.headColumn;
	neu.preferredColumn = seed.preferredColumn;
	neu.collapseToHead();
	if (lineDelta < 0)
		view->cursorUp(neu);
	else
		view->cursorDown(neu);
	neu.collapseToHead();

	if (neu.headRow == seed.headRow)
		return; // document edge

	for (const Selection &s : view->selections)
	{
		if (s.headRow == neu.headRow && s.headColumn == neu.headColumn)
			return;
	}

	view->selections.push_back(neu);
	view->primaryIndex = static_cast<int>(view->selections.size()) - 1;
	view->mergeSelections();
	for (size_t i = 0; i < view->selections.size(); ++i)
	{
		if (view->selections[i].headRow == neu.headRow &&
			view->selections[i].headColumn == neu.headColumn)
		{
			view->primaryIndex = static_cast<int>(i);
			break;
		}
	}
	view->syncPrimaryMirrors();
	requestEnsureVisible();
}

void EditorCommands::addCursorAbove() { addCursorVertical(-1); }

void EditorCommands::addCursorBelow() { addCursorVertical(+1); }

void EditorCommands::selectWordAt(int row, int column)
{
	if (!ready())
		return;
	row = std::clamp(row, 0, state->lineCount() - 1);
	const std::string line = state->line(row);
	column = EditorUtils::SnapToUtf8CharBoundary(line, column);
	int start = 0, end = 0;
	EditorUtils::FindWordBoundaries(line, column, start, end);
	setSelection(row, start, row, end);
}

// ---------------------------------------------------------------------------
// Text edits
// ---------------------------------------------------------------------------

void EditorCommands::deleteSelection()
{
	if (!ready() || !view->hasSelection())
		return;

	beginBatch();
	const std::vector<int> order = selectionOrderReverse();
	for (int idx : order)
	{
		Selection &sel = view->selections[static_cast<size_t>(idx)];
		if (sel.empty())
			continue;
		int sr, sc, er, ec;
		sel.getOrdered(sr, sc, er, ec);
		TextOp op;
		op.kind = OpKind::Delete;
		op.row = sr;
		op.column = sc;
		op.length = ops->measureLength(sr, sc, er, ec);
		if (op.length <= 0)
			continue;
		ApplyOptions opt;
		opt.selectionIndex = idx;
		opt.collapseSelection = true;
		opt.caret = CaretAfter::toResult;
		applyUserEdit(op, opt);
	}
	view->mergeSelections();
	view->syncPrimaryMirrors();
	endBatch();
	for (Selection &s : view->selections)
		s.preferredColumn = 0;
	view->syncPrimaryMirrors();
}

void EditorCommands::typeText(const std::string &utf8)
{
	if (!ready() || utf8.empty())
		return;

	beginBatch();
	const std::vector<int> order = selectionOrderReverse();
	for (int idx : order)
	{
		Selection &sel = view->selections[static_cast<size_t>(idx)];
		if (!sel.empty())
		{
			int sr, sc, er, ec;
			sel.getOrdered(sr, sc, er, ec);
			TextOp del;
			del.kind = OpKind::Delete;
			del.row = sr;
			del.column = sc;
			del.length = ops->measureLength(sr, sc, er, ec);
			if (del.length > 0)
			{
				ApplyOptions opt;
				opt.selectionIndex = idx;
				opt.collapseSelection = true;
				applyUserEdit(del, opt);
			}
		}
		// Re-read after possible delete (head moved).
		Selection &after = view->selections[static_cast<size_t>(idx)];
		TextOp op;
		op.kind = OpKind::Insert;
		op.row = after.headRow;
		op.column = after.headColumn;
		op.text = utf8;
		ApplyOptions opt;
		opt.selectionIndex = idx;
		opt.collapseSelection = true;
		applyUserEdit(op, opt);
	}
	view->mergeSelections();
	view->syncPrimaryMirrors();
	endBatch();
}

static std::string leadingIndent(const std::string &line, int upToColumn)
{
	const int n = std::min(upToColumn, static_cast<int>(line.size()));
	int i = 0;
	while (i < n && (line[i] == ' ' || line[i] == '\t'))
		++i;
	return line.substr(0, static_cast<size_t>(std::min(i, n)));
}

void EditorCommands::insertNewline()
{
	if (!ready())
		return;

	beginBatch();
	const std::vector<int> order = selectionOrderReverse();
	for (int idx : order)
	{
		Selection &sel = view->selections[static_cast<size_t>(idx)];
		if (!sel.empty())
		{
			int sr, sc, er, ec;
			sel.getOrdered(sr, sc, er, ec);
			TextOp del;
			del.kind = OpKind::Delete;
			del.row = sr;
			del.column = sc;
			del.length = ops->measureLength(sr, sc, er, ec);
			if (del.length > 0)
			{
				ApplyOptions opt;
				opt.selectionIndex = idx;
				opt.collapseSelection = true;
				applyUserEdit(del, opt);
			}
		}
		Selection &after = view->selections[static_cast<size_t>(idx)];
		const std::string indent =
			leadingIndent(state->line(after.headRow), after.headColumn);
		TextOp op;
		op.kind = OpKind::Insert;
		op.row = after.headRow;
		op.column = after.headColumn;
		op.text = state->lineEnding + indent;
		ApplyOptions opt;
		opt.selectionIndex = idx;
		opt.collapseSelection = true;
		applyUserEdit(op, opt);
		view->selections[static_cast<size_t>(idx)].preferredColumn = 0;
	}
	view->mergeSelections();
	view->syncPrimaryMirrors();
	endBatch();
}

void EditorCommands::deleteLeft(bool byWord)
{
	if (!ready())
		return;
	if (view->hasSelection())
	{
		deleteSelection();
		return;
	}

	beginBatch();
	const std::vector<int> order = selectionOrderReverse();
	for (int idx : order)
	{
		Selection &sel = view->selections[static_cast<size_t>(idx)];

		if (byWord)
		{
			const int endRow = sel.headRow;
			const int endCol = sel.headColumn;
			view->moveWordBackward(sel);
			const int startRow = sel.headRow;
			const int startCol = sel.headColumn;
			sel.headRow = endRow;
			sel.headColumn = endCol;
			TextOp op;
			op.kind = OpKind::Delete;
			op.row = startRow;
			op.column = startCol;
			op.length = ops->measureLength(startRow, startCol, endRow, endCol);
			if (op.length > 0)
			{
				ApplyOptions opt;
				opt.selectionIndex = idx;
				applyUserEdit(op, opt);
			}
			view->selections[static_cast<size_t>(idx)].preferredColumn = 0;
			continue;
		}

		if (sel.headColumn > 0)
		{
			std::string line = state->line(sel.headRow);
			int pos = EditorUtils::SnapToUtf8CharBoundary(line, sel.headColumn);
			auto it = line.begin() + pos;
			auto prev = it;
			EditorUtils::MoveToPrevUtf8Char(prev);
			const int deleteStart = static_cast<int>(std::distance(line.begin(), prev));
			TextOp op;
			op.kind = OpKind::Delete;
			op.row = sel.headRow;
			op.column = deleteStart;
			op.length = pos - deleteStart;
			ApplyOptions opt;
			opt.selectionIndex = idx;
			applyUserEdit(op, opt);
		} else if (sel.headRow > 0)
		{
			const int prevRow = sel.headRow - 1;
			const int prevCol = state->lineLength(prevRow);
			TextOp op;
			op.kind = OpKind::Delete;
			op.row = prevRow;
			op.column = prevCol;
			op.length = static_cast<int>(state->lineEnding.size());
			ApplyOptions opt;
			opt.selectionIndex = idx;
			applyUserEdit(op, opt);
		}
		view->selections[static_cast<size_t>(idx)].preferredColumn = 0;
	}
	view->mergeSelections();
	view->syncPrimaryMirrors();
	endBatch();
}

void EditorCommands::deleteRight(bool byWord)
{
	if (!ready())
		return;
	if (view->hasSelection())
	{
		deleteSelection();
		return;
	}

	beginBatch();
	const std::vector<int> order = selectionOrderReverse();
	for (int idx : order)
	{
		Selection &sel = view->selections[static_cast<size_t>(idx)];

		if (byWord)
		{
			const int startRow = sel.headRow;
			const int startCol = sel.headColumn;
			view->moveWordForward(sel);
			const int endRow = sel.headRow;
			const int endCol = sel.headColumn;
			sel.headRow = startRow;
			sel.headColumn = startCol;
			TextOp op;
			op.kind = OpKind::Delete;
			op.row = startRow;
			op.column = startCol;
			op.length = ops->measureLength(startRow, startCol, endRow, endCol);
			if (op.length > 0)
			{
				ApplyOptions opt;
				opt.selectionIndex = idx;
				applyUserEdit(op, opt);
			}
			view->selections[static_cast<size_t>(idx)].preferredColumn = 0;
			continue;
		}

		std::string line = state->line(sel.headRow);
		if (sel.headColumn < static_cast<int>(line.size()))
		{
			int pos = EditorUtils::SnapToUtf8CharBoundary(line, sel.headColumn);
			auto it = line.begin() + pos;
			EditorUtils::MoveToNextUtf8Char(it);
			const int deleteEnd = static_cast<int>(std::distance(line.begin(), it));
			TextOp op;
			op.kind = OpKind::Delete;
			op.row = sel.headRow;
			op.column = pos;
			op.length = deleteEnd - pos;
			ApplyOptions opt;
			opt.selectionIndex = idx;
			applyUserEdit(op, opt);
		} else if (sel.headRow + 1 < state->lineCount())
		{
			TextOp op;
			op.kind = OpKind::Delete;
			op.row = sel.headRow;
			op.column = sel.headColumn;
			op.length = static_cast<int>(state->lineEnding.size());
			ApplyOptions opt;
			opt.selectionIndex = idx;
			applyUserEdit(op, opt);
		}
		view->selections[static_cast<size_t>(idx)].preferredColumn = 0;
	}
	view->mergeSelections();
	view->syncPrimaryMirrors();
	endBatch();
}

// ---------------------------------------------------------------------------
// Clipboard
// ---------------------------------------------------------------------------

bool EditorCommands::documentUsesTabs() const
{
	return state && state->containsByte('\t');
}

std::string EditorCommands::selectionAsClipboardText() const
{
	if (view->selectionEmpty())
		return {};

	int sr, sc, er, ec;
	view->getOrdered(sr, sc, er, ec);

	std::string selected = ops->extractText(sr, sc, er, ec);
	const std::string &docLe = state->lineEnding;
	const std::string platLe = EditorState::platformLineEnding();
	if (docLe == platLe || docLe.empty())
		return selected;

	std::string out;
	for (size_t i = 0; i < selected.size();)
	{
		if (selected.compare(i, docLe.size(), docLe) == 0)
		{
			out += platLe;
			i += docLe.size();
		} else
		{
			out += selected[i++];
		}
	}
	return out;
}

std::string EditorCommands::normalizePaste(const std::string &raw) const
{
	std::string text = raw;
	if (documentUsesTabs())
	{
		std::string result;
		result.reserve(text.size());
		for (size_t i = 0; i < text.size(); ++i)
		{
			if (text[i] == ' ')
			{
				int spaceCount = 0;
				while (i < text.size() && text[i] == ' ' && spaceCount < 4)
				{
					++spaceCount;
					++i;
				}
				if (spaceCount == 4)
					result += '\t';
				else
					result.append(static_cast<size_t>(spaceCount), ' ');
				--i;
			} else
			{
				result += text[i];
			}
		}
		text = std::move(result);
	} else
	{
		std::string result;
		result.reserve(text.size() * 4);
		for (char c : text)
		{
			if (c == '\t')
				result += "    ";
			else
				result += c;
		}
		text = std::move(result);
	}
	return ops->normalizeLineEndings(text);
}

void EditorCommands::copy()
{
	if (!ready())
		return;
	if (view->selectionEmpty())
		return;
	const std::string text = selectionAsClipboardText();
	if (!text.empty())
		ImGui::SetClipboardText(text.c_str());
}

void EditorCommands::cut()
{
	if (!ready())
		return;
	if (!view->selectionEmpty())
	{
		copy();
		deleteSelection();
		return;
	}

	// Cut whole line when no selection (primary only).
	const int r = view->row;
	const bool lastLine = (r + 1 >= state->lineCount());
	std::string lineText = state->line(r) + EditorState::platformLineEnding();
	ImGui::SetClipboardText(lineText.c_str());

	TextOp op;
	op.kind = OpKind::Delete;
	if (lastLine && r > 0)
	{
		op.row = r - 1;
		op.column = state->lineLength(r - 1);
		op.length = static_cast<int>(state->lineEnding.size()) + state->lineLength(r);
	} else if (lastLine)
	{
		op.row = 0;
		op.column = 0;
		op.length = state->lineLength(0);
	} else
	{
		op.row = r;
		op.column = 0;
		op.length = state->lineLength(r) + static_cast<int>(state->lineEnding.size());
	}
	ApplyOptions opt;
	opt.selectionIndex = view->primaryIndex;
	applyUserEdit(op, opt);
	view->collapseToPrimary();
}

void EditorCommands::paste()
{
	if (!ready())
		return;
	const char *clip = ImGui::GetClipboardText();
	if (!clip)
		return;
	std::string paste_content = normalizePaste(clip);
	if (paste_content.empty())
		return;

	beginBatch();
	const std::vector<int> order = selectionOrderReverse();
	for (int idx : order)
	{
		Selection &sel = view->selections[static_cast<size_t>(idx)];
		if (!sel.empty())
		{
			int sr, sc, er, ec;
			sel.getOrdered(sr, sc, er, ec);
			TextOp del;
			del.kind = OpKind::Delete;
			del.row = sr;
			del.column = sc;
			del.length = ops->measureLength(sr, sc, er, ec);
			if (del.length > 0)
			{
				ApplyOptions opt;
				opt.selectionIndex = idx;
				opt.collapseSelection = true;
				applyUserEdit(del, opt);
			}
		}
		Selection &after = view->selections[static_cast<size_t>(idx)];
		TextOp op;
		op.kind = OpKind::Insert;
		op.row = after.headRow;
		op.column = after.headColumn;
		op.text = paste_content;
		ApplyOptions opt;
		opt.selectionIndex = idx;
		opt.collapseSelection = true;
		applyUserEdit(op, opt);
	}
	view->mergeSelections();
	view->syncPrimaryMirrors();
	endBatch();
	requestEnsureVisible();
}

// ---------------------------------------------------------------------------
// Indent
// ---------------------------------------------------------------------------

void EditorCommands::indent()
{
	if (!ready())
		return;
	if (!view->selectionEmpty())
		indentMultiLine();
	else
		indentSingleLine();
}

void EditorCommands::indentSingleLine()
{
	// Indent at every caret (column 0 insert tab).
	beginBatch();
	const std::vector<int> order = selectionOrderReverse();
	for (int idx : order)
	{
		Selection &sel = view->selections[static_cast<size_t>(idx)];
		const int beforeCol = sel.headColumn;
		TextOp op;
		op.kind = OpKind::Insert;
		op.row = sel.headRow;
		op.column = 0;
		op.text = "\t";
		ApplyOptions opt;
		opt.caret = CaretAfter::atPosition;
		opt.explicitRow = sel.headRow;
		opt.explicitColumn = beforeCol + 1;
		opt.collapseSelection = true;
		opt.selectionIndex = idx;
		applyUserEdit(op, opt);
		view->selections[static_cast<size_t>(idx)].preferredColumn = 0;
	}
	view->mergeSelections();
	view->syncPrimaryMirrors();
	endBatch();
}

void EditorCommands::indentMultiLine()
{
	// Multi-line indent uses the primary selection range only.
	int sr, sc, er, ec;
	view->getOrdered(sr, sc, er, ec);
	if (ec == 0 && er > sr)
		--er;

	beginBatch();
	for (int r = er; r >= sr; --r)
	{
		TextOp op;
		op.kind = OpKind::Insert;
		op.row = r;
		op.column = 0;
		op.text = "\t";
		ApplyOptions opt;
		opt.caret = CaretAfter::leave;
		opt.collapseSelection = false;
		opt.selectionIndex = view->primaryIndex;
		applyUserEdit(op, opt);
	}
	endBatch();

	Selection &p = view->primary();
	if (p.anchorRow >= sr && p.anchorRow <= er)
		++p.anchorColumn;
	if (p.headRow >= sr && p.headRow <= er)
		++p.headColumn;
	view->syncPrimaryMirrors();
}

void EditorCommands::outdent()
{
	if (!ready())
		return;
	outdentRange();
}

void EditorCommands::outdentRange()
{
	int sr, sc, er, ec;
	if (!view->selectionEmpty())
		view->getOrdered(sr, sc, er, ec);
	else
	{
		sr = er = view->row;
		sc = ec = view->column;
	}
	if (ec == 0 && er > sr)
		--er;

	std::vector<int> removed(static_cast<size_t>(er - sr + 1), 0);

	beginBatch();
	for (int r = er; r >= sr; --r)
	{
		const std::string line = state->line(r);
		int n = 0;
		if (line.size() >= 4 && line.compare(0, 4, "    ") == 0)
			n = 4;
		else if (!line.empty() && line[0] == '\t')
			n = 1;
		if (n <= 0)
			continue;

		TextOp op;
		op.kind = OpKind::Delete;
		op.row = r;
		op.column = 0;
		op.length = n;
		ApplyOptions opt;
		opt.caret = CaretAfter::leave;
		opt.collapseSelection = false;
		opt.selectionIndex = view->primaryIndex;
		if (applyUserEdit(op, opt))
			removed[static_cast<size_t>(r - sr)] = n;
	}
	endBatch();

	auto adjust = [&](int row, int &col) {
		if (row < sr || row > er)
			return;
		col = std::max(0, col - removed[static_cast<size_t>(row - sr)]);
		col = std::min(col, state->lineLength(row));
	};

	Selection &p = view->primary();
	if (p.empty())
	{
		adjust(p.headRow, p.headColumn);
		p.collapseToHead();
	} else
	{
		adjust(p.anchorRow, p.anchorColumn);
		adjust(p.headRow, p.headColumn);
	}
	view->syncPrimaryMirrors();
}

// ---------------------------------------------------------------------------
// History / file
// ---------------------------------------------------------------------------

void EditorCommands::applyHistory(const HistoryEdit &edit, bool isUndo)
{
	if (!ops || !state || !view)
		return;

	int dirtyLo = 0;
	int dirtyHi = -1;
	auto noteDirty = [&](int a, int b) {
		const int lo = std::min(a, b);
		const int hi = std::max(a, b);
		if (dirtyHi < dirtyLo)
		{
			dirtyLo = lo;
			dirtyHi = hi;
		} else
		{
			dirtyLo = std::min(dirtyLo, lo);
			dirtyHi = std::max(dirtyHi, hi);
		}
	};

	bool anyOk = false;
	if (isUndo)
	{
		for (auto it = edit.steps.rbegin(); it != edit.steps.rend(); ++it)
		{
			const TextOp applied = EditorOperations::invert(it->op, it->deletedText);
			const ApplyResult result = ops->apply(applied);
			if (result.ok)
			{
				anyOk = true;
				noteDirty(applied.row, result.endRow);
			}
		}
		restoreSelections(edit.selectionsBefore, edit.primaryBefore);
	} else
	{
		for (const HistoryStep &step : edit.steps)
		{
			const ApplyResult result = ops->apply(step.op);
			if (result.ok)
			{
				anyOk = true;
				noteDirty(step.op.row, result.endRow);
			}
		}
		restoreSelections(edit.selectionsAfter, edit.primaryAfter);
	}

	view->clampAll();
	if (events && anyOk)
	{
		if (dirtyHi < dirtyLo)
		{
			dirtyLo = 0;
			dirtyHi = std::max(0, state->lineCount() - 1);
		}
		events->emitDidEdit(
			{state->version, dirtyLo, dirtyHi, documentChangesFromPending(ops)});
	}
	view->ensureCursorVisible.vertical = true;
	view->ensureCursorVisible.horizontal = true;
}

void EditorCommands::undo()
{
	if (!ready())
		return;
	// Seal any in-flight group before popping.
	if (undoHasBefore && !undoSteps.empty())
		commitUndoGroup();
	HistoryEdit edit;
	if (!projectUndo->undo(state->path, edit))
		return;
	applyHistory(edit, true);
	requestEnsureVisible();
}

void EditorCommands::redo()
{
	if (!ready())
		return;
	if (undoHasBefore && !undoSteps.empty())
		commitUndoGroup();
	HistoryEdit edit;
	if (!projectUndo->redo(state->path, edit))
		return;
	applyHistory(edit, false);
	requestEnsureVisible();
}

void EditorCommands::save()
{
	if (!ready() || !saveService)
		return;
	saveService->save();
}
