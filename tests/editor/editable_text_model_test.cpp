/*
	applyEdits suite: multi-op inserts/deletes and undo invert path.
	Fixtures use LF-joined lines unless noted.
*/
#include "monaco/text_model.h"
#include "third_party/catch.hpp"

using monaco::editOp;
using monaco::SingleEditOperation;
using monaco::TextModel;

namespace {

void testApplyEditsWithSyncedModels(const std::vector<std::string> &original,
									const std::vector<SingleEditOperation> &edits,
									const std::vector<std::string> &expected)
{
	auto model = TextModel::createFromLines(original);
	model.setEOL("\n");

	const auto inverse = model.applyEdits(edits, true);

	REQUIRE(model.getLineCount() == static_cast<int>(expected.size()));
	for (size_t i = 0; i < expected.size(); ++i)
		REQUIRE(model.getLineContent(static_cast<int>(i) + 1) == expected[i]);

	// Apply inverse → back to original
	model.applyEdits(inverse, false);
	REQUIRE(model.getLineCount() == static_cast<int>(original.size()));
	for (size_t i = 0; i < original.size(); ++i)
		REQUIRE(model.getLineContent(static_cast<int>(i) + 1) == original[i]);
}

} // namespace

TEST_CASE("applyEdits insert empty text", "[monaco][applyEdits]")
{
	testApplyEditsWithSyncedModels(
		{"My First Line", "\t\tMy Second Line", "    Third Line", "", "1"},
		{editOp(1, 1, 1, 1, {""})},
		{"My First Line", "\t\tMy Second Line", "    Third Line", "", "1"});
}

TEST_CASE("applyEdits insert text without newline 1", "[monaco][applyEdits]")
{
	testApplyEditsWithSyncedModels(
		{"My First Line", "\t\tMy Second Line", "    Third Line", "", "1"},
		{editOp(1, 1, 1, 1, {"foo "})},
		{"foo My First Line", "\t\tMy Second Line", "    Third Line", "", "1"});
}

TEST_CASE("applyEdits insert text without newline 2", "[monaco][applyEdits]")
{
	testApplyEditsWithSyncedModels(
		{"My First Line", "\t\tMy Second Line", "    Third Line", "", "1"},
		{editOp(1, 3, 1, 3, {" foo"})},
		{"My foo First Line", "\t\tMy Second Line", "    Third Line", "", "1"});
}

TEST_CASE("applyEdits insert one newline", "[monaco][applyEdits]")
{
	testApplyEditsWithSyncedModels(
		{"My First Line", "\t\tMy Second Line", "    Third Line", "", "1"},
		{editOp(1, 4, 1, 4, {"", ""})},
		{"My ", "First Line", "\t\tMy Second Line", "    Third Line", "", "1"});
}

TEST_CASE("applyEdits insert text with one newline", "[monaco][applyEdits]")
{
	testApplyEditsWithSyncedModels(
		{"My First Line", "\t\tMy Second Line", "    Third Line", "", "1"},
		{editOp(1, 3, 1, 3, {" new line", "No longer"})},
		{"My new line",
		 "No longer First Line",
		 "\t\tMy Second Line",
		 "    Third Line",
		 "",
		 "1"});
}

TEST_CASE("applyEdits insert text with two newlines", "[monaco][applyEdits]")
{
	testApplyEditsWithSyncedModels(
		{"My First Line", "\t\tMy Second Line", "    Third Line", "", "1"},
		{editOp(1, 3, 1, 3, {" new line", "One more line in the middle", "No longer"})},
		{"My new line",
		 "One more line in the middle",
		 "No longer First Line",
		 "\t\tMy Second Line",
		 "    Third Line",
		 "",
		 "1"});
}

TEST_CASE("applyEdits insert text with many newlines", "[monaco][applyEdits]")
{
	testApplyEditsWithSyncedModels(
		{"My First Line", "\t\tMy Second Line", "    Third Line", "", "1"},
		{editOp(1, 3, 1, 3, {"", "", "", "", ""})},
		{"My", "", "", "", " First Line", "\t\tMy Second Line", "    Third Line", "", "1"});
}

TEST_CASE("applyEdits delete text from one line", "[monaco][applyEdits]")
{
	testApplyEditsWithSyncedModels(
		{"My First Line", "\t\tMy Second Line", "    Third Line", "", "1"},
		{editOp(1, 1, 1, 2, {""})},
		{"y First Line", "\t\tMy Second Line", "    Third Line", "", "1"});
}

TEST_CASE("applyEdits delete text from one line 2", "[monaco][applyEdits]")
{
	testApplyEditsWithSyncedModels(
		{"My First Line", "\t\tMy Second Line", "    Third Line", "", "1"},
		{editOp(1, 1, 1, 3, {"a"})},
		{"a First Line", "\t\tMy Second Line", "    Third Line", "", "1"});
}

TEST_CASE("applyEdits delete all text from a line", "[monaco][applyEdits]")
{
	testApplyEditsWithSyncedModels(
		{"My First Line", "\t\tMy Second Line", "    Third Line", "", "1"},
		{editOp(1, 1, 1, 14, {""})},
		{"", "\t\tMy Second Line", "    Third Line", "", "1"});
}

TEST_CASE("applyEdits delete text from two lines", "[monaco][applyEdits]")
{
	testApplyEditsWithSyncedModels(
		{"My First Line", "\t\tMy Second Line", "    Third Line", "", "1"},
		{editOp(1, 4, 2, 6, {""})},
		{"My Second Line", "    Third Line", "", "1"});
}

TEST_CASE("applyEdits delete text from many lines", "[monaco][applyEdits]")
{
	testApplyEditsWithSyncedModels(
		{"My First Line", "\t\tMy Second Line", "    Third Line", "", "1"},
		{editOp(1, 4, 3, 5, {""})},
		{"My Third Line", "", "1"});
}

TEST_CASE("applyEdits delete everything", "[monaco][applyEdits]")
{
	testApplyEditsWithSyncedModels(
		{"My First Line", "\t\tMy Second Line", "    Third Line", "", "1"},
		{editOp(1, 1, 5, 2, {""})},
		{""});
}

TEST_CASE("applyEdits two unrelated edits", "[monaco][applyEdits]")
{
	testApplyEditsWithSyncedModels(
		{"My First Line", "\t\tMy Second Line", "    Third Line", "", "123"},
		{editOp(2, 1, 2, 3, {"\t"}), editOp(3, 1, 3, 5, {""})},
		{"My First Line", "\tMy Second Line", "Third Line", "", "123"});
}

TEST_CASE("applyEdits two edits on one line", "[monaco][applyEdits]")
{
	testApplyEditsWithSyncedModels({"\t\tfirst\t    ",
									"\t\tsecond line",
									"\tthird line",
									"fourth line",
									"\t\t<!@#fifth#@!>\t\t"},
								   {editOp(5, 3, 5, 7, {""}), editOp(5, 12, 5, 16, {""})},
								   {"\t\tfirst\t    ",
									"\t\tsecond line",
									"\tthird line",
									"fourth line",
									"\t\tfifth\t\t"});
}

TEST_CASE("applyEdits Bug 19872 Undo is funky", "[monaco][applyEdits]")
{
	testApplyEditsWithSyncedModels({"something", " A", "", " B", "something else"},
								   {editOp(2, 1, 2, 2, {""}), editOp(3, 1, 4, 2, {""})},
								   {"something", "A", "B", "something else"});
}

TEST_CASE("applyEdits Bug 19872 Undo is funky (2)", "[monaco][applyEdits]")
{
	testApplyEditsWithSyncedModels(
		{"something", "A", "B", "something else"},
		{editOp(2, 1, 2, 1, {" "}), editOp(3, 1, 3, 1, {"", " "})},
		{"something", " A", "", " B", "something else"});
}

TEST_CASE("applyEdits last op is no-op", "[monaco][applyEdits]")
{
	testApplyEditsWithSyncedModels(
		{"My First Line", "\t\tMy Second Line", "    Third Line", "", "1"},
		{editOp(1, 1, 1, 2, {""}), editOp(4, 1, 4, 1, {""})},
		{"y First Line", "\t\tMy Second Line", "    Third Line", "", "1"});
}

TEST_CASE("applyEdits many edits", "[monaco][applyEdits]")
{
	// Edits use original coordinates; apply bottom-up for non-overlapping ops.
	testApplyEditsWithSyncedModels({"{\"x\" : 1}"},
								   {editOp(1, 2, 1, 2, {"", "  "}),
									editOp(1, 5, 1, 6, {""}),
									editOp(1, 9, 1, 9, {"", ""})},
								   {"{", "  \"x\": 1", "}"});
}

TEST_CASE("applyEdits many edits reversed", "[monaco][applyEdits]")
{
	testApplyEditsWithSyncedModels(
		{"{", "  \"x\": 1", "}"},
		{editOp(1, 2, 2, 3, {""}), editOp(2, 6, 2, 6, {" "}), editOp(2, 9, 3, 1, {""})},
		{"{\"x\" : 1}"});
}

TEST_CASE("applyEdits high-low surrogates / multi-byte UTF-8 1",
		  "[monaco][applyEdits][unicode]")
{
	// Insert before emoji: column 2 is the start of the multi-byte sequence.
	testApplyEditsWithSyncedModels({"📚some", "very nice", "text"},
								   {editOp(1, 2, 1, 2, {"a"})},
								   {"a📚some", "very nice", "text"});
}

TEST_CASE("applyEdits high-low surrogates / multi-byte UTF-8 2",
		  "[monaco][applyEdits][unicode]")
{
	// Replace whole emoji at line start: UTF-8 cols 1..5 (4-byte emoji, col 5 = 's').
	// editOp(1,1,1,3) replaces the first multi-byte character.
	testApplyEditsWithSyncedModels({"📚some", "very nice", "text"},
								   {editOp(1, 1, 1, 5, {"a"})},
								   {"asome", "very nice", "text"});
}

TEST_CASE("applyEdits issue #47733 Undo mangles unicode", "[monaco][applyEdits][unicode]")
{
	testApplyEditsWithSyncedModels({"'👁'"}, {editOp(1, 1, 1, 1, {"a"})}, {"a'👁'"});
}
