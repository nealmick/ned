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

// Declare language parser functions
extern "C" TSLanguage *tree_sitter_cpp();
extern "C" TSLanguage *tree_sitter_javascript();
extern "C" TSLanguage *tree_sitter_python();
extern "C" TSLanguage *tree_sitter_c_sharp();
extern "C" TSLanguage *tree_sitter_html();

std::map<std::string, TSQuery *> TreeSitter::languageQueries;

TSParser *TreeSitter::getParser()
{
	if (!parser)
	{
		parser = ts_parser_new();
		ts_parser_set_language(parser, tree_sitter_cpp());
	}
	return parser;
}

void TreeSitter::cleanupParser()
{
	std::lock_guard<std::mutex> lock(parserMutex);
	if (parser)
	{
		ts_parser_delete(parser);
		parser = nullptr;
	}
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

	TSLanguage *lang = nullptr;
	std::string query_path;

	// Language detection
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
	} else
	{
		lang = tree_sitter_cpp();
	}

	if (!lang)
	{
		std::cerr << "No parser for extension: " << extension << "\n";
		return;
	}

	ts_parser_set_language(parser, lang);
	TSTree *tree = ts_parser_parse_string(parser, nullptr, fileContent.c_str(), fileContent.size());
	if (tree)
	{ // Make sure the tree was actually created
		TSNode root_node = ts_tree_root_node(tree);
		std::cout << "\n======================================================\n";
		std::cout << "                 TREE-SITTER AST DUMP                 \n";
		std::cout << "======================================================\n";
		print_ast_node(root_node, fileContent, 0); // Start printing from root node
		std::cout << "======================================================\n\n";
	} else
	{
		std::cerr << "Failed to parse content, cannot print AST." << std::endl;
	}
	// Load query
	std::ifstream file(query_path);
	if (!file.is_open())
	{
		std::cerr << "Failed to open query file: " << query_path << "\n";
		ts_tree_delete(tree);
		return;
	}

	std::string query_src((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());

	uint32_t error_offset;
	TSQueryError error_type;
	TSQuery *query =
		ts_query_new(lang, query_src.c_str(), query_src.size(), &error_offset, &error_type);

	if (!query)
	{
		std::cerr << "Query error (" << error_type << ") at offset " << error_offset << "\n";
		ts_tree_delete(tree);
		return;
	}

	// Execute query
	TSQueryCursor *cursor = ts_query_cursor_new();
	ts_query_cursor_exec(cursor, query, ts_tree_root_node(tree));

	// Color mapping
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

			// Get the capture name using the new API function
			uint32_t name_length;
			const char *name_ptr =
				ts_query_capture_name_for_id(query, match.captures[i].index, &name_length);

			// Convert the C-style string (pointer + length) to std::string for map lookup
			std::string name(name_ptr, name_length);

			// Now use the std::string 'name' for the lookup
			auto it = capture_colors.find(name);
			if (it != capture_colors.end())
			{
				uint32_t start = ts_node_start_byte(node);
				uint32_t end = ts_node_end_byte(node);
				setColors(fileContent, fileColors, start, end, it->second);
			}
		}
	}

	// Cleanup
	ts_query_cursor_delete(cursor);
	ts_query_delete(query);

	ts_tree_delete(tree);
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
	std::cout << "keyword: old" << cachedColors.keyword << " -> new" << newColor << std::endl;
	cachedColors.keyword = newColor;

	// --- String ---
	newColor = loadColor("string");
	std::cout << "string:  old" << cachedColors.string << " -> new" << newColor << std::endl;
	cachedColors.string = newColor;

	// --- Number ---
	newColor = loadColor("number");
	std::cout << "number:  old" << cachedColors.number << " -> new" << newColor << std::endl;
	cachedColors.number = newColor;

	// --- Comment ---
	newColor = loadColor("comment");
	std::cout << "comment: old" << cachedColors.comment << " -> new" << newColor << std::endl;
	cachedColors.comment = newColor;

	// --- Text ---
	newColor = loadColor("text");
	std::cout << "text:    old" << cachedColors.text << " -> new" << newColor << std::endl;
	cachedColors.text = newColor;

	// --- Function ---
	newColor = loadColor("function");
	std::cout << "function:old" << cachedColors.function << " -> new" << newColor << std::endl;
	cachedColors.function = newColor;

	// --- Type ---
	newColor = loadColor("type");
	std::cout << "type:    old" << cachedColors.type << " -> new" << newColor << std::endl;
	cachedColors.type = newColor;

	// --- Variable ---
	newColor = loadColor("variable");
	std::cout << "variable:old" << cachedColors.variable << " -> new" << newColor << std::endl;
	cachedColors.variable = newColor;

	// --- Add any other colors similarly ---

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
std::string readFile(const std::string &path)
{
	std::ifstream file(path);
	std::stringstream buffer;
	buffer << file.rdbuf();
	return buffer.str();
}
