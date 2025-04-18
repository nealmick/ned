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
extern "C" TSLanguage *tree_sitter_python();
extern "C" TSLanguage *tree_sitter_c_sharp();

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
	} else if (extension == ".py")
	{
		lang = tree_sitter_python();
	} else if (extension == ".cs")
	{
		lang = tree_sitter_c_sharp();
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
		color = convertNodeTypeCPP(type, parentNodeType, nodeText);

	} else if (extension == ".js" || extension == ".jsx")
	{
		color = convertNodeTypeJS(type, parentNodeType, nodeText);
	} else if (extension == ".py")
	{
		color = convertNodeTypePython(type, parentNodeType, nodeText);
	} else if (extension == ".cs")
	{
		color = convertNodeTypeCSharp(type, parentNodeType, nodeText);
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
const ImVec4 &TreeSitter::convertNodeTypeCSharp(const std::string &nodeType,
												const std::string &parentNodeType,
												const std::string &nodeText)
{
	// --- Keywords ---
	// Using a set for faster keyword lookup
	static const std::unordered_set<std::string> keywords = {
		"abstract",
		"as",
		"base",
		"bool",
		"break",
		"byte",
		"case",
		"catch",
		"char",
		"checked",
		"class",
		"const",
		"continue",
		"decimal",
		"default",
		"delegate",
		"do",
		"double",
		"else",
		"enum",
		"event",
		"explicit",
		"extern",
		"false",
		"finally",
		"fixed",
		"float",
		"for",
		"foreach",
		"goto",
		"if",
		"implicit",
		"in",
		"int",
		"interface",
		"internal",
		"is",
		"lock",
		"long",
		"namespace",
		"new",
		"null",
		"object",
		"operator",
		"out",
		"override",
		"params",
		"private",
		"protected",
		"public",
		"readonly",
		"ref",
		"return",
		"sbyte",
		"sealed",
		"short",
		"sizeof",
		"stackalloc",
		"static",
		"string",
		"struct",
		"switch",
		"this",
		"throw",
		"true",
		"try",
		"typeof",
		"uint",
		"ulong",
		"unchecked",
		"unsafe",
		"ushort",
		"using", // using keyword itself
		"virtual",
		"void",
		"volatile",
		"while",
		// Contextual keywords (often treated as keywords)
		"add",
		"alias",
		"async",
		"await",
		"dynamic",
		"get",
		"global",
		"nameof",
		"partial",
		"remove",
		"set",
		"value",
		"var",
		"when",
		"where",
		"yield"
		// Note: 'value' is handled specially below for property setters
		// Note: 'var' could be type or keyword depending on preference, often keyword
	};

	if (keywords.count(nodeType)) // Direct keyword node types (like 'if', 'while')
	{
		return cachedColors.keyword;
	}
	if (keywords.count(nodeText) &&
		(nodeType == "keyword" || nodeType == "identifier" ||
		 nodeType ==
			 "contextual_keyword")) // Check nodeText if nodeType is generic like 'identifier'
	{
		// Special case for 'value' in setters - treat as variable
		if (nodeText == "value" &&
			(parentNodeType == "set_accessor_declaration" ||
			 parentNodeType == "expression_statement")) // Adjust parent based on grammar
		{
			return cachedColors.variable;
		}
		return cachedColors.keyword;
	}
	if (nodeType == "modifier")
	{ // Modifiers like public, private, static etc.
		return cachedColors.keyword;
	}
	if (nodeType == "predefined_type")
	{							  // Built-in types like int, string, bool, void
		return cachedColors.type; // Use Type color for built-in types
	}

	// --- Literals ---
	if (nodeType == "integer_literal" || nodeType == "real_literal")
	{
		return cachedColors.number;
	}
	if (nodeType == "string_literal" || nodeType == "verbatim_string_literal" ||
		nodeType == "interpolated_string_literal" || nodeType == "character_literal")
	{
		// TODO: Could potentially color interpolation parts differently
		return cachedColors.string;
	}
	if (nodeType == "boolean_literal" || nodeType == "null_literal")
	{
		return cachedColors.keyword; // Treat true, false, null as keywords
	}

	// --- Comments ---
	if (nodeType == "comment" || nodeType == "documentation_comment")
	{
		return cachedColors.comment;
	}

	// --- Types (Classes, Structs, Interfaces, Enums, Delegates, Records) ---
	if (nodeType == "class_declaration" || nodeType == "struct_declaration" ||
		nodeType == "interface_declaration" || nodeType == "enum_declaration" ||
		nodeType == "delegate_declaration" || nodeType == "record_declaration" ||
		nodeType == "record_struct_declaration")
	{
		// Color the entire declaration block? Or just the name?
		// Let's color specific parts below, default the block to text.
		// return cachedColors.type; // This would color the whole block
	}
	if (nodeType == "type_identifier" || nodeType == "generic_name" || nodeType == "nullable_type")
	{
		// This catches usage of type names (e.g., List<string>, MyClass, int?)
		return cachedColors.type;
	}

	// --- Functions / Methods / Constructors ---
	if (nodeType == "method_declaration" || nodeType == "constructor_declaration" ||
		nodeType == "operator_declaration" || nodeType == "conversion_operator_declaration" ||
		nodeType == "destructor_declaration" || nodeType == "local_function_statement")
	{
		// Color the whole block? Or just the name?
		// Let's color the identifier name below.
		// return cachedColors.function;
	}
	// Catch function/method calls
	if (nodeType == "invocation_expression" && parentNodeType != "object_creation_expression")
	{
		// The expression itself might be complex (e.g., member access),
		// color the final identifier (method name) via the identifier logic below.
	}

	// --- Variables & Parameters ---
	if (nodeType == "variable_declaration" || nodeType == "local_declaration_statement")
	{
		// Declaration itself doesn't get special color, identifier inside does
	}
	if (nodeType == "parameter")
	{
		// Parameter declaration itself doesn't get special color, identifier inside does
	}
	if (nodeType == "property_declaration" || nodeType == "field_declaration" ||
		nodeType == "event_field_declaration")
	{
		// Declaration itself doesn't get special color, identifier inside does
	}

	// --- Identifiers (Context is Key!) ---
	if (nodeType == "identifier")
	{
		// Is it a parameter name?
		if (parentNodeType == "parameter")
		{
			return cachedColors.variable; // Use variable color for parameters
		}
		// Is it a variable name being declared?
		if (parentNodeType == "variable_declarator" || parentNodeType == "catch_declaration")
		{
			return cachedColors.variable;
		}
		// Is it a loop variable? (foreach)
		if (parentNodeType == "foreach_statement")
		{
			// Need to check if it's the identifier specifically for the loop variable
			// This depends on the exact grammar structure. Assuming it is for now.
			return cachedColors.variable;
		}
		// Is it a class, struct, interface, enum, delegate, or record name being declared?
		if (parentNodeType == "class_declaration" || parentNodeType == "struct_declaration" ||
			parentNodeType == "interface_declaration" || parentNodeType == "enum_declaration" ||
			parentNodeType == "delegate_declaration" || parentNodeType == "record_declaration" ||
			nodeType == "record_struct_declaration")
		{
			return cachedColors.type; // Use type color for type definition names
		}
		// Is it a method, constructor, operator, or local function name being declared?
		if (parentNodeType == "method_declaration" || parentNodeType == "constructor_declaration" ||
			parentNodeType == "operator_declaration" ||
			parentNodeType == "conversion_operator_declaration" ||
			parentNodeType == "destructor_declaration" ||
			parentNodeType == "local_function_statement")
		{
			return cachedColors.function; // Use function color for function definition names
		}
		// Is it a property, field, or event name being declared?
		if (parentNodeType == "property_declaration" || parentNodeType == "field_declaration" ||
			parentNodeType == "event_field_declaration" ||
			parentNodeType == "enum_member_declaration")
		{
			return cachedColors.variable; // Use variable color for fields/properties
		}
		// Is it a function or method being called?
		// This requires looking at the parent/grandparent structure, e.g., identifier inside
		// member_access inside invocation
		if (parentNodeType == "member_access_expression" ||
			parentNodeType == "invocation_expression" ||
			parentNodeType == "object_creation_expression")
		{
			// Very simplified: assume identifiers in these contexts are function/method calls or
			// properties being accessed. A more robust solution might check if the grandparent is
			// invocation_expression.
			return cachedColors.function; // Tentatively color as function call
		}
		// Is it part of a namespace or qualified name?
		if (parentNodeType == "qualified_name" || parentNodeType == "namespace_declaration")
		{
			// Could be namespace part or type part. Let's use 'Type' color for consistency.
			return cachedColors.type;
		}
		// Is it an attribute name?
		if (parentNodeType == "attribute")
		{
			return cachedColors.type; // Attributes often map to Type names
		}
		// Is it a label?
		if (parentNodeType == "labeled_statement")
		{
			return cachedColors.text; // Labels usually aren't highlighted specially
		}

		// If none of the above, it's likely a variable usage.
		// return cachedColors.variable; // Default identifier usage to variable? Risky.
		// Let's default to text if context is uncertain.
		return cachedColors.text;
	}

	// --- Other ---
	if (nodeType == "using_directive")
	{ // The whole 'using Blah.Blah;' statement
	  // Let parts inside (keyword, qualified_name) be colored by their rules.
	  // return cachedColors.keyword; // Or color the whole line keyword? Let's use text.
	}
	if (nodeType == "attribute")
	{ // [AttributeName(...)]
		// Color the brackets/commas? Probably text. Color the name via identifier logic.
		return cachedColors.type; // Color the whole attribute block? Let's try type.
	}
	if (nodeType == "preprocessor_directive" || nodeType == "preprocessor_call")
	{
		return cachedColors.comment; // Style preprocessor like comments or a distinct color
	}

	// --- Fallback ---
	// If the node type didn't match any specific rule above
	return cachedColors.text;
}

const ImVec4 &TreeSitter::convertNodeTypePython(const std::string &nodeType,
												const std::string &parentNodeType,
												const std::string &nodeText)
{
	static const std::unordered_map<std::string, ImVec4> baseMap = {
		{"string", cachedColors.string},
		{"string_start", cachedColors.string},
		{"string_end", cachedColors.string},
		{"string_content", cachedColors.string},
		{"fstring_start", cachedColors.string},
		{"fstring_end", cachedColors.string},
		{"fstring_content", cachedColors.string},
		{"comment", cachedColors.comment},
		{"integer", cachedColors.number},
		{"float", cachedColors.number},
		{"none", cachedColors.keyword},
		{"true", cachedColors.keyword},
		{"false", cachedColors.keyword},
		{"escape_sequence", cachedColors.string},
		{"fstring_start", cachedColors.string},
		{"fstring_end", cachedColors.string}};

	static const std::unordered_set<std::string> keywords = {
		"and",	  "as",	  "assert", "async",  "await",	"break",   "class",	   "continue",
		"def",	  "del",  "elif",	"else",	  "except", "finally", "for",	   "from",
		"global", "if",	  "import", "in",	  "is",		"lambda",  "nonlocal", "not",
		"or",	  "pass", "raise",	"return", "try",	"while",   "with",	   "yield"};

	// Handle keywords
	if (keywords.count(nodeType))
	{
		return cachedColors.keyword;
	}

	// Function and class definitions
	if (nodeType == "identifier")
	{
		if (parentNodeType == "function_definition" || parentNodeType == "class_definition" ||
			parentNodeType == "decorator")
		{
			return cachedColors.function;
		}
		if (parentNodeType == "call")
		{
			return cachedColors.function;
		}
	}

	// Parameters
	if (nodeType == "parameters" || nodeType == "default_parameter")
	{
		return cachedColors.text;
	}

	// Decorators
	if (nodeType == "decorator")
	{
		return cachedColors.function;
	}

	// Type annotations
	if (parentNodeType == "type_alias" || parentNodeType == "typed_parameter")
	{
		return cachedColors.keyword;
	}

	// String formatting
	if (nodeType == "format_specifier")
	{
		return cachedColors.string;
	}

	// Import statements
	if (nodeType == "dotted_name" &&
		(parentNodeType == "import_from_statement" || parentNodeType == "import_statement"))
	{
		return cachedColors.keyword;
	}

	// Handle base mappings
	if (auto it = baseMap.find(nodeType); it != baseMap.end())
	{
		return it->second;
	}

	return cachedColors.text;
}
const ImVec4 &TreeSitter::convertNodeTypeJS(const std::string &nodeType,
											const std::string &parentNodeType,
											const std::string &nodeText)
{
	static const std::unordered_map<std::string, ImVec4> pythonTypeMap = {
		// Basic syntax
		{"import", cachedColors.keyword},
		{"from", cachedColors.keyword},
		{"def", cachedColors.keyword},
		{"class", cachedColors.keyword},
		{"return", cachedColors.keyword},

		// Literals
		{"string", cachedColors.string},
		{"integer", cachedColors.number},
		{"float", cachedColors.number},
		{"true", cachedColors.keyword},
		{"false", cachedColors.keyword},
		{"none", cachedColors.keyword},

		// Functions/classes
		{"function_definition", cachedColors.function},
		{"class_definition", cachedColors.function},
		{"call", cachedColors.function},

		// Comments
		{"comment", cachedColors.comment},

		// Imports
		{"import_statement", cachedColors.keyword},
		{"import_from_statement", cachedColors.keyword},
		{"dotted_name", cachedColors.function}};

	if (auto it = pythonTypeMap.find(nodeType); it != pythonTypeMap.end())
	{
		return it->second;
	}

	return cachedColors.text;
}
const ImVec4 &TreeSitter::convertNodeTypeCPP(const std::string &nodeType,
											 const std::string &parentNodeType,
											 const std::string &nodeText)
{
	// --- Keywords ---
	static const std::unordered_set<std::string> keywords = {
		"alignas",		 "alignof",		"and",
		"and_eq",		 "asm",			"auto",
		"bitand",		 "bitor",		"bool",
		"break",		 "case",		"catch",
		"char",			 "char8_t",		"char16_t",
		"char32_t",		 "class",		"compl",
		"concept",		 "const",		"consteval",
		"constexpr",	 "constinit",	"const_cast",
		"continue",		 "co_await",	"co_return",
		"co_yield",		 "decltype",	"default",
		"delete",		 "do",			"double",
		"dynamic_cast",	 "else",		"enum",
		"explicit",		 "export",		"extern",
		"false",		 "float",		"for",
		"friend",		 "goto",		"if",
		"inline",		 "int",			"long",
		"mutable",		 "namespace",	"new",
		"noexcept",		 "not",			"not_eq",
		"nullptr",		 "operator",	"or",
		"or_eq",		 "private",		"protected",
		"public",		 "register",	"reinterpret_cast",
		"requires",		 "return",		"short",
		"signed",		 "sizeof",		"static",
		"static_assert", "static_cast", "struct",
		"switch",		 "template",	"this",
		"thread_local",	 "throw",		"true",
		"try",			 "typedef",		"typeid",
		"typename",		 "union",		"unsigned",
		"using",		 "virtual",		"void",
		"volatile",		 "wchar_t",		"while",
		"xor",			 "xor_eq"
		// Note: Some node types like 'if_statement' might contain the keyword 'if',
		// so checking nodeText can be useful too.
	};

	// Check if the node type itself is a keyword or if the text content is a keyword
	if (keywords.count(nodeType) || (nodeType == "identifier" && keywords.count(nodeText)))
	{
		return cachedColors.keyword;
	}
	if (nodeType == "storage_class_specifier" || // extern, static, register, mutable, thread_local
		nodeType == "type_qualifier" ||			 // const, volatile, restrict, _Atomic
		nodeType == "access_specifier")			 // public, private, protected
	{
		return cachedColors.keyword;
	}

	// --- Literals ---
	if (nodeType == "number_literal" || nodeType == "user_defined_literal")
	{ // Catches 123, 1.23, 1.23f, 123_udl
		return cachedColors.number;
	}
	if (nodeType == "string_literal" || nodeType == "raw_string_literal" ||
		nodeType == "concatenated_string")
	{
		return cachedColors.string;
	}
	if (nodeType == "char_literal")
	{
		return cachedColors.string; // Often styled like strings
	}
	if (nodeType == "boolean_literal" || nodeType == "null_literal")
	{ // true, false, nullptr
		return cachedColors.keyword;
	}

	// --- Comments ---
	if (nodeType == "comment")
	{
		return cachedColors.comment;
	}

	// --- Preprocessor ---
	if (nodeType.rfind("preproc_", 0) == 0)
	{ // Matches preproc_if, preproc_def, etc.
		// Could use a dedicated 'macro' color, but comment is common
		return cachedColors.comment;
	}
	if (nodeType == "system_lib_string" ||
		nodeType == "string_literal" && parentNodeType == "preproc_include")
	{ // <iostream> or "myheader.h" in #include
		return cachedColors.string;
	}

	// --- Types ---
	if (nodeType == "primitive_type")
	{ // int, float, bool, void, etc.
		return cachedColors.type;
	}
	if (nodeType == "type_identifier")
	{ // Usage of a type like MyClass, std::vector
		return cachedColors.type;
	}
	if (nodeType == "namespace_identifier" || nodeType == "template_type" ||
		nodeType == "template_function")
	{
		return cachedColors.type; // Often refer to or contain types
	}
	if (nodeType == "sized_type_specifier")
	{ // e.g. long long int
		return cachedColors.type;
	}

	// --- Functions ---
	if (nodeType == "function_definition")
	{
		// Definition itself doesn't get special color, identifier inside does (handled below)
	}
	if (nodeType == "call_expression")
	{
		// The expression itself doesn't get special color, identifier inside does (handled below)
	}
	if (nodeType == "operator_name")
	{ // operator+, operator=, etc.
		return cachedColors.function;
	}

	// --- Identifiers (Context is Key!) ---
	if (nodeType == "identifier")
	{
		// Is it a function name being declared?
		if (parentNodeType == "function_declarator")
		{
			return cachedColors.function;
		}
		// Is it a function name being called? (Simplistic check)
		if (parentNodeType == "call_expression")
		{
			return cachedColors.function;
		}
		// Is it a class, struct, enum, union, or namespace name being declared?
		if (parentNodeType == "class_specifier" || parentNodeType == "struct_specifier" ||
			parentNodeType == "enum_specifier" || parentNodeType == "union_specifier" ||
			parentNodeType == "namespace_definition" ||
			parentNodeType == "namespace_alias_definition")
		{
			return cachedColors.type;
		}
		// Is it a parameter name?
		if (parentNodeType == "parameter_declaration")
		{
			return cachedColors.variable; // Use variable color for parameters
		}
		// Is it a variable name being declared?
		if (parentNodeType == "init_declarator" || parentNodeType == "declaration")
		{
			// Needs more context, check siblings/grandparents? For now, assume variable.
			return cachedColors.variable;
		}
		// Is it a template parameter name?
		if (parentNodeType == "type_parameter_declaration")
		{
			return cachedColors.type; // Template params often represent types
		}
		// Is it part of a using declaration/directive?
		if (parentNodeType == "using_declaration")
		{
			return cachedColors.type; // Usually brings in types or namespaces
		}
		// Is it part of a qualified identifier (namespace::member)?
		if (parentNodeType == "qualified_identifier")
		{
			// Could be namespace, type, function, or variable. Needs more context.
			// Defaulting to 'type' might be okay as often it is namespace::Type or Type::member.
			return cachedColors.type;
		}

		// Default for unclassified identifiers - could be variable usage
		return cachedColors.variable; // Default identifier usage to variable (can be refined)
	}

	// --- Member Access ---
	if (nodeType == "field_identifier")
	{ // obj.member or ptr->member
		// This could be a data member or a function. Without more context (like trailing ()),
		// it's ambiguous. Defaulting to variable is a common choice.
		return cachedColors.variable;
	}

	// --- Fallback ---
	// If the node type didn't match any specific rule above
	return cachedColors.text;
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
	cachedColors.type = loadColor("type");
	cachedColors.variable = loadColor("variable");

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
