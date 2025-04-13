#include <iostream>
#include <stack> // Useful for brace/tag matching
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
    // JS Types
    Function,
    Whitespace,
    Identifier,
    Keyword,
    String,
    TemplateString,
    TemplateExprStart, // ${
    TemplateExprEnd,   // } closing an expression in a template string  // <<< ADDED
    Number,
    Comment,
    Operator,
    Parenthesis,
    Bracket,
    Brace, // { } JS Block/Object or JSX Expression Start/End
    Colon,
    Comma,
    Dot,
    ArrowFunction,
    Destructuring,
    ScopeOperator,
    Unknown,
    // JSX Types
    JSXTagStart, // < or </
    JSXTagEnd,   // > or />
    JSXTagName,
    JSXComponentName,
    JSXAttributeName,
    JSXAttributeEquals,
    JSXAttributeValue, // Usually String, maybe remove if redundant
    JSXText,
    JSXExprStart, // { inside JSX (distinct from JS brace)
    JSXExprEnd,   // } closing an expression in JSX (distinct from JS brace)
    ReactHook,
};

struct Token
{
    TokenType type;
    size_t start;
    size_t length;
};

// Represents the current parsing context
enum class LexerState {
    JS,                       // Default JavaScript mode
    JSX_OPEN_TAG,             // Immediately after < expecting tag name
    JSX_CLOSING_TAG_NAME,     // Immediately after </ expecting tag name   // <<< ADDED
    JSX_TAG_BODY,             // Inside <tag ... > before >, parsing attributes
    JSX_EXPECT_CLOSE_BRACKET, // After </tag expecting >             // <<< ADDED
    JSX_CHILDREN,             // Between <tag> and </tag> or after <tag/>
    JSX_EXPRESSION,           // Inside { ... } expression within JSX
    TEMPLATE_LITERAL          // Inside `...` template literal
};

class Lexer
{
    // ... (Constructor, ThemeColors, themeChanged(), etc. remain the same) ...
  public:
    struct ThemeColors
    {
        ImVec4 keyword;
        ImVec4 string;
        ImVec4 number;
        ImVec4 comment;
        ImVec4 text;
        ImVec4 function;
        ImVec4 componentName; // Different color for Components?
        ImVec4 jsxTag;        // For standard HTML-like tags
        ImVec4 jsxAttribute;
        ImVec4 reactHook;
        ImVec4 operatorColor; // Keep distinct operator color
    };

    Lexer()
    {
        // Consolidate keywords
        keywords = {"function", "const", "let", "var", "if", "else", "return", "import", "export", "default", "class", "extends", "super", "this", "new", "try", "catch", "throw", "typeof", "instanceof", "async", "await", "for", "of", "while", "do", "switch", "case", "break", "continue", "static", "true", "false", "null", "undefined", "void", "delete", "yield", "interface", "type", "as", "from", "in", "is"};

        reactKeywords = {"useState", "useEffect", "useContext", "useReducer", "useCallback", "useMemo", "useRef", "useLayoutEffect", "useImperativeHandle", "Fragment", "createContext", "createRef", "forwardRef"};

        // Define operators more explicitly if needed, separate from punctuation
        // Keep simple for now
        operators = {"=", "+", "-", "*", "/", "%", "==", "===", "!=", "!==", ">", "<", ">=", "<=", "&&", "||", "!", "?", "=>", "...", "++", "--", "+=", "-=", "*=", "/=", "%=", "??", "&", "|", "^", "~", "<<", ">>"};

        colorsNeedUpdate = true;
    }

    void themeChanged() { colorsNeedUpdate = true; }

    std::vector<Token> tokenize(const std::string &code)
    {
        std::vector<Token> tokens;
        size_t pos = 0;
        size_t codeLen = code.length();
        size_t maxIterations = codeLen * 3;
        size_t iterations = 0;

        std::stack<LexerState> stateStack;
        stateStack.push(LexerState::JS);
        std::stack<int> braceDepthStack; // For {} inside JSX expressions ONLY

        TokenType lastTokenType = TokenType::Unknown;

        while (pos < codeLen && iterations < maxIterations) {
            iterations++;
            size_t start = pos;
            if (stateStack.empty()) { // Safety check
                std::cerr << "Error: Lexer state stack unexpectedly empty at pos " << pos << std::endl;
                stateStack.push(LexerState::JS); // Attempt recovery
            }
            LexerState currentState = stateStack.top();
            Token currentToken = {TokenType::Unknown, start, 0};

            // Handle Whitespace universally
            if (isWhitespace(code[pos])) {
                currentToken = {TokenType::Whitespace, start, 1};
                pos++;
            }
            // Handle Comments universally
            else if (code[pos] == '/' && pos + 1 < codeLen) {
                if (code[pos + 1] == '/') {
                    currentToken = lexSingleLineComment(code, pos);
                } else if (code[pos + 1] == '*') {
                    // Check for JSX comment {/* */} only if in appropriate state
                    if (currentState == LexerState::JSX_CHILDREN && start > 0 && code[start - 1] == '{') {
                        // This check is tricky, lexJsxChildren handles it better
                        currentToken = lexMultiLineComment(code, pos); // Or a specific JSX comment lexer?
                    } else {
                        currentToken = lexMultiLineComment(code, pos);
                    }
                }
            }

            // --- State-Specific Logic ---
            if (currentToken.type == TokenType::Unknown) {
                switch (currentState) {
                case LexerState::JS:
                    currentToken = lexJS(code, pos, stateStack, braceDepthStack, lastTokenType);
                    break;
                case LexerState::JSX_OPEN_TAG:
                    currentToken = lexJsxOpenTag(code, pos, stateStack);
                    break;
                // --- ADDED STATES ---
                case LexerState::JSX_CLOSING_TAG_NAME:
                    currentToken = lexJsxClosingTagName(code, pos, stateStack);
                    break;
                case LexerState::JSX_EXPECT_CLOSE_BRACKET:
                    currentToken = lexJsxExpectCloseBracket(code, pos, stateStack);
                    break;
                // --- END ADDED STATES ---
                case LexerState::JSX_TAG_BODY:
                    currentToken = lexJsxTagBody(code, pos, stateStack, braceDepthStack);
                    break;
                case LexerState::JSX_CHILDREN:
                    currentToken = lexJsxChildren(code, pos, stateStack, braceDepthStack);
                    break;
                case LexerState::JSX_EXPRESSION:
                    currentToken = lexJsxExpression(code, pos, stateStack, braceDepthStack, lastTokenType);
                    break;
                case LexerState::TEMPLATE_LITERAL:
                    currentToken = lexTemplateLiteral(code, pos, stateStack);
                    break;
                default:
                    std::cerr << "Error: Unknown lexer state! State: " << (int)currentState << std::endl;
                    pos++;
                    currentToken = {TokenType::Unknown, start, 1};
                }
            }

            if (currentToken.length == 0 && pos < codeLen && currentToken.type != TokenType::Unknown) {
                // Handle cases where a state transition occurred but no characters were consumed
                // This usually means the next iteration handles the character in the new state.
            } else if (currentToken.length == 0 && pos < codeLen) {
                std::cerr << "Warning: Lexer stalled at pos " << pos << " char '" << code[pos] << "' state " << (int)currentState << ". Advancing." << std::endl;
                currentToken = {TokenType::Unknown, start, 1};
                pos++;
            }

            // Store last non-whitespace token type for context checks
            if (currentToken.type != TokenType::Whitespace && currentToken.type != TokenType::Comment) {
                lastTokenType = currentToken.type;
            }
            tokens.push_back(currentToken);

            if (iterations >= maxIterations) {
                std::cerr << "Error: JSX Tokenizer exceeded maximum iterations." << std::endl;
                if (pos < codeLen) {
                    tokens.push_back({TokenType::Unknown, pos, codeLen - pos});
                }
                break; // Exit loop
            }
        }
        return tokens;
    }

    // ... (applyHighlighting remains the same conceptually) ...
    void applyHighlighting(const std::string &code, std::vector<ImVec4> &colors, int start_pos)
    {
        try {
            // Tokenize the relevant part of the code if performance is an issue,
            // but tokenizing the whole thing might be needed for accurate context.
            // For simplicity, let's tokenize from the start (adjust if too slow).
            // If start_pos > 0, context might be lost. A full re-lex might be safer.
            std::vector<Token> tokens = tokenize(code);
            colors.assign(code.length(), getColorForTokenType(TokenType::Unknown)); // Reset colors

            for (const auto &token : tokens) {
                if (token.start >= code.size()) // Safety check
                    continue;
                size_t end = std::min(token.start + token.length, code.size());

                ImVec4 color = getColorForTokenType(token.type);
                for (size_t i = token.start; i < end; ++i) {
                    // Adjust index based on start_pos if needed, but applying to whole vector is safer
                    if (i < colors.size()) {
                        colors[i] = color;
                    }
                }
            }
        } catch (const std::exception &e) {
            std::cerr << "JSX Highlight Error: " << e.what() << "\n";
            // Fallback: set remaining colors to default text
            size_t current_size = colors.size();
            colors.resize(start_pos + code.length());                                                         // Ensure vector is large enough
            std::fill(colors.begin() + start_pos, colors.end(), getColorForTokenType(TokenType::Identifier)); // Use default text color
        } catch (...) {
            std::cerr << "JSX Highlight Error: Unknown exception\n";
            size_t current_size = colors.size();
            colors.resize(start_pos + code.length());
            std::fill(colors.begin() + start_pos, colors.end(), getColorForTokenType(TokenType::Identifier));
        }
    }
    void forceColorUpdate() { colorsNeedUpdate = true; }

  private:
    // ... (Members: keywords, reactKeywords, operators, cachedColors, colorsNeedUpdate remain same) ...
    std::unordered_set<std::string> keywords;
    std::unordered_set<std::string> reactKeywords;
    std::unordered_set<std::string> operators; // Use set for faster lookup if type is always Operator
    mutable ThemeColors cachedColors;
    mutable bool colorsNeedUpdate = true;

    // ... (updateThemeColors remains the same) ...
    void updateThemeColors() const
    {
        if (!colorsNeedUpdate)
            return;

        auto &theme = gSettings.getSettings()["themes"][gSettings.getCurrentTheme()];
        auto loadColor = [&theme](const char *key, ImVec4 defaultColor = {1, 1, 1, 1}) -> ImVec4 {
            try {
                if (theme.contains(key) && theme[key].is_array() && theme[key].size() >= 4) {
                    return ImVec4(theme[key][0].get<float>(), theme[key][1].get<float>(), theme[key][2].get<float>(), theme[key][3].get<float>());
                }
            } catch (const std::exception &e) {
                std::cerr << "Error loading color '" << key << "': " << e.what() << std::endl;
            }
            // std::cerr << "Warning: Missing or invalid color key: '" << key << "' in theme '" << gSettings.getCurrentTheme() << "'. Using default." << std::endl;
            return defaultColor;
        };

        cachedColors.keyword = loadColor("keyword");
        cachedColors.string = loadColor("string");
        cachedColors.number = loadColor("number");
        cachedColors.comment = loadColor("comment");
        cachedColors.text = loadColor("text"); // Default/Identifier color
        cachedColors.function = loadColor("function");
        cachedColors.componentName = loadColor("className");                                                                                                                 // Use existing className or add a new theme key "componentName"
        cachedColors.jsxTag = loadColor("tag");                                                                                                                              // Use existing HTML tag color
        cachedColors.jsxAttribute = loadColor("attribute", cachedColors.text);                                                                                               // Add theme key "attribute" or fallback
        cachedColors.reactHook = loadColor("reactHook", cachedColors.function);                                                                                              // Add theme key "reactHook" or fallback
        cachedColors.operatorColor = loadColor("operator", ImVec4(cachedColors.text.x * 0.8f, cachedColors.text.y * 0.8f, cachedColors.text.z * 0.8f, cachedColors.text.w)); // Add theme key "operator" or fallback

        colorsNeedUpdate = false;
    }

    // ... (Character Checks: isWhitespace, isAlpha, isDigit, isAlphaNumeric remain same) ...
    bool isWhitespace(char c) const { return c == ' ' || c == '\t' || c == '\n' || c == '\r'; }
    bool isAlpha(char c) const { return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'); }
    bool isDigit(char c) const { return c >= '0' && c <= '9'; }
    bool isAlphaNumeric(char c) const { return isAlpha(c) || isDigit(c) || c == '_' || c == '$'; } // Include $ for JS vars

    // ... (Lexing Helpers: lexSingleLineComment, lexMultiLineComment, lexIdentifierOrKeyword, lexNumber, lexString, lexOperatorOrPunctuation remain same) ...
    Token lexSingleLineComment(const std::string &code, size_t &pos)
    {
        size_t start = pos;
        pos += 2; // Skip //
        while (pos < code.length() && code[pos] != '\n') {
            pos++;
        }
        return {TokenType::Comment, start, pos - start};
    }

    Token lexMultiLineComment(const std::string &code, size_t &pos)
    {
        size_t start = pos;
        pos += 2; // Skip /*
        while (pos + 1 < code.length()) {
            if (code[pos] == '*' && code[pos + 1] == '/') {
                pos += 2;
                break;
            }
            pos++;
        }
        // Handle unclosed comment? Maybe just lex till end of file.
        return {TokenType::Comment, start, pos - start};
    }

    Token lexIdentifierOrKeyword(const std::string &code, size_t &pos)
    {
        size_t start = pos;
        while (pos < code.length() && isAlphaNumeric(code[pos])) {
            pos++;
        }
        std::string word = code.substr(start, pos - start);

        if (keywords.count(word)) {
            return {TokenType::Keyword, start, pos - start};
        }
        if (reactKeywords.count(word)) {
            // Could potentially check context (e.g., if it's being called)
            return {TokenType::ReactHook, start, pos - start};
        }
        // Basic check for function call/definition (look for '(') - can be improved
        size_t nextCharPos = pos;
        while (nextCharPos < code.length() && isWhitespace(code[nextCharPos])) {
            nextCharPos++;
        }
        // Check if it's immediately followed by '(', indicating function call/def
        bool followedByParen = (nextCharPos < code.length() && code[nextCharPos] == '(');
        // Check if it *looks* like a Component based on capitalization (less reliable alone)
        bool isCapitalized = (word.length() > 0 && word[0] >= 'A' && word[0] <= 'Z');

        // Prioritize Function if followed by (
        if (followedByParen) {
            // If also capitalized, it might be a Class constructor call like new MyClass()
            // or a Function Component call like MyComponent(). We'll treat both as Function for now.
            return {TokenType::Function, start, pos - start};
        }

        // If capitalized but NOT followed by (, treat as potential Component/Class name reference
        if (isCapitalized) {
            // Check context? If previous token was 'import' or 'class', it's more likely.
            // For simplicity, tag capitalized identifiers as JSXComponentName for now.
            // This might miscolor regular capitalized variables, but is common for JSX.
            return {TokenType::JSXComponentName, start, pos - start};
        }

        return {TokenType::Identifier, start, pos - start};
    }

    Token lexNumber(const std::string &code, size_t &pos)
    {
        size_t start = pos;
        // Handle hex (0x), binary (0b), octal (0o) prefixes
        if (pos + 1 < code.length() && code[pos] == '0') {
            char next = code[pos + 1];
            if (next == 'x' || next == 'X' || next == 'b' || next == 'B' || next == 'o' || next == 'O') {
                pos += 2;
            }
        }
        while (pos < code.length() && (isAlphaNumeric(code[pos]) || code[pos] == '.')) {
            // Allow alphanumeric for hex digits (a-f)
            // Allow '.' for decimals
            // Stop if it's not a valid number char in the current context (e.g., second '.')
            if (code[pos] == '.' && code.substr(start, pos - start).find('.') != std::string::npos)
                break; // Only one dot
            if (!isDigit(code[pos]) && code[pos] != '.' && !((code[pos] >= 'a' && code[pos] <= 'f') || (code[pos] >= 'A' && code[pos] <= 'F') || code[pos] == 'x' || code[pos] == 'X' || code[pos] == 'b' || code[pos] == 'B' || code[pos] == 'o' || code[pos] == 'O'))
                break; // Allow hex/bin/oct digits/prefixes

            pos++;
        }
        // Add exponent handling ('e' or 'E')
        if (pos < code.length() && (code[pos] == 'e' || code[pos] == 'E')) {
            size_t expStart = pos;
            pos++;
            if (pos < code.length() && (code[pos] == '+' || code[pos] == '-')) {
                pos++;
            }
            size_t digitStart = pos;
            while (pos < code.length() && isDigit(code[pos])) {
                pos++;
            }
            if (pos == digitStart) { // No digits after E[+/-]? Revert exponent part.
                pos = expStart;
            }
        }

        return {TokenType::Number, start, pos - start};
    }

    Token lexString(const std::string &code, size_t &pos)
    {
        size_t start = pos;
        char quote = code[pos];
        pos++; // Skip opening quote
        while (pos < code.length()) {
            if (code[pos] == '\\' && pos + 1 < code.length()) {
                pos += 2; // Skip escape sequence
            } else if (code[pos] == quote) {
                pos++; // Skip closing quote
                break;
            } else {
                pos++;
            }
        }
        return {TokenType::String, start, pos - start};
    }

    Token lexOperatorOrPunctuation(const std::string &code, size_t &pos)
    {
        size_t start = pos;
        size_t maxLength = 3; // Max operator length (e.g., ===, !==, ??=)
        std::string currentOp;
        std::string longestMatch;

        // Find longest matching operator
        for (size_t len = 1; len <= maxLength && start + len <= code.length(); ++len) {
            std::string sub = code.substr(start, len);
            if (operators.count(sub)) {
                longestMatch = sub;
            }
        }

        if (!longestMatch.empty()) {
            pos += longestMatch.length();
            TokenType type = TokenType::Operator;
            if (longestMatch == "=>")
                type = TokenType::ArrowFunction;
            // Add other specific operator types if needed
            return {type, start, longestMatch.length()};
        }

        // If no multi-char operator matched, check single known punctuation chars
        char c = code[pos];
        pos++; // Consume the char
        if (c == '(' || c == ')')
            return {TokenType::Parenthesis, start, 1};
        if (c == '[' || c == ']')
            return {TokenType::Bracket, start, 1};
        if (c == '{')
            return {TokenType::Brace, start, 1}; // Caller might adjust based on context
        if (c == '}')
            return {TokenType::Brace, start, 1}; // Caller might adjust based on context
        if (c == ':')
            return {TokenType::Colon, start, 1};
        if (c == ',')
            return {TokenType::Comma, start, 1};
        if (c == '.')
            return {TokenType::Dot, start, 1};

        // Check if the single char itself is an operator (e.g., +, -, !)
        std::string singleCharStr(1, c);
        if (operators.count(singleCharStr)) {
            return {TokenType::Operator, start, 1};
        }

        // Otherwise unknown
        pos = start + 1; // Ensure position advanced if we fell through
        return {TokenType::Unknown, start, 1};
    }

    // --- State-Specific Lexing Functions ---

    Token lexJS(const std::string &code, size_t &pos, std::stack<LexerState> &stateStack, std::stack<int> &braceDepthStack, TokenType lastTokenType)
    {
        size_t start = pos;
        char c = code[pos];

        // Potential JSX Start? Check <
        if (c == '<') {
            // Stronger Heuristic: Check if it's immediately followed by '/' or an alpha char.
            // And consider context (e.g., after return, =, (, :, ?, => etc.)
            bool likelyTag = false;
            if (pos + 1 < code.length() && (isAlpha(code[pos + 1]) || code[pos + 1] == '/')) {
                // Could be a tag. Check context.
                if (lastTokenType == TokenType::Operator || lastTokenType == TokenType::Parenthesis || // After ( e.g. `render(<App />)`
                    lastTokenType == TokenType::Bracket ||                                             // After [ e.g. `[<div />]`
                    lastTokenType == TokenType::Brace ||                                               // After { e.g. `const x = { node: <div/> }` or start of block
                    lastTokenType == TokenType::Comma ||                                               // After , e.g. `render(a, <App />)`
                    lastTokenType == TokenType::Colon ||                                               // After : e.g. `cond ? <A/> : <B/>` or `{ label: <A/> }`
                    lastTokenType == TokenType::Keyword ||                                             // After 'return'
                    lastTokenType == TokenType::ArrowFunction ||                                       // After => e.g. `() => <div />`
                    lastTokenType == TokenType::JSXExprEnd ||                                          // After } in JSX e.g. `{x} <div/>`
                    lastTokenType == TokenType::JSXTagEnd ||                                           // After > or /> e.g. `<div> <span /> <b /> </div>`
                    lastTokenType == TokenType::Unknown)                                               // Start of file/block likely
                {
                    likelyTag = true;
                }
            }

            if (likelyTag) {
                if (code[pos + 1] == '/') {
                    pos += 2;                                          // Consume </
                    stateStack.push(LexerState::JSX_CLOSING_TAG_NAME); // <<< Use new state
                    return {TokenType::JSXTagStart, start, 2};
                } else {
                    pos++; // Consume <
                    stateStack.push(LexerState::JSX_OPEN_TAG);
                    return {TokenType::JSXTagStart, start, 1};
                }
            } else {
                // Treat as operator (less than)
                return lexOperatorOrPunctuation(code, pos);
            }
        }
        // Template Literal Start
        else if (c == '`') {
            pos++;
            stateStack.push(LexerState::TEMPLATE_LITERAL);
            // Return the opening backtick itself as a string-like token?
            // Or just transition state and let next iteration handle content?
            // Let's return a token for the backtick itself.
            return {TokenType::TemplateString, start, 1}; // Token for '`'
        }
        // Identifier or Keyword
        else if (isAlpha(c) || c == '_' || c == '$') {
            return lexIdentifierOrKeyword(code, pos);
        }
        // Number
        else if (isDigit(c)) {
            return lexNumber(code, pos);
        }
        // String
        else if (c == '"' || c == '\'') {
            return lexString(code, pos);
        }
        // Braces
        else if (c == '{') {
            pos++;
            // Could be object literal, block start, or destructuring.
            // No special state change needed here at JS level for lexing.
            return {TokenType::Brace, start, 1};
        } else if (c == '}') {
            pos++;
            // *** FIX for Template Literals ***
            if (stateStack.size() > 1) {                      // Check if there's a state below JS
                LexerState currentJSState = stateStack.top(); // Should be JS
                stateStack.pop();                             // Pop JS temporarily
                if (stateStack.top() == LexerState::TEMPLATE_LITERAL) {
                    // It was closing an expression within Template Literal!
                    // State is now TEMPLATE_LITERAL. Correct.
                    return {TokenType::TemplateExprEnd, start, 1}; // Return specific token
                } else {
                    // It was a regular JS brace. Put JS state back.
                    stateStack.push(currentJSState);
                }
            }
            // Return standard brace if not closing template expr
            return {TokenType::Brace, start, 1};
        }
        // Other operators or punctuation
        else {
            return lexOperatorOrPunctuation(code, pos);
        }
    }

    // Lex the tag name after '<' (opening tag)
    Token lexJsxOpenTag(const std::string &code, size_t &pos, std::stack<LexerState> &stateStack)
    {
        size_t start = pos;
        // Skip leading whitespace (shouldn't be any?)
        while (pos < code.length() && isWhitespace(code[pos])) {
            pos++;
            start++; // Adjust start if we skip whitespace
        }

        // Lex tag name (identifier, component name, or member expression like obj.Component)
        bool isMemberExpr = false;
        while (pos < code.length()) {
            if (isAlphaNumeric(code[pos]) || code[pos] == '-') { // Allow alphanum and hyphen
                pos++;
            } else if (code[pos] == '.' && !isMemberExpr) { // Allow first dot for member expr
                isMemberExpr = true;
                pos++;
            } else if (code[pos] == '.' && isMemberExpr) { // Allow subsequent dots
                pos++;
            } else {
                break; // Stop on other chars (whitespace, >, /)
            }
        }

        if (pos > start) {
            std::string name = code.substr(start, pos - start);
            stateStack.pop();                          // Pop JSX_OPEN_TAG
            stateStack.push(LexerState::JSX_TAG_BODY); // Move to parsing attributes/end
            // Decide if ComponentName or TagName
            // Simple: Check first char capitalization if not member expression
            if (!isMemberExpr && name.length() > 0 && name[0] >= 'A' && name[0] <= 'Z') {
                return {TokenType::JSXComponentName, start, pos - start};
            } else {
                // Could be member expression (Comp.Sub) or standard tag (div)
                // Treat member expressions as tags for now, maybe refine later
                return {TokenType::JSXTagName, start, pos - start};
            }
        } else {
            // Error: Expected tag name after <
            std::cerr << "Error: Expected tag name at pos " << start << std::endl;
            stateStack.pop(); // Pop JSX_OPEN_TAG
            // Don't push JS, let next iteration handle char in whatever state was previous
            // Return Unknown token for the character that caused the error
            return {TokenType::Unknown, start, 1}; // Consume the char
        }
    }

    // *** ADDED: Lex the tag name after '</' ***
    Token lexJsxClosingTagName(const std::string &code, size_t &pos, std::stack<LexerState> &stateStack)
    {
        size_t start = pos;
        // Skip leading whitespace (shouldn't be any?)
        while (pos < code.length() && isWhitespace(code[pos])) {
            pos++;
            start++; // Adjust start if we skip whitespace
        }

        // Lex tag name (same logic as open tag, maybe refactor)
        bool isMemberExpr = false;
        while (pos < code.length()) {
            if (isAlphaNumeric(code[pos]) || code[pos] == '-') {
                pos++;
            } else if (code[pos] == '.' && !isMemberExpr) {
                isMemberExpr = true;
                pos++;
            } else if (code[pos] == '.' && isMemberExpr) {
                pos++;
            } else {
                break;
            }
        }

        if (pos > start) {
            std::string name = code.substr(start, pos - start);
            stateStack.pop();                                      // Pop JSX_CLOSING_TAG_NAME
            stateStack.push(LexerState::JSX_EXPECT_CLOSE_BRACKET); // Expect > next
            if (!isMemberExpr && name.length() > 0 && name[0] >= 'A' && name[0] <= 'Z') {
                return {TokenType::JSXComponentName, start, pos - start};
            } else {
                return {TokenType::JSXTagName, start, pos - start};
            }
        } else {
            // Error: Expected tag name after </
            std::cerr << "Error: Expected closing tag name at pos " << start << std::endl;
            stateStack.pop();                      // Pop JSX_CLOSING_TAG_NAME
            return {TokenType::Unknown, start, 1}; // Consume the char
        }
    }

    // *** ADDED: Expect '>' after a closing tag name ***
    Token lexJsxExpectCloseBracket(const std::string &code, size_t &pos, std::stack<LexerState> &stateStack)
    {
        size_t start = pos;
        // Skip leading whitespace (shouldn't be any?)
        while (pos < code.length() && isWhitespace(code[pos])) {
            pos++;
            start++; // Adjust start if we skip whitespace
        }

        if (pos < code.length() && code[pos] == '>') {
            pos++;
            stateStack.pop(); // Pop JSX_EXPECT_CLOSE_BRACKET

            // Now, also pop the JSX_CHILDREN state that this tag was closing
            if (!stateStack.empty() && stateStack.top() == LexerState::JSX_CHILDREN) {
                stateStack.pop(); // Pop JSX_CHILDREN
            } else {
                // Should normally be JSX_CHILDREN, handle error/unexpected state?
                std::cerr << "Warning: Expected JSX_CHILDREN state below JSX_EXPECT_CLOSE_BRACKET at pos " << start << std::endl;
                // Attempt recovery: If stack not empty, just continue in whatever state is now top.
                // If stack IS empty, push JS?
                if (stateStack.empty())
                    stateStack.push(LexerState::JS);
            }
            return {TokenType::JSXTagEnd, start, 1};
        } else {
            // Error: Expected > after closing tag name
            std::cerr << "Error: Expected '>' after closing tag name at pos " << start << std::endl;
            stateStack.pop(); // Pop JSX_EXPECT_CLOSE_BRACKET
            // Attempt recovery like above
            if (!stateStack.empty() && stateStack.top() == LexerState::JSX_CHILDREN) {
                stateStack.pop();
            }
            if (stateStack.empty())
                stateStack.push(LexerState::JS);
            // Consume the unexpected character?
            if (pos < code.length()) {
                return {TokenType::Unknown, start, 1}; // Consume the char
            } else {
                return {TokenType::Unknown, start, 0}; // Reached end of file unexpectedly
            }
        }
    }

    // Lex attributes or the end of a tag ( > or /> )
    Token lexJsxTagBody(const std::string &code, size_t &pos, std::stack<LexerState> &stateStack, std::stack<int> &braceDepthStack)
    {
        size_t start = pos;
        // Skip whitespace first
        if (isWhitespace(code[pos])) {
            pos++;
            return {TokenType::Whitespace, start, 1};
        }

        char c = code[pos];

        // End of Tag?
        if (c == '>') {
            pos++;
            stateStack.pop();                          // Pop JSX_TAG_BODY
            stateStack.push(LexerState::JSX_CHILDREN); // Enter children mode
            return {TokenType::JSXTagEnd, start, 1};
        }
        // Self-closing Tag?
        else if (c == '/' && pos + 1 < code.length() && code[pos + 1] == '>') {
            pos += 2;
            stateStack.pop(); // Pop JSX_TAG_BODY. Parent state (JS or JSX_CHILDREN) is now current. <<< FIX APPLIED
            return {TokenType::JSXTagEnd, start, 2};
        }
        // JSX Expression Attribute Start? {
        else if (c == '{') {
            pos++;
            stateStack.push(LexerState::JSX_EXPRESSION); // Enter expression mode
            braceDepthStack.push(1);                     // Start tracking braces for this expression
            return {TokenType::JSXExprStart, start, 1};
        }
        // Attribute Name?
        else if (isAlpha(c) || c == '_') { // Attributes start with alpha/_ (or data-, aria-)
            // Lex attribute name (can contain hyphens)
            while (pos < code.length() && (isAlphaNumeric(code[pos]) || code[pos] == '-')) {
                pos++;
            }
            return {TokenType::JSXAttributeName, start, pos - start};
        }
        // Equals sign for attribute value?
        else if (c == '=') {
            pos++;
            return {TokenType::JSXAttributeEquals, start, 1};
        }
        // String attribute value?
        else if (c == '"' || c == '\'') {
            // Lex the string, but return specific JSXAttributeValue type?
            Token stringToken = lexString(code, pos);
            // stringToken.type = TokenType::JSXAttributeValue; // Option to override type
            return stringToken; // Return as String for now, colorer can use context
        }
        // Unknown char inside tag body
        else {
            std::cerr << "Warning: Unexpected char '" << c << "' in JSX tag body at pos " << start << std::endl;
            pos++;
            return {TokenType::Unknown, start, 1};
        }
    }

    // Lex content between JSX tags (text, nested tags, expressions)
    Token lexJsxChildren(const std::string &code, size_t &pos, std::stack<LexerState> &stateStack, std::stack<int> &braceDepthStack)
    {
        size_t start = pos;
        // Look for first significant character: <, {, or other (text)
        while (pos < code.length()) {
            char c = code[pos];
            if (c == '<') { // Start of nested tag or closing tag?
                if (pos > start)
                    break; // Found text before the '<'
                // Let JS state handle the '<' detection
                stateStack.pop(); // Exit JSX_CHILDREN temporarily
                // No need to push JS, the main loop will call lexJS next
                return {TokenType::Unknown, start, 0}; // Indicate state change, no token consumed
            } else if (c == '{') {                     // Start of expression or JSX comment?
                if (pos > start)
                    break; // Found text before the '{'
                // Check for JSX comment {/* ... */}
                if (pos + 1 < code.length() && code[pos + 1] == '*') {
                    size_t commentStart = pos;
                    pos += 2; // Skip {/*
                    bool closed = false;
                    while (pos + 1 < code.length()) {
                        if (code[pos] == '*' && code[pos + 1] == '}') {
                            pos += 2;
                            closed = true;
                            break;
                        }
                        pos++;
                    }
                    return {TokenType::Comment, commentStart, pos - commentStart};
                } else {
                    // Regular JSX expression start
                    pos++;
                    stateStack.push(LexerState::JSX_EXPRESSION);
                    braceDepthStack.push(1);
                    return {TokenType::JSXExprStart, start, 1};
                }
            }
            // Handle XML entities like   ? Maybe later.
            // Ignore other characters for now, they are part of text
            pos++;
        }

        // If we broke loop or reached end, return the text content
        if (pos > start) {
            // Trim trailing whitespace from JSXText? Maybe not.
            return {TokenType::JSXText, start, pos - start};
        } else {
            // Only happens if start == code.length() or stuck at '<'/'{'?
            return {TokenType::Unknown, start, 0}; // Let outer loop handle stall if needed
        }
    }

    // Lex JS code inside a { ... } expression within JSX
    Token lexJsxExpression(const std::string &code, size_t &pos, std::stack<LexerState> &stateStack, std::stack<int> &braceDepthStack, TokenType lastTokenType)
    {
        size_t start = pos;
        char c = code[pos];

        // Skip whitespace within expression? Let lexJS handle it.

        // Check for closing brace at the correct depth for THIS expression
        if (c == '}') {
            if (!braceDepthStack.empty()) {
                braceDepthStack.top()--;
                if (braceDepthStack.top() == 0) {
                    // Matched the opening brace of THIS JSX expression
                    braceDepthStack.pop();
                    stateStack.pop(); // Exit JSX_EXPRESSION mode
                    pos++;
                    return {TokenType::JSXExprEnd, start, 1};
                }
                // Else, it's a closing brace *within* the JS expression, handled by lexJS call below
            } else {
                // Error: Unmatched '}' or brace stack empty?
                std::cerr << "Error: Brace depth stack empty on '}' in JSX expression context at pos " << start << std::endl;
                stateStack.pop(); // Try to recover by exiting expr mode
                pos++;
                return {TokenType::JSXExprEnd, start, 1}; // Or Unknown?
            }
        }
        // Check for nested opening brace within the expression
        if (c == '{') {
            if (!braceDepthStack.empty()) {
                braceDepthStack.top()++;
            } else {
                // Error: Saw '{' but brace stack was empty? Should not happen.
                std::cerr << "Error: Unexpected '{' led to empty brace stack in JSX expression context at pos " << start << std::endl;
                // Recover by pushing a depth?
                braceDepthStack.push(1); // Assume starting depth of 1 now? Risky.
            }
            // Fall through to lex it as a regular JS brace using lexJS
        }

        // Otherwise, lex using the standard JS lexer logic.
        // Temporarily switch state *just* for the call to lexJS, doesn't affect loop's perception
        stateStack.pop();                                                             // Temporarily remove JSX_EXPRESSION
        stateStack.push(LexerState::JS);                                              // Pretend we are in JS
        Token jsToken = lexJS(code, pos, stateStack, braceDepthStack, lastTokenType); // Lex one JS token
        stateStack.pop();                                                             // Remove the temporary JS state
        stateStack.push(LexerState::JSX_EXPRESSION);                                  // Restore JSX_EXPRESSION state for next loop iteration

        // Need to handle if lexJS itself changed the state (e.g., entered template literal)
        // If the top state *after* the call isn't JSX_EXPRESSION, lexJS pushed something.
        // We need to put JSX_EXPRESSION back *under* the state lexJS pushed.
        if (stateStack.top() != LexerState::JSX_EXPRESSION) {
            LexerState statePushedByJs = stateStack.top();
            stateStack.pop(); // Pop the state pushed by JS (e.g., TEMPLATE_LITERAL)
            // Ensure JSX_EXPRESSION is still there (it should be unless stack got corrupted)
            if (!stateStack.empty() && stateStack.top() == LexerState::JSX_EXPRESSION) {
                // Correct: Stack is now ... -> JSX_EXPRESSION
                stateStack.push(statePushedByJs); // Push back the state from JS on top
                // Stack is now ... -> JSX_EXPRESSION -> TEMPLATE_LITERAL (correct order)
            } else {
                std::cerr << "Error: State stack corrupted after JS call within JSX expression." << std::endl;
                // Attempt recovery: Clear stack and push JSX_EXPRESSION? Or JS?
                while (!stateStack.empty())
                    stateStack.pop();
                stateStack.push(LexerState::JS); // Fall back to JS?
            }
        }

        return jsToken;
    }

    // Lex inside a template literal `...`
    Token lexTemplateLiteral(const std::string &code, size_t &pos, std::stack<LexerState> &stateStack)
    {
        size_t start = pos;

        while (pos < code.length()) {
            if (code[pos] == '\\' && pos + 1 < code.length()) {
                // Emit text before escape? No, consume escape as part of literal.
                 // Need to handle specific escapes? \`, \${, \\
                 pos += 2;
            } else if (code[pos] == '$' && pos + 1 < code.length() && code[pos + 1] == '{') {
                // Found start of expression ${
                if (pos > start) {
                    // Emit the text part before the ${ first
                    return {TokenType::TemplateString, start, pos - start};
                } else {
                    // Start of expression right away
                    pos += 2;
                    stateStack.push(LexerState::JS); // Enter JS mode for the expression
                    // The handling of '}' in lexJS will pop this JS state later.
                    return {TokenType::TemplateExprStart, start, 2};
                }
            } else if (code[pos] == '`') {
                // Found end of template literal
                if (pos > start) {
                    // Emit the last text part before the `
                    return {TokenType::TemplateString, start, pos - start};
                } else {
                    // End of literal right away (empty literal ``)
                    pos++;
                    stateStack.pop();                             // Exit TEMPLATE_LITERAL state
                    return {TokenType::TemplateString, start, 1}; // Token for the closing '`'
                }
            } else {
                pos++; // Consume literal character
            }
        }

        // Reached end of code within template literal (unclosed)
        if (pos > start) {
            return {TokenType::TemplateString, start, pos - start};
        } else {
            // Error: unclosed template literal at end of file
            stateStack.pop();                      // Pop the TEMPLATE_LITERAL state anyway?
            return {TokenType::Unknown, start, 0}; // Let outer loop handle stall
        }
    }

    // --- Coloring ---

    ImVec4 getColorForTokenType(TokenType type) const
    {
        updateThemeColors(); // Ensure colors are loaded

        switch (type) {
        case TokenType::Keyword:
            return cachedColors.keyword;
        case TokenType::ReactHook:
            return cachedColors.reactHook;
        case TokenType::String:
        case TokenType::JSXAttributeValue: // Color attribute strings like regular strings
            return cachedColors.string;
        case TokenType::TemplateString:
            return cachedColors.string; // Color literal parts like strings
        case TokenType::Number:
            return cachedColors.number;
        case TokenType::Comment:
            return cachedColors.comment;
        case TokenType::Function:
            return cachedColors.function;
        case TokenType::JSXComponentName:
            return cachedColors.componentName;
        case TokenType::Operator:
        case TokenType::ArrowFunction:
        case TokenType::Destructuring:
        case TokenType::ScopeOperator:
        case TokenType::Parenthesis:
        case TokenType::Bracket:
        case TokenType::Colon:
        case TokenType::Comma:
        case TokenType::Dot:
        case TokenType::JSXAttributeEquals:
            return cachedColors.operatorColor;

        case TokenType::Brace:                 // JS { }
        case TokenType::TemplateExprStart:     // ${
        case TokenType::TemplateExprEnd:       // } closing template expr
        case TokenType::JSXExprStart:          // JSX {
        case TokenType::JSXExprEnd:            // JSX }
            return cachedColors.operatorColor; // Style all braces/expr delimiters same?

        case TokenType::JSXTagStart:    // < or </
        case TokenType::JSXTagEnd:      // > or />
            return cachedColors.jsxTag; // Color < > like tags
        case TokenType::JSXTagName:
            return cachedColors.jsxTag;
        case TokenType::JSXAttributeName:
            return cachedColors.jsxAttribute;
        case TokenType::JSXText:
            return cachedColors.text; // Default text color for content

        case TokenType::Identifier:
        case TokenType::Whitespace:
        case TokenType::Unknown:
        default:
            return cachedColors.text;
        }
    }
};
} // namespace JsxLexer