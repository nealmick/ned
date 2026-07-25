/*
	EditorSave unit tests — real disk I/O in a temp directory.
*/

#include "harness/editor_fixture.h"
#include "third_party/catch.hpp"

#include <chrono>
#include <filesystem>
#include <fstream>
#include <sstream>

namespace fs = std::filesystem;
using test::EditorFixture;

namespace {

// Must match FileExplorer::readFileContent notice / EditorSave guard needle.
constexpr const char *kTruncatedMarker = "[File truncated - No Edits - showing first";

fs::path makeTempDir(const char *prefix)
{
	const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
	fs::path p =
		fs::temp_directory_path() / (std::string(prefix) + std::to_string(stamp));
	fs::create_directories(p);
	return p;
}

std::string readAll(const fs::path &path)
{
	std::ifstream in(path, std::ios::binary);
	std::ostringstream ss;
	ss << in.rdbuf();
	return ss.str();
}

void writeAll(const fs::path &path, const std::string &data)
{
	std::ofstream out(path, std::ios::binary);
	out << data;
}

} // namespace

TEST_CASE("EditorSave dirty path writes file and clears dirty", "[ned][save]")
{
	const fs::path dir = makeTempDir("ned-save-");
	const fs::path file = dir / "note.txt";
	writeAll(file, "old");

	EditorFixture f;
	f.setDocument("new content");
	f.state.path = file.string();
	f.state.dirty = true;
	f.state.version = 3;

	bool didSave = false;
	f.events.subscribeDidSave([&](const EditorEvents::DidSave &e) {
		didSave = true;
		REQUIRE(e.path == file.string());
		REQUIRE(e.version == 3);
	});

	f.save.save();
	REQUIRE(didSave);
	REQUIRE_FALSE(f.state.dirty);
	REQUIRE(readAll(file) == "new content");

	fs::remove_all(dir);
}

TEST_CASE("EditorSave preserves UTF-8 BOM stripped on load", "[ned][save][bom]")
{
	const fs::path dir = makeTempDir("ned-save-bom-");
	const fs::path file = dir / "bom.txt";
	const std::string bom = "\xEF\xBB\xBF";
	writeAll(file, bom + "hello");

	EditorFixture f;
	f.setDocument(bom + "hello");
	REQUIRE(f.state.utf8Bom);
	REQUIRE(f.content() == "hello"); // BOM not in buffer
	f.state.path = file.string();
	f.state.dirty = true;

	f.save.save();
	REQUIRE(readAll(file) == bom + "hello");

	// No BOM on load → no BOM on save
	f.setDocument("plain");
	REQUIRE_FALSE(f.state.utf8Bom);
	f.state.path = file.string();
	f.state.dirty = true;
	f.save.save();
	REQUIRE(readAll(file) == "plain");

	fs::remove_all(dir);
}

TEST_CASE("EditorSave no-op when not dirty", "[ned][save]")
{
	const fs::path dir = makeTempDir("ned-save-clean-");
	const fs::path file = dir / "note.txt";
	writeAll(file, "keep-me");

	EditorFixture f;
	f.setDocument("would-overwrite");
	f.state.path = file.string();
	f.state.dirty = false;

	f.save.save();
	REQUIRE(readAll(file) == "keep-me");

	fs::remove_all(dir);
}

TEST_CASE("EditorSave no-op when path empty", "[ned][save]")
{
	EditorFixture f;
	f.setDocument("x");
	f.state.path.clear();
	f.state.dirty = true;
	f.save.save();
	REQUIRE(f.state.dirty); // still dirty — nothing to save to
}

TEST_CASE("EditorSave refuses truncated buffer (marker guard)", "[ned][save]")
{
	const fs::path dir = makeTempDir("ned-save-trunc-");
	const fs::path file = dir / "big.txt";
	writeAll(file, "ORIGINAL_ON_DISK");

	EditorFixture f;
	// Same shape of notice FileExplorer prepends on open of a large file.
	const std::string notice = std::string("\n\n") + kTruncatedMarker + " 1MB of 5MB]\n";
	f.state.setFromString(notice + "partial body");
	f.state.lineEnding = "\n";
	f.state.path = file.string();
	f.state.dirty = true;

	REQUIRE(f.state.join().find(kTruncatedMarker) != std::string::npos);

	f.save.save();
	// Guard must refuse: disk unchanged, still dirty.
	REQUIRE(f.state.dirty);
	REQUIRE(readAll(file) == "ORIGINAL_ON_DISK");

	fs::remove_all(dir);
}

TEST_CASE("EditorSave commands.save delegates", "[ned][save][commands]")
{
	const fs::path dir = makeTempDir("ned-save-cmd-");
	const fs::path file = dir / "via-cmd.txt";
	writeAll(file, "");

	EditorFixture f;
	f.setDocument("from-command");
	f.state.path = file.string();
	f.state.dirty = true;
	f.commands.save();
	REQUIRE_FALSE(f.state.dirty);
	REQUIRE(readAll(file) == "from-command");

	fs::remove_all(dir);
}

TEST_CASE("EditorSave autosave is idle-debounced", "[ned][save][autosave]")
{
	const fs::path dir = makeTempDir("ned-save-auto-");
	const fs::path file = dir / "note.txt";
	writeAll(file, "old");

	EditorFixture f;
	f.setDocument("new");
	f.state.path = file.string();
	f.state.dirty = true;

	int saves = 0;
	f.events.subscribeDidSave([&](const EditorEvents::DidSave &) { ++saves; });

	// Default idle: schedule only — no write yet.
	f.save.onDidEdit();
	f.save.poll();
	REQUIRE(saves == 0);
	REQUIRE(readAll(file) == "old");

	// Zero idle: poll flushes; explicit save clears pending.
	f.save.setAutosaveIdleMs(0);
	f.save.onDidEdit();
	f.save.poll();
	REQUIRE(saves == 1);
	REQUIRE_FALSE(f.state.dirty);
	REQUIRE(readAll(file) == "new");

	f.state.setFromString("again");
	f.state.dirty = true;
	f.save.onDidEdit();
	f.save.save(); // immediate; cancels schedule
	REQUIRE(saves == 2);
	f.save.poll();
	REQUIRE(saves == 2);

	// No path: never schedules.
	f.state.path.clear();
	f.state.dirty = true;
	f.save.onDidEdit();
	f.save.poll();
	REQUIRE(f.state.dirty);

	fs::remove_all(dir);
}
