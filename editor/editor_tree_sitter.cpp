#include "editor_tree_sitter.h"
#include "editor.h"
#include <fstream>
#include <iostream>
#include <mutex>
#include <vector>

// Define static members
bool TreeSitter::colorsNeedUpdate = true;
ThemeColors TreeSitter::cachedColors;
TSParser *TreeSitter::parser = nullptr;
std::mutex TreeSitter::parserMutex;

// Declare language parser functions
extern "C" TSLanguage *tree_sitter_cpp();
extern "C" TSLanguage *tree_sitter_javascript();

// ANSI color codes
#define COLOR_NODE "\033[33m" // Yellow
#define COLOR_POS "\033[36m"  // Cyan
#define COLOR_RESET "\033[0m"

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
void TreeSitter::parse(const std::string &fileContent,
					   std::vector<ImVec4> &fileColors,
					   const std::string &extension)
{
	std::lock_guard<std::mutex> lock(parserMutex); // Critical section

	if (fileContent.empty())
	{
		std::cerr << "No content to parse!\n";
		return;
	}

	updateThemeColors();
	TSParser *parser = getParser();

	TSLanguage *lang = nullptr;

	if (extension == ".cpp" || extension == ".h" || extension == ".hpp")
	{
		lang = tree_sitter_cpp();
	} else if (extension == ".js" || extension == ".jsx")
	{
		lang = tree_sitter_javascript();
	} else
	{
		lang = tree_sitter_cpp(); // Default fallback
	}

	ts_parser_set_language(parser, lang);

	TSTree *tree = ts_parser_parse_string(parser, nullptr, fileContent.c_str(), fileContent.size());

	traverse(extension, fileContent, fileColors, ts_tree_root_node(tree), 0, false, {}, "");
	// printColors();

	ts_tree_delete(tree); // Only delete the tree, keep parser alive
}
void TreeSitter::traverse(const std::string &extension,
						  const std::string &fileContent,
						  std::vector<ImVec4> &fileColors,
						  TSNode node,
						  int depth,
						  bool is_last,
						  const std::vector<bool> &hierarchy,
						  const std::string &parentNodeType)
{
	if (ts_node_is_null(node))
	{
		return;
	}

	if (!ts_node_is_named(node) && ts_node_is_extra(node))
	{
		return;
	}
	std::string prefix;
	for (size_t i = 0; i < hierarchy.size(); ++i)
	{ // Use size_t for loop index
		prefix += hierarchy[i] ? "    " : "│   ";
	}
	prefix += is_last ? "└── " : "├── ";

	uint32_t start_byte = ts_node_start_byte(node);
	uint32_t end_byte = ts_node_end_byte(node);
	const char *type = ts_node_type(node);

	std::string content = "ERR"; // Default error string
	if (start_byte <= end_byte && end_byte <= fileContent.size())
	{
		size_t length = end_byte - start_byte;
		if (start_byte + length <= fileContent.size())
		{
			content = fileContent.substr(start_byte, length);
		} else
		{
			content = "ERR_BOUNDS";
		}

		// Clean up snippet: remove internal newlines and limit length for
		// display
		size_t first_newline = content.find_first_of("\r\n");
		if (first_newline != std::string::npos)
		{
			content = content.substr(0, first_newline) + "..."; // Show only first line part
		}
		if (content.size() > 40)
		{
			content = content.substr(0, 37) + "..."; // Truncate long snippets
		}
	} else
	{
		content = "ERR_RANGE";
	}
	ImVec4 color = cachedColors.text;
	std::string nodeText = fileContent.substr(start_byte, end_byte - start_byte);

	// Assign based on file type (no "const ImVec4 &" redeclaration)
	if (extension == ".cpp" || extension == ".h" || extension == ".hpp")
	{
		color = convertNodeTypeCPP(type); // <-- Assignment, not declaration
	} else if (extension == ".js" || extension == ".jsx")
	{
		color = convertNodeTypeJS(type, parentNodeType, nodeText);
	}

	std::cout << COLOR_POS << "Idx " << start_byte << "-" << end_byte << COLOR_RESET << " "
			  << prefix << " color: " << int(color.x * 255) << "," << int(color.y * 255) << ","
			  << int(color.z * 255) << ": " << type << " -- " << content << std::endl;
	setColors(
		fileContent, fileColors, static_cast<int>(start_byte), static_cast<int>(end_byte), color);
	std::vector<bool> child_hierarchy = hierarchy;
	child_hierarchy.push_back(is_last); // Add current node's 'last' status for
										// child prefix calculation

	uint32_t child_count = ts_node_child_count(node);
	for (uint32_t i = 0; i < child_count; i++)
	{
		TSNode child = ts_node_child(node, i);
		bool last_child = (i == child_count - 1);
		traverse(
			extension, fileContent, fileColors, child, depth + 1, last_child, child_hierarchy, type);
	}
}

const ImVec4 &TreeSitter::convertNodeTypeJS(const std::string &nodeType,
											const std::string &parentNodeType,
											const std::string &nodeText)
{
	static const std::unordered_map<std::string, ImVec4> baseMap = {
		{"string", cachedColors.string},
		{"number", cachedColors.number},
		{"comment", cachedColors.comment},
		{"string_fragment", cachedColors.string},
		{"jsx_text", cachedColors.text},
		{"template_string", cachedColors.string},
		{"true", cachedColors.keyword},
		{"false", cachedColors.keyword},
		{"null", cachedColors.keyword}};

	// JSX/HTML specific rules (keep your preferred version)
	if (parentNodeType == "jsx_attribute")
	{
		if (nodeType == "property_identifier")
			return cachedColors.keyword;
		if (nodeType == "string")
			return cachedColors.string;
	}

	if ((parentNodeType == "jsx_opening_element" || parentNodeType == "jsx_closing_element") &&
		nodeType == "identifier")
	{
		return (isupper(nodeText[0])) ? cachedColors.function : cachedColors.keyword;
	}

	if (nodeType == "jsx_opening_element" || nodeType == "jsx_closing_element")
	{
		return cachedColors.function;
	}

	if (nodeType == "<" || nodeType == ">" || nodeType == "/>" || nodeType == "</")
	{
		return cachedColors.text;
	}

	// Enhanced JavaScript keyword handling
	static const std::unordered_set<std::string> keywords = {
		"return",	 "function", "if",		   "else",	   "for",	  "while",	  "do",
		"switch",	 "case",	 "try",		   "catch",	   "finally", "throw",	  "new",
		"delete",	 "typeof",	 "instanceof", "void",	   "break",	  "continue", "debugger",
		"var",		 "let",		 "const",	   "class",	   "extends", "super",	  "this",
		"async",	 "await",	 "yield",	   "of",	   "in",	  "static",	  "get",
		"set",		 "export",	 "import",	   "default",  "from",	  "as",		  "typeof",
		"interface", "type",	 "implements", "namespace"};

	if (keywords.count(nodeType))
	{
		return cachedColors.keyword;
	}

	// Function handling
	if (nodeType == "function_declaration" || nodeType == "arrow_function" ||
		nodeType == "method_definition" || nodeType == "call_expression")
	{
		return cachedColors.function;
	}

	// React hooks pattern
	if (nodeType == "identifier" && parentNodeType == "lexical_declaration")
	{
		static const std::unordered_set<std::string> hooks = {"useState",
															  "useEffect",
															  "useContext",
															  "useReducer",
															  "useCallback",
															  "useMemo",
															  "useRef",
															  "useImperativeHandle",
															  "useLayoutEffect"};
		if (hooks.count(nodeText))
			return cachedColors.function;
	}

	// JSX expressions
	if (parentNodeType == "jsx_expression")
	{
		if (nodeType == "identifier" || nodeType == "member_expression")
		{
			return cachedColors.function;
		}
	}

	// Object/class properties
	if (nodeType == "property_identifier" && parentNodeType != "jsx_attribute")
	{
		return cachedColors.function;
	}

	// Default mappings
	if (auto it = baseMap.find(nodeType); it != baseMap.end())
	{
		return it->second;
	}

	return cachedColors.text;
}
const ImVec4 &TreeSitter::convertNodeTypeCPP(const std::string &nodeType)
{
	// C++ specific node mappings
	static const std::unordered_map<std::string, ImVec4 &> cppTypeMap = {
		{"number_literal", cachedColors.number},
		{"string_literal", cachedColors.string},
		{"char_literal", cachedColors.string},
		{"comment", cachedColors.comment},
		{"function_definition", cachedColors.function},
		{"call_expression", cachedColors.function},
		{"field_identifier", cachedColors.function},
		{"primitive_type", cachedColors.keyword},
		{"type_identifier", cachedColors.keyword},
		{"if", cachedColors.keyword},
		{"for", cachedColors.keyword},
		{"while", cachedColors.keyword},
		{"return", cachedColors.keyword},
		{"class_specifier", cachedColors.keyword},
		{"namespace_identifier", cachedColors.function}};

	if (auto it = cppTypeMap.find(nodeType); it != cppTypeMap.end())
	{
		return it->second;
	}

	return cachedColors.text; // Default fallback
}

void TreeSitter::printColors()
{
	std::cout << "\nTheme Colors:" << std::endl;
	std::cout << "Keyword: (" << cachedColors.keyword.x * 255 << ", "
			  << cachedColors.keyword.y * 255 << ", " << cachedColors.keyword.z * 255 << ", "
			  << cachedColors.keyword.w * 255 << ")" << std::endl;
	std::cout << "String: (" << cachedColors.string.x * 255 << ", " << cachedColors.string.y * 255
			  << ", " << cachedColors.string.z * 255 << ", " << cachedColors.string.w * 255 << ")"
			  << std::endl;
	std::cout << "Number: (" << cachedColors.number.x * 255 << ", " << cachedColors.number.y * 255
			  << ", " << cachedColors.number.z * 255 << ", " << cachedColors.number.w * 255 << ")"
			  << std::endl;
	std::cout << "Comment: (" << cachedColors.comment.x * 255 << ", "
			  << cachedColors.comment.y * 255 << ", " << cachedColors.comment.z * 255 << ", "
			  << cachedColors.comment.w * 255 << ")" << std::endl;
	std::cout << "Text: (" << cachedColors.text.x * 255 << ", " << cachedColors.text.y * 255 << ", "
			  << cachedColors.text.z * 255 << ", " << cachedColors.text.w * 255 << ")" << std::endl;
	std::cout << "Function: (" << cachedColors.function.x * 255 << ", "
			  << cachedColors.function.y * 255 << ", " << cachedColors.function.z * 255 << ", "
			  << cachedColors.function.w * 255 << ")" << std::endl;
}

void TreeSitter::updateThemeColors()
{
	if (!colorsNeedUpdate)
		return;

	auto &theme = gSettings.getSettings()["themes"][gSettings.getCurrentTheme()];

	auto loadColor = [&theme](const char *key) -> ImVec4 {
		auto &c = theme[key];
		return ImVec4(c[0], c[1], c[2], c[3]);
	};

	cachedColors.keyword = loadColor("keyword");
	cachedColors.string = loadColor("string");
	cachedColors.number = loadColor("number");
	cachedColors.comment = loadColor("comment");
	cachedColors.text = loadColor("text");
	cachedColors.function = loadColor("function");

	colorsNeedUpdate = false;
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
