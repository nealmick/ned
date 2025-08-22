#include "editor_tree_sitter.h"
#include "../files/files.h"
#include "editor.h"
#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>

#ifndef PLATFORM_WINDOWS
#include <libgen.h> // For dirname
#include <unistd.h> // For getcwd, readlink
#else
#define PATH_MAX 260
#include <windows.h> // For GetModuleFileNameA
#endif
#include <limits.h> // Or <climits> for C++ style

#include <algorithm>
#include <iostream>

#ifdef __APPLE__
#include <CoreFoundation/CoreFoundation.h> // For macOS bundle functions
#endif

// Define static members
bool TreeSitter::colorsNeedUpdate = true;
ThemeColors TreeSitter::cachedColors;
TSParser *TreeSitter::parser = nullptr;
std::mutex TreeSitter::parserMutex;
std::unordered_map<std::string, TSQuery *> TreeSitter::queryCache;

// incremental parsing
TSTree *TreeSitter::previousTree = nullptr;
std::string TreeSitter::previousContent;

const TSLanguage *TreeSitter::currentLanguage = nullptr;
std::string TreeSitter::currentExtension = "";

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

	// First check if we're running in a bundle
	static bool isBundle = []() {
#ifdef __APPLE__
		CFURLRef bundleURL = CFBundleCopyBundleURL(CFBundleGetMainBundle());
		return bundleURL != nullptr;
#else
		return false;
#endif
	}();

	std::string query_prefix = isBundle ? "queries/" : "editor/queries/";
#ifdef __APPLE__
	query_prefix = isBundle ? "queries/" : "editor/queries/";
#else
	query_prefix = "queries/";
#endif
	if (extension == ".c")
	{
		return {tree_sitter_c(), query_prefix + "c.scm"};
	} else if (extension == ".cpp" || extension == ".h" || extension == ".hpp" ||
			   extension == ".mm")
	{
		return {tree_sitter_cpp(), query_prefix + "cpp.scm"};
	} else if (extension == ".js" || extension == ".jsx")
	{
		return {tree_sitter_javascript(), query_prefix + "jsx.scm"};
	} else if (extension == ".py")
	{
		return {tree_sitter_python(), query_prefix + "python.scm"};
	} else if (extension == ".cs")
	{
		return {tree_sitter_c_sharp(), query_prefix + "csharp.scm"};
	} else if (extension == ".html" || extension == ".cshtml")
	{
		return {tree_sitter_html(), query_prefix + "html.scm"};
	} else if (extension == ".tsx" || extension == ".ts")
	{
		return {tree_sitter_tsx(), query_prefix + "tsx.scm"};
	} else if (extension == ".css")
	{
		return {tree_sitter_css(), query_prefix + "css.scm"};
	} else if (extension == ".java")
	{
		return {tree_sitter_java(), query_prefix + "java.scm"};
	} else if (extension == ".go")
	{
		return {tree_sitter_go(), query_prefix + "go.scm"};
	} else if (extension == ".tf")
	{
		return {tree_sitter_hcl(), query_prefix + "hcl.scm"};
	} else if (extension == ".json")
	{
		return {tree_sitter_json(), query_prefix + "json.scm"};
	} else if (extension == ".sh")
	{
		return {tree_sitter_bash(), query_prefix + "sh.scm"};
	} else if (extension == ".kt")
	{
		return {tree_sitter_kotlin(), query_prefix + "kotlin.scm"};
	} else if (extension == ".rs")
	{
		return {tree_sitter_rust(), query_prefix + "rs.scm"};
	} else if (extension == ".toml")
	{
		return {tree_sitter_toml(), query_prefix + "toml.scm"};
	} else if (extension == ".rb")
	{
		return {tree_sitter_ruby(), query_prefix + "rb.scm"};
	}
	return {};
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

TSTree *
TreeSitter::createNewTree(TSParser *parser, bool initialParse, const std::string &content)
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

std::string TreeSitter::getResourcePath(const std::string &relativePath)
{
#ifdef __APPLE__
	CFBundleRef mainBundle = CFBundleGetMainBundle();
	if (mainBundle)
	{
		CFStringRef relPath = CFStringCreateWithCString(kCFAllocatorDefault,
														relativePath.c_str(),
														kCFStringEncodingUTF8);
		CFURLRef resourceURL = CFBundleCopyResourceURL(mainBundle, relPath, NULL, NULL);
		if (resourceURL)
		{
			char path[PATH_MAX];
			if (CFURLGetFileSystemRepresentation(
					resourceURL, true, (UInt8 *)path, PATH_MAX))
			{
				CFRelease(resourceURL);
				CFRelease(relPath);
				return std::string(path);
			}
			CFRelease(resourceURL);
		}
		CFRelease(relPath);
	}
#elif !defined(PLATFORM_WINDOWS)
	// --- Linux/Ubuntu Fix ---
	char exePath[PATH_MAX];
	ssize_t len = readlink("/proc/self/exe", exePath, sizeof(exePath) - 1);
	if (len != -1)
	{
		exePath[len] = '\0';
		std::string exeDir = dirname(exePath);

		// Construct path directly to "queries" folder (no "editor/queries" here)
		std::string path = exeDir + "/" + relativePath;

		// Debug
		std::cout << "[DEBUG] Final Query Path: " << path << std::endl;

		return path;
	}
	// Fallback (for development builds)
	return "queries/" + relativePath; // Not "editor/queries"
#else
	// Windows - get executable path and construct relative path
	char exePath[PATH_MAX];
	DWORD pathLength = GetModuleFileNameA(NULL, exePath, PATH_MAX);
	if (pathLength > 0 && pathLength < PATH_MAX)
	{
		// Get directory containing the executable
		std::string exeDir(exePath);
		size_t lastSlash = exeDir.find_last_of("\\");
		if (lastSlash != std::string::npos)
		{
			exeDir = exeDir.substr(0, lastSlash);

			// For portable builds: Check if queries folder exists relative to exe
			std::string portablePath = exeDir + "\\" + relativePath;
			std::ifstream testFile(portablePath);
			if (testFile.good())
			{
				std::cout << "[DEBUG] Windows Portable Query Path: " << portablePath
						  << std::endl;
				return portablePath;
			}

			// For development builds: Go up one level from Release to build directory
			size_t secondLastSlash = exeDir.find_last_of("\\");
			if (secondLastSlash != std::string::npos)
			{
				std::string buildDir = exeDir.substr(0, secondLastSlash);
				std::string devPath = buildDir + "\\" + relativePath;
				std::cout << "[DEBUG] Windows Dev Query Path: " << devPath << std::endl;
				return devPath;
			}
		}
	}
	// Fallback for development builds
	return "..\\" + relativePath;
#endif
	// Fallback for development environment
	return "editor/queries/" + relativePath;
}

TSQuery *TreeSitter::loadQueryFromCacheOrFile(TSLanguage *lang,
											  const std::string &query_path)
{
	std::string full_path = getResourcePath(query_path);

	// Use full_path as the cache key consistently
	auto cacheIt = queryCache.find(full_path);
	if (cacheIt != queryCache.end())
	{
		return cacheIt->second;
	}

	std::ifstream file(full_path);
	if (!file.is_open())
	{
		std::cerr << "Failed to open query file: " << full_path << "\n";
		return nullptr;
	}
	std::string query_src((std::istreambuf_iterator<char>(file)),
						  std::istreambuf_iterator<char>());
	file.close();

	uint32_t error_offset;
	TSQueryError error_type;
	TSQuery *query = ts_query_new(
		lang, query_src.c_str(), query_src.size(), &error_offset, &error_type);

	if (!query)
	{
		std::cerr << "Query error (" << error_type << ") at offset " << error_offset
				  << "\n";
		return nullptr;
	}

	// Store using full_path as key
	queryCache[full_path] = query;
	return query;
}
void TreeSitter::clearQueryCache()
{
	std::lock_guard<std::mutex> lock(parserMutex);
	for (auto &[key, query] : queryCache)
	{
		ts_query_delete(query);
	}
	queryCache.clear();
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
	ts_query_cursor_exec(cursor, query, ts_tree_root_node(tree));

	// Get theme colors once
	const ImVec4 text_color = cachedColors.text;

	// FIRST: Set ALL text to default color
	std::fill(colors.begin(), colors.end(), text_color);

	// THEN apply syntax highlights
	const std::unordered_map<std::string, ImVec4> capture_colors = {
		{"keyword", cachedColors.keyword},
		{"string", cachedColors.string},
		{"number", cachedColors.number},
		{"comment", cachedColors.comment},
		{"type", cachedColors.type},
		{"function", cachedColors.function},
		{"variable", cachedColors.variable},
		{"tag", cachedColors.type},			 // Components
		{"attribute", cachedColors.number},	 // JSX attributes
		{"property", cachedColors.variable}, // Object properties
		{"hook", cachedColors.function},	 // React hooks
		{"variable.parameter", cachedColors.variable},
		{"punctuation.special", cachedColors.string}};

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

			const ImVec4 color = capture_colors.count(name)
									 ? capture_colors.at(name)
									 : text_color; // Fallback to text color

			const uint32_t start = ts_node_start_byte(node);
			const uint32_t end = ts_node_end_byte(node);
			setColors(content, colors, start, end, color);
		}
	}

	ts_query_cursor_delete(cursor);
}

void TreeSitter::parse(const std::string &fileContent,
					   std::vector<ImVec4> &fileColors,
					   const std::string &extension,
					   bool fullRehighlight)
{

	std::lock_guard<std::mutex> lock(parserMutex);
	if (fileContent.empty())
	{
		std::cerr << "No content to parse!\n";
		return;
	}
	static std::string lastFile;
	if (lastFile != gFileExplorer.currentFile)
	{
		if (previousTree)
		{
			ts_tree_delete(previousTree);
			previousTree = nullptr;
		}
		previousContent.clear();
		lastFile = gFileExplorer.currentFile;
	}
	if (fullRehighlight)
	{
		if (previousTree)
		{
			ts_tree_delete(previousTree);
			previousTree = nullptr;
		}
		previousContent.clear();
		lastFile.clear(); // Force reinitialization
	}
	updateThemeColors();
	TSParser *parser = getParser();

	// Language detection
	auto [lang, query_path] = detectLanguageAndQuery(extension);
	if (!lang)
	{
		// std::cerr << "No parser for extension: " << extension << std::endl;
		return;
	}

	// Reset state if language changed
	if (currentLanguage != lang || currentExtension != extension)
	{
		if (previousTree)
		{
			ts_tree_delete(previousTree);
			previousTree = nullptr;
		}
		previousContent.clear();
		currentLanguage = lang;
		currentExtension = extension;
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
	// printAST(newTree, fileContent); // <-- This line replaces the lambda
	//   Update state
	if (previousTree)
		ts_tree_delete(previousTree);
	previousTree = newTree;
	previousContent = fileContent;

	// Handle query execution
	TSQuery *query = loadQueryFromCacheOrFile(lang, query_path);
	if (!query)
		return;

	executeQueryAndHighlight(
		query, newTree, fileContent, fileColors, initialParse, start, newEnd);
}

void TreeSitter::printAST(TSTree *tree, const std::string &fileContent)
{
	if (!tree)
		return;

	std::cout << std::endl << "🌳 \033[1;34mABSTRACT SYNTAX TREE\033[0m 🌳" << std::endl;
	printASTNode(ts_tree_root_node(tree), fileContent);
	std::cout << "──────────────────────────────────────" << std::endl;
}

void TreeSitter::printASTNode(TSNode node, const std::string &fileContent, int depth)
{
	if (ts_node_is_null(node))
		return;

	uint32_t start_byte = ts_node_start_byte(node);
	uint32_t end_byte = ts_node_end_byte(node);
	std::string_view node_text_sv(fileContent.c_str() + start_byte, end_byte - start_byte);

	// Existing: Skip nodes that are purely punctuation or empty
	if (node_text_sv.empty() ||
		node_text_sv.find_first_not_of("(){};,") == std::string_view::npos)
		return;

	// --- Simplified Indentation ---
	std::string simple_indent(depth * 2, ' '); // Each depth level is 2 spaces

	// --- Snippet Preparation (same as before) ---
	std::string inline_node_snippet = std::string(node_text_sv);
	size_t pos = 0;
	while ((pos = inline_node_snippet.find('\n', pos)) != std::string::npos)
	{
		inline_node_snippet.replace(pos, 1, " ");
		pos += 1;
	}
	const char *whitespace_chars = " \t\r\n\v\f";
	size_t first_char = inline_node_snippet.find_first_not_of(whitespace_chars);
	if (std::string::npos == first_char)
	{
		inline_node_snippet.clear();
	} else
	{
		size_t last_char = inline_node_snippet.find_last_not_of(whitespace_chars);
		inline_node_snippet =
			inline_node_snippet.substr(first_char, (last_char - first_char + 1));
	}
	const size_t MAX_INLINE_SNIPPET_LENGTH = 35;
	if (inline_node_snippet.length() > MAX_INLINE_SNIPPET_LENGTH)
	{
		inline_node_snippet =
			inline_node_snippet.substr(0, MAX_INLINE_SNIPPET_LENGTH - 3) + "...";
	}
	// --- End Snippet Preparation ---

	// --- Second line preview (same as before) ---
	std::string second_line_preview(node_text_sv);
	second_line_preview = second_line_preview.substr(0, second_line_preview.find('\n'));
	if (second_line_preview.length() > 40)
	{
		second_line_preview = second_line_preview.substr(0, 37) + "...";
	}
	// --- End Second line preview ---

	// --- Simplified Print Output ---
	std::cout << simple_indent << "\033[33m" << ts_node_type(node) << "\033[0m"
			  << " \033[90m[\033[37m" << inline_node_snippet << "\033[90m]\033[0m"
			  << std::endl;
	// Optionally print the second line preview if you still find it useful
	if (!second_line_preview.empty() && second_line_preview != inline_node_snippet)
	{									   // Avoid redundant print
		std::cout << simple_indent << "  " // Indent second line slightly more
				  << "\033[37m" << second_line_preview << "\033[0m" << std::endl;
	}

	// Recursive children call (same as before)
	uint32_t child_count = ts_node_child_count(node);
	for (uint32_t i = 0; i < child_count; i++)
	{
		TSNode child = ts_node_child(node, i);
		printASTNode(child, fileContent,
					 depth + 1); // Depth is correctly incremented
	}
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
	// Use content.size() for clamping instead of colors.size()
	start = std::clamp(start, 0, static_cast<int>(content.size()) - 1);
	end = std::clamp(end, 0, static_cast<int>(content.size()));

	for (int i = start; i < end; ++i)
	{
		// Ensure colors vector is accessed safely
		if (i < colors.size())
		{
			colors[i] = color;
		}
	}
}
