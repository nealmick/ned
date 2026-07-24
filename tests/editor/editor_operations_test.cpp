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
