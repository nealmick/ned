#include "editor_tree_sitter.h"
#include "editor.h"
#include <fstream>
#include <iostream>
#include <vector>

// Define static members
bool TreeSitter::colorsNeedUpdate = true;
ThemeColors TreeSitter::cachedColors;

// Declare C++ language parser function
extern "C" TSLanguage *tree_sitter_cpp();

// ANSI color codes
#define COLOR_NODE "\033[33m" // Yellow
#define COLOR_POS "\033[36m"  // Cyan
#define COLOR_RESET "\033[0m"

void TreeSitter::parseAndPrintAST(const std::string &fileContent, std::vector<ImVec4> &fileColors)
{
    if (fileContent.empty()) {
        std::cerr << "No content to parse!\n";
        return;
    } else {
        updateThemeColors();
    }

    TSParser *parser = ts_parser_new();
    ts_parser_set_language(parser, tree_sitter_cpp());
    TSTree *tree = ts_parser_parse_string(parser, nullptr, fileContent.c_str(), fileContent.size());

    // std::cout << "\n" << COLOR_NODE "Abstract Syntax Tree:" COLOR_RESET "\n";
    traverseAndPrint(fileContent, fileColors, ts_tree_root_node(tree), 0, false, {});

    printColors();

    ts_tree_delete(tree);
    ts_parser_delete(parser);
}

void TreeSitter::traverseAndPrint(const std::string &fileContent, std::vector<ImVec4> &fileColors, TSNode node, int depth, bool is_last, const std::vector<bool> &hierarchy)
{

    if (ts_node_is_null(node)) {
        return;
    }

    // --- Skipping Logic ---
    // Using the skipping condition from your provided code.
    // This skips nodes that are UNNAMED unless they are NOT EXTRA and have CHILDREN.
    // It also skips nodes that are LEAF nodes (no children) AND are EXTRA (like comments).
    // Consider adjusting this if it filters too much (e.g., unnamed literals) or too little.
    // A simpler alternative might be: if (ts_node_is_extra(node)) return; // Skips comments etc.
    // Or: if (!ts_node_is_named(node) && ts_node_child_count(node) == 0) return; // Skips unnamed leaves
    if (!ts_node_is_named(node) && ts_node_is_extra(node)) {
        // Let's try only skipping unnamed extra nodes (often comments, whitespace nodes)
        return;
    }
    // Original complex condition:
    // if (ts_node_is_named(node) == false || (ts_node_child_count(node) == 0 && ts_node_is_extra(node))) {
    //     return;
    // }

    // --- Hierarchy Visualization ---
    std::string prefix;
    for (size_t i = 0; i < hierarchy.size(); ++i) { // Use size_t for loop index
        prefix += hierarchy[i] ? "    " : "│   ";
    }
    prefix += is_last ? "└── " : "├── ";

    // --- Position Info (using byte offsets) ---
    uint32_t start_byte = ts_node_start_byte(node);
    uint32_t end_byte = ts_node_end_byte(node);
    const char *type = ts_node_type(node);

    // --- Node Text Snippet ---
    std::string content = "ERR"; // Default error string
    // Basic bounds check: ensure end is not before start and within the file content size
    if (start_byte <= end_byte && end_byte <= fileContent.size()) {
        size_t length = end_byte - start_byte;
        // Double-check substr parameters are valid relative to string size
        if (start_byte + length <= fileContent.size()) {
            content = fileContent.substr(start_byte, length);
        } else {
            content = "ERR_BOUNDS"; // Indicate an issue with calculated length
        }

        // Clean up snippet: remove internal newlines and limit length for display
        size_t first_newline = content.find_first_of("\r\n");
        if (first_newline != std::string::npos) {
            content = content.substr(0, first_newline) + "..."; // Show only first line part
        }
        if (content.size() > 40) {
            content = content.substr(0, 37) + "..."; // Truncate long snippets
        }
    } else {
        // Indicate invalid byte range from tree-sitter or inconsistent file content size
        content = "ERR_RANGE";
    }

    const ImVec4 &color = convertNodeType(type);
    std::cout << COLOR_POS << "Idx " << start_byte << "-" << end_byte << COLOR_RESET << " " << prefix << "\033[38;2;" << int(color.x * 255) << ";" << int(color.y * 255) << ";" << int(color.z * 255) << "m" << type << COLOR_RESET << " \"" << content << "\"" << std::endl;
    setColors(fileContent, fileColors, static_cast<int>(start_byte), static_cast<int>(end_byte), color);
    // --- Recurse for Children ---
    std::vector<bool> child_hierarchy = hierarchy;
    child_hierarchy.push_back(is_last); // Add current node's 'last' status for child prefix calculation

    uint32_t child_count = ts_node_child_count(node);
    for (uint32_t i = 0; i < child_count; i++) {
        TSNode child = ts_node_child(node, i);
        bool last_child = (i == child_count - 1);
        // Recursive call: The skipping logic is handled at the start of the function now.
        traverseAndPrint(fileContent, fileColors, child, depth + 1, last_child, child_hierarchy);
    }
}
const ImVec4 &TreeSitter::convertNodeType(const std::string &nodeType)
{
    // Simple type-to-color mapping
    static const std::unordered_map<std::string, ImVec4 &> typeMap = {{"number_literal", cachedColors.number}, {"string_literal", cachedColors.string}, {"string_content", cachedColors.string}, {"char_literal", cachedColors.string}, {"comment", cachedColors.comment}, {"function_definition", cachedColors.function}, {"call_expression", cachedColors.function}, {"field_identifier", cachedColors.function}, {"primitive_type", cachedColors.keyword}, {"type_identifier", cachedColors.keyword}, {"if", cachedColors.keyword}, {"for", cachedColors.keyword}, {"while", cachedColors.keyword}, {"return", cachedColors.keyword}};

    if (auto it = typeMap.find(nodeType); it != typeMap.end()) {
        return it->second;
    }
    return cachedColors.text; // Default color
}

void TreeSitter::printColors()
{
    std::cout << "\nTheme Colors:" << std::endl;
    std::cout << "Keyword: (" << cachedColors.keyword.x * 255 << ", " << cachedColors.keyword.y * 255 << ", " << cachedColors.keyword.z * 255 << ", " << cachedColors.keyword.w * 255 << ")" << std::endl;
    std::cout << "String: (" << cachedColors.string.x * 255 << ", " << cachedColors.string.y * 255 << ", " << cachedColors.string.z * 255 << ", " << cachedColors.string.w * 255 << ")" << std::endl;
    std::cout << "Number: (" << cachedColors.number.x * 255 << ", " << cachedColors.number.y * 255 << ", " << cachedColors.number.z * 255 << ", " << cachedColors.number.w * 255 << ")" << std::endl;
    std::cout << "Comment: (" << cachedColors.comment.x * 255 << ", " << cachedColors.comment.y * 255 << ", " << cachedColors.comment.z * 255 << ", " << cachedColors.comment.w * 255 << ")" << std::endl;
    std::cout << "Text: (" << cachedColors.text.x * 255 << ", " << cachedColors.text.y * 255 << ", " << cachedColors.text.z * 255 << ", " << cachedColors.text.w * 255 << ")" << std::endl;
    std::cout << "Function: (" << cachedColors.function.x * 255 << ", " << cachedColors.function.y * 255 << ", " << cachedColors.function.z * 255 << ", " << cachedColors.function.w * 255 << ")" << std::endl;
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
void TreeSitter::setColors(const std::string &content, std::vector<ImVec4> &colors, int start, int end, const ImVec4 &color)
{
    // Always keep colors matching content size
    if (colors.size() != content.size()) {
        std::cout << " rsize mismatch red alert size mismatch !! " << std::endl;
        colors.resize(content.size(), cachedColors.text);
    }

    // Clamp indices to valid range
    start = std::clamp(start, 0, (int)colors.size() - 1);
    end = std::clamp(end, 0, (int)colors.size());

    for (int i = start; i < end; ++i) {
        colors[i] = color;
    }
}
