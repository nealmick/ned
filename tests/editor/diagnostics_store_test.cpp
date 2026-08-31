#include "services/diagnostics/diagnostics_store.h"
#include "third_party/catch.hpp"

TEST_CASE("LSPDiagnostics replace and query by line", "[ned][lsp]")
{
	LSPDiagnostics store;
	DiagnosticItem err;
	err.startLine = 2;
	err.endLine = 2;
	err.startCharacter = 0;
	err.endCharacter = 4;
	err.severity = 1;
	err.message = "undeclared";
	err.source = "clang";

	DiagnosticItem warn;
	warn.startLine = 2;
	warn.endLine = 3;
	warn.severity = 2;
	warn.message = "unused";

	store.replace("/tmp/a.cpp", {err, warn});
	const auto sev = store.maxSeverityByLine("/tmp/a.cpp", 5);
	REQUIRE(sev.size() == 5);
	REQUIRE(sev[2] == 1);
	REQUIRE(sev[3] == 2);
	REQUIRE(sev[0] == 0);
	REQUIRE(store.forLine("/tmp/a.cpp", 2).size() == 2);

	store.clear("/tmp/a.cpp");
	REQUIRE(store.maxSeverityByLine("/tmp/a.cpp", 5)[2] == 0);
}

TEST_CASE("LSPDiagnostics rejects stale versions", "[ned][lsp]")
{
	LSPDiagnostics store;
	DiagnosticItem err;
	err.message = "v2";
	store.replace("/tmp/stale.cpp", {err}, 2);
	REQUIRE(store.forLine("/tmp/stale.cpp", 0).size() == 1);

	DiagnosticItem older;
	older.message = "v1";
	store.replace("/tmp/stale.cpp", {older}, 1); // slow server, older version
	const auto items = store.forLine("/tmp/stale.cpp", 0);
	REQUIRE(items.size() == 1);
	REQUIRE(items[0].message == "v2");

	DiagnosticItem newer;
	newer.message = "v3";
	store.replace("/tmp/stale.cpp", {newer}, 3);
	REQUIRE(store.forLine("/tmp/stale.cpp", 0)[0].message == "v3");
}

TEST_CASE("LSPDiagnostics version-less publishes do not block updates", "[ned][lsp]")
{
	LSPDiagnostics store;
	DiagnosticItem err;
	err.message = "anon";
	store.replace("/tmp/anon.cpp", {err}); // server omitted version
	REQUIRE(store.forLine("/tmp/anon.cpp", 0).size() == 1);

	DiagnosticItem next;
	next.message = "v1";
	store.replace("/tmp/anon.cpp", {next}, 1);
	REQUIRE(store.forLine("/tmp/anon.cpp", 0)[0].message == "v1");
}
