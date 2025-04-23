#include "editor_tree_sitter.h"
#include "editor.h"
#include <fstream>
#include <iostream>
#include <mutex>
#include <sstream>
#include <vector>

// Define static members
bool TreeSitter::colorsNeedUpdate = true;
ThemeColors TreeSitter::cachedColors;
TSParser *TreeSitter::parser = nullptr;
std::mutex TreeSitter::parserMutex;
std::unordered_map<std::string, TSQuery *> TreeSitter::queryCache;

// incremental parsing
TSTree *TreeSitter::previousTree = nullptr;
std::string TreeSitter::previousContent;

// Declare language parser functions
extern "C" TSLanguage *tree_sitter_cpp();
extern "C" TSLanguage *tree_sitter_javascript();
extern "C" TSLanguage *tree_sitter_python();
extern "C" TSLanguage *tree_sitter_c_sharp();
extern "C" TSLanguage *tree_sitter_html();
extern "C" TSLanguage *tree_sitter_tsx();
extern "C" TSLanguage *tree_sitter_css();
extern "C" TSLanguage *tree_sitter_java();
extern "C" TSLanguage *tree_sitter_go();
extern "C" TSLanguage *tree_sitter_hcl();
extern "C" TSLanguage *tree_sitter_json();
extern "C" TSLanguage *tree_sitter_kotlin();
extern "C" TSLanguage *tree_sitter_bash();
extern "C" TSLanguage *tree_sitter_c();
extern "C" TSLanguage *tree_sitter_rust();
extern "C" TSLanguage *tree_sitter_toml();
extern "C" TSLanguage *tree_sitter_ruby();

TSParser *TreeSitter::getParser()
{
	if (!parser)
	{
		parser = ts_parser_new();
		ts_parser_set_language(parser, tree_sitter_cpp());
	}
	return parser;
}
std::pair<TSLanguage *, std::string>
TreeSitter::detectLanguageAndQuery(const std::string &extension)
{
	if (extension == ".cpp" || extension == ".h" || extension == ".hpp")
	{
		return {tree_sitter_cpp(), "editor/queries/cpp.scm"};
	} else if (extension == ".js" || extension == ".jsx")
	{
		return {tree_sitter_javascript(), "editor/queries/jsx.scm"};
	} else if (extension == ".py")
	{
		return {tree_sitter_python(), "editor/queries/python.scm"};
	} else if (extension == ".cs")
	{
		return {tree_sitter_c_sharp(), "editor/queries/csharp.scm"};
	} else if (extension == ".html")
	{
		return {tree_sitter_html(), "editor/queries/html.scm"};
	} else if (extension == ".tsx" || extension == ".ts")
	{
		return {tree_sitter_tsx(), "editor/queries/tsx.scm"};
	} else if (extension == ".css")
	{
		return {tree_sitter_css(), "editor/queries/css.scm"};
	} else if (extension == ".java")
	{
		return {tree_sitter_java(), "editor/queries/java.scm"};
	} else if (extension == ".go")
	{
		return {tree_sitter_go(), "editor/queries/go.scm"};
	} else if (extension == ".tf")
	{
		return {tree_sitter_hcl(), "editor/queries/hcl.scm"};
	} else if (extension == ".json")
	{
		return {tree_sitter_json(), "editor/queries/json.scm"};
	} else if (extension == ".sh")
	{
		return {tree_sitter_bash(), "editor/queries/sh.scm"};
	} else if (extension == ".kt")
	{
		return {tree_sitter_kotlin(), "editor/queries/kotlin.scm"};
	} else if (extension == ".rs")
	{
		return {tree_sitter_rust(), "editor/queries/rs.scm"};
	} else if (extension == ".toml")
	{
		return {tree_sitter_toml(), "editor/queries/toml.scm"};
	} else if (extension == ".rb")
	{
		return {tree_sitter_ruby(), "editor/queries/rb.scm"};
	} else if (extension == ".c")
	{
		return {tree_sitter_c(), "editor/queries/c.scm"};
	}
	return {tree_sitter_cpp(), "editor/queries/cpp.scm"};
}

void TreeSitter::computeEditRange(const std::string &newContent,
								  size_t &start,
								  size_t &newEnd,
								  size_t &oldEnd)
{
	if (previousContent == newContent)
	{
		start = 0;
		oldEnd = previousContent.size();
		newEnd = newContent.size();
		return;
	}

	oldEnd = previousContent.size();
	newEnd = newContent.size();

	while (start < oldEnd && start < newEnd && previousContent[start] == newContent[start])
	{
		start++;
	}

	while (oldEnd > start && newEnd > start &&
		   previousContent[oldEnd - 1] == newContent[newEnd - 1])
	{
		oldEnd--;
		newEnd--;
	}
}
TSInputEdit TreeSitter::createEdit(size_t start, size_t oldEnd, size_t newEnd)
{
	TSInputEdit edit;
	edit.start_byte = static_cast<uint32_t>(start);
	edit.old_end_byte = static_cast<uint32_t>(oldEnd);
	edit.new_end_byte = static_cast<uint32_t>(newEnd);
	edit.start_point = {0, static_cast<uint32_t>(start)};
	edit.old_end_point = {0, static_cast<uint32_t>(oldEnd)};
	edit.new_end_point = {0, static_cast<uint32_t>(newEnd)};
	return edit;
}

TSInput TreeSitter::createInput(const std::string &content)
{
	return {.payload = (void *)content.data(),
			.read =
				[](void *payload, uint32_t byte, TSPoint position, uint32_t *bytes_read) {
					const char *data = static_cast<const char *>(payload);
					*bytes_read = static_cast<uint32_t>(strlen(data + byte));
					return data + byte;
				},
			.encoding = TSInputEncodingUTF8};
}

TSTree *TreeSitter::createNewTree(TSParser *parser, bool initialParse, const std::string &content)
{
	if (initialParse)
	{
		return ts_parser_parse_string(parser, nullptr, content.c_str(), content.size());
	} else
	{
		TSInput input = createInput(content);
		return ts_parser_parse(parser, previousTree, input);
	}
}

TSQuery *TreeSitter::loadQueryFromCacheOrFile(TSLanguage *lang, const std::string &query_path)
{
	auto cacheIt = queryCache.find(query_path);
	if (cacheIt != queryCache.end())
	{
		return cacheIt->second;
	}

	std::ifstream file(query_path);
	if (!file.is_open())
	{
		std::cerr << "Failed to open query file: " << query_path << "\n";
		return nullptr;
	}

	std::string query_src((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
	file.close();

	uint32_t error_offset;
	TSQueryError error_type;
	TSQuery *query =
		ts_query_new(lang, query_src.c_str(), query_src.size(), &error_offset, &error_type);
	if (!query)
	{
		std::cerr << "Query error (" << error_type << ") at offset " << error_offset << "\n";
		return nullptr;
	}

	queryCache[query_path] = query;
	return query;
}

void TreeSitter::executeQueryAndHighlight(TSQuery *query,
										  TSTree *tree,
										  const std::string &content,
										  std::vector<ImVec4> &colors,
										  bool initialParse,
										  size_t start,
										  size_t end)
{
	TSQueryCursor *cursor = ts_query_cursor_new();

	if (!initialParse)
	{
		ts_query_cursor_set_byte_range(cursor,
									   static_cast<uint32_t>(start),
									   static_cast<uint32_t>(end));
	}

	ts_query_cursor_exec(cursor, query, ts_tree_root_node(tree));

	const std::unordered_map<std::string, ImVec4> capture_colors = {
		{"keyword", cachedColors.keyword},
		{"string", cachedColors.string},
		{"number", cachedColors.number},
		{"comment", cachedColors.comment},
		{"type", cachedColors.type},
		{"function", cachedColors.function},
		{"variable", cachedColors.variable}};

	TSQueryMatch match;
	while (ts_query_cursor_next_match(cursor, &match))
	{
		for (uint32_t i = 0; i < match.capture_count; ++i)
		{
			TSNode node = match.captures[i].node;
			uint32_t name_length;
			const char *name_ptr =
				ts_query_capture_name_for_id(query, match.captures[i].index, &name_length);
			std::string name(name_ptr, name_length);

			auto it = capture_colors.find(name);
			if (it != capture_colors.end())
			{
				uint32_t start = ts_node_start_byte(node);
				uint32_t end = ts_node_end_byte(node);
				setColors(content, colors, start, end, it->second);
			}
		}
	}

	ts_query_cursor_delete(cursor);
}

void TreeSitter::parse(const std::string &fileContent,
					   std::vector<ImVec4> &fileColors,
					   const std::string &extension)
{
	std::lock_guard<std::mutex> lock(parserMutex);

	if (fileContent.empty())
	{
		std::cerr << "No content to parse!\n";
		return;
	}

	updateThemeColors();
	TSParser *parser = getParser();

	// Language detection
	auto [lang, query_path] = detectLanguageAndQuery(extension);
	if (!lang)
	{
		std::cerr << "No parser for extension: " << extension << "\n";
		return;
	}

	bool initialParse = previousContent.empty();
	ts_parser_set_language(parser, lang);

	// Handle incremental parsing
	size_t start = 0;
	size_t newEnd = fileContent.size();
	size_t oldEnd = previousContent.size();

	if (!initialParse)
	{
		computeEditRange(fileContent, start, newEnd,
						 oldEnd); // Pass new content
		TSInputEdit edit = createEdit(start, oldEnd, newEnd);
		ts_tree_edit(previousTree, &edit);
	}

	// Create new parse tree
	TSTree *newTree = createNewTree(parser, initialParse, fileContent);

	// Update state
	if (previousTree)
		ts_tree_delete(previousTree);
	previousTree = newTree;
	previousContent = fileContent;

	// Handle query execution
	TSQuery *query = loadQueryFromCacheOrFile(lang, query_path);
	if (!query)
		return;

	executeQueryAndHighlight(query, newTree, fileContent, fileColors, initialParse, start, newEnd);
}

void TreeSitter::updateThemeColors()
{
	if (!colorsNeedUpdate)
		return;
	std::cout << "inside updated colors" << std::endl;

	auto &theme = gSettings.getSettings()["themes"][gSettings.getCurrentTheme()];

	auto loadColor = [&theme](const char *key) -> ImVec4 {
		auto &c = theme[key];
		return ImVec4(c[0], c[1], c[2], c[3]);
	};
	ImVec4 newColor;
	// --- Keyword ---
	newColor = loadColor("keyword");
	cachedColors.keyword = newColor;

	// --- String ---
	newColor = loadColor("string");
	cachedColors.string = newColor;

	// --- Number ---
	newColor = loadColor("number");
	cachedColors.number = newColor;

	// --- Comment ---
	newColor = loadColor("comment");
	cachedColors.comment = newColor;

	// --- Text ---
	newColor = loadColor("text");
	cachedColors.text = newColor;

	// --- Function ---
	newColor = loadColor("function");
	cachedColors.function = newColor;

	// --- Type ---
	newColor = loadColor("type");
	cachedColors.type = newColor;

	// --- Variable ---
	newColor = loadColor("variable");
	cachedColors.variable = newColor;

	colorsNeedUpdate = false;
	std::cout << "--- Theme Colors Updated ---" << std::endl;
}

void TreeSitter::setColors(const std::string &content,
						   std::vector<ImVec4> &colors,
						   int start,
						   int end,
						   const ImVec4 &color)
{
	if (end > content.size() || start > end)
	{
		return;
	}
	start = std::clamp(start, 0, (int)colors.size() - 1);
	end = std::clamp(end, 0, (int)colors.size());

	for (int i = start; i < end; ++i)
	{
		colors[i] = color;
	}
}
