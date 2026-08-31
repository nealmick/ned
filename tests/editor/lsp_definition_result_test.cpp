#include "lsp/types.h"
#include <lsp/json/json.h>
#include <lsp/serialization.h>

#include "third_party/catch.hpp"

TEST_CASE("Definition result accepts an array of normal Locations", "[ned][lsp]")
{
	auto payload = lsp::json::parse(R"([
        {
            "uri": "file:///tmp/example.cpp",
            "range": {
                "start": { "line": 3, "character": 5 },
                "end": { "line": 3, "character": 8 }
            }
        }
    ])");

	lsp::TextDocument_DefinitionResult result;
	lsp::fromJson(std::move(payload), result);

	REQUIRE_FALSE(result.isNull());
	REQUIRE(std::holds_alternative<lsp::Definition>(result.value()));
	const auto &definition = std::get<lsp::Definition>(result.value());
	REQUIRE(std::holds_alternative<lsp::Array<lsp::Location>>(definition));
	const auto &locations = std::get<lsp::Array<lsp::Location>>(definition);
	REQUIRE(locations.size() == 1);
	REQUIRE(locations.front().range.start.line == 3);
}
