/*
	Single-edit round-trips: apply edit, invert, re-apply, assert buffer.
*/
#include "monaco/text_model.h"
#include "third_party/catch.hpp"

using monaco::createSingleEditOp;
using monaco::SingleEditOperation;
using monaco::TextModel;

namespace {

const std::string LINE1 = "My First Line";
const std::string LINE2 = "\t\tMy Second Line";
const std::string LINE3 = "    Third Line";
const std::string LINE4 = "";
const std::string LINE5 = "1";

TextModel makeModel()
{
	// Mixed EOLs; primary becomes CRLF when CRLF is present.
	const std::string text =
		LINE1 + "\r\n" + LINE2 + "\n" + LINE3 + "\n" + LINE4 + "\r\n" + LINE5;
	return TextModel::create(text);
}

void assertSingleEditOp(TextModel &model,
						const SingleEditOperation &singleEditOp,
						const std::vector<std::string> &editedLines)
{
	const auto editOp = std::vector<SingleEditOperation>{singleEditOp};
	const auto inverseEditOp = model.applyEdits(editOp, true);

	REQUIRE(model.getLineCount() == static_cast<int>(editedLines.size()));
	for (size_t i = 0; i < editedLines.size(); ++i)
		REQUIRE(model.getLineContent(static_cast<int>(i) + 1) == editedLines[i]);

	const auto originalOp = model.applyEdits(inverseEditOp, true);

	REQUIRE(model.getLineCount() == 5);
	REQUIRE(model.getLineContent(1) == LINE1);
	REQUIRE(model.getLineContent(2) == LINE2);
	REQUIRE(model.getLineContent(3) == LINE3);
	REQUIRE(model.getLineContent(4) == LINE4);
	REQUIRE(model.getLineContent(5) == LINE5);

	// Round-trip: inverse-of-inverse recovers the forward edit
	// (EOL-normalized text; ranges compared in ordered form).
	REQUIRE(originalOp.size() == editOp.size());
	auto orderRange = [](monaco::Range r) {
		if (r.startLineNumber > r.endLineNumber ||
			(r.startLineNumber == r.endLineNumber && r.startColumn > r.endColumn))
		{
			std::swap(r.startLineNumber, r.endLineNumber);
			std::swap(r.startColumn, r.endColumn);
		}
		return r;
	};
	// Insert text may be LF in the test fixture but CRLF in a CRLF document.
	auto stripEOL = [](std::string s) {
		std::string out;
		for (char c : s)
			if (c != '\r')
				out += c;
		return out;
	};
	REQUIRE(stripEOL(originalOp[0].text) == stripEOL(editOp[0].text));
	REQUIRE(orderRange(originalOp[0].range) == orderRange(editOp[0].range));
}

} // namespace

TEST_CASE("Model Edit Operation — Insert inline", "[monaco][editOp]")
{
	auto model = makeModel();
	assertSingleEditOp(model,
					   createSingleEditOp("a", 1, 1),
					   {"aMy First Line", LINE2, LINE3, LINE4, LINE5});
}

TEST_CASE("Model Edit Operation — Replace inline/inline 1", "[monaco][editOp]")
{
	auto model = makeModel();
	assertSingleEditOp(model,
					   createSingleEditOp(" incredibly awesome", 1, 3),
					   {"My incredibly awesome First Line", LINE2, LINE3, LINE4, LINE5});
}

TEST_CASE("Model Edit Operation — Replace inline/inline 2", "[monaco][editOp]")
{
	auto model = makeModel();
	assertSingleEditOp(
		model,
		createSingleEditOp(" with text at the end.", 1, 14),
		{"My First Line with text at the end.", LINE2, LINE3, LINE4, LINE5});
}

TEST_CASE("Model Edit Operation — Replace inline/inline 3", "[monaco][editOp]")
{
	auto model = makeModel();
	assertSingleEditOp(model,
					   createSingleEditOp("My new First Line.", 1, 1, 1, 14),
					   {"My new First Line.", LINE2, LINE3, LINE4, LINE5});
}

TEST_CASE("Model Edit Operation — Replace inline/multi line 1", "[monaco][editOp]")
{
	auto model = makeModel();
	assertSingleEditOp(model,
					   createSingleEditOp("My new First Line.", 1, 1, 3, 15),
					   {"My new First Line.", LINE4, LINE5});
}

TEST_CASE("Model Edit Operation — Replace inline/multi line 2", "[monaco][editOp]")
{
	auto model = makeModel();
	assertSingleEditOp(model,
					   createSingleEditOp("My new First Line.", 1, 2, 3, 15),
					   {"MMy new First Line.", LINE4, LINE5});
}

TEST_CASE("Model Edit Operation — Replace inline/multi line 3", "[monaco][editOp]")
{
	auto model = makeModel();
	assertSingleEditOp(model,
					   createSingleEditOp("My new First Line.", 1, 2, 3, 2),
					   {"MMy new First Line.   Third Line", LINE4, LINE5});
}

TEST_CASE("Model Edit Operation — Replace multi line/multi line", "[monaco][editOp]")
{
	auto model = makeModel();
	assertSingleEditOp(model,
					   createSingleEditOp("1\n2\n3\n4\n", 1, 1),
					   {"1", "2", "3", "4", LINE1, LINE2, LINE3, LINE4, LINE5});
}
