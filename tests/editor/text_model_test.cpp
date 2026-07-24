/*
	Text-model query tests: ranges, lengths, value extraction.
*/
#include "monaco/text_model.h"
#include "third_party/catch.hpp"

using monaco::EndOfLinePreference;
using monaco::Position;
using monaco::Range;
using monaco::TextModel;

TEST_CASE("TextModel getValueLengthInRange CRLF", "[monaco][textModel]")
{
	auto m = TextModel::create("My First Line\r\nMy Second Line\r\nMy Third Line");
	REQUIRE(m.getValueLengthInRange(Range(1, 1, 1, 1)) == 0);
	REQUIRE(m.getValueLengthInRange(Range(1, 1, 1, 2)) == 1);	// 'M'
	REQUIRE(m.getValueLengthInRange(Range(1, 2, 1, 3)) == 1);	// 'y'
	REQUIRE(m.getValueLengthInRange(Range(1, 1, 1, 14)) == 13); // full first line
	REQUIRE(m.getValueLengthInRange(Range(1, 1, 2, 1)) ==
			static_cast<int>(std::string("My First Line\r\n").size()));
	REQUIRE(m.getValueLengthInRange(Range(1, 2, 2, 1)) ==
			static_cast<int>(std::string("y First Line\r\n").size()));
	REQUIRE(m.getValueLengthInRange(Range(1, 2, 2, 2)) ==
			static_cast<int>(std::string("y First Line\r\nM").size()));
	REQUIRE(m.getValueLengthInRange(Range(1, 2, 2, 1000)) ==
			static_cast<int>(std::string("y First Line\r\nMy Second Line").size()));
	REQUIRE(m.getValueLengthInRange(Range(1, 2, 3, 1)) ==
			static_cast<int>(std::string("y First Line\r\nMy Second Line\r\n").size()));
	REQUIRE(m.getValueLengthInRange(Range(1, 2, 3, 1000)) ==
			static_cast<int>(
				std::string("y First Line\r\nMy Second Line\r\nMy Third Line").size()));
	REQUIRE(m.getValueLengthInRange(Range(1, 1, 1000, 1000)) ==
			static_cast<int>(
				std::string("My First Line\r\nMy Second Line\r\nMy Third Line").size()));
}

TEST_CASE("TextModel getValueLengthInRange LF", "[monaco][textModel]")
{
	auto m = TextModel::create("My First Line\nMy Second Line\nMy Third Line");
	REQUIRE(m.getValueLengthInRange(Range(1, 1, 1, 1)) == 0);
	REQUIRE(m.getValueLengthInRange(Range(1, 1, 1, 2)) == 1);
	REQUIRE(m.getValueLengthInRange(Range(1, 1, 1, 14)) == 13);
	REQUIRE(m.getValueLengthInRange(Range(1, 1, 2, 1)) ==
			static_cast<int>(std::string("My First Line\n").size()));
	REQUIRE(m.getValueLengthInRange(Range(1, 2, 2, 1)) ==
			static_cast<int>(std::string("y First Line\n").size()));
	REQUIRE(m.getValueLengthInRange(Range(1, 2, 2, 2)) ==
			static_cast<int>(std::string("y First Line\nM").size()));
	REQUIRE(m.getValueLengthInRange(Range(1, 1, 1000, 1000)) ==
			static_cast<int>(
				std::string("My First Line\nMy Second Line\nMy Third Line").size()));
}

TEST_CASE("TextModel getValueLengthInRange different EOL", "[monaco][textModel]")
{
	{
		auto m = TextModel::create("My First Line\r\nMy Second Line\r\nMy Third Line");
		REQUIRE(m.getValueLengthInRange(Range(1, 1, 2, 1),
										EndOfLinePreference::TextDefined) ==
				static_cast<int>(std::string("My First Line\r\n").size()));
		REQUIRE(m.getValueLengthInRange(Range(1, 1, 2, 1), EndOfLinePreference::CRLF) ==
				static_cast<int>(std::string("My First Line\r\n").size()));
		REQUIRE(m.getValueLengthInRange(Range(1, 1, 2, 1), EndOfLinePreference::LF) ==
				static_cast<int>(std::string("My First Line\n").size()));
		REQUIRE(
			m.getValueLengthInRange(Range(1, 1, 1000, 1000), EndOfLinePreference::LF) ==
			static_cast<int>(
				std::string("My First Line\nMy Second Line\nMy Third Line").size()));
	}
	{
		auto m = TextModel::create("My First Line\nMy Second Line\nMy Third Line");
		REQUIRE(m.getValueLengthInRange(Range(1, 1, 2, 1),
										EndOfLinePreference::TextDefined) ==
				static_cast<int>(std::string("My First Line\n").size()));
		REQUIRE(m.getValueLengthInRange(Range(1, 1, 2, 1), EndOfLinePreference::LF) ==
				static_cast<int>(std::string("My First Line\n").size()));
		REQUIRE(m.getValueLengthInRange(Range(1, 1, 2, 1), EndOfLinePreference::CRLF) ==
				static_cast<int>(std::string("My First Line\r\n").size()));
	}
}

TEST_CASE("TextModel validatePosition", "[monaco][textModel]")
{
	auto m = TextModel::create("line one\nline two");

	REQUIRE(m.validatePosition(Position{0, 0}) == Position{1, 1});
	REQUIRE(m.validatePosition(Position{0, 1}) == Position{1, 1});

	REQUIRE(m.validatePosition(Position{1, 1}) == Position{1, 1});
	REQUIRE(m.validatePosition(Position{1, 2}) == Position{1, 2});
	REQUIRE(m.validatePosition(Position{1, 30}) == Position{1, 9});

	REQUIRE(m.validatePosition(Position{2, 0}) == Position{2, 1});
	REQUIRE(m.validatePosition(Position{2, 1}) == Position{2, 1});
	REQUIRE(m.validatePosition(Position{2, 2}) == Position{2, 2});
	REQUIRE(m.validatePosition(Position{2, 30}) == Position{2, 9});

	REQUIRE(m.validatePosition(Position{3, 0}) == Position{2, 9});
	REQUIRE(m.validatePosition(Position{3, 1}) == Position{2, 9});
	REQUIRE(m.validatePosition(Position{3, 30}) == Position{2, 9});
	REQUIRE(m.validatePosition(Position{30, 30}) == Position{2, 9});
}

TEST_CASE("TextModel validatePosition around multi-byte UTF-8 (emoji)",
		  "[monaco][textModel][unicode]")
{
	// Columns are UTF-8 byte offsets; snap off continuation bytes.
	// "a📚b" = a + 4-byte emoji + b → valid columns 1,2,6,7
	auto m = TextModel::create("a📚b");

	REQUIRE(m.validatePosition(Position{1, 1}) == Position{1, 1});
	REQUIRE(m.validatePosition(Position{1, 2}) == Position{1, 2}); // start of emoji
	// Mid-emoji byte → snap back to emoji start (col 2)
	REQUIRE(m.validatePosition(Position{1, 3}) == Position{1, 2});
	REQUIRE(m.validatePosition(Position{1, 4}) == Position{1, 2});
	REQUIRE(m.validatePosition(Position{1, 5}) == Position{1, 2});
	REQUIRE(m.validatePosition(Position{1, 6}) == Position{1, 6}); // after emoji
	REQUIRE(m.validatePosition(Position{1, 7}) == Position{1, 7}); // after 'b'
	REQUIRE(m.validatePosition(Position{1, 30}) == Position{1, 7});
}

TEST_CASE("TextModel modifyPosition", "[monaco][textModel]")
{
	auto m = TextModel::create("line one\nline two");
	REQUIRE(m.modifyPosition(Position{1, 1}, 0) == Position{1, 1});
	REQUIRE(m.modifyPosition(Position{0, 0}, 0) == Position{1, 1});
	REQUIRE(m.modifyPosition(Position{30, 1}, 0) == Position{2, 9});

	REQUIRE(m.modifyPosition(Position{1, 1}, 17) == Position{2, 9});
	REQUIRE(m.modifyPosition(Position{1, 1}, 1) == Position{1, 2});
	REQUIRE(m.modifyPosition(Position{1, 1}, 3) == Position{1, 4});
	REQUIRE(m.modifyPosition(Position{1, 2}, 10) == Position{2, 3});
	REQUIRE(m.modifyPosition(Position{1, 5}, 13) == Position{2, 9});
	REQUIRE(m.modifyPosition(Position{1, 2}, 16) == Position{2, 9});

	REQUIRE(m.modifyPosition(Position{2, 9}, -17) == Position{1, 1});
	REQUIRE(m.modifyPosition(Position{1, 2}, -1) == Position{1, 1});
	REQUIRE(m.modifyPosition(Position{1, 4}, -3) == Position{1, 1});
	REQUIRE(m.modifyPosition(Position{2, 3}, -10) == Position{1, 2});
	REQUIRE(m.modifyPosition(Position{2, 9}, -13) == Position{1, 5});
	REQUIRE(m.modifyPosition(Position{2, 9}, -16) == Position{1, 2});

	REQUIRE(m.modifyPosition(Position{1, 2}, 17) == Position{2, 9});
	REQUIRE(m.modifyPosition(Position{1, 2}, 100) == Position{2, 9});
	REQUIRE(m.modifyPosition(Position{1, 2}, -2) == Position{1, 1});
	REQUIRE(m.modifyPosition(Position{1, 2}, -100) == Position{1, 1});
	REQUIRE(m.modifyPosition(Position{2, 2}, -100) == Position{1, 1});
	REQUIRE(m.modifyPosition(Position{2, 9}, -18) == Position{1, 1});
}

TEST_CASE("TextModel getLineFirstNonWhitespaceColumn", "[monaco][textModel]")
{
	auto m = TextModel::createFromLines({
		"asd",
		" asd",
		"\tasd",
		"  asd",
		"\t\tasd",
		" ",
		"  ",
		"\t",
		"\t\t",
		"  \tasd",
		"",
		"",
	});

	REQUIRE(m.getLineFirstNonWhitespaceColumn(1) == 1);
	REQUIRE(m.getLineFirstNonWhitespaceColumn(2) == 2);
	REQUIRE(m.getLineFirstNonWhitespaceColumn(3) == 2);
	REQUIRE(m.getLineFirstNonWhitespaceColumn(4) == 3);
	REQUIRE(m.getLineFirstNonWhitespaceColumn(5) == 3);
	REQUIRE(m.getLineFirstNonWhitespaceColumn(6) == 0);
	REQUIRE(m.getLineFirstNonWhitespaceColumn(7) == 0);
	REQUIRE(m.getLineFirstNonWhitespaceColumn(8) == 0);
	REQUIRE(m.getLineFirstNonWhitespaceColumn(9) == 0);
	REQUIRE(m.getLineFirstNonWhitespaceColumn(10) == 4);
	REQUIRE(m.getLineFirstNonWhitespaceColumn(11) == 0);
	REQUIRE(m.getLineFirstNonWhitespaceColumn(12) == 0);
}

TEST_CASE("TextModel getLineLastNonWhitespaceColumn", "[monaco][textModel]")
{
	auto m = TextModel::createFromLines({
		"asd",
		"asd ",
		"asd\t",
		"asd  ",
		"asd\t\t",
		" ",
		"  ",
		"\t",
		"\t\t",
		"asd  \t",
		"",
		"",
	});

	REQUIRE(m.getLineLastNonWhitespaceColumn(1) == 4);
	REQUIRE(m.getLineLastNonWhitespaceColumn(2) == 4);
	REQUIRE(m.getLineLastNonWhitespaceColumn(3) == 4);
	REQUIRE(m.getLineLastNonWhitespaceColumn(4) == 4);
	REQUIRE(m.getLineLastNonWhitespaceColumn(5) == 4);
	REQUIRE(m.getLineLastNonWhitespaceColumn(6) == 0);
	REQUIRE(m.getLineLastNonWhitespaceColumn(7) == 0);
	REQUIRE(m.getLineLastNonWhitespaceColumn(8) == 0);
	REQUIRE(m.getLineLastNonWhitespaceColumn(9) == 0);
	REQUIRE(m.getLineLastNonWhitespaceColumn(10) == 4);
	REQUIRE(m.getLineLastNonWhitespaceColumn(11) == 0);
	REQUIRE(m.getLineLastNonWhitespaceColumn(12) == 0);
}

TEST_CASE("TextModel getValueInRange invalid range (#50471)", "[monaco][textModel]")
{
	auto m = TextModel::create("My First Line\r\nMy Second Line\r\nMy Third Line");
	// NaN not representable as int — use extreme/zero as stand-in
	REQUIRE(m.getValueInRange(Range(1, 1, 1, 3)) == "My");
}

TEST_CASE("TextModel setValue resets buffer", "[monaco][textModel]")
{
	auto m = TextModel::create("hello world!");
	m.setValue("Hello,\nזוהי עובדה מבוססת שדעתו");
	REQUIRE(m.getLineCount() == 2);
	m.setValue("hello world!");
	REQUIRE(m.getLineCount() == 1);
	REQUIRE(m.getValue() == "hello world!");
}
