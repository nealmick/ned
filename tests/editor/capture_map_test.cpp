#include "editor/services/highlight/capture_map.h"
#include "third_party/catch.hpp"

TEST_CASE("capturePriority: specific roles beat bare variable", "[ned][highlight]")
{
	REQUIRE(capturePriority("variable") < capturePriority("function"));
	REQUIRE(capturePriority("variable") < capturePriority("type"));
	REQUIRE(capturePriority("variable") < capturePriority("keyword"));
	REQUIRE(capturePriority("property") < capturePriority("function"));
	REQUIRE(capturePriority("function") <= capturePriority("function.builtin"));
	REQUIRE(capturePriority("punctuation.bracket") < capturePriority("function"));
}

TEST_CASE("themeKeyForCapture hierarchical map", "[ned][highlight]")
{
	REQUIRE(std::string(themeKeyForCapture("keyword")) == "keyword");
	REQUIRE(std::string(themeKeyForCapture("keyword.import")) == "keyword");
	REQUIRE(std::string(themeKeyForCapture("function")) == "function");
	REQUIRE(std::string(themeKeyForCapture("function.method")) == "function");
	REQUIRE(std::string(themeKeyForCapture("function.method.call")) == "function");
	REQUIRE(std::string(themeKeyForCapture("type.builtin")) == "special");
	REQUIRE(std::string(themeKeyForCapture("string.escape")) == "special");
	REQUIRE(std::string(themeKeyForCapture("comment.documentation")) == "comment");
	REQUIRE(std::string(themeKeyForCapture("variable.parameter")) == "parameter");
	REQUIRE(std::string(themeKeyForCapture("variable.member")) == "property");
	REQUIRE(std::string(themeKeyForCapture("property")) == "property");
	REQUIRE(std::string(themeKeyForCapture("constructor")) == "type");
	REQUIRE(std::string(themeKeyForCapture("constant.builtin")) == "special");
	REQUIRE(capturePriority("function.macro") > capturePriority("keyword.exception"));
	REQUIRE(std::string(themeKeyForCapture("constant")) == "constant");
	REQUIRE(std::string(themeKeyForCapture("operator")) == "operator");
	REQUIRE(std::string(themeKeyForCapture("punctuation.bracket")) == "punctuation");
	REQUIRE(std::string(themeKeyForCapture("tag")) == "type");
	REQUIRE(std::string(themeKeyForCapture("unknown.capture.xyz")) == "text");
	REQUIRE(std::string(themeKeyForCapture("default")) == "text");
}
