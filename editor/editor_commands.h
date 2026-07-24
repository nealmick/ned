/*
	File: editor_commands.h
	Description: User-facing editor actions. Input maps keys/mouse → these methods.
	User text mutations: ops.apply + selections + projectUndo.record + did-edit.
	Navigation updates view state only. Multi-caret edits apply reverse document order.
*/

#pragma once

#include "editor_operations.h"
#include "editor_view_state.h"
#include "util/project_undo.h"
#include <string>
#include <vector>

class EditorState;
class EditorViewState;
class EditorOperations;
class EditorEvents;
class EditorSave;

class EditorCommands
{
  public:
	// How the viewport follows the caret after navigation.
	// ensure = keep caret in view; center = vertical center (goto-line / find).
	enum class CursorReveal { ensure, center };

	EditorCommands(EditorState &document,
				   EditorViewState &viewState,
				   EditorOperations &operations,
				   ProjectUndo &undo,
				   EditorEvents &ev,
				   EditorSave &saveService)
		: state(&document),
		  view(&viewState),
		  ops(&operations),
		  projectUndo(&undo),
		  events(&ev),
		  saveService(&saveService)
	{
	}

	// Navigation / selection (no did-edit)
	void moveLeft(bool select);
	void moveRight(bool select);
	void moveUp(bool select);
	void moveDown(bool select);
	void moveWordLeft(bool select);
	void moveWordRight(bool select);
	void moveLineStart(bool select);
	void moveLineEnd(bool select);
	void moveDocStart(bool select);
	void moveDocEnd(bool select);
	void moveLines(int delta, bool select); // Ctrl/Cmd+Up/Down style jump

	void setCursor(int row,
				   int column,
				   bool select = false,
				   CursorReveal reveal = CursorReveal::ensure);
	void setSelection(int anchorRow,
					  int anchorCol,
					  int activeRow,
					  int activeCol,
					  CursorReveal reveal = CursorReveal::ensure);
	// Replace the selection set (clamps + merge). Used by find-all multi spawn.
	void setSelections(std::vector<Selection> selections,
					   int primaryIndex = 0,
					   CursorReveal reveal = CursorReveal::ensure);
	void goToLine(int line); // 0-based, column 0, center vertically
	void selectAll();
	void collapseSelection();
	void selectWordAt(int row, int column);
	// Clone primary one line up/down at preferred column; new caret becomes primary.
	void addCursorAbove();
	void addCursorBelow();
	void requestEnsureVisible();

	// Text edits (apply + record undo + did-edit)
	void typeText(const std::string &utf8);
	void insertNewline();
	void deleteLeft(bool byWord = false);
	void deleteRight(bool byWord = false);
	void deleteSelection();
	void copy();
	void cut();
	void paste();
	void indent();
	void outdent();

	// History / file
	void undo();
	void redo();
	void save();

  private:
	void addCursorVertical(int lineDelta);

	enum class CaretAfter { toResult, leave, atPosition };

	struct ApplyOptions
	{
		CaretAfter caret = CaretAfter::toResult;
		int explicitRow = 0;
		int explicitColumn = 0;
		bool collapseSelection = true;
		// When set, caret updates apply to this selection index instead of primary.
		int selectionIndex = -1;
	};

	bool ready() const { return state && view && ops && projectUndo && events; }

	// Apply one TextOp; accumulate undo group steps; update selection caret.
	bool applyUserEdit(const TextOp &op);
	bool applyUserEdit(const TextOp &op, ApplyOptions opt);

	void beginBatch();
	void endBatch();
	void finishUserEdit();
	void commitUndoGroup();
	void captureSelectionsBeforeIfNeeded();
	std::vector<SelectionSnapshot> snapshotSelections() const;
	void restoreSelections(const std::vector<SelectionSnapshot> &snaps, int primary);

	void applyHistory(const HistoryEdit &edit, bool isUndo);

	void beginSelectGesture(bool select);
	void endSelectGesture(bool select);
	void applyReveal(CursorReveal reveal);
	// begin/end select around a view caret move on every selection.
	void navMove(bool select, void (EditorViewState::*fn)(Selection &));

	// Indices of selections sorted by head descending (for multi-edit).
	std::vector<int> selectionOrderReverse() const;
	// After one op, shift other selection anchors/heads that sit after the edit.
	void adjustSelectionsAfterEdit(int editedIndex,
								   const TextOp &op,
								   const ApplyResult &result);

	std::string selectionAsClipboardText() const;
	std::string normalizePaste(const std::string &raw) const;
	bool documentUsesTabs() const;

	void indentMultiLine();
	void indentSingleLine();
	void outdentRange();

	EditorState *state;
	EditorViewState *view;
	EditorOperations *ops;
	ProjectUndo *projectUndo;
	EditorEvents *events;
	EditorSave *saveService;

	int editBatchDepth = 0;
	bool editBatchPending = false;
	// Batch union for DidEdit row span (valid when editDirtyHi >= editDirtyLo).
	int editDirtyLo = 0;
	int editDirtyHi = -1;

	// Undo group accumulated across applyUserEdit calls until batch ends.
	std::vector<HistoryStep> undoSteps;
	std::vector<SelectionSnapshot> undoSelectionsBefore;
	int undoPrimaryBefore = 0;
	bool undoHasBefore = false;
};