/*
	UTF-8 helper unit tests (EditorUtils) — encode path used by character input.
*/

#include "editor/util/editor_utils.h"
#include "third_party/catch.hpp"

#include <string>

static std::string u8s(const char8_t *s)
{
	return std::string(reinterpret_cast<const char *>(s));
}

TEST_CASE("AppendUtf8Codepoint ASCII", "[ned][utf8]")
{
	std::string out;
	REQUIRE(EditorUtils::AppendUtf8Codepoint(out, 'A') == 1);
	REQUIRE(out == "A");
}

TEST_CASE("AppendUtf8Codepoint 2-byte (é U+00E9)", "[ned][utf8]")
{
	std::string out;
	REQUIRE(EditorUtils::AppendUtf8Codepoint(out, 0x00E9) == 2);
	REQUIRE(out == u8s(u8"é"));
}

TEST_CASE("AppendUtf8Codepoint 3-byte (€ U+20AC)", "[ned][utf8]")
{
	std::string out;
	REQUIRE(EditorUtils::AppendUtf8Codepoint(out, 0x20AC) == 3);
	REQUIRE(out == u8s(u8"€"));
}

TEST_CASE("AppendUtf8Codepoint 4-byte emoji", "[ned][utf8]")
{
	std::string out;
	// U+1F4DA 📚
	REQUIRE(EditorUtils::AppendUtf8Codepoint(out, 0x1F4DA) == 4);
	REQUIRE(out == u8s(u8"📚"));
}

TEST_CASE("AppendUtf8Codepoint rejects invalid", "[ned][utf8]")
{
	std::string out;
	REQUIRE(EditorUtils::AppendUtf8Codepoint(out, 0x110000) == 0);
	REQUIRE(out.empty());
}

TEST_CASE("SnapToUtf8CharBoundary mid-sequence", "[ned][utf8]")
{
	const std::string s = "a" + u8s(u8"📚") + "b";
	// Bytes: a | F0 9F 93 9A | b
	REQUIRE(EditorUtils::SnapToUtf8CharBoundary(s, 0) == 0);
	REQUIRE(EditorUtils::SnapToUtf8CharBoundary(s, 1) == 1);
	REQUIRE(EditorUtils::SnapToUtf8CharBoundary(s, 2) == 1);
	REQUIRE(EditorUtils::SnapToUtf8CharBoundary(s, 3) == 1);
	REQUIRE(EditorUtils::SnapToUtf8CharBoundary(s, 4) == 1);
	REQUIRE(EditorUtils::SnapToUtf8CharBoundary(s, 5) == 5);
}
