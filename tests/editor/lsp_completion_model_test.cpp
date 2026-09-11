#include "lsp/lsp_completion_model.h"

#include "third_party/catch.hpp"

#include <string>

namespace {

LSPCompletionEntry
entry(std::string label, std::string sortText = "", std::string filterText = "")
{
	LSPCompletionEntry e;
	e.label = std::move(label);
	e.insertText = e.label;
	e.sortText = std::move(sortText);
	e.filterText = std::move(filterText);
	return e;
}

} // namespace

TEST_CASE("completion word start tracks identifier bytes", "[ned][lsp_completion]")
{
	REQUIRE(lspCompletionWordStart("hello", 5) == 0);
	REQUIRE(lspCompletionWordStart("hello", 2) == 0);
	REQUIRE(lspCompletionWordStart("foo bar", 7) == 4);
	REQUIRE(lspCompletionWordStart("foo bar", 3) == 0);
	// Separators end the word both ways: after '.', the word starts fresh.
	REQUIRE(lspCompletionWordStart("obj.member", 10) == 4);
	REQUIRE(lspCompletionWordStart("a::b", 4) == 3);
	// Unicode identifier bytes stay one word.
	REQUIRE(lspCompletionWordStart("caf\xc3\xa9_", 6) == 0);
	// Nothing typed after a separator: word is empty at the caret.
	REQUIRE(lspCompletionWordStart("obj.", 4) == 4);
}

TEST_CASE("completion short label strips scope qualifiers for display",
		  "[ned][lsp_completion]")
{
	REQUIRE(lspCompletionShortLabel("ned::EditorState") == "EditorState");
	REQUIRE(lspCompletionShortLabel("a::b::c::method") == "method");
	// Dots are NOT scope separators: file paths and member-style labels
	// render whole.
	REQUIRE(lspCompletionShortLabel("CMakeLists.txt") == "CMakeLists.txt");
	REQUIRE(lspCompletionShortLabel("self.field") == "self.field");
	REQUIRE(lspCompletionShortLabel("obj.member") == "obj.member");
	// Plain labels pass through untouched.
	REQUIRE(lspCompletionShortLabel("completionList") == "completionList");
	// Trailing separator keeps everything before it (don't return empty).
	REQUIRE(lspCompletionShortLabel("std::") == "std::");
}

TEST_CASE("completion filter ranks prefix over boundary over subsequence",
		  "[ned][lsp_completion]")
{
	std::vector<LSPCompletionEntry> items{
		entry("getConfigColumn"), // subsequence "gcc"
		entry("someCompletion"),  // boundary "comp"
		entry("completionList"),  // prefix "comp"
		entry("compiler"),		  // prefix "comp"
	};

	const auto gcc = lspFilterCompletionItems(items, "gcc");
	REQUIRE(gcc.size() == 1);
	REQUIRE(items[gcc[0]].label == "getConfigColumn");

	const auto comp = lspFilterCompletionItems(items, "comp");
	// "getConfigColumn" has no c-o-m-p subsequence after the boundary 'C'
	// (no 'm' follows), so it drops out entirely.
	REQUIRE(comp.size() == 3);
	// Rank 0 (prefix) first, ordered by label among equals ("compiler" <
	// "completionList" — 'i' sorts before 'l').
	REQUIRE(items[comp[0]].label == "compiler");
	REQUIRE(items[comp[1]].label == "completionList");
	// Rank 1 (boundary) last of the matches.
	REQUIRE(items[comp[2]].label == "someCompletion");
}

TEST_CASE("completion filter is case-insensitive and honors filter/sort text",
		  "[ned][lsp_completion]")
{
	std::vector<LSPCompletionEntry> items{
		entry("InputStream", "2"),	// rank 0, sortText 2
		entry("outputStream", "1"), // boundary match "str", sortText 1
		entry("nothing_here"),
	};

	const auto str = lspFilterCompletionItems(items, "Str");
	REQUIRE(str.size() == 2);
	REQUIRE(items[str[0]].label == "outputStream"); // lower rank wins over sortText
	REQUIRE(items[str[1]].label == "InputStream");

	// filterText overrides the label for matching.
	std::vector<LSPCompletionEntry> filtered{
		entry("m", "", "myVeryLongServerLabel"),
	};
	REQUIRE(lspFilterCompletionItems(filtered, "myVer").size() == 1);

	// Empty prefix matches everything (manual trigger pre-filter).
	REQUIRE(lspFilterCompletionItems(items, "").size() == 3);
	// No match drops the item.
	REQUIRE(lspFilterCompletionItems(items, "zzz").empty());
}

TEST_CASE("completion filter matches the SHORT (visible) label", "[ned][lsp_completion]")
{
	std::vector<LSPCompletionEntry> items{
		entry("std::vector"),
		entry("ned::EditorState"),
		entry("stateMachine"),
	};

	// "st" must rank the visibly-st-prefixed item first; "EditorState"
	// still fuzzy-matches as a subsequence (rank 2), which is wanted —
	// but "std::vector" (visible "vector") must NOT appear.
	const auto st = lspFilterCompletionItems(items, "st");
	REQUIRE(st.size() == 2);
	REQUIRE(items[st[0]].label == "stateMachine");
	REQUIRE(items[st[1]].label == "ned::EditorState");

	// Filtering by the scope prefix finds nothing — the user types the
	// bare name after "::".
	REQUIRE(lspFilterCompletionItems(items, "std").empty());

	// After "::" the word restarts: "Edi" finds the class by its short name.
	REQUIRE(lspFilterCompletionItems(items, "Edi").size() == 1);
}
