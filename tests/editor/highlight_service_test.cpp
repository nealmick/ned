#include "editor/services/highlight/highlight_service.h"
#include "harness/editor_fixture.h"
#include "third_party/catch.hpp"
#include "util/settings.h"

#include <chrono>
#include <string>
#include <thread>

// tree_sitter.cpp resolves queries via Settings::getAppResourcesPath.
// Tests do not link settings.cpp — point at the source tree.
#ifndef CMAKE_SOURCE_DIR
#define CMAKE_SOURCE_DIR "."
#endif

std::string Settings::getAppResourcesPath() { return CMAKE_SOURCE_DIR; }

namespace {

std::string manyCLines(int n)
{
	std::string s;
	s.reserve(static_cast<size_t>(n) * 24);
	for (int i = 0; i < n; ++i)
	{
		s += "int x";
		s += std::to_string(i);
		s += " = 0;\n";
	}
	return s;
}

bool waitSpans(EditorHighlight &hl, int row, int ms = 3000)
{
	const auto deadline =
		std::chrono::steady_clock::now() + std::chrono::milliseconds(ms);
	while (std::chrono::steady_clock::now() < deadline)
	{
		hl.poll();
		if (!hl.spansForLine(row).empty())
			return true;
		std::this_thread::sleep_for(std::chrono::milliseconds(5));
	}
	hl.poll();
	return !hl.spansForLine(row).empty();
}

} // namespace

TEST_CASE("highlightSnippet colors a C++ signature", "[ned][highlight][hover]")
{
	const auto spans = TreeSitter::highlightSnippet("cpp", "int foo(int x);");
	REQUIRE_FALSE(spans.empty());
	bool nonText = false;
	for (const auto &line : spans)
	{
		for (const auto &s : line)
		{
			if (s.slot != ThemeSlot::Text)
				nonText = true;
		}
	}
	REQUIRE(nonText);
}

TEST_CASE("full rebuild colors the prime window before poll", "[ned][highlight]")
{
	test::EditorFixture fx;
	fx.setDocument(manyCLines(2000), "test.c");
	fx.state.languageId = "c";

	EditorHighlight hl(fx.state, fx.ops);
	hl.resetForDocument(static_cast<size_t>(fx.state.lineCount()));
	hl.highlightContent();
	// First pass may compile the query on a worker.
	REQUIRE(waitSpans(hl, 0));

	hl.resetForDocument(static_cast<size_t>(fx.state.lineCount()));
	hl.highlightContent();

	const int prime = EditorHighlight::kPrimeQueryLines;
	REQUIRE_FALSE(hl.spansForLine(0).empty());
	REQUIRE(hl.spansForLine(0)[0].slot == ThemeSlot::Special);
	REQUIRE_FALSE(hl.spansForLine(prime - 1).empty());
	REQUIRE(hl.spansForLine(prime).empty());

	REQUIRE(waitSpans(hl, prime));
	REQUIRE_FALSE(hl.spansForLine(prime).empty());
}

TEST_CASE("insert at end of a span keeps the slot before recolor", "[ned][highlight]")
{
	test::EditorFixture fx;
	// Big enough that recolor is async — morph must not flash default.
	fx.setDocument("// hi\n" + manyCLines(2000), "test.c");
	fx.state.languageId = "c";

	EditorHighlight hl(fx.state, fx.ops);
	hl.resetForDocument(static_cast<size_t>(fx.state.lineCount()));
	hl.highlightContent();
	REQUIRE(waitSpans(hl, 0));

	const auto before = hl.spansForLine(0);
	REQUIRE_FALSE(before.empty());
	REQUIRE(before.back().slot == ThemeSlot::Comment);

	const int col = fx.state.lineLength(0);
	REQUIRE(fx.ops.apply({OpKind::Insert, 0, col, "x", 0}).ok);
	hl.highlightContent();

	const auto after = hl.spansForLine(0);
	REQUIRE_FALSE(after.empty());
	REQUIRE(after.back().slot == ThemeSlot::Comment);
	REQUIRE(after.back().end >= col + 1);
}

TEST_CASE("theme swap remaps palette without dropping spans", "[ned][highlight]")
{
	test::EditorFixture fx;
	fx.setDocument("int main(void) { return 0; }\n", "test.c");
	fx.state.languageId = "c";

	EditorHighlight hl(fx.state, fx.ops);
	hl.resetForDocument(static_cast<size_t>(fx.state.lineCount()));
	hl.highlightContent();
	REQUIRE(waitSpans(hl, 0));

	const ThemeSlot slot = hl.spansForLine(0)[0].slot;
	const uint64_t gen = hl.visualGeneration();
	hl.forceColorUpdate();

	REQUIRE(hl.visualGeneration() > gen);
	REQUIRE_FALSE(hl.spansForLine(0).empty());
	REQUIRE(hl.spansForLine(0)[0].slot == slot);
}

TEST_CASE("language switch replaces c spans with python spans", "[ned][highlight]")
{
	test::EditorFixture fx;
	fx.setDocument("def foo():\n    return 1\n", "test.py");
	fx.state.languageId = "c";

	EditorHighlight hl(fx.state, fx.ops);
	hl.resetForDocument(static_cast<size_t>(fx.state.lineCount()));
	hl.highlightContent();
	REQUIRE(waitSpans(hl, 0));

	fx.state.languageId = "py";
	fx.ops.bumpGeneration();
	hl.highlightContent();

	const auto deadline =
		std::chrono::steady_clock::now() + std::chrono::milliseconds(3000);
	bool sawKeyword = false;
	while (std::chrono::steady_clock::now() < deadline)
	{
		hl.poll();
		for (const ColorSpan &s : hl.spansForLine(0))
		{
			if (s.slot == ThemeSlot::Keyword)
				sawKeyword = true;
		}
		if (sawKeyword)
			break;
		std::this_thread::sleep_for(std::chrono::milliseconds(5));
	}
	REQUIRE(sawKeyword);
}

TEST_CASE("startBackgroundPrewarm compiles shipped queries", "[ned][highlight]")
{
	TreeSitter::startBackgroundPrewarm();
	TreeSitter ts;
	const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(20);
	while (std::chrono::steady_clock::now() < deadline)
	{
		if (ts.queryReady("cpp") && ts.queryReady("json") && ts.queryReady("py"))
			break;
		std::this_thread::sleep_for(std::chrono::milliseconds(20));
	}
	REQUIRE(ts.queryReady("cpp"));
	REQUIRE(ts.queryReady("json"));
	REQUIRE(ts.queryReady("py"));
}

TEST_CASE("empty buffer stays spannless", "[ned][highlight]")
{
	test::EditorFixture fx;
	fx.setDocument("", "empty.c");
	fx.state.languageId = "c";

	EditorHighlight hl(fx.state, fx.ops);
	hl.resetForDocument(static_cast<size_t>(fx.state.lineCount()));
	hl.highlightContent();
	hl.poll();

	REQUIRE(hl.spansForLine(0).empty());
	REQUIRE(hl.spansForLine(1).empty());
}
