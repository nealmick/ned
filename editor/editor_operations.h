/*
	File: editor_operations.h
	Description: Sole session API for document insert/delete.

	Mutates EditorState text only — not colors, cursor, or UI.
	All edits go through apply(). Undo and syntax parse share the same TextOp
	records. Rope mutation is TextBuffer; this layer maps TextOp → bytes and
	queues pending tree edits + generation.
*/

#pragma once

#include <cstdint>
#include <string>
#include <vector>

class EditorState;

enum class OpKind { Insert, Delete };

// Pure document mutation. No cursor fields — UI state is recorded separately.
struct TextOp
{
	OpKind kind = OpKind::Insert;
	int row = 0;
	int column = 0;
	std::string text; // Insert: payload; uses document line endings for breaks
	int length = 0;	  // Delete: byte length in joined-document space
};

// Precomputed joined-byte span for tree-sitter (filled at apply time, before mutate).
struct PendingTreeEdit
{
	TextOp op;
	uint32_t startByte = 0;
	uint32_t oldEndByte = 0;
	uint32_t newEndByte = 0;
	// Delete only: removed joined bytes (for ts_tree_edit points without a full
	// document clone). Empty on Insert.
	std::string removedBytes;
};

struct ApplyResult
{
	bool ok = false;
	std::string deletedText; // Delete: removed bytes in joined form (with line endings)
	int endRow = 0;			 // caret position after the op
	int endColumn = 0;
};

class EditorOperations
{
  public:
	explicit EditorOperations(EditorState &document) : state(&document) {}

	// Mutate document text only. Queues a tree-sitter edit. Does not move cursor
	// or touch highlight colors (highlight resizes on did-edit).
	ApplyResult apply(const TextOp &op);

	// Inverse op for undo (Insert <-> Delete).
	static TextOp invert(const TextOp &op, const std::string &deletedText);

	// Joined-document byte length of [start, end) in row/column space.
	int measureLength(int startRow, int startCol, int endRow, int endCol) const;

	// Extract joined text for a row/column range (uses document lineEnding).
	std::string extractText(int startRow, int startCol, int endRow, int endCol) const;

	// Normalize foreign newlines in paste text to the document's lineEnding.
	std::string normalizeLineEndings(const std::string &text) const;

	// Highlight consumes pending edits after each DidEdit.
	std::vector<PendingTreeEdit> takePending();
	void clearPending();

	uint64_t generation() const { return gen; }
	// Load / replace document — invalidate highlight without a TextOp.
	void bumpGeneration() { ++gen; }

  private:
	void pushPending(const PendingTreeEdit &edit);
	ApplyResult applyInsert(const TextOp &op, size_t startByte);
	ApplyResult applyDelete(const TextOp &op, size_t startByte);
	// Clamp + order a row/col range. False if document empty.
	bool normalizeRange(int &startRow, int &startCol, int &endRow, int &endCol) const;

	EditorState *state;
	std::vector<PendingTreeEdit> pending;
	uint64_t gen = 0;
};
