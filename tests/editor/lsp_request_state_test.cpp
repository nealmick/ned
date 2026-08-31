#include "lsp/lsp_request.h"
#include "third_party/catch.hpp"

// LSPRequestState is the seam between the UI thread and LSP response
// callbacks (goto results, hover text). These tests pin the contract that
// the old ad-hoc pending/requestId code got wrong.
TEST_CASE("LSPRequestState empty result clears pending", "[ned][lsp]")
{
	LSPRequestState<std::vector<int>> state;
	const auto t = state.begin();
	REQUIRE(state.isPending());
	REQUIRE_FALSE(state.snapshot().has_value());

	state.deliver(t, std::vector<int>{}); // "no definitions found" is an answer
	REQUIRE_FALSE(state.isPending());
	REQUIRE(state.snapshot().has_value());
	REQUIRE(state.snapshot()->empty());
}

TEST_CASE("LSPRequestState drops stale deliveries", "[ned][lsp]")
{
	LSPRequestState<std::string> state;
	const auto first = state.begin();
	const auto second = state.begin(); // supersede before the first replies

	state.deliver(first, std::string("stale"));
	REQUIRE(state.isPending()); // stale reply must not clear pending

	state.deliver(second, std::string("fresh"));
	REQUIRE_FALSE(state.isPending());
	REQUIRE(*state.snapshot() == "fresh");
}

TEST_CASE("LSPRequestState cancel invalidates in-flight replies", "[ned][lsp]")
{
	LSPRequestState<std::string> state;
	const auto t = state.begin();
	state.cancel();
	REQUIRE_FALSE(state.isPending());
	REQUIRE_FALSE(state.snapshot().has_value());

	state.deliver(t, std::string("late"));
	REQUIRE_FALSE(state.snapshot().has_value()); // dropped, not resurrected
}
