/*
	Load-path tests: fromString / EOL detection / line split.
*/
#include "monaco/text_model.h"
#include "third_party/catch.hpp"

using monaco::TextModel;

TEST_CASE("TextModelData.fromString — one line text", "[monaco][textModelData]")
{
	auto m = TextModel::create("Hello world!");
	const std::string eol = m.getEOL();
	REQUIRE((eol == "\n" || eol == "\r\n")); // platform if no EOL in text
	// No newline → platform EOL from EditorState
	REQUIRE(m.getLinesContent() == std::vector<std::string>{"Hello world!"});
}

TEST_CASE("TextModelData.fromString — multiline text", "[monaco][textModelData]")
{
	// Primary EOL is CRLF when mixed endings include CRLF.
	auto m = TextModel::create("Hello,\r\ndear friend\nHow\rare\r\nyou?");
	REQUIRE(m.getEOL() == "\r\n");
	REQUIRE(m.getLinesContent() ==
			std::vector<std::string>{"Hello,", "dear friend", "How", "are", "you?"});
}

TEST_CASE("TextModelData.fromString — Non Basic ASCII 1", "[monaco][textModelData]")
{
	auto m = TextModel::create("Hello,\nZürich");
	REQUIRE(m.getEOL() == "\n");
	REQUIRE(m.getLinesContent() == std::vector<std::string>{"Hello,", "Zürich"});
}

TEST_CASE("TextModelData.fromString — containsRTL 1", "[monaco][textModelData]")
{
	auto m = TextModel::create("Hello,\nזוהי עובדה מבוססת שדעתו");
	REQUIRE(m.getEOL() == "\n");
	REQUIRE(m.getLineCount() == 2);
	REQUIRE(m.getLineContent(1) == "Hello,");
	REQUIRE(m.getLineContent(2) == "זוהי עובדה מבוססת שדעתו");
}

TEST_CASE("TextModelData.fromString — containsRTL 2", "[monaco][textModelData]")
{
	auto m = TextModel::create("Hello,\nهناك حقيقة مثبتة منذ زمن طويل");
	REQUIRE(m.getEOL() == "\n");
	REQUIRE(m.getLineCount() == 2);
	REQUIRE(m.getLineContent(1) == "Hello,");
}
