/*
	1-based line/column text-model facade over EditorState + EditorOperations.

	Coordinate system (ASCII / BMP):
	  - Positions and ranges are 1-based (line, column).
	  - Column is a UTF-8 byte offset within the line for this port.
	  - (Some host APIs use UTF-16 columns; for BMP/ASCII they match.)

	Used by the model unit-test suites only.
*/
#pragma once

#include "editor/editor_operations.h"
#include "editor/editor_state.h"

#include <string>
#include <utility>
#include <vector>

namespace monaco {

struct Position
{
	int lineNumber = 1;
	int column = 1;

	bool operator==(const Position &o) const
	{
		return lineNumber == o.lineNumber && column == o.column;
	}
};

struct Range
{
	int startLineNumber = 1;
	int startColumn = 1;
	int endLineNumber = 1;
	int endColumn = 1;

	Range() = default;
	Range(int sl, int sc, int el, int ec)
		: startLineNumber(sl), startColumn(sc), endLineNumber(el), endColumn(ec)
	{
	}

	bool isEmpty() const
	{
		return startLineNumber == endLineNumber && startColumn == endColumn;
	}

	bool operator==(const Range &o) const
	{
		return startLineNumber == o.startLineNumber && startColumn == o.startColumn &&
			   endLineNumber == o.endLineNumber && endColumn == o.endColumn;
	}
};

// End-of-line preference for getValue helpers
enum class EndOfLinePreference { TextDefined, LF, CRLF };

struct SingleEditOperation
{
	Range range;
	std::string text; // may contain \n; normalized to model EOL on apply
};

// Thin document model — no UI, no ImGui.
class TextModel
{
  public:
	// Create from full buffer text (EOL detection like EditorState::setFromString).
	static TextModel create(const std::string &text);
	// Create from lines joined with LF (test helper).
	static TextModel createFromLines(const std::vector<std::string> &lines);

	std::string getValue() const;
	std::string getEOL() const { return state_.lineEnding; }
	void setEOL(const std::string &eol);

	int getLineCount() const;
	std::string getLineContent(int lineNumber) const; // 1-based
	std::vector<std::string> getLinesContent() const;

	std::string
	getValueInRange(const Range &range,
					EndOfLinePreference eol = EndOfLinePreference::TextDefined) const;
	int getValueLengthInRange(
		const Range &range,
		EndOfLinePreference eol = EndOfLinePreference::TextDefined) const;

	Position validatePosition(Position p) const;
	Range validateRange(Range r) const;
	// Move by offset in joined-document byte space (TextDefined EOL).
	Position modifyPosition(Position p, int offset) const;

	int getLineFirstNonWhitespaceColumn(int lineNumber) const;
	int getLineLastNonWhitespaceColumn(int lineNumber) const;

	void setValue(const std::string &text);

	// Apply edits (specified against the *current* model, bottom-up for multi).
	// If computeUndoEdits, returns inverse ops that restore the previous value
	// (applyEdits with computeUndoEdits = true).
	std::vector<SingleEditOperation>
	applyEdits(const std::vector<SingleEditOperation> &operations,
			   bool computeUndoEdits = false);

	// Direct access for assertions that need raw ned state.
	const EditorState &state() const { return state_; }
	EditorState &state() { return state_; }

  private:
	EditorState state_;
	EditorOperations ops_;

	TextModel();
	TextModel(const TextModel &) = delete;
	TextModel &operator=(const TextModel &) = delete;
	TextModel(TextModel &&other) noexcept;
	TextModel &operator=(TextModel &&other) noexcept;

	static Range order(Range r);
	void clampRange(Range &r) const;

	// 0-based ned row/col
	void toNed(const Position &p, int &row, int &col) const;
	Position fromNed(int row, int col) const;

	// Apply one replace; returns inverse op (range of inserted text + deleted text).
	SingleEditOperation applyOne(const SingleEditOperation &op);
};

// Test helpers
inline SingleEditOperation editOp(int startLine,
								  int startCol,
								  int endLine,
								  int endCol,
								  const std::vector<std::string> &textLines)
{
	SingleEditOperation op;
	op.range = Range(startLine, startCol, endLine, endCol);
	if (textLines.empty())
	{
		op.text.clear();
	} else
	{
		op.text = textLines[0];
		for (size_t i = 1; i < textLines.size(); ++i)
		{
			op.text += '\n';
			op.text += textLines[i];
		}
	}
	return op;
}

inline SingleEditOperation createSingleEditOp(const std::string &text,
											  int positionLineNumber,
											  int positionColumn,
											  int selectionLineNumber = -1,
											  int selectionColumn = -1)
{
	if (selectionLineNumber < 0)
		selectionLineNumber = positionLineNumber;
	if (selectionColumn < 0)
		selectionColumn = positionColumn;
	// Build a single edit covering a selection → insert position.
	SingleEditOperation op;
	op.range =
		Range(selectionLineNumber, selectionColumn, positionLineNumber, positionColumn);
	op.text = text;
	return op;
}

} // namespace monaco
