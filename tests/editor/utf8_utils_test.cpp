/*
	UTF-8 helper unit tests (EditorUtils) — encode path used by character input.

	Expected sequences use hex escapes only (not u8"…" / source glyphs) so MSVC
	matches clang/gcc even if a TU is compiled without /utf-8.
*/

#include "editor/util/editor_utils.h"
#include "third_party/catch.hpp"

#include <string>

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
	// U+00E9 → C3 A9
	REQUIRE(out == std::string("\xC3\xA9", 2));
}

TEST_CASE("AppendUtf8Codepoint 3-byte (€ U+20AC)", "[ned][utf8]")
{
	std::string out;
	REQUIRE(EditorUtils::AppendUtf8Codepoint(out, 0x20AC) == 3);
	// U+20AC → E2 82 AC
	REQUIRE(out == std::string("\xE2\x82\xAC", 3));
}

TEST_CASE("AppendUtf8Codepoint 4-byte emoji", "[ned][utf8]")
{
	std::string out;
	// U+1F4DA 📚 → F0 9F 93 9A
	REQUIRE(EditorUtils::AppendUtf8Codepoint(out, 0x1F4DA) == 4);
	REQUIRE(out == std::string("\xF0\x9F\x93\x9A", 4));
}

TEST_CASE("AppendUtf8Codepoint rejects invalid", "[ned][utf8]")
{
	std::string out;
	REQUIRE(EditorUtils::AppendUtf8Codepoint(out, 0x110000) == 0);
	REQUIRE(out.empty());
}

TEST_CASE("SnapToUtf8CharBoundary mid-sequence", "[ned][utf8]")
{
	// a | F0 9F 93 9A | b
	const std::string s = "a" + std::string("\xF0\x9F\x93\x9A", 4) + "b";
	REQUIRE(EditorUtils::SnapToUtf8CharBoundary(s, 0) == 0);
	REQUIRE(EditorUtils::SnapToUtf8CharBoundary(s, 1) == 1);
	REQUIRE(EditorUtils::SnapToUtf8CharBoundary(s, 2) == 1);
	REQUIRE(EditorUtils::SnapToUtf8CharBoundary(s, 3) == 1);
	REQUIRE(EditorUtils::SnapToUtf8CharBoundary(s, 4) == 1);
	REQUIRE(EditorUtils::SnapToUtf8CharBoundary(s, 5) == 5);
}
