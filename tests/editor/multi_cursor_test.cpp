/*
	Multi-caret edit + compound undo/redo.
*/

#include "harness/editor_fixture.h"
#include "third_party/catch.hpp"

using test::EditorFixture;

TEST_CASE("Multi-cursor typeText inserts at every caret", "[ned][multi]")
{
	EditorFixture f;
	f.setDocument("abc\ndef\nghi");
	f.setCarets({{0, 1}, {1, 1}, {2, 1}});
	f.commands.typeText("X");
	REQUIRE(f.content() == "aXbc\ndXef\ngXhi");
	REQUIRE(f.view.selectionCount() == 3);
}

TEST_CASE("Multi-cursor typeText one undo restores text and carets", "[ned][multi][undo]")
{
	EditorFixture f;
	f.setDocument("abc\ndef");
	f.setCarets({{0, 1}, {1, 1}}, 0);
	f.commands.typeText("Z");
	REQUIRE(f.content() == "aZbc\ndZef");
	REQUIRE(f.view.selectionCount() == 2);

	f.commands.undo();
	REQUIRE(f.content() == "abc\ndef");
	REQUIRE(f.view.selectionCount() == 2);
	REQUIRE(f.view.selections[0].headRow == 0);
	REQUIRE(f.view.selections[0].headColumn == 1);
	REQUIRE(f.view.selections[1].headRow == 1);
	REQUIRE(f.view.selections[1].headColumn == 1);

	f.commands.redo();
	REQUIRE(f.content() == "aZbc\ndZef");
	REQUIRE(f.view.selectionCount() == 2);
}

TEST_CASE("Multi-cursor same-line inserts shift later carets", "[ned][multi]")
{
	EditorFixture f;
	f.setDocument("abcdef");
	f.setCarets({{0, 1}, {0, 4}});
	f.commands.typeText("X");
	REQUIRE(f.content() == "aXbcdXef");
	// Heads after each insert: col 2 and col 6.
	REQUIRE(f.view.selectionCount() == 2);
	REQUIRE(f.view.selections[0].headColumn == 2);
	REQUIRE(f.view.selections[1].headColumn == 6);
}

TEST_CASE("Multi-cursor deleteLeft at each caret", "[ned][multi]")
{
	// "aXbYc" — carets after X (col 2) and after Y (col 4); backspace removes X and Y.
	EditorFixture f;
	f.setDocument("aXbYc");
	f.setCarets({{0, 2}, {0, 4}});
	f.commands.deleteLeft();
	REQUIRE(f.content() == "abc");
}

TEST_CASE("Multi-cursor replace ranges with typeText", "[ned][multi][undo]")
{
	EditorFixture f;
	f.setDocument("one two three");
	// Select "one" and "three"
	Selection a;
	a.anchorRow = 0;
	a.anchorColumn = 0;
	a.headRow = 0;
	a.headColumn = 3;
	Selection b;
	b.anchorRow = 0;
	b.anchorColumn = 8;
	b.headRow = 0;
	b.headColumn = 13;
	f.view.setSelections({a, b}, 0);
	f.commands.typeText("X");
	REQUIRE(f.content() == "X two X");

	f.commands.undo();
	REQUIRE(f.content() == "one two three");
	REQUIRE(f.view.selectionCount() == 2);
	REQUIRE(f.view.hasSelection());
}

TEST_CASE("Single caret still coalesces adjacent inserts", "[ned][multi][undo]")
{
	EditorFixture f;
	f.setDocument("");
	f.commands.typeText("a");
	f.commands.typeText("b");
	f.commands.typeText("c");
	REQUIRE(f.content() == "abc");
	f.commands.undo();
	REQUIRE(f.content() == "");
}

TEST_CASE("Selection replace is one undo group", "[ned][multi][undo]")
{
	EditorFixture f;
	f.setDocument("abcdef");
	f.setSelection(0, 1, 0, 4); // "bcd"
	f.commands.typeText("Z");
	REQUIRE(f.content() == "aZef");
	f.commands.undo();
	REQUIRE(f.content() == "abcdef");
}

TEST_CASE("collapseSelection drops secondary carets", "[ned][multi]")
{
	EditorFixture f;
	f.setDocument("a\nb\nc");
	f.setCarets({{0, 0}, {1, 0}, {2, 0}}, 1);
	REQUIRE(f.view.selectionCount() == 3);
	f.commands.collapseSelection();
	REQUIRE(f.view.selectionCount() == 1);
	REQUIRE(f.view.row == 1);
	REQUIRE(f.view.column == 0);
}

TEST_CASE("Nav moves every caret", "[ned][multi]")
{
	EditorFixture f;
	f.setDocument("aa\nbb\ncc");
	f.setCarets({{0, 0}, {1, 0}});
	f.commands.moveRight(false);
	REQUIRE(f.view.selections[0].headColumn == 1);
	REQUIRE(f.view.selections[1].headColumn == 1);
}

TEST_CASE("Plain move does not create a selection", "[ned][multi][nav]")
{
	EditorFixture f;
	f.setDocument("abcdef");
	f.setCaret(0, 2);
	f.commands.moveRight(false);
	REQUIRE_FALSE(f.view.hasSelection());
	REQUIRE(f.view.primary().empty());
	REQUIRE(f.view.column == 3);
	f.commands.moveLeft(false);
	f.commands.moveDown(false); // still one line — no selection either way
	REQUIRE_FALSE(f.view.hasSelection());
}

TEST_CASE("Shift-move extends selection; plain move clears it", "[ned][multi][nav]")
{
	EditorFixture f;
	f.setDocument("abcdef");
	f.setCaret(0, 1);
	f.commands.moveRight(true);
	f.commands.moveRight(true);
	REQUIRE(f.view.hasSelection());
	int sr, sc, er, ec;
	f.view.getOrdered(sr, sc, er, ec);
	REQUIRE(sc == 1);
	REQUIRE(ec == 3);

	f.commands.moveRight(false);
	REQUIRE_FALSE(f.view.hasSelection());
	REQUIRE(f.view.column == 4);
}

TEST_CASE("Multi plain move does not create ranges", "[ned][multi][nav]")
{
	EditorFixture f;
	f.setDocument("aaa\nbbb");
	f.setCarets({{0, 0}, {1, 0}});
	f.commands.moveRight(false);
	REQUIRE_FALSE(f.view.hasSelection());
	for (const Selection &s : f.view.selections)
		REQUIRE(s.empty());
}

TEST_CASE("addCursorBelow builds a column of carets", "[ned][multi][spawn]")
{
	EditorFixture f;
	f.setDocument("aaa\nbbb\nccc\nddd");
	f.setCaret(0, 1);
	f.commands.addCursorBelow();
	REQUIRE(f.view.selectionCount() == 2);
	REQUIRE(f.view.primary().headRow == 1);
	REQUIRE(f.view.primary().headColumn == 1);

	f.commands.addCursorBelow();
	REQUIRE(f.view.selectionCount() == 3);
	REQUIRE(f.view.primary().headRow == 2);

	// Typing hits every line.
	f.commands.typeText("X");
	REQUIRE(f.content() == "aXaa\nbXbb\ncXcc\nddd");
}

TEST_CASE("addCursorAbove grows column upward; primary follows new caret",
		  "[ned][multi][spawn]")
{
	EditorFixture f;
	f.setDocument("aaa\nbbb\nccc");
	f.setCaret(2, 1);
	f.commands.addCursorAbove();
	REQUIRE(f.view.selectionCount() == 2);
	REQUIRE(f.view.primary().headRow == 1);
	f.commands.addCursorAbove();
	REQUIRE(f.view.selectionCount() == 3);
	REQUIRE(f.view.primary().headRow == 0);
	// Top edge: no-op.
	f.commands.addCursorAbove();
	REQUIRE(f.view.selectionCount() == 3);
}

TEST_CASE("addCursorBelow no-op when caret already on target line", "[ned][multi][spawn]")
{
	EditorFixture f;
	f.setDocument("aaa\nbbb");
	f.setCarets({{0, 1}, {1, 1}}, 0);
	// Primary on row 0; below would be row 1 which already has a caret.
	f.commands.addCursorBelow();
	REQUIRE(f.view.selectionCount() == 2);
}

TEST_CASE("setSelections from match ranges (find-all spawn shape)", "[ned][multi][spawn]")
{
	EditorFixture f;
	f.setDocument("foo bar foo baz foo");
	// Matches of "foo" at cols 0, 8, 16.
	std::vector<Selection> sels;
	for (int col : {0, 8, 16})
	{
		Selection s;
		s.anchorRow = 0;
		s.anchorColumn = col;
		s.headRow = 0;
		s.headColumn = col + 3;
		sels.push_back(s);
	}
	f.commands.setSelections(std::move(sels), /*primary*/ 1);
	REQUIRE(f.view.selectionCount() == 3);
	REQUIRE(f.view.hasSelection());
	REQUIRE(f.view.primaryIndex == 1);
	REQUIRE(f.view.primary().anchorColumn == 8);

	f.commands.typeText("X");
	REQUIRE(f.content() == "X bar X baz X");
	f.commands.undo();
	REQUIRE(f.content() == "foo bar foo baz foo");
	REQUIRE(f.view.selectionCount() == 3);
}
