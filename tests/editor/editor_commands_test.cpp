/*
	Command-layer tests: type, delete, indent, undo path.
	Assert buffer text and invertibility after user edits.
*/
#include "harness/editor_fixture.h"
#include "third_party/catch.hpp"

#include <string>

using test::EditorFixture;

// Explicit UTF-8 bytes (not u8"…" / source glyphs) so MSVC matches other compilers
// even without /utf-8 on this TU.
static const std::string kEAcute("\xC3\xA9", 2);			 // U+00E9 é
static const std::string kBookEmoji("\xF0\x9F\x93\x9A", 4); // U+1F4DA 📚

TEST_CASE("EditorCommands typeText inserts and advances caret", "[ned][commands]")
{
	EditorFixture f;
	f.setDocument("hello");
	f.setCaret(0, 5);
	f.commands.typeText("!");
	REQUIRE(f.content() == "hello!");
	REQUIRE(f.view.row == 0);
	REQUIRE(f.view.column == 6);
	REQUIRE(f.state.dirty);
}

TEST_CASE("EditorCommands typeText multi-byte UTF-8 (é)", "[ned][commands][unicode]")
{
	EditorFixture f;
	f.setDocument("");
	f.commands.typeText(kEAcute);
	REQUIRE(f.content() == kEAcute);
	// Column is UTF-8 byte offset.
	REQUIRE(f.view.column == static_cast<int>(kEAcute.size()));
	REQUIRE(f.state.line(0) == kEAcute);
}

TEST_CASE("EditorCommands typeText multi-byte UTF-8 (emoji)", "[ned][commands][unicode]")
{
	EditorFixture f;
	f.setDocument("a");
	f.setCaret(0, 1);
	f.commands.typeText(kBookEmoji);
	f.commands.typeText("b");
	REQUIRE(f.content() == "a" + kBookEmoji + "b");
	REQUIRE(f.view.column == static_cast<int>(("a" + kBookEmoji + "b").size()));
}

TEST_CASE("EditorCommands typeText replaces selection", "[ned][commands]")
{
	EditorFixture f;
	f.setDocument("abcdef");
	f.setSelection(0, 1, 0, 4); // "bcd"
	int edits = 0;
	f.events.subscribeDidEdit([&](const EditorEvents::DidEdit &) { ++edits; });
	f.commands.typeText("X");
	REQUIRE(f.content() == "aXef");
	REQUIRE(f.view.row == 0);
	REQUIRE(f.view.column == 2);
	REQUIRE_FALSE(f.view.hasSelection());
	REQUIRE(edits == 1); // delete+insert batched into one did-edit
}

TEST_CASE("EditorCommands typeText empty is no-op", "[ned][commands]")
{
	EditorFixture f;
	f.setDocument("x");
	f.setCaret(0, 1);
	const int ver = f.state.version;
	f.commands.typeText("");
	REQUIRE(f.content() == "x");
	REQUIRE(f.state.version == ver);
	REQUIRE(f.view.column == 1);
}

TEST_CASE("EditorCommands deleteLeft mid-line", "[ned][commands]")
{
	EditorFixture f;
	f.setDocument("abcd");
	f.setCaret(0, 3); // before 'd'
	f.commands.deleteLeft();
	REQUIRE(f.content() == "abd");
	REQUIRE(f.view.column == 2);
}

TEST_CASE("EditorCommands deleteLeft merges lines at column 0", "[ned][commands]")
{
	EditorFixture f;
	f.setDocument("ab\ncd");
	f.setCaret(1, 0);
	f.commands.deleteLeft();
	REQUIRE(f.content() == "abcd");
	REQUIRE(f.view.row == 0);
	REQUIRE(f.view.column == 2);
}

TEST_CASE("EditorCommands deleteLeft removes whole UTF-8 char", "[ned][commands][unicode]")
{
	EditorFixture f;
	f.setDocument("a" + kEAcute + "x");
	// Place caret after é (byte length of "a" + "é").
	f.setCaret(0, static_cast<int>(("a" + kEAcute).size()));
	f.commands.deleteLeft();
	REQUIRE(f.content() == "ax");
	REQUIRE(f.view.column == 1);
}

TEST_CASE("EditorCommands deleteLeft with selection deletes selection", "[ned][commands]")
{
	EditorFixture f;
	f.setDocument("hello world");
	f.setSelection(0, 0, 0, 5);
	f.commands.deleteLeft();
	REQUIRE(f.content() == " world");
	REQUIRE(f.view.column == 0);
}

TEST_CASE("EditorCommands deleteRight mid-line", "[ned][commands]")
{
	EditorFixture f;
	f.setDocument("abcd");
	f.setCaret(0, 1);
	f.commands.deleteRight();
	REQUIRE(f.content() == "acd");
	REQUIRE(f.view.column == 1);
}

TEST_CASE("EditorCommands insertNewline copies leading indent", "[ned][commands]")
{
	EditorFixture f;
	f.setDocument("\tfoo");
	f.setCaret(0, 4); // after "\tfoo"
	f.commands.insertNewline();
	REQUIRE(f.state.lineCount() == 2);
	REQUIRE(f.state.line(0) == "\tfoo");
	REQUIRE(f.state.line(1) == "\t");
	REQUIRE(f.view.row == 1);
	REQUIRE(f.view.column == 1);
}

TEST_CASE("EditorCommands indent single line inserts tab at column 0", "[ned][commands]")
{
	EditorFixture f;
	f.setDocument("foo");
	f.setCaret(0, 2);
	f.commands.indent();
	REQUIRE(f.content() == "\tfoo");
	// Caret shifts by one tab character.
	REQUIRE(f.view.column == 3);
}

TEST_CASE("EditorCommands indent multi-line selection", "[ned][commands]")
{
	EditorFixture f;
	f.setDocument("a\nb\nc");
	f.setSelection(0, 0, 1, 1); // lines 0–1
	f.commands.indent();
	REQUIRE(f.state.line(0) == "\ta");
	REQUIRE(f.state.line(1) == "\tb");
	REQUIRE(f.state.line(2) == "c");
}

TEST_CASE("EditorCommands outdent removes leading tab", "[ned][commands]")
{
	EditorFixture f;
	f.setDocument("\tfoo");
	f.setCaret(0, 2);
	f.commands.outdent();
	REQUIRE(f.content() == "foo");
}

TEST_CASE("EditorCommands outdent removes leading four spaces", "[ned][commands]")
{
	EditorFixture f;
	f.setDocument("    foo");
	f.setCaret(0, 4);
	f.commands.outdent();
	REQUIRE(f.content() == "foo");
}

TEST_CASE("EditorCommands paste inserts clipboard text", "[ned][commands]")
{
	EditorFixture f;
	f.setDocument("ab");
	f.setCaret(0, 1);
	f.setClip("XY");
	f.commands.paste();
	REQUIRE(f.content() == "aXYb");
	REQUIRE(f.view.column == 3);
}

TEST_CASE("EditorCommands paste replaces selection", "[ned][commands]")
{
	EditorFixture f;
	f.setDocument("hello");
	f.setSelection(0, 1, 0, 4); // "ell"
	f.setClip("XX");
	f.commands.paste();
	REQUIRE(f.content() == "hXXo");
}

TEST_CASE("EditorCommands typeText multi-line inserts correctly", "[ned][commands]")
{
	// Same shape as EditorOperations multi-line insert: one line "ab", splice mid-line.
	EditorFixture f;
	f.setDocument("ab");
	f.state.lineEnding = "\n";
	f.setCaret(0, 1);
	f.commands.typeText("x\ny");
	REQUIRE(f.state.lines() == std::vector<std::string>{"ax", "yb"});
}

TEST_CASE("EditorCommands paste multi-line LF text", "[ned][commands]")
{
	EditorFixture f;
	f.setDocument("ab");
	f.state.lineEnding = "\n";
	f.setCaret(0, 1);
	f.setClip("x\ny");
	REQUIRE(f.getClip() == "x\ny");
	f.commands.paste();
	REQUIRE(f.state.lines() == std::vector<std::string>{"ax", "yb"});
}

TEST_CASE("EditorOperations normalizeLineEndings CRLF to LF", "[ned][ops]")
{
	EditorFixture f;
	f.setDocument("a\nb");
	REQUIRE(f.state.lineEnding == "\n");
	std::string crlf;
	crlf.push_back('x');
	crlf.push_back('\r');
	crlf.push_back('\n');
	crlf.push_back('y');
	REQUIRE(f.ops.normalizeLineEndings(crlf) == "x\ny");
}

TEST_CASE("EditorCommands copy writes selection to clipboard", "[ned][commands]")
{
	EditorFixture f;
	f.setDocument("hello world");
	f.setSelection(0, 0, 0, 5);
	f.commands.copy();
	REQUIRE(f.getClip() == "hello");
	REQUIRE(f.content() == "hello world"); // unchanged
}

TEST_CASE("EditorCommands cut selection removes and copies", "[ned][commands]")
{
	EditorFixture f;
	f.setDocument("hello world");
	f.setSelection(0, 0, 0, 5);
	f.commands.cut();
	REQUIRE(f.getClip() == "hello");
	REQUIRE(f.content() == " world");
}

TEST_CASE("EditorCommands selectAll then deleteSelection", "[ned][commands]")
{
	EditorFixture f;
	f.setDocument("one\ntwo");
	f.commands.selectAll();
	REQUIRE(f.view.hasSelection());
	f.commands.deleteSelection();
	// Empty document is one empty line.
	REQUIRE(f.state.lineCount() == 1);
	REQUIRE(f.state.line(0).empty());
	REQUIRE(f.content().empty());
}

TEST_CASE("EditorCommands moveLeft/Right update caret", "[ned][commands]")
{
	EditorFixture f;
	f.setDocument("ab");
	f.setCaret(0, 1);
	f.commands.moveRight(false);
	REQUIRE(f.view.column == 2);
	f.commands.moveLeft(false);
	REQUIRE(f.view.column == 1);
}

TEST_CASE("EditorCommands moveLeft with select creates selection", "[ned][commands]")
{
	EditorFixture f;
	f.setDocument("abc");
	f.setCaret(0, 2);
	f.commands.moveLeft(true);
	REQUIRE(f.view.hasSelection());
	int sr, sc, er, ec;
	f.view.getOrdered(sr, sc, er, ec);
	REQUIRE(sr == 0);
	REQUIRE(sc == 1);
	REQUIRE(er == 0);
	REQUIRE(ec == 2);
}
