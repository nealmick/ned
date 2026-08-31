/*
	Native op tests — invert, measure, multi-line insert, offsets.
	Complements the 1-based text-model suites.
*/
#include "editor/editor_operations.h"
#include "editor/editor_state.h"
#include "third_party/catch.hpp"

TEST_CASE("EditorOperations insert/delete invert round-trip", "[ned][ops]")
{
	EditorState state;
	state.setFromString("hello world");
	EditorOperations ops(state);

	TextOp del;
	del.kind = OpKind::Delete;
	del.row = 0;
	del.column = 5;
	del.length = 1;
	const auto r = ops.apply(del);
	REQUIRE(r.ok);
	REQUIRE(state.join() == "helloworld");

	const TextOp inv = EditorOperations::invert(del, r.deletedText);
	REQUIRE(ops.apply(inv).ok);
	REQUIRE(state.join() == "hello world");
}

TEST_CASE("EditorOperations multi-line insert", "[ned][ops]")
{
	EditorState state;
	state.setFromString("ab");
	state.lineEnding = "\n";
	EditorOperations ops(state);

	TextOp ins;
	ins.kind = OpKind::Insert;
	ins.row = 0;
	ins.column = 1;
	ins.text = "x\ny";
	const auto r = ops.apply(ins);
	REQUIRE(r.ok);
	REQUIRE(state.lines() == std::vector<std::string>{"ax", "yb"});
}

TEST_CASE("EditorOperations pending edit records UTF-16 range", "[ned][ops][lsp]")
{
	EditorState state;
	state.setFromString("caf\xC3\xA9 x");
	state.lineEnding = "\n";
	EditorOperations ops(state);

	TextOp ins;
	ins.kind = OpKind::Insert;
	ins.row = 0;
	ins.column = 5; // after é (5 UTF-8 bytes == 4 UTF-16)
	ins.text = "y";
	REQUIRE(ops.apply(ins).ok);
	REQUIRE(ops.pendingEdits().size() == 1);
	const PendingEdit &edit = ops.pendingEdits().front();
	REQUIRE(edit.rangeStartLine == 0);
	REQUIRE(edit.rangeStartCharacter == 4);
	REQUIRE(edit.rangeEndLine == 0);
	REQUIRE(edit.rangeEndCharacter == 4);
	REQUIRE(edit.op.text == "y");
}

TEST_CASE("EditorOperations pending delete spans UTF-16 range", "[ned][ops][lsp]")
{
	EditorState state;
	state.setFromString("hello");
	EditorOperations ops(state);

	TextOp del;
	del.kind = OpKind::Delete;
	del.row = 0;
	del.column = 1;
	del.length = 3;
	REQUIRE(ops.apply(del).ok);
	REQUIRE(ops.pendingEdits().size() == 1);
	const PendingEdit &edit = ops.pendingEdits().front();
	REQUIRE(edit.rangeStartLine == 0);
	REQUIRE(edit.rangeStartCharacter == 1);
	REQUIRE(edit.rangeEndLine == 0);
	REQUIRE(edit.rangeEndCharacter == 4);
}

TEST_CASE("EditorState offset/rowCol round-trip", "[ned][state]")
{
	EditorState state;
	state.setFromString("aa\nbbb\nc");
	for (size_t off = 0; off <= state.join().size(); ++off)
	{
		int row = 0, col = 0;
		state.rowColFromOffset(off, row, col);
		REQUIRE(state.offsetFromRowCol(row, col) == off);
	}
}
