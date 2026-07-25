/*
	TextBuffer / EditorState document storage tests.
	Mutations go through EditorState + EditorOperations (buffer mutators are private).
*/

#include "editor/buffer/text_buffer.h"
#include "editor/editor_operations.h"
#include "editor/editor_state.h"
#include "third_party/catch.hpp"

#include <string>
#include <vector>

namespace {

void insertAt(EditorState &state, EditorOperations &ops, int row, int col, std::string text)
{
	TextOp op;
	op.kind = OpKind::Insert;
	op.row = row;
	op.column = col;
	op.text = std::move(text);
	REQUIRE(ops.apply(op).ok);
}

void eraseAt(EditorState &state, EditorOperations &ops, int row, int col, int length)
{
	TextOp op;
	op.kind = OpKind::Delete;
	op.row = row;
	op.column = col;
	op.length = length;
	REQUIRE(ops.apply(op).ok);
}

} // namespace

TEST_CASE("TextBuffer empty defaults", "[ned][buffer]")
{
	EditorState state;
	REQUIRE(state.byteSize() == 0);
	REQUIRE(state.lineCount() == 1);
	REQUIRE(state.line(0).empty());
	REQUIRE(state.join().empty());
	REQUIRE(state.lineLength(0) == 0);
	REQUIRE(state.offsetFromRowCol(0, 0) == 0);
}

TEST_CASE("TextBuffer assign single line", "[ned][buffer]")
{
	EditorState state;
	state.setFromString("hello");
	REQUIRE(state.byteSize() == 5);
	REQUIRE(state.lineCount() == 1);
	REQUIRE(state.line(0) == "hello");
	REQUIRE(state.join() == "hello");
	REQUIRE(state.lineLength(0) == 5);
}

TEST_CASE("TextBuffer multi-line LF", "[ned][buffer]")
{
	EditorState state;
	state.setFromString("aa\nbbb\nc");
	REQUIRE(state.lineCount() == 3);
	REQUIRE(state.line(0) == "aa");
	REQUIRE(state.line(1) == "bbb");
	REQUIRE(state.line(2) == "c");
	REQUIRE(state.join() == "aa\nbbb\nc");
	REQUIRE(state.lineLength(0) == 2);
	REQUIRE(state.lineLength(1) == 3);
	REQUIRE(state.lineLength(2) == 1);
}

TEST_CASE("TextBuffer multi-line CRLF", "[ned][buffer]")
{
	EditorState state;
	state.setFromString("aa\r\nbbb\r\nc");
	REQUIRE(state.lineCount() == 3);
	REQUIRE(state.line(0) == "aa");
	REQUIRE(state.line(1) == "bbb");
	REQUIRE(state.line(2) == "c");
	REQUIRE(state.join() == "aa\r\nbbb\r\nc");
}

TEST_CASE("TextBuffer large CRLF file keeps line bodies after load", "[ned][buffer]")
{
	// Force multi-leaf rope (kMaxLeafBytes == 128) with many CRLF lines.
	std::string raw;
	std::vector<std::string> expect;
	for (int i = 0; i < 40; ++i)
	{
		const std::string body = "line-" + std::to_string(i) + "-xxxxxxxx";
		expect.push_back(body);
		raw += body;
		raw += "\r\n";
	}
	raw += "tail";
	expect.push_back("tail");

	EditorState state;
	state.setFromString(raw);
	REQUIRE(state.lineCount() == static_cast<int>(expect.size()));
	for (size_t i = 0; i < expect.size(); ++i)
		REQUIRE(state.line(static_cast<int>(i)) == expect[i]);
}

TEST_CASE("TextBuffer insert newline mid-file preserves later lines", "[ned][buffer]")
{
	std::string raw;
	for (int i = 0; i < 30; ++i)
	{
		raw += "row" + std::to_string(i) + "abcdef";
		raw += (i + 1 < 30) ? "\n" : "";
	}
	// Ensure multi-leaf.
	while (raw.size() < 300)
		raw += "padding-line\n";

	EditorState state;
	EditorOperations ops(state);
	state.setFromString(raw);
	const int linesBefore = state.lineCount();
	REQUIRE(linesBefore > 5);

	const std::string line3 = state.line(3);
	const std::string line4 = state.line(4);
	const std::string last = state.line(linesBefore - 1);

	// Press Enter at start of line 3 (same as insertNewline at col 0).
	insertAt(state, ops, 3, 0, "\n");
	REQUIRE(state.lineCount() == linesBefore + 1);
	REQUIRE(state.line(3).empty());
	REQUIRE(state.line(4) == line3);
	REQUIRE(state.line(5) == line4);
	REQUIRE(state.line(state.lineCount() - 1) == last);
}

TEST_CASE("TextBuffer insert CRLF mid large CRLF file preserves later lines",
		  "[ned][buffer]")
{
	std::string raw;
	std::vector<std::string> bodies;
	for (int i = 0; i < 40; ++i)
	{
		bodies.push_back("L" + std::to_string(i) + "-yyyyyyyy");
		raw += bodies.back();
		raw += "\r\n";
	}

	EditorState state;
	EditorOperations ops(state);
	state.setFromString(raw);
	state.lineEnding = "\r\n";
	const int n = state.lineCount();
	REQUIRE(n >= 40);

	const std::string keep = state.line(10);
	insertAt(state, ops, 10, 0, "\r\n");
	REQUIRE(state.line(10).empty());
	REQUIRE(state.line(11) == keep);
	// Spot-check a few later lines still match original bodies.
	REQUIRE(state.line(20) == bodies[static_cast<size_t>(19)]);
}

TEST_CASE("TextBuffer trailing newline yields empty last line", "[ned][buffer]")
{
	EditorState state;
	state.setFromString("x\n");
	REQUIRE(state.lineCount() == 2);
	REQUIRE(state.line(0) == "x");
	REQUIRE(state.line(1).empty());
}

TEST_CASE("TextBuffer offset/rowCol round-trip LF", "[ned][buffer]")
{
	EditorState state;
	state.setFromString("aa\nbbb\nc");
	for (size_t off = 0; off <= state.byteSize(); ++off)
	{
		int row = 0, col = 0;
		state.rowColFromOffset(off, row, col);
		const size_t back = state.offsetFromRowCol(row, col);
		REQUIRE(back <= state.byteSize());
		int r2 = 0, c2 = 0;
		state.rowColFromOffset(back, r2, c2);
		REQUIRE(r2 == row);
		REQUIRE(c2 == col);
	}
}

TEST_CASE("TextBuffer offsetFromRowCol known positions", "[ned][buffer]")
{
	EditorState state;
	state.setFromString("aa\nbbb\nc");
	REQUIRE(state.offsetFromRowCol(0, 0) == 0);
	REQUIRE(state.offsetFromRowCol(0, 2) == 2);
	REQUIRE(state.offsetFromRowCol(1, 0) == 3);
	REQUIRE(state.offsetFromRowCol(1, 3) == 6);
	REQUIRE(state.offsetFromRowCol(2, 0) == 7);
	REQUIRE(state.offsetFromRowCol(2, 1) == 8);
}

TEST_CASE("TextBuffer insert mid-line via ops", "[ned][buffer]")
{
	EditorState state;
	state.setFromString("hello");
	EditorOperations ops(state);
	insertAt(state, ops, 0, 5, "!");
	REQUIRE(state.join() == "hello!");
	insertAt(state, ops, 0, 0, "X");
	REQUIRE(state.join() == "Xhello!");
	insertAt(state, ops, 0, 2, "y");
	REQUIRE(state.join() == "Xhyello!");
}

TEST_CASE("TextBuffer insert multi-line via ops", "[ned][buffer]")
{
	EditorState state;
	state.setFromString("ab");
	state.lineEnding = "\n";
	// setFromString already set LE from content; re-set content with LF
	state.setFromString("ab");
	EditorOperations ops(state);
	insertAt(state, ops, 0, 1, "x\ny");
	REQUIRE(state.join() == "ax\nyb");
	REQUIRE(state.lineCount() == 2);
	REQUIRE(state.line(0) == "ax");
	REQUIRE(state.line(1) == "yb");
}

TEST_CASE("TextBuffer erase via ops", "[ned][buffer]")
{
	EditorState state;
	state.setFromString("hello world");
	EditorOperations ops(state);
	eraseAt(state, ops, 0, 5, 1);
	REQUIRE(state.join() == "helloworld");
	eraseAt(state, ops, 0, 0, 5);
	REQUIRE(state.join() == "world");
	eraseAt(state, ops, 0, 0, static_cast<int>(state.byteSize()));
	REQUIRE(state.join().empty());
	REQUIRE(state.lineCount() == 1);
}

TEST_CASE("TextBuffer erase across lines", "[ned][buffer]")
{
	EditorState state;
	state.setFromString("aa\nbbb\nc");
	EditorOperations ops(state);
	// Delete "\nbbb\n" from after "aa" → "aac"
	eraseAt(state, ops, 0, 2, 5);
	REQUIRE(state.join() == "aac");
	REQUIRE(state.lineCount() == 1);
}

TEST_CASE("TextBuffer copyBytes", "[ned][buffer]")
{
	EditorState state;
	state.setFromString("hello world");
	char tmp[6] = {};
	state.copyBytes(6, 5, tmp);
	REQUIRE(std::string(tmp, 5) == "world");
}

TEST_CASE("TextBuffer snapshot is stable across later edits", "[ned][buffer]")
{
	EditorState state;
	state.setFromString("hello");
	auto snap = state.snapshot();
	REQUIRE(snap.str() == "hello");
	REQUIRE(snap.size() == 5);

	EditorOperations ops(state);
	insertAt(state, ops, 0, 5, " world");
	REQUIRE(state.join() == "hello world");
	REQUIRE(snap.str() == "hello");
	REQUIRE(snap.size() == 5);
}

TEST_CASE("TextBuffer large assign and edit", "[ned][buffer]")
{
	std::string big;
	big.reserve(4000);
	for (int i = 0; i < 100; ++i)
	{
		big += "line ";
		big += std::to_string(i);
		big += '\n';
	}
	EditorState state;
	state.setFromString(big);
	REQUIRE(state.join() == big);
	REQUIRE(state.lineCount() == 101);

	EditorOperations ops(state);
	insertAt(state, ops, 0, 0, "HEAD\n");
	REQUIRE(state.line(0) == "HEAD");
	REQUIRE(state.join().substr(0, 5) == "HEAD\n");

	eraseAt(state, ops, 0, 0, 5);
	REQUIRE(state.join() == big);
}

TEST_CASE("TextBuffer many small inserts stay coherent", "[ned][buffer]")
{
	EditorState state;
	EditorOperations ops(state);
	for (int i = 0; i < 200; ++i)
		insertAt(state, ops, 0, static_cast<int>(state.byteSize()), "x");
	REQUIRE(state.byteSize() == 200);
	REQUIRE(state.join() == std::string(200, 'x'));
	REQUIRE(state.lineCount() == 1);
}

TEST_CASE("TextBuffer lineStarts and containsByte", "[ned][buffer]")
{
	EditorState state;
	state.setFromString("aa\nbbb\nc");
	std::vector<uint32_t> starts;
	state.lineStarts(starts);
	REQUIRE(starts.size() == 3);
	REQUIRE(starts[0] == 0);
	REQUIRE(starts[1] == 3);
	REQUIRE(starts[2] == 7);
	REQUIRE_FALSE(state.containsByte('\t'));

	EditorOperations ops(state);
	insertAt(state, ops, 0, 1, "\t");
	REQUIRE(state.containsByte('\t'));

	std::string line;
	state.lineInto(1, line);
	REQUIRE(line == "bbb");
	state.lineInto(1, line);
	REQUIRE(line == "bbb");
}

TEST_CASE("EditorState setLineEnding rebuilds rope", "[ned][buffer]")
{
	EditorState state;
	state.setFromString("a\nb");
	REQUIRE(state.lineEnding == "\n");
	state.setLineEnding("\r\n");
	REQUIRE(state.lineEnding == "\r\n");
	REQUIRE(state.join() == "a\r\nb");
	REQUIRE(state.lineCount() == 2);
}
