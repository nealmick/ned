/*
	Compile every shipped highlights query against its tree-sitter language.
	Catches grammar/query skew before runtime "Query error at offset…".
*/
#include "third_party/catch.hpp"

#include <filesystem>
#include <fstream>
#include <string>
#include <tree_sitter/api.h>
#include <vector>

extern "C" {
TSLanguage *tree_sitter_c();
TSLanguage *tree_sitter_cpp();
TSLanguage *tree_sitter_python();
TSLanguage *tree_sitter_javascript();
TSLanguage *tree_sitter_tsx();
TSLanguage *tree_sitter_css();
TSLanguage *tree_sitter_html();
TSLanguage *tree_sitter_java();
TSLanguage *tree_sitter_go();
TSLanguage *tree_sitter_json();
TSLanguage *tree_sitter_bash();
TSLanguage *tree_sitter_rust();
TSLanguage *tree_sitter_toml();
TSLanguage *tree_sitter_ruby();
TSLanguage *tree_sitter_c_sharp();
TSLanguage *tree_sitter_kotlin();
TSLanguage *tree_sitter_hcl();
}

namespace {

std::filesystem::path queryDir()
{
	namespace fs = std::filesystem;
	const fs::path candidates[] = {
		fs::path(CMAKE_SOURCE_DIR) / "editor" / "services" / "highlight" / "queries",
		fs::current_path() / "editor" / "services" / "highlight" / "queries",
		fs::current_path().parent_path() / "editor" / "services" / "highlight" / "queries",
	};
	for (const auto &p : candidates)
	{
		if (fs::is_directory(p))
			return p;
	}
	return candidates[0];
}

std::string readFile(const std::filesystem::path &p)
{
	std::ifstream in(p);
	REQUIRE(in.good());
	return std::string((std::istreambuf_iterator<char>(in)),
					   std::istreambuf_iterator<char>());
}

struct LangQuery
{
	const char *scm;
	TSLanguage *(*lang)();
};

} // namespace

#ifndef CMAKE_SOURCE_DIR
#define CMAKE_SOURCE_DIR "."
#endif

TEST_CASE("highlight queries compile for all languages", "[ned][highlight][queries]")
{
	const auto dir = queryDir();
	REQUIRE(std::filesystem::is_directory(dir));

	const LangQuery entries[] = {
		{"c.scm", tree_sitter_c},
		{"cpp.scm", tree_sitter_cpp},
		{"python.scm", tree_sitter_python},
		{"jsx.scm", tree_sitter_javascript},
		{"tsx.scm", tree_sitter_tsx},
		{"css.scm", tree_sitter_css},
		{"html.scm", tree_sitter_html},
		{"java.scm", tree_sitter_java},
		{"go.scm", tree_sitter_go},
		{"json.scm", tree_sitter_json},
		{"sh.scm", tree_sitter_bash},
		{"rs.scm", tree_sitter_rust},
		{"toml.scm", tree_sitter_toml},
		{"rb.scm", tree_sitter_ruby},
		{"csharp.scm", tree_sitter_c_sharp},
		{"kotlin.scm", tree_sitter_kotlin},
		{"hcl.scm", tree_sitter_hcl},
	};

	for (const LangQuery &e : entries)
	{
		INFO("query file: " << e.scm);
		const auto path = dir / e.scm;
		REQUIRE(std::filesystem::exists(path));
		const std::string src = readFile(path);
		REQUIRE_FALSE(src.empty());

		TSLanguage *lang = e.lang();
		REQUIRE(lang != nullptr);

		uint32_t errOff = 0;
		TSQueryError errType{};
		TSQuery *q = ts_query_new(lang, src.c_str(), src.size(), &errOff, &errType);
		if (!q)
		{
			FAIL("ts_query_new failed for " << e.scm
											<< " error_type=" << static_cast<int>(errType)
											<< " offset=" << errOff);
		}
		ts_query_delete(q);
	}
}
