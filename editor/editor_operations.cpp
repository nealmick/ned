#include "editor_operations.h"
#include "editor_state.h"
#include "util/utf8.h"

#include <algorithm>

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

std::string EditorOperations::normalizeLineEndings(const std::string &text) const
{
	if (!state || text.empty())
		return text;

	const std::string &le = state->lineEnding;
	std::string out;
	out.reserve(text.size());

	for (size_t i = 0; i < text.size();)
	{
		if (text[i] == '\r' && i + 1 < text.size() && text[i + 1] == '\n')
		{
			out += le;
			i += 2;
		} else if (text[i] == '\n' || text[i] == '\r')
		{
			out += le;
			++i;
		} else
		{
			out += text[i];
			++i;
		}
	}
	return out;
}

bool EditorOperations::normalizeRange(int &startRow,
									  int &startCol,
									  int &endRow,
									  int &endCol) const
{
	if (!state || state->lineCount() <= 0)
		return false;

	startRow = std::clamp(startRow, 0, state->lineCount() - 1);
	endRow = std::clamp(endRow, 0, state->lineCount() - 1);
	startCol = std::clamp(startCol, 0, state->lineLength(startRow));
	endCol = std::clamp(endCol, 0, state->lineLength(endRow));

	if (startRow > endRow || (startRow == endRow && startCol > endCol))
	{
		std::swap(startRow, endRow);
		std::swap(startCol, endCol);
	}
	return true;
}

int EditorOperations::measureLength(int startRow,
									int startCol,
									int endRow,
									int endCol) const
{
	if (!normalizeRange(startRow, startCol, endRow, endCol))
		return 0;
	const size_t a = state->offsetFromRowCol(startRow, startCol);
	const size_t b = state->offsetFromRowCol(endRow, endCol);
	return static_cast<int>(b - a);
}

std::string
EditorOperations::extractText(int startRow, int startCol, int endRow, int endCol) const
{
	if (!normalizeRange(startRow, startCol, endRow, endCol))
		return {};
	const size_t a = state->offsetFromRowCol(startRow, startCol);
	const size_t b = state->offsetFromRowCol(endRow, endCol);
	if (b <= a)
		return {};
	std::string out(b - a, '\0');
	state->copyBytes(a, b - a, out.data());
	return out;
}

TextOp EditorOperations::invert(const TextOp &op, const std::string &deletedText)
{
	if (op.kind == OpKind::Insert)
	{
		TextOp inv;
		inv.kind = OpKind::Delete;
		inv.row = op.row;
		inv.column = op.column;
		inv.length = static_cast<int>(op.text.size());
		return inv;
	}

	TextOp inv;
	inv.kind = OpKind::Insert;
	inv.row = op.row;
	inv.column = op.column;
	inv.text = deletedText;
	return inv;
}

// ---------------------------------------------------------------------------
// Pending edits (tree-sitter + LSP sync)
// ---------------------------------------------------------------------------

EditorEvents::DocumentChange PendingEdit::toDocumentChange() const
{
	EditorEvents::DocumentChange change;
	change.startLine = rangeStartLine;
	change.startCharacter = rangeStartCharacter;
	change.endLine = rangeEndLine;
	change.endCharacter = rangeEndCharacter;
	if (op.kind == OpKind::Insert)
		change.text = op.text;
	return change;
}

void EditorOperations::pushPending(const PendingEdit &edit)
{
	pending.push_back(edit);
	bumpGeneration();
}

std::vector<PendingEdit> EditorOperations::takePending()
{
	std::vector<PendingEdit> out;
	out.swap(pending);
	return out;
}

void EditorOperations::clearPending() { pending.clear(); }

// ---------------------------------------------------------------------------
// Apply (document only)
// ---------------------------------------------------------------------------

ApplyResult EditorOperations::apply(const TextOp &op)
{
	if (!state)
		return {};

	// Snapshot joined offsets + UTF-16 range before mutation (tree-sitter + LSP).
	PendingEdit treeEdit;
	treeEdit.op = op;
	const int row = std::clamp(op.row, 0, std::max(0, state->lineCount() - 1));
	const std::string startLine =
		state->lineCount() > 0 ? state->line(row) : std::string{};
	const int col = std::clamp(op.column, 0, static_cast<int>(startLine.size()));
	treeEdit.rangeStartLine = row;
	treeEdit.rangeStartCharacter = EditorUtils::Utf8ByteOffsetToUtf16(startLine, col);

	const size_t start = state->offsetFromRowCol(op.row, op.column);
	treeEdit.startByte = static_cast<uint32_t>(start);

	ApplyResult result;
	if (op.kind == OpKind::Insert)
	{
		treeEdit.oldEndByte = treeEdit.startByte;
		treeEdit.newEndByte = treeEdit.startByte + static_cast<uint32_t>(op.text.size());
		treeEdit.rangeEndLine = treeEdit.rangeStartLine;
		treeEdit.rangeEndCharacter = treeEdit.rangeStartCharacter;
		result = applyInsert(op, start);
	} else
	{
		treeEdit.oldEndByte =
			treeEdit.startByte + static_cast<uint32_t>(std::max(0, op.length));
		treeEdit.newEndByte = treeEdit.startByte;
		int endRow = row;
		int endCol = col;
		if (op.length > 0)
		{
			const size_t docSize = state->byteSize();
			const size_t endOff =
				std::min(start + static_cast<size_t>(op.length), docSize);
			state->rowColFromOffset(endOff, endRow, endCol);
		}
		const std::string endLine =
			state->lineCount() > 0 ? state->line(endRow) : std::string{};
		treeEdit.rangeEndLine = endRow;
		treeEdit.rangeEndCharacter = EditorUtils::Utf8ByteOffsetToUtf16(endLine, endCol);
		result = applyDelete(op, start);
		if (result.ok)
			treeEdit.removedBytes = result.deletedText;
	}

	if (result.ok)
	{
		pushPending(treeEdit);
		state->markEdited();
	}
	return result;
}

ApplyResult EditorOperations::applyInsert(const TextOp &op, size_t startByte)
{
	ApplyResult result;
	const int row = std::clamp(op.row, 0, std::max(0, state->lineCount() - 1));
	const int col = std::clamp(op.column, 0, state->lineLength(row));

	if (op.text.empty())
	{
		result.ok = true;
		result.endRow = row;
		result.endColumn = col;
		return result;
	}

	// Byte insert into joined rope (newlines in payload become line breaks).
	state->bufferInsert(startByte, op.text);
	state->rowColFromOffset(startByte + op.text.size(), result.endRow, result.endColumn);
	result.ok = true;
	return result;
}

ApplyResult EditorOperations::applyDelete(const TextOp &op, size_t startByte)
{
	ApplyResult result;
	if (op.length <= 0)
	{
		result.ok = op.length == 0;
		result.endRow = std::clamp(op.row, 0, std::max(0, state->lineCount() - 1));
		result.endColumn = std::clamp(op.column, 0, state->lineLength(result.endRow));
		return result;
	}

	const size_t docSize = state->byteSize();
	const size_t start = std::min(startByte, docSize);
	const size_t len =
		std::min(static_cast<size_t>(std::max(0, op.length)), docSize - start);

	if (len > 0)
	{
		result.deletedText.assign(len, '\0');
		state->copyBytes(start, len, result.deletedText.data());
		state->bufferErase(start, len);
	}

	state->rowColFromOffset(start, result.endRow, result.endColumn);
	result.ok = true;
	return result;
}
