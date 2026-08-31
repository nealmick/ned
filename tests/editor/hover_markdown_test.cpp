#include "third_party/catch.hpp"
#include "views/hover_markdown.h"

TEST_CASE("ParseHoverMarkdown splits fenced code from prose", "[ned][lsp][hover]")
{
	const std::string src = "hello `int`\n```cpp\nint x;\n```\nworld";
	const auto blocks = ParseHoverMarkdown(src);
	REQUIRE(blocks.size() >= 2);
	bool sawCode = false;
	bool sawProse = false;
	for (const auto &b : blocks)
	{
		if (b.code)
		{
			sawCode = true;
			REQUIRE(b.language == "cpp");
			REQUIRE(b.text.find("int x;") != std::string::npos);
		} else if (b.text.find("hello") != std::string::npos)
			sawProse = true;
	}
	REQUIRE(sawCode);
	REQUIRE(sawProse);
}

TEST_CASE("ParseHoverMarkdown treats --- as a rule", "[ned][lsp][hover]")
{
	const auto blocks = ParseHoverMarkdown("a\n---\nb");
	REQUIRE(blocks.size() == 3);
	REQUIRE(blocks[1].text == "---");
	REQUIRE_FALSE(blocks[1].code);
}

TEST_CASE("ParseHoverMarkdown trims fence-adjacent blank lines but keeps paragraphs",
		  "[ned][lsp][hover]")
{
	const auto blocks = ParseHoverMarkdown(
		"```cpp\nint foo();\n```\n\nFirst paragraph.\n\nSecond paragraph.\n");
	REQUIRE(blocks.size() == 2);
	REQUIRE(blocks[0].code);
	REQUIRE(blocks[1].text == "First paragraph.\n\nSecond paragraph.");
}

TEST_CASE("SplitHoverLines handles terminators and CR", "[ned][lsp][hover]")
{
	const auto lines = SplitHoverLines("a\r\nb\nc");
	REQUIRE(lines.size() == 3);
	REQUIRE(lines[0] == "a");
	REQUIRE(lines[1] == "b");
	REQUIRE(lines[2] == "c");
	REQUIRE(SplitHoverLines("").size() == 1);
	REQUIRE(SplitHoverLines("x\n").size() == 2);
	REQUIRE(SplitHoverLines("x\n")[1].empty());
}
