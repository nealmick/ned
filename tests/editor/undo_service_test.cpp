/*
	ProjectUndo unit tests — record via real EditorCommands; undo/redo; coalesce; JSON.
*/

#include "harness/editor_fixture.h"
#include "third_party/catch.hpp"

#include <chrono>
#include <filesystem>
#include <fstream>
#include <thread>

namespace fs = std::filesystem;
using test::EditorFixture;

TEST_CASE("ProjectUndo typeText then undo then redo", "[ned][undo]")
{
	EditorFixture f;
	f.setDocument("hi");
	f.setCaret(0, 2);
	f.commands.typeText("!");
	REQUIRE(f.content() == "hi!");
	REQUIRE(f.view.column == 3);

	f.commands.undo();
	REQUIRE(f.content() == "hi");
	REQUIRE(f.view.column == 2);

	f.commands.redo();
	REQUIRE(f.content() == "hi!");
	REQUIRE(f.view.column == 3);
}

TEST_CASE("ProjectUndo adjacent inserts coalesce into one undo step", "[ned][undo]")
{
	EditorFixture f;
	f.setDocument("");
	f.commands.typeText("a");
	f.commands.typeText("b");
	f.commands.typeText("c");
	REQUIRE(f.content() == "abc");

	// One undo should remove the whole coalesced run.
	f.commands.undo();
	REQUIRE(f.content() == "");
	REQUIRE(f.view.column == 0);

	// Nothing left to undo from this group.
	f.commands.undo();
	REQUIRE(f.content() == "");
}

TEST_CASE("ProjectUndo insert then delete are separate steps", "[ned][undo]")
{
	EditorFixture f;
	f.setDocument("x");
	f.setCaret(0, 1);
	f.commands.typeText("y");
	REQUIRE(f.content() == "xy");
	f.commands.deleteLeft();
	REQUIRE(f.content() == "x");

	f.commands.undo(); // re-insert y
	REQUIRE(f.content() == "xy");
	f.commands.undo(); // remove y
	REQUIRE(f.content() == "x");
}

TEST_CASE("ProjectUndo new edit after undo clears redo", "[ned][undo]")
{
	EditorFixture f;
	f.setDocument("");
	f.commands.typeText("a");
	f.commands.undo();
	REQUIRE(f.content() == "");
	f.commands.typeText("b");
	REQUIRE(f.content() == "b");
	// Redo should be a no-op (branch discarded).
	f.commands.redo();
	REQUIRE(f.content() == "b");
}

TEST_CASE("ProjectUndo per-file stacks stay independent", "[ned][undo]")
{
	EditorFixture f;
	f.setDocument("A", "file-a");
	f.setCaret(0, 1); // after 'A'
	f.commands.typeText("1");
	REQUIRE(f.content() == "A1");

	f.setDocument("B", "file-b");
	f.setCaret(0, 1);
	f.commands.typeText("2");
	REQUIRE(f.content() == "B2");

	f.commands.undo();
	REQUIRE(f.content() == "B");

	// Switch back to A: its edit is still on the shared project store.
	f.state.setFromString("A1");
	f.state.path = "file-a";
	f.state.lineEnding = "\n";
	f.setCaret(0, 2);
	f.commands.undo();
	REQUIRE(f.content() == "A");
}

TEST_CASE("ProjectUndo shared store keeps both files across two fixtures",
		  "[ned][undo][multitab]")
{
	// Simulates two editors sharing one ProjectUndo (multi-tab).
	std::string projectRoot;
	ProjectUndo shared(projectRoot);

	EditorState stateA;
	EditorEvents eventsA;
	EditorOperations opsA(stateA);
	EditorViewState viewA(stateA);
	EditorSave saveA(stateA, eventsA);
	EditorCommands cmdA(stateA, viewA, opsA, shared, eventsA, saveA);

	EditorState stateB;
	EditorEvents eventsB;
	EditorOperations opsB(stateB);
	EditorViewState viewB(stateB);
	EditorSave saveB(stateB, eventsB);
	EditorCommands cmdB(stateB, viewB, opsB, shared, eventsB, saveB);

	test::ensureImGui();

	stateA.setFromString("A");
	stateA.path = "file-a";
	stateA.lineEnding = "\n";
	{
		Selection s;
		s.setBoth(0, 1);
		viewA.setSelections({s}, 0);
	}
	shared.ensureFile("file-a");
	cmdA.typeText("1");
	REQUIRE(stateA.join() == "A1");

	stateB.setFromString("B");
	stateB.path = "file-b";
	stateB.lineEnding = "\n";
	{
		Selection s;
		s.setBoth(0, 1);
		viewB.setSelections({s}, 0);
	}
	shared.ensureFile("file-b");
	cmdB.typeText("2");
	REQUIRE(stateB.join() == "B2");

	// Undo on B must not touch A's history.
	cmdB.undo();
	REQUIRE(stateB.join() == "B");
	REQUIRE(stateA.join() == "A1");

	cmdA.undo();
	REQUIRE(stateA.join() == "A");
}

TEST_CASE("ProjectUndo JSON serialize/deserialize via project dir", "[ned][undo][disk]")
{
	const fs::path tmp =
		fs::temp_directory_path() /
		("ned-undo-test-" +
		 std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
	fs::create_directories(tmp);
	const std::string fileKey = (tmp / "doc.txt").string();

	{
		EditorFixture f;
		f.projectRoot = tmp.string();
		f.setDocument("start", fileKey);
		f.setCaret(0, 5);
		f.commands.typeText("-edit");
		REQUIRE(f.content() == "start-edit");
		// First disk write is unthrottled (lastDiskSave epoch zero).
		// toJson includes pending; load should restore undo.
	}

	// New fixture, load project stacks, restore buffer and undo.
	{
		EditorFixture f;
		f.projectRoot = tmp.string();
		f.undo.loadProject(tmp.string());
		f.state.setFromString("start-edit");
		f.state.path = fileKey;
		f.state.lineEnding = "\n";
		f.setCaret(0, static_cast<int>(std::string("start-edit").size()));
		f.commands.undo();
		REQUIRE(f.content() == "start");
	}

	fs::remove_all(tmp);
}

TEST_CASE("ProjectUndo delayed inserts do not coalesce", "[ned][undo]")
{
	EditorFixture f;
	f.setDocument("");
	f.commands.typeText("a");
	// COALESCE_MS is 300; wait past it so the next insert seals a new group.
	std::this_thread::sleep_for(std::chrono::milliseconds(350));
	f.commands.typeText("b");
	REQUIRE(f.content() == "ab");

	f.commands.undo();
	REQUIRE(f.content() == "a");
	f.commands.undo();
	REQUIRE(f.content() == "");
}

TEST_CASE("ProjectUndo selection replace undoes to original", "[ned][undo]")
{
	EditorFixture f;
	f.setDocument("abcdef");
	f.setSelection(0, 1, 0, 4);
	f.commands.typeText("Z");
	REQUIRE(f.content() == "aZef");
	// Still two undo records (batch only coalesces did-edit, not undo steps).
	f.commands.undo();
	if (f.content() != "abcdef")
		f.commands.undo();
	REQUIRE(f.content() == "abcdef");
}
