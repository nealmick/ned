#pragma once

#include <iostream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "../util/settings.h"
#include "imgui.h"

class Settings;
extern Settings gSettings;

namespace JsxLexer {
enum class TokenType {
    Function,
    Whitespace,
    Identifier,
    Keyword,
    String,
    TemplateString,
    Number,
    Comment,
    Operator,
    Parenthesis,
    Bracket,
    Brace,
    Colon,
    Comma,
    Dot,
    Unknown,
    // JSX-specific types
    JSXTag,
    JSXAttribute,
    JSXText,
    JSXComment,
    ReactHook,
    ComponentName,
    TemplateLiteral,
    ArrowFunction,
    Destructuring,
    ScopeOperator
};
struct Token {
    TokenType type;
    size_t start;
    size_t length;
};

class Lexer {
  public:
    struct ThemeColors {
        ImVec4 keyword;
        ImVec4 string;
        ImVec4 number;
        ImVec4 comment;
        ImVec4 text;
        ImVec4 function;
        ImVec4 className;
        ImVec4 jsxTag;
    };

    Lexer() {
        keywords = {"function", "const", "let", "var", "if", "else", "return", "import", "export", "default", "class", "extends", "super", "this", "new", "try", "catch", "throw", "typeof", "instanceof", "async", "await", "for", "of", "while", "do", "switch", "case", "break", "continue", "static", "true", "false", "null", "undefined", "void", "delete", "yield", "interface", "type", "as", "from", "in", "is"};

        reactKeywords = {"useState", "useEffect", "useContext", "useReducer", "useCallback", "useMemo", "useRef", "useLayoutEffect", "useImperativeHandle", "Fragment", "createContext", "createRef", "forwardRef"};

        operators = {{"=", TokenType::Operator}, {"+", TokenType::Operator}, {"-", TokenType::Operator}, {"*", TokenType::Operator}, {"/", TokenType::Operator}, {"%", TokenType::Operator}, {"==", TokenType::Operator}, {"===", TokenType::Operator}, {"!=", TokenType::Operator}, {"!==", TokenType::Operator}, {">", TokenType::Operator}, {"<", TokenType::Operator}, {">=", TokenType::Operator}, {"<=", TokenType::Operator}, {"&&", TokenType::Operator}, {"||", TokenType::Operator}, {"!", TokenType::Operator}, {"?", TokenType::Operator}, {":", TokenType::Colon}, {"=>", TokenType::ArrowFunction}, {"...", TokenType::Destructuring}, {"::", TokenType::ScopeOperator}};

        colorsNeedUpdate = true;
    }

    void themeChanged() { colorsNeedUpdate = true; }

    std::vector<Token> tokenize(const std::string &code) {
        std::vector<Token> tokens;
        size_t pos = 0;
        size_t maxIterations = code.length() * 2;
        size_t iterations = 0;
        bool inJSX = false;
        bool inTemplate = false;

        try {
            while (pos < code.length() && iterations < maxIterations) {
                if (inJSX) {
                    Token jsxToken = lexJSX(code, pos, inJSX);
                    tokens.push_back(jsxToken);
                } else if (inTemplate) {
                    Token templateToken = lexTemplateString(code, pos, inTemplate);
                    tokens.push_back(templateToken);
                } else if (isWhitespace(code[pos])) {
                    tokens.push_back({TokenType::Whitespace, pos, 1});
                    pos++;
                } else if (code[pos] == '/' && pos + 1 < code.length() && code[pos + 1] == '/') {
                    tokens.push_back(lexComment(code, pos));
                } else if (code[pos] == '<' && !inTemplate) {
                    inJSX = true;
                    tokens.push_back(lexJSX(code, pos, inJSX));
                } else if (code[pos] == '`') {
                    inTemplate = true;
                    tokens.push_back(lexTemplateString(code, pos, inTemplate));
                } else if (isAlpha(code[pos]) || code[pos] == '_' || code[pos] == '$') {
                    tokens.push_back(lexIdentifierOrKeyword(code, pos));
                } else if (isDigit(code[pos])) {
                    tokens.push_back(lexNumber(code, pos));
                } else if (code[pos] == '"' || code[pos] == '\'') {
                    tokens.push_back(lexString(code, pos));
                } else {
                    tokens.push_back(lexOperatorOrPunctuation(code, pos));
                }

                iterations++;
            }
        } catch (...) {
            std::cerr << "Error in JSX tokenizer" << std::endl;
        }

        return tokens;
    }
    void applyHighlighting(const std::string &code, std::vector<ImVec4> &colors, int start_pos) {
        try {
            std::vector<Token> tokens = tokenize(code);
            for (const auto &token : tokens) {
                // Add safety checks like C++ lexer
                if (token.start >= code.size())
                    continue;
                size_t end = std::min(token.start + token.length, code.size());

                ImVec4 color = getColorForTokenType(token.type);
                for (size_t i = token.start; i < end; ++i) {
                    if ((start_pos + i) < colors.size()) {
                        colors[start_pos + i] = color;
                    }
                }
            }
        } catch (const std::exception &e) {
            std::cerr << "JSX Highlight Error: " << e.what() << "\n";
            std::fill(colors.begin() + start_pos, colors.end(), ImVec4(1, 1, 1, 1));
        }
    }
    void forceColorUpdate() { colorsNeedUpdate = true; }

  private:
    std::unordered_set<std::string> keywords;
    std::unordered_set<std::string> reactKeywords;
    std::unordered_map<std::string, TokenType> operators;
    mutable ThemeColors cachedColors;
    mutable bool colorsNeedUpdate = true;
    void updateThemeColors() const {
        if (!colorsNeedUpdate)
            return;

        auto &theme = gSettings.getSettings()["themes"][gSettings.getCurrentTheme()];

        auto loadColor = [&theme](const char *key) -> ImVec4 {
            if (theme.contains(key) && theme[key].is_array() && theme[key].size() >= 4) {
                return ImVec4(theme[key][0], theme[key][1], theme[key][2], theme[key][3]);
            }
            std::cerr << "Missing color key: " << key << " - using white\n";
            return ImVec4(1, 1, 1, 1);
        };

        cachedColors.keyword = loadColor("keyword");
        cachedColors.string = loadColor("string");
        cachedColors.number = loadColor("number");
        cachedColors.comment = loadColor("comment");
        cachedColors.text = loadColor("text");
        cachedColors.function = loadColor("function");
        cachedColors.jsxTag = loadColor("tag"); // Use existing HTML tag color
        cachedColors.className = loadColor("className");

        colorsNeedUpdate = false;
    }

    bool isWhitespace(char c) const { return c == ' ' || c == '\t' || c == '\n' || c == '\r'; }
    bool isAlpha(char c) const { return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'); }
    bool isDigit(char c) const { return c >= '0' && c <= '9'; }
    bool isAlphaNumeric(char c) const { return isAlpha(c) || isDigit(c) || c == '_' || c == '$'; }

    Token lexIdentifierOrKeyword(const std::string &code, size_t &pos) {
        size_t start = pos;
        while (pos < code.length() && isAlphaNumeric(code[pos]))
            pos++;
        std::string word = code.substr(start, pos - start);

        if (reactKeywords.count(word)) {
            return {TokenType::ReactHook, start, pos - start};
        }
        if (keywords.count(word)) {
            return {TokenType::Keyword, start, pos - start};
        }
        if (word[0] >= 'A' && word[0] <= 'Z') {
            return {TokenType::ComponentName, start, pos - start};
        }
        return {TokenType::Identifier, start, pos - start};
    }

    Token lexJSX(const std::string &code, size_t &pos, bool &inJSX) {
        size_t start = pos;
        if (code[pos] == '<') {
            pos++;
            if (code[pos] == '/') { // Closing tag
                // Existing closing tag logic
            }

            // Detect tag type
            bool isComponent = (pos < code.length() && code[pos] >= 'A' && code[pos] <= 'Z');
            TokenType type = isComponent ? TokenType::ComponentName : TokenType::JSXTag;

            // Skip tag name
            while (pos < code.length() && !isWhitespace(code[pos]) && code[pos] != '>' && code[pos] != '/') {
                pos++;
            }

            // Parse attributes
            while (pos < code.length() && code[pos] != '>') {
                if (code[pos] == '{')
                    break;

                // Attribute detection
                if (code[pos] == '=') {
                    size_t attrStart = pos;
                    while (attrStart > start && !isWhitespace(code[attrStart - 1]))
                        attrStart--;
                    // Could store attributes here if needed
                }
                pos++;
            }

            if (code[pos] == '>')
                pos++;
            return {type, start, pos - start};
        } else {
            // JSX text content
            while (pos < code.length() && code[pos] != '<' && code[pos] != '{')
                pos++;
            return {TokenType::JSXText, start, pos - start};
        }
    }

    Token lexTemplateString(const std::string &code, size_t &pos, bool &inTemplate) {
        size_t start = pos;
        pos++; // Skip opening `
        while (pos < code.length()) {
            if (code[pos] == '`') {
                pos++;
                inTemplate = false;
                break;
            }
            if (code[pos] == '$' && pos + 1 < code.length() && code[pos + 1] == '{') {
                pos += 2;
                break;
            }
            pos++;
        }
        return {TokenType::TemplateString, start, pos - start};
    }

    Token lexComment(const std::string &code, size_t &pos) {
        size_t start = pos;
        pos += 2; // Skip //
        while (pos < code.length() && code[pos] != '\n')
            pos++;
        return {TokenType::Comment, start, pos - start};
    }

    Token lexString(const std::string &code, size_t &pos) {
        size_t start = pos;
        char quote = code[pos++];
        while (pos < code.length() && code[pos] != quote) {
            if (code[pos] == '\\' && pos + 1 < code.length())
                pos++;
            pos++;
        }
        if (pos < code.length())
            pos++;
        return {TokenType::String, start, pos - start};
    }

    Token lexNumber(const std::string &code, size_t &pos) {
        size_t start = pos;
        while (pos < code.length() && (isDigit(code[pos]) || code[pos] == '.'))
            pos++;
        return {TokenType::Number, start, pos - start};
    }

    Token lexOperatorOrPunctuation(const std::string &code, size_t &pos) {
        size_t start = pos;
        char c = code[pos];

        // Check multi-character operators
        std::string op(1, c);
        if (pos + 1 < code.length()) {
            std::string twoChar = code.substr(pos, 2);
            if (operators.count(twoChar)) {
                pos += 2;
                return {operators[twoChar], start, 2};
            }
        }

        // Single character operators
        if (c == '(' || c == ')') {
            pos++;
            return {TokenType::Parenthesis, start, 1};
        }
        if (c == '{' || c == '}') {
            pos++;
            return {TokenType::Brace, start, 1};
        }
        if (c == '[' || c == ']') {
            pos++;
            return {TokenType::Bracket, start, 1};
        }
        if (c == ',') {
            pos++;
            return {TokenType::Comma, start, 1};
        }
        if (c == '.') {
            pos++;
            return {TokenType::Dot, start, 1};
        }

        // Default operator handling
        pos++;
        return {operators.count(op) ? operators[op] : TokenType::Unknown, start, 1};
    }

    ImVec4 getColorForTokenType(TokenType type) const {
        updateThemeColors();

        switch (type) {
        case TokenType::Keyword:
        case TokenType::ReactHook:
        case TokenType::Destructuring:
            return cachedColors.keyword;

        case TokenType::String:
        case TokenType::TemplateString:
            return cachedColors.string;

        case TokenType::Number:
            return cachedColors.number;

        case TokenType::Comment:
            return cachedColors.comment;

        case TokenType::JSXTag:
        case TokenType::ComponentName:
            return cachedColors.jsxTag; // Now safely mapped

        case TokenType::Function:
        case TokenType::ArrowFunction:
            return cachedColors.function;

        case TokenType::Operator:
            return ImVec4(cachedColors.text.x * 0.8f, cachedColors.text.y * 0.8f, cachedColors.text.z * 0.8f, cachedColors.text.w);

        default:
            return cachedColors.text;
        }
    }
};
} // namespace JsxLexer