/*
	Git module tests: GitRepo + line_diff (real git fixture for repo).
*/

#include "editor/editor_state.h"
#include "editor/services/git/git_repo.h"
#include "editor/services/git/line_diff.h"
#include "third_party/catch.hpp"

#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <string>

namespace fs = std::filesystem;

namespace {

bool run(const std::string &cmd) { return std::system(cmd.c_str()) == 0; }

fs::path makeTempDir()
{
	const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
	fs::path p = fs::temp_directory_path() / ("ned-git-" + std::to_string(stamp));
	fs::create_directories(p);
	return p;
}

bool makeDirtyRepo(const fs::path &root)
{
	const std::string r = root.string();
	if (!run("git -C \"" + r + "\" init -q"))
		return false;
	if (!run("git -C \"" + r + "\" config user.email \"ned-test@example.com\""))
		return false;
	if (!run("git -C \"" + r + "\" config user.name \"ned-test\""))
		return false;

	{
		std::ofstream out(root / "README");
		out << "line1\n";
	}
	if (!run("git -C \"" + r + "\" add README"))
		return false;
	if (!run("git -C \"" + r + "\" commit -q -m init"))
		return false;

	{
		std::ofstream out(root / "README");
		out << "line1\nline2-modified\n";
	}
	return true;
}

} // namespace

TEST_CASE("GitRepo open fails on non-repo", "[ned][git]")
{
	const fs::path dir = makeTempDir();
	GitRepo repo;
	REQUIRE_FALSE(repo.open(dir.string()));
	fs::remove_all(dir);
}

TEST_CASE("GitRepo modifiedPaths detects dirty worktree", "[ned][git]")
{
	const fs::path dir = makeTempDir();
	if (!makeDirtyRepo(dir))
	{
		fs::remove_all(dir);
		WARN("git fixture setup failed — skipping");
		return;
	}

	GitRepo repo;
	REQUIRE(repo.open(dir.string()));
	REQUIRE(repo.modifiedPaths().count("README") == 1);

	REQUIRE(run("git -C \"" + dir.string() + "\" checkout -q -- README"));
	REQUIRE(repo.modifiedPaths().count("README") == 0);

	fs::remove_all(dir);
}

TEST_CASE("GitRepo headLines then line_diff sees buffer edits", "[ned][git]")
{
	const fs::path dir = makeTempDir();
	if (!makeDirtyRepo(dir))
	{
		fs::remove_all(dir);
		WARN("git fixture setup failed — skipping");
		return;
	}
	REQUIRE(run("git -C \"" + dir.string() + "\" checkout -q -- README"));

	GitRepo repo;
	REQUIRE(repo.open(dir.string()));

	std::vector<std::string> head;
	REQUIRE(repo.headLines("README", head));
	REQUIRE(head.size() >= 1);

	// Unsaved buffer: second line changed (editor stores lines without terminators).
	std::vector<std::string> buf = head;
	if (buf.size() == 1)
		buf.push_back("unsaved");
	else
		buf[1] = "unsaved";

	const LineDiff d = diffLines(head, buf);
	REQUIRE(d.additions > 0);
	REQUIRE_FALSE(d.addedLines.empty());

	fs::remove_all(dir);
}

TEST_CASE("alignLines ordered ops", "[ned][git]")
{
	// Replace block: b..d -> X..Y plus tail insert.
	const std::vector<std::string> a{"h", "b", "c", "d", "t"};
	const std::vector<std::string> b{"h", "X", "Y", "d", "t", "z"};
	const std::vector<DiffOp> ops = alignLines(a, b);

	struct R
	{
		DiffOp::Kind k;
		int o, n;
	};
	std::vector<R> got;
	for (const DiffOp &op : ops)
		got.push_back({op.kind, op.oldLine, op.newLine});

	// Expected alignment (1-based; delete block precedes its adds).
	const std::vector<R> want{
		{DiffOp::Kind::Keep, 1, 1},	  // h
		{DiffOp::Kind::Delete, 2, 0}, // b
		{DiffOp::Kind::Delete, 3, 0}, // c
		{DiffOp::Kind::Add, 0, 2},	  // X
		{DiffOp::Kind::Add, 0, 3},	  // Y
		{DiffOp::Kind::Keep, 4, 4},	  // d
		{DiffOp::Kind::Keep, 5, 5},	  // t
		{DiffOp::Kind::Add, 0, 6},	  // z
	};
	REQUIRE(got.size() == want.size());
	for (size_t i = 0; i < want.size(); ++i)
	{
		REQUIRE(got[i].k == want[i].k);
		REQUIRE(got[i].o == want[i].o);
		REQUIRE(got[i].n == want[i].n);
	}

	// diffLines stays consistent with the alignment.
	const LineDiff d = diffLines(a, b);
	REQUIRE(d.additions == 3);
	REQUIRE(d.deletions == 2);
}

TEST_CASE("diffLines pure unit", "[ned][git]")
{
	const std::vector<std::string> a{"a", "b", "c"};
	const std::vector<std::string> b{"a", "x", "c"};
	const LineDiff d = diffLines(a, b);
	REQUIRE(d.additions == 1);
	REQUIRE(d.deletions == 1);
	REQUIRE(d.addedLines.count(2) == 1);
}

TEST_CASE("diffLines single inserted blank marks only that line", "[ned][git]")
{
	std::vector<std::string> a;
	for (int i = 0; i < 20; ++i)
		a.push_back("content-" + std::to_string(i));
	std::vector<std::string> b = a;
	b.insert(b.begin() + 5, "");
	const LineDiff d = diffLines(a, b);
	REQUIRE(d.additions == 1);
	REQUIRE(d.deletions == 0);
	REQUIRE(d.addedLines.size() == 1);
	REQUIRE(d.addedLines.count(6) == 1); // 1-based index of the blank
}

TEST_CASE("diffLines single insert in large file does not mark every later line",
		  "[ned][git]")
{
	// Old fallback was index-aligned: after one insert, a[i]!=b[i] for all i
	// past the hole, so the whole suffix lit up in the gutter.
	std::vector<std::string> a;
	a.reserve(3000);
	for (int i = 0; i < 3000; ++i)
		a.push_back("unique-line-body-" + std::to_string(i));
	std::vector<std::string> b = a;
	b.insert(b.begin() + 100, "ONLY-NEW-LINE");
	const LineDiff d = diffLines(a, b);
	REQUIRE(d.additions == 1);
	REQUIRE(d.deletions == 0);
	REQUIRE(d.addedLines.size() == 1);
	REQUIRE(d.addedLines.count(101) == 1);
}
