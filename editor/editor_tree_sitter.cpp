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

void print_ast_node(TSNode node, const std::string &source_code, int indent_level)
{
	// Print indentation
	for (int i = 0; i < indent_level; ++i)
	{
		std::cout << "  ";
	}

	// Get node type and range
	const char *node_type = ts_node_type(node);
	uint32_t start_byte = ts_node_start_byte(node);
	uint32_t end_byte = ts_node_end_byte(node);

	// Print node info: (TYPE) [start_byte - end_byte] "optional_text"
	std::cout << "(" << node_type << ") [" << start_byte << " - " << end_byte << "]";

	// Optionally print the text content for small nodes (helps identify)
	if ((end_byte - start_byte) < 40)
	{ // Only print text for reasonably small nodes
		std::cout << " \"";
		// Be careful with multi-byte characters if source is UTF-8; this simple print might be
		// imperfect
		for (uint32_t i = start_byte; i < end_byte && i < source_code.length(); ++i)
		{
			char c = source_code[i];
			if (c == '\n')
				std::cout << "\\n"; // Escape newline for readability
			else if (c == '\r')
				std::cout << "\\r";
			else if (c == '\t')
				std::cout << "\\t";
			else
				std::cout << c;
		}
		std::cout << "\"";
	}
	std::cout << std::endl;

	// Recurse for children
	uint32_t child_count = ts_node_child_count(node);
	for (uint32_t i = 0; i < child_count; ++i)
	{
		TSNode child_node = ts_node_child(node, i);
		print_ast_node(child_node, source_code, indent_level + 1); // Increase indent
	}
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
	TSLanguage *lang = nullptr;
	std::string query_path;

	if (extension == ".cpp" || extension == ".h" || extension == ".hpp")
	{
		lang = tree_sitter_cpp();
		query_path = "editor/queries/cpp.scm";
	} else if (extension == ".js" || extension == ".jsx")
	{
		lang = tree_sitter_javascript();
		query_path = "editor/queries/jsx.scm";
	} else if (extension == ".py")
	{
		lang = tree_sitter_python();
		query_path = "editor/queries/python.scm";
	} else if (extension == ".cs")
	{
		lang = tree_sitter_c_sharp();
		query_path = "editor/queries/csharp.scm";
	} else if (extension == ".html")
	{
		lang = tree_sitter_html();
		query_path = "editor/queries/html.scm";
	} else if (extension == ".tsx" || extension == ".ts")
	{
		lang = tree_sitter_tsx();
		query_path = "editor/queries/tsx.scm";
	} else if (extension == ".css")
	{
		lang = tree_sitter_css();
		query_path = "editor/queries/css.scm";
	} else if (extension == ".java")
	{
		lang = tree_sitter_java();
		query_path = "editor/queries/java.scm";
	} else if (extension == ".go")
	{
		lang = tree_sitter_go();
		query_path = "editor/queries/go.scm";
	} else if (extension == ".tf")
	{
		lang = tree_sitter_hcl();
		query_path = "editor/queries/hcl.scm";
	} else if (extension == ".json")
	{
		lang = tree_sitter_json();
		query_path = "editor/queries/json.scm";
	} else if (extension == ".sh")
	{
		lang = tree_sitter_bash();
		query_path = "editor/queries/sh.scm";
	} else if (extension == ".kt")
	{
		lang = tree_sitter_kotlin();
		query_path = "editor/queries/kotlin.scm";
	} else if (extension == ".rs")
	{
		lang = tree_sitter_rust();
		query_path = "editor/queries/rs.scm";
	} else if (extension == ".toml")
	{
		lang = tree_sitter_toml();
		query_path = "editor/queries/toml.scm";
	} else if (extension == ".rb")
	{
		lang = tree_sitter_ruby();
		query_path = "editor/queries/rb.scm";
	} else if (extension == ".c")
	{
		lang = tree_sitter_c();
		query_path = "editor/queries/c.scm";
	} else
	{
		lang = tree_sitter_cpp();
	}

	if (!lang)
	{
		std::cerr << "No parser for extension: " << extension << "\n";
		return;
	}

	TSTree *newTree = nullptr;
	bool initialParse = previousContent.empty();

	ts_parser_set_language(parser, lang);
	size_t start = 0;
	size_t newEnd = fileContent.size();

	if (!initialParse)
	{
		size_t oldEnd = previousContent.size();

		// Find first differing byte
		while (start < oldEnd && start < newEnd && previousContent[start] == fileContent[start])
		{
			start++;
		}

		// Find last differing byte
		while (oldEnd > start && newEnd > start &&
			   previousContent[oldEnd - 1] == fileContent[newEnd - 1])
		{
			oldEnd--;
			newEnd--;
		}
		TSInputEdit edit;
		edit.start_byte = static_cast<uint32_t>(start);
		edit.old_end_byte = static_cast<uint32_t>(oldEnd);
		edit.new_end_byte = static_cast<uint32_t>(newEnd);
		edit.start_point = {0, static_cast<uint32_t>(start)};
		edit.old_end_point = {0, static_cast<uint32_t>(oldEnd)};
		edit.new_end_point = {0, static_cast<uint32_t>(newEnd)};

		// Apply edit to previous tree (correct ts_tree_edit signature)
		ts_tree_edit(previousTree, &edit);

		// Create input for incremental parsing
		TSInput input = {.payload = (void *)fileContent.data(),
						 .read = [](void *payload,
									uint32_t byte,
									TSPoint position,
									uint32_t *bytes_read) -> const char * {
							 const char *data = static_cast<const char *>(payload);
							 *bytes_read = static_cast<uint32_t>(strlen(data + byte));
							 return data + byte;
						 },
						 .encoding = TSInputEncodingUTF8};

		// Reparse incrementally (correct ts_parser_parse signature)
		newTree = ts_parser_parse(parser, previousTree, input);

	} else
	{
		// Initial full parse using string-based API
		newTree = ts_parser_parse_string(parser, nullptr, fileContent.c_str(), fileContent.size());
	}

	// Cleanup previous tree and update state
	if (previousTree)
	{
		ts_tree_delete(previousTree);
	}
	previousTree = newTree;
	previousContent = fileContent;

	// Load query
	TSQuery *query = nullptr;
	auto cacheIt = queryCache.find(query_path);
	if (cacheIt != queryCache.end())
	{
		query = cacheIt->second;
	} else
	{
		std::ifstream file(query_path);
		if (!file.is_open())
		{
			std::cerr << "Failed to open query file: " << query_path << "\n";
			return;
		}
		std::string query_src((std::istreambuf_iterator<char>(file)),
							  std::istreambuf_iterator<char>());
		file.close();

		uint32_t error_offset;
		TSQueryError error_type;
		query = ts_query_new(lang, query_src.c_str(), query_src.size(), &error_offset, &error_type);
		if (!query)
		{
			std::cerr << "Query error (" << error_type << ") at offset " << error_offset << "\n";
			return;
		}
		queryCache[query_path] = query;
	}

	// Execute query with incremental range
	TSQueryCursor *cursor = ts_query_cursor_new();
	if (!initialParse)
	{
		// Use the edit range from earlier
		uint32_t start_byte = static_cast<uint32_t>(start);
		uint32_t end_byte = static_cast<uint32_t>(newEnd);
		ts_query_cursor_set_byte_range(cursor, start_byte, end_byte);
	}
	ts_query_cursor_exec(cursor, query, ts_tree_root_node(previousTree));

	// Apply highlighting
	const std::unordered_map<std::string, ImVec4> capture_colors = {
		{"keyword", cachedColors.keyword},
		{"string", cachedColors.string},
		{"comment", cachedColors.comment},
		{"type", cachedColors.type},
		{"function", cachedColors.function},
		{"variable", cachedColors.variable},
		{"number", cachedColors.number}};

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
				setColors(fileContent, fileColors, start, end, it->second);
			}
		}
	}

	// Cleanup query resources
	ts_query_cursor_delete(cursor);
	// ts_query_delete(query);
}
std::ostream &operator<<(std::ostream &os, const ImVec4 &color)
{
	os << "(" << color.x << ", " << color.y << ", " << color.z << ", " << color.w << ")";
	return os;
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
