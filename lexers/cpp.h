#pragma once

#include <string>
#include <vector>
#include <unordered_set>
#include <unordered_map>
#include "imgui.h"
#include <iostream>
#include "../util/settings.h"  // Add this include

class Settings;  // Forward declaration
extern Settings gSettings;  // Declare the external global variable

namespace CppLexer {


enum class TokenType {
    Whitespace,
    Identifier,
    Keyword,
    String,
    Number,
    Comment,
    Preprocessor,
    Operator,
    Parenthesis,
    Bracket,
    Brace,
    Semicolon,
    Comma,
    Dot,
    Unknown
};

struct Token {
    TokenType type;
    size_t start;
    size_t length;
};

class Lexer {
public:
    // Theme colors cache
    struct ThemeColors {
        ImVec4 keyword;
        ImVec4 string;
        ImVec4 number;
        ImVec4 comment;
        ImVec4 text;
    };

    Lexer() {
        keywords = {"auto", "break", "case", "char", "const", "continue", "default", "do", "double",
                    "else", "enum", "extern", "float", "for", "goto", "if", "int", "long", "register",
                    "return", "short", "signed", "sizeof", "static", "struct", "switch", "typedef",
                    "union", "unsigned", "void", "volatile", "while", "class", "namespace", "try",
                    "catch", "throw", "new", "delete", "public", "private", "protected", "virtual",
                    "friend", "inline", "template", "typename", "using", "bool", "true", "false",
                    "nullptr", "and", "or", "not", "xor", "and_eq", "or_eq", "not_eq", "xor_eq",
                    "bitand", "bitor", "compl", "constexpr", "decltype", "mutable", "noexcept",
                    "static_assert", "thread_local", "alignas", "alignof", "char16_t", "char32_t",
                    "export", "explicit", "final", "override", "operator", "this"};
        
        operators = {
            {"+", TokenType::Operator}, {"-", TokenType::Operator}, {"*", TokenType::Operator}, {"/", TokenType::Operator},
            {"%", TokenType::Operator}, {"=", TokenType::Operator}, {"==", TokenType::Operator}, {"!=", TokenType::Operator},
            {">", TokenType::Operator}, {"<", TokenType::Operator}, {">=", TokenType::Operator}, {"<=", TokenType::Operator},
            {"&&", TokenType::Operator}, {"||", TokenType::Operator}, {"!", TokenType::Operator}, {"&", TokenType::Operator},
            {"|", TokenType::Operator}, {"^", TokenType::Operator}, {"~", TokenType::Operator}, {"<<", TokenType::Operator},
            {">>", TokenType::Operator}, {"++", TokenType::Operator}, {"--", TokenType::Operator}, {"->", TokenType::Operator},
            {".*", TokenType::Operator}, {"->*", TokenType::Operator}, {"::", TokenType::Operator}
        };
    }
    void themeChanged() {
        colorsNeedUpdate = true;
    }
    std::vector<Token> tokenize(const std::string& code) {
        std::cout << "Inside C++ tokenizer.." << std::endl;
        std::vector<Token> tokens;
        size_t pos = 0;
        size_t lastPos = 0;
        size_t maxIterations = code.length() * 2;
        size_t iterations = 0;

        try {
            std::cout << "Starting C++ tokenization loop" << std::endl;
            while (pos < code.length() && iterations < maxIterations) {
                lastPos = pos;
                if (isWhitespace(code[pos])) {
                    tokens.push_back({TokenType::Whitespace, pos, 1});
                    pos++;
                } else if (code[pos] == '#') {
                    tokens.push_back(lexPreprocessor(code, pos));
                } else if (isAlpha(code[pos]) || code[pos] == '_') {
                    tokens.push_back(lexIdentifierOrKeyword(code, pos));
                } else if (isDigit(code[pos])) {
                    tokens.push_back(lexNumber(code, pos));
                } else if (code[pos] == '/' && pos + 1 < code.length() && (code[pos + 1] == '/' || code[pos + 1] == '*')) {
                    tokens.push_back(lexComment(code, pos));
                } else if (code[pos] == '"' || code[pos] == '\'') {
                    tokens.push_back(lexString(code, pos));
                } else {
                    tokens.push_back(lexOperatorOrPunctuation(code, pos));
                }
                
                if (pos == lastPos) {
                    std::cerr << "C++ Tokenizer stuck at position " << pos << ", character: '" << code[pos] << "'" << std::endl;
                    pos++;
                }
                
                iterations++;
            }
            
            if (iterations >= maxIterations) {
                std::cerr << "C++ Tokenizer exceeded maximum iterations. Possible infinite loop." << std::endl;
            }
            
            std::cout << "Finished C++ tokenization loop" << std::endl;
        } catch (const std::exception& e) {
            std::cerr << "Exception in C++ tokenize: " << e.what() << std::endl;
        } catch (...) {
            std::cerr << "Unknown exception in C++ tokenize" << std::endl;
        }
        
        std::cout << "Exiting C++ tokenizer, tokens size: " << tokens.size() << std::endl;
        return tokens;
    }
    void applyHighlighting(const std::string& code, std::vector<ImVec4>& colors, int start_pos) {
        std::cout << "Entering C++ applyHighlighting, code length: " << code.length()
                << ", colors size: " << colors.size() << ", start_pos: " << start_pos << std::endl;
        try {
            std::vector<Token> tokens = tokenize(code.substr(start_pos));
            std::cout << "After tokenize, tokens size: " << tokens.size() << std::endl;
            
            int colorChanges = 0;
            for (const auto& token : tokens) {
                ImVec4 color = getColorForTokenType(token.type);
                for (size_t i = 0; i < token.length; ++i) {
                    size_t index = start_pos + token.start + i;
                    if (index < colors.size()) {
                        colors[index] = color;
                        colorChanges++;
                    }
                }
            }
            std::cout << "Exiting C++ applyHighlighting, changed " << colorChanges << " color values" << std::endl;
        } catch (const std::exception& e) {
            std::cerr << "Exception in C++ applyHighlighting: " << e.what() << std::endl;
            std::fill(colors.begin() + start_pos, colors.end(), ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
        } catch (...) {
            std::cerr << "Unknown exception in C++ applyHighlighting" << std::endl;
            std::fill(colors.begin() + start_pos, colors.end(), ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
        }
    }
    void forceColorUpdate() {
       colorsNeedUpdate = true;
    }
private:
    std::unordered_set<std::string> keywords;
    std::unordered_map<std::string, TokenType> operators;
    mutable ThemeColors cachedColors;
    mutable bool colorsNeedUpdate = true;
    void updateThemeColors() const {
        if (!colorsNeedUpdate) return;  // Already have this
        
        auto& theme = gSettings.getSettings()["themes"][gSettings.getCurrentTheme()];
        
        auto loadColor = [&theme](const char* key) -> ImVec4 {
            auto& c = theme[key];
            return ImVec4(c[0], c[1], c[2], c[3]);
        };

        cachedColors.keyword = loadColor("keyword");
        cachedColors.string = loadColor("string");
        cachedColors.number = loadColor("number");
        cachedColors.comment = loadColor("comment");
        cachedColors.text = loadColor("text");
        
        colorsNeedUpdate = false;
    }


    bool isWhitespace(char c) const { return c == ' ' || c == '\t' || c == '\n' || c == '\r'; }
    bool isAlpha(char c) const { return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'); }
    bool isDigit(char c) const { return c >= '0' && c <= '9'; }
    bool isAlphaNumeric(char c) const { return isAlpha(c) || isDigit(c) || c == '_'; }

    Token lexIdentifierOrKeyword(const std::string& code, size_t& pos) {
        size_t start = pos;
        while (pos < code.length() && isAlphaNumeric(code[pos])) pos++;
        std::string word = code.substr(start, pos - start);
        return {keywords.find(word) != keywords.end() ? TokenType::Keyword : TokenType::Identifier, start, pos - start};
    }

    Token lexNumber(const std::string& code, size_t& pos) {
        size_t start = pos;
        while (pos < code.length() && (isDigit(code[pos]) || code[pos] == '.' || code[pos] == 'e' || code[pos] == 'E' || 
               code[pos] == '+' || code[pos] == '-' || code[pos] == 'f' || code[pos] == 'F' || 
               code[pos] == 'l' || code[pos] == 'L' || code[pos] == 'u' || code[pos] == 'U')) {
            pos++;
        }
        return {TokenType::Number, start, pos - start};
    }

    Token lexString(const std::string& code, size_t& pos) {
        size_t start = pos;
        char quote = code[pos++];
        while (pos < code.length() && code[pos] != quote) {
            if (code[pos] == '\\' && pos + 1 < code.length()) pos++;
            pos++;
        }
        if (pos < code.length()) pos++;
        return {TokenType::String, start, pos - start};
    }

    Token lexComment(const std::string& code, size_t& pos) {
        size_t start = pos;
        if (code[pos + 1] == '/') {
            pos += 2;
            while (pos < code.length() && code[pos] != '\n') pos++;
        } else {
            pos += 2;
            while (pos + 1 < code.length() && !(code[pos] == '*' && code[pos + 1] == '/')) pos++;
            if (pos + 1 < code.length()) pos += 2;
        }
        return {TokenType::Comment, start, pos - start};
    }

    Token lexPreprocessor(const std::string& code, size_t& pos) {
        size_t start = pos;
        while (pos < code.length() && code[pos] != '\n') {
            if (code[pos] == '\\' && pos + 1 < code.length() && code[pos + 1] == '\n') {
                pos += 2;
            } else {
                pos++;
            }
        }
        return {TokenType::Preprocessor, start, pos - start};
    }
    Token lexOperatorOrPunctuation(const std::string& code, size_t& pos) {
        size_t start = pos;
        char c = code[pos];

        // Handle single-character tokens
        if (c == '(' || c == ')') return {TokenType::Parenthesis, start, 1};
        if (c == '[' || c == ']') return {TokenType::Bracket, start, 1};
        if (c == '{' || c == '}') return {TokenType::Brace, start, 1};
        if (c == ';') return {TokenType::Semicolon, start, 1};
        if (c == ',') return {TokenType::Comma, start, 1};
        if (c == '.') return {TokenType::Dot, start, 1};

        // Handle multi-character operators
        std::string op;
        while (pos < code.length() && !isAlphaNumeric(code[pos]) && !isWhitespace(code[pos])) {
            op += code[pos];
            if (operators.find(op) != operators.end()) {
                pos++;
                return {TokenType::Operator, start, op.length()};
            }
            pos++;
        }

        // If we didn't find a known operator, treat it as a single-character unknown token
        if (op.empty()) {
            pos++;
            return {TokenType::Unknown, start, 1};
        }

        // We found some unknown multi-character operator
        return {TokenType::Unknown, start, op.length()};
    }

    ImVec4 getColorForTokenType(TokenType type) const {
        updateThemeColors();
        
        switch (type) {
            case TokenType::Keyword:  return cachedColors.keyword;
            case TokenType::String:   return cachedColors.string;
            case TokenType::Number:   return cachedColors.number;
            case TokenType::Comment:  return cachedColors.comment;
            case TokenType::Identifier:
            default:                  return cachedColors.text;
        }
    }
};

} // namespace CppLexer