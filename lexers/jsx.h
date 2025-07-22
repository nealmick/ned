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
	ComponentName, // Capitalized identifiers
	Keyword,
	String,
	TemplateString,
	TemplateExprStart, // ${
	TemplateExprEnd,   // } closing an expression in a template string
	Number,
	Comment,
	Operator,
	Parenthesis, // ( )
	Bracket,	 // [ ]
	Brace,		 // { } JS Block/Object
	Colon,
	Comma,
	Dot,
	ArrowFunction, // =>
	Unknown,
	// Special Types
	ReactHook,			 // JS identifiers like useState
	RETURN_BLOCK_CONTENT // <<< Single type for the pink block
};

struct Token
{
	TokenType type;
	size_t start;
	size_t length;
};

// Represents the current parsing context
enum class LexerState {
	JS,				 // Default JavaScript mode
	IN_RETURN_BLOCK, // <<< Inside the potential JSX block of a return statement
	TEMPLATE_LITERAL // Inside `...` template literal
};

class Lexer
{
  public:
	// --- ThemeColors struct remains the same ---
	struct ThemeColors
	{
		ImVec4 keyword, string, number, comment, text, function, componentName, reactHook,
			operatorColor, returnBlock;
	};

	Lexer()
	{
		// --- Keyword, ReactKeyword, Operator sets remain the same ---
		keywords = {"function", "const",   "let",	 "var",		  "if",
					"else",		"return",  "import", "export",	  "default",
					"class",	"extends", "super",	 "this",	  "new",
					"try",		"catch",   "throw",	 "typeof",	  "instanceof",
					"async",	"await",   "for",	 "of",		  "while",
					"do",		"switch",  "case",	 "break",	  "continue",
					"static",	"true",	   "false",	 "null",	  "undefined",
					"void",		"delete",  "yield",	 "interface", "type",
					"as",		"from",	   "in",	 "is"};
		reactKeywords = {"useState",
						 "useEffect",
						 "useContext",
						 "useReducer",
						 "useCallback",
						 "useMemo",
						 "useRef",
						 "useLayoutEffect",
						 "useImperativeHandle",
						 "Fragment",
						 "createContext",
						 "createRef",
						 "forwardRef"};
		operators = {"=",	"+",   "-",	 "*",  "/",	 "%",  "==", "===", "!=",
					 "!==", ">",   "<",	 ">=", "<=", "&&", "||", "!",	"?",
					 "=>",	"...", "++", "--", "+=", "-=", "*=", "/=",	"%=",
					 "??",	"&",   "|",	 "^",  "~",	 "<<", ">>", ";"};
		colorsNeedUpdate = true;
	}

	void themeChanged() { colorsNeedUpdate = true; }

	// ========================================================================
	// TOKENIZE - Rewritten Logic
	// ========================================================================
	std::vector<Token> tokenize(const std::string &code)
	{
		std::vector<Token> tokens;
		size_t pos = 0;
		size_t codeLen = code.length();
		size_t maxIterations = codeLen * 5; // Increased safety margin
		size_t iterations = 0;

		std::stack<LexerState> stateStack;
		stateStack.push(LexerState::JS);

		TokenType lastNonWsToken = TokenType::Unknown;

		while (pos < codeLen && iterations < maxIterations)
		{
			iterations++;
			size_t start = pos;
			LexerState currentState = stateStack.top();
			Token currentToken = {TokenType::Unknown, start, 0};

			// 1. Handle Whitespace & Comments (Universally)
			if (isWhitespace(code[pos]))
			{
				currentToken = {TokenType::Whitespace, start, 1};
				pos++;
				tokens.push_back(currentToken);
				continue; // Process next character
			}
			if (pos + 1 < codeLen && code[pos] == '/')
			{
				if (code[pos + 1] == '/')
				{
					currentToken = lexSingleLineComment(code, pos);
					tokens.push_back(currentToken);
					lastNonWsToken = TokenType::Comment; // Track last token type
					continue;
				} else if (code[pos + 1] == '*')
				{
					currentToken = lexMultiLineComment(code, pos);
					tokens.push_back(currentToken);
					lastNonWsToken = TokenType::Comment; // Track last token type
					continue;
				}
			}

			// 2. State-Specific Logic
			switch (currentState)
			{
			case LexerState::JS: {
				currentToken = lexJS(code, pos, stateStack, lastNonWsToken);
				if (currentToken.length > 0)
				{
					tokens.push_back(currentToken);
					lastNonWsToken =
						currentToken.type; // Update last non-whitespace token

					// Check for 'return' followed by potential JSX starter
					if (currentToken.type == TokenType::Keyword &&
						code.substr(currentToken.start, currentToken.length) == "return")
					{
						size_t nextCharPos = pos; // Position after the 'return' token
						// Skip whitespace
						while (nextCharPos < codeLen && isWhitespace(code[nextCharPos]))
						{
							nextCharPos++;
						}
						// Check if the next non-whitespace char starts a block
						if (nextCharPos < codeLen &&
							(code[nextCharPos] == '(' || code[nextCharPos] == '{' ||
							 code[nextCharPos] == '<'))
						{
							// Consume the whitespace between 'return' and
							// the starter
							if (nextCharPos > pos)
							{
								tokens.push_back(
									{TokenType::Whitespace, pos, nextCharPos - pos});
							}
							// Update position and switch state
							pos = nextCharPos;
							stateStack.push(LexerState::IN_RETURN_BLOCK);
							// 'continue' to let the next iteration handle
							// the
							// '(', '{', or '<' in the new state
							continue;
						}
						// If not followed by starter, remain in JS state.
						// 'pos' is already correct.
					}
				} else if (currentToken.type != TokenType::Unknown)
				{
					// lexJS might have just changed state (e.g., to
					// TEMPLATE_LITERAL) without consuming chars. Let the next
					// iteration handle it.
				} else
				{
					// Lexer stall in JS state
					if (pos < codeLen)
					{
						// std::cerr << "Warning: Lexer stall in JS state at pos
						// " << pos << ". Consuming char '" << code[pos] << "'"
						// << std::endl;
						tokens.push_back({TokenType::Unknown, pos, 1});
						pos++;
					}
				}
				break; // End of LexerState::JS case
			}

			case LexerState::IN_RETURN_BLOCK: {
				// lexReturnBlock consumes the entire block in one go
				currentToken = lexReturnBlock(
					code,
					pos,
					stateStack); // stateStack is popped inside lexReturnBlock
				if (currentToken.length > 0)
				{
					tokens.push_back(currentToken);
					lastNonWsToken = currentToken.type;
				} else
				{
					// Error or EOF within block, state already popped. Let JS
					// handle next.
					if (pos < codeLen)
					{
						// std::cerr << "Warning: lexReturnBlock returned 0
						// length. Pos: " << pos << std::endl; Let outer loop
						// handle potential stall if pos hasn't advanced
					}
				}
				// Let the next iteration run in the new state (should be JS)
				break; // End of LexerState::IN_RETURN_BLOCK case
			}

			case LexerState::TEMPLATE_LITERAL: {
				currentToken =
					lexTemplateLiteral(code,
									   pos,
									   stateStack); // Handles state changes internally
				if (currentToken.length > 0)
				{
					tokens.push_back(currentToken);
					lastNonWsToken = currentToken.type;
				} else if (currentToken.type != TokenType::Unknown)
				{
					// State change occurred (e.g. entered JS expression ${})
				} else
				{
					// Lexer stall in template literal
					if (pos < codeLen)
					{
						// std::cerr << "Warning: Lexer stall in Template
						// Literal state at pos " << pos << ". Consuming char '"
						// << code[pos] << "'" << std::endl;
						tokens.push_back({TokenType::Unknown, pos, 1});
						pos++;
					}
					// Pop state if stalled at end? lexTemplateLiteral should
					// handle this.
				}
				break; // End of LexerState::TEMPLATE_LITERAL case
			}

			default: // Should not happen
				std::cerr << "Error: Unknown lexer state! State: " << (int)currentState
						  << std::endl;
				if (pos < codeLen)
				{
					tokens.push_back({TokenType::Unknown, pos, 1});
					pos++;
				}
				break;
			} // End Switch

			// Max iteration check
			if (iterations >= maxIterations)
			{
				std::cerr << "Error: JSX Tokenizer exceeded maximum iterations."
						  << std::endl;
				if (pos < codeLen)
				{
					tokens.push_back({TokenType::Unknown, pos, codeLen - pos});
				}
				break;
			}
		} // End While Loop

		return tokens;
	}

	// --- applyHighlighting (unchanged, uses tokenize) ---
	void
	applyHighlighting(const std::string &code, std::vector<ImVec4> &colors, int start_pos)
	{ /* ... unchanged ... */
		if (colors.size() < code.length())
		{
			try
			{
				colors.resize(code.length(), getColorForTokenType(TokenType::Unknown));
			} catch (...)
			{ /* error handling */
				return;
			}
		} else
		{
			std::fill(colors.begin(),
					  colors.begin() + code.length(),
					  getColorForTokenType(TokenType::Unknown));
		}
		try
		{
			std::vector<Token> tokens = tokenize(code);
			for (const auto &token : tokens)
			{
				if (token.start >= code.size() || token.start + token.length > code.size())
					continue;
				ImVec4 color = getColorForTokenType(token.type);
				for (size_t i = token.start; i < token.start + token.length; ++i)
				{
					if (i < colors.size())
					{
						colors[i] = color;
					}
				}
			}
		} catch (...)
		{ /* error handling */
		}
	}
	// --- forceColorUpdate (unchanged) ---
	void forceColorUpdate() { colorsNeedUpdate = true; }

  private:
	std::unordered_set<std::string> keywords;
	std::unordered_set<std::string> reactKeywords;
	std::unordered_set<std::string> operators;
	mutable ThemeColors cachedColors;
	mutable bool colorsNeedUpdate = true;

	// --- updateThemeColors (unchanged) ---
	void updateThemeColors() const
	{ /* ... unchanged ... */
		if (!colorsNeedUpdate)
			return;
		auto &theme = gSettings.getSettings()["themes"][gSettings.getCurrentTheme()];
		auto loadColor = [&theme](const char *key,
								  ImVec4 defaultColor = {1, 1, 1, 1}) -> ImVec4 {
			try
			{
				if (theme.contains(key) && theme[key].is_array() && theme[key].size() >= 4)
				{
					return ImVec4(theme[key][0].get<float>(),
								  theme[key][1].get<float>(),
								  theme[key][2].get<float>(),
								  theme[key][3].get<float>());
				}
			} catch (...)
			{
			}
			return defaultColor;
		};
		cachedColors.keyword = loadColor("keyword");
		cachedColors.string = loadColor("string");
		cachedColors.number = loadColor("number");
		cachedColors.comment = loadColor("comment");
		cachedColors.text = loadColor("text");
		cachedColors.function = loadColor("function");
		cachedColors.componentName = loadColor("className", cachedColors.text);
		cachedColors.reactHook = loadColor("reactHook", cachedColors.function);
		cachedColors.operatorColor = loadColor("operator",
											   ImVec4(cachedColors.text.x * 0.8f,
													  cachedColors.text.y * 0.8f,
													  cachedColors.text.z * 0.8f,
													  cachedColors.text.w));
		cachedColors.returnBlock = ImVec4(1.0f, 0.7f, 0.75f, 1.0f);
		colorsNeedUpdate = false;
	}
	// --- Character Checks (unchanged) ---
	bool isWhitespace(char c) const
	{
		return c == ' ' || c == '\t' || c == '\n' || c == '\r';
	}
	bool isAlpha(char c) const
	{
		return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
	}
	bool isDigit(char c) const { return c >= '0' && c <= '9'; }
	bool isAlphaNumeric(char c) const
	{
		return isAlpha(c) || isDigit(c) || c == '_' || c == '$';
	}

	// --- Basic Lexing Helpers (unchanged) ---
	Token lexSingleLineComment(const std::string &code, size_t &pos)
	{ /* ... unchanged ... */
		size_t start = pos;
		pos += 2;
		while (pos < code.length() && code[pos] != '\n')
			pos++;
		return {TokenType::Comment, start, pos - start};
	}
	Token lexMultiLineComment(const std::string &code, size_t &pos)
	{ /* ... unchanged ... */
		size_t start = pos;
		pos += 2;
		while (pos + 1 < code.length())
		{
			if (code[pos] == '*' && code[pos + 1] == '/')
			{
				pos += 2;
				break;
			}
			pos++;
		}
		return {TokenType::Comment, start, pos - start};
	}
	Token lexIdentifierOrKeyword(const std::string &code, size_t &pos)
	{ /* ... unchanged ... */
		size_t start = pos;
		while (pos < code.length() && isAlphaNumeric(code[pos]))
			pos++;
		std::string word = code.substr(start, pos - start);
		if (keywords.count(word))
			return {TokenType::Keyword, start, pos - start};
		if (reactKeywords.count(word))
			return {TokenType::ReactHook, start, pos - start};
		size_t nextCharPos = pos;
		while (nextCharPos < code.length() && isWhitespace(code[nextCharPos]))
			nextCharPos++;
		bool followedByParen = (nextCharPos < code.length() && code[nextCharPos] == '(');
		if (followedByParen)
			return {TokenType::Function, start, pos - start};
		bool isCapitalized = (word.length() > 0 && word[0] >= 'A' && word[0] <= 'Z');
		if (isCapitalized)
			return {TokenType::ComponentName, start, pos - start};
		return {TokenType::Identifier, start, pos - start};
	}
	Token lexNumber(const std::string &code, size_t &pos)
	{ /* ... unchanged ... */
		size_t start = pos;
		if (pos + 1 < code.length() && code[pos] == '0')
		{
			char next = code[pos + 1];
			if (next == 'x' || next == 'X' || next == 'b' || next == 'B' || next == 'o' ||
				next == 'O')
				pos += 2;
		}
		while (pos < code.length() && (isAlphaNumeric(code[pos]) || code[pos] == '.'))
		{
			if (code[pos] == '.' &&
				code.substr(start, pos - start).find('.') != std::string::npos)
				break;
			if (!isDigit(code[pos]) && code[pos] != '.' &&
				!((code[pos] >= 'a' && code[pos] <= 'f') ||
				  (code[pos] >= 'A' && code[pos] <= 'F') || code[pos] == 'x' ||
				  code[pos] == 'X' || code[pos] == 'b' || code[pos] == 'B' ||
				  code[pos] == 'o' || code[pos] == 'O'))
				break;
			pos++;
		}
		if (pos < code.length() && (code[pos] == 'e' || code[pos] == 'E'))
		{
			size_t expStart = pos;
			pos++;
			if (pos < code.length() && (code[pos] == '+' || code[pos] == '-'))
				pos++;
			size_t digitStart = pos;
			while (pos < code.length() && isDigit(code[pos]))
				pos++;
			if (pos == digitStart)
				pos = expStart;
		}
		return {TokenType::Number, start, pos - start};
	}
	Token lexString(const std::string &code, size_t &pos)
	{ /* ... unchanged ... */
		size_t start = pos;
		char quote = code[pos];
		pos++;
		while (pos < code.length())
		{
			if (code[pos] == '\\' && pos + 1 < code.length())
				pos += 2;
			else if (code[pos] == quote)
			{
				pos++;
				break;
			} else
				pos++;
		}
		return {TokenType::String, start, pos - start};
	}
	Token lexOperatorOrPunctuation(const std::string &code, size_t &pos)
	{ /* ... unchanged ... */
		size_t start = pos;
		size_t maxLength = 3;
		std::string longestMatch;
		for (size_t len = 1; len <= maxLength && start + len <= code.length(); ++len)
		{
			std::string sub = code.substr(start, len);
			if (operators.count(sub))
				longestMatch = sub;
		}
		if (!longestMatch.empty())
		{
			pos += longestMatch.length();
			TokenType type = TokenType::Operator;
			if (longestMatch == "=>")
				type = TokenType::ArrowFunction;
			if (longestMatch == ";")
				type = TokenType::Operator;
			return {type, start, longestMatch.length()};
		}
		char c = code[pos];
		if (c == '(' || c == ')')
		{
			pos++;
			return {TokenType::Parenthesis, start, 1};
		}
		if (c == '[' || c == ']')
		{
			pos++;
			return {TokenType::Bracket, start, 1};
		}
		if (c == '{' || c == '}')
		{
			pos++;
			return {TokenType::Brace, start, 1};
		}
		if (c == ':')
		{
			pos++;
			return {TokenType::Colon, start, 1};
		}
		if (c == ',')
		{
			pos++;
			return {TokenType::Comma, start, 1};
		}
		if (c == '.')
		{
			pos++;
			return {TokenType::Dot, start, 1};
		}
		std::string singleCharStr(1, c);
		if (operators.count(singleCharStr))
		{
			pos++;
			return {TokenType::Operator, start, 1};
		}
		pos++;
		return {TokenType::Unknown, start, 1};
	}

	// --- State-Specific Lexing Functions ---

	// Lexes standard JavaScript (mostly unchanged, handles Template Literal
	// state)
	Token lexJS(const std::string &code,
				size_t &pos,
				std::stack<LexerState> &stateStack,
				TokenType lastNonWsToken)
	{
		size_t start = pos;
		if (pos >= code.length())
			return {TokenType::Unknown, start, 0}; // EOF check

		char c = code[pos];

		if (c == '`')
		{
			pos++;
			stateStack.push(LexerState::TEMPLATE_LITERAL);
			return {TokenType::TemplateString, start, 1}; // Token for '`'
		} else if (isAlpha(c) || c == '_' || c == '$')
		{
			return lexIdentifierOrKeyword(code, pos);
		} else if (isDigit(c))
		{
			return lexNumber(code, pos);
		} else if (c == '"' || c == '\'')
		{
			return lexString(code, pos);
		} else if (c == '{')
		{
			pos++;
			return {TokenType::Brace, start, 1};
		} else if (c == '}')
		{
			pos++;
			// Check if closing an expression inside a template literal
			// Needs careful check: ensure stack isn't empty before peeking
			if (stateStack.size() > 1)
			{
				LexerState parentState = LexerState::JS; // Placeholder
				// Peek at the state below JS
				LexerState currentJSState = stateStack.top(); // Should be JS
				stateStack.pop();							  // Pop JS temporarily
				if (!stateStack.empty())
				{
					parentState = stateStack.top();
				}
				stateStack.push(currentJSState); // Put JS back

				if (parentState == LexerState::TEMPLATE_LITERAL)
				{
					// Pop the actual JS state we just put back, the caller
					// (lexTemplateLiteral) manages the actual pop later
					stateStack.pop();
					return {TokenType::TemplateExprEnd,
							start,
							1}; // Return specific token
				}
			}
			// If not closing a template expr, it's a standard JS brace
			return {TokenType::Brace, start, 1};
		}
		// '<' is now just an operator in JS context
		else if (c == '<')
		{
			return lexOperatorOrPunctuation(code, pos);
		} else
		{
			return lexOperatorOrPunctuation(code, pos);
		}
	}

	// ========================================================================
	// LEX RETURN BLOCK - Rewritten Logic
	// ========================================================================
	Token lexReturnBlock(const std::string &code,
						 size_t &pos,
						 std::stack<LexerState> &stateStack)
	{
		size_t block_start = pos; // Position of the '(', '{', or '<'

		if (pos >= code.length())
		{ // Safety check at start
			if (!stateStack.empty() && stateStack.top() == LexerState::IN_RETURN_BLOCK)
				stateStack.pop();
			return {TokenType::Unknown, block_start, 0};
		}

		char start_char = code[pos];
		char end_char = 0;
		int balance = 0;		   // For () or {} balancing ONLY
		bool seek_jsx_end = false; // True if starting with <

		// Determine ending condition and consume opener if needed
		if (start_char == '(')
		{
			end_char = ')';
			balance++;
			pos++; // Consume '('
		} else if (start_char == '{')
		{
			end_char = '}';
			balance++;
			pos++; // Consume '{'
		} else if (start_char == '<')
		{
			seek_jsx_end = true;
			// DO NOT consume '<', it's part of the content
			// Termination is ';' or '}' at level 0 (checked below)
		} else
		{
			// Error case: Should not be in this state without valid starter
			std::cerr << "Error: lexReturnBlock unexpected start char '" << start_char
					  << "' at pos " << pos << std::endl;
			if (!stateStack.empty() && stateStack.top() == LexerState::IN_RETURN_BLOCK)
				stateStack.pop();
			return {TokenType::Unknown, block_start, 0};
		}

		// Balance counters for tracking nesting *within* the return block,
		// especially important for the seek_jsx_end case.
		int internal_brace_balance = 0;
		int internal_paren_balance = 0;

		while (pos < code.length())
		{
			char c = code[pos];

			// --- Skip Comments, Strings, Template Literals ---
			// (Essential to avoid misinterpreting delimiters inside them)
			if (c == '"' || c == '\'')
			{ /* Skip String */
				char q = c;
				pos++;
				while (pos < code.length())
				{
					if (code[pos] == '\\' && pos + 1 < code.length())
						pos += 2;
					else if (code[pos] == q)
					{
						pos++;
						break;
					} else
						pos++;
				}
				continue;
			}
			if (c == '/' && pos + 1 < code.length())
			{
				if (code[pos + 1] == '/')
				{ /* Skip Single Line Comment */
					pos += 2;
					while (pos < code.length() && code[pos] != '\n')
						pos++;
					if (pos < code.length())
						pos++;
					continue;
				} else if (code[pos + 1] == '*')
				{ /* Skip Multi Line Comment */
					pos += 2;
					bool closed = false;
					while (pos + 1 < code.length())
					{
						if (code[pos] == '*' && code[pos + 1] == '/')
						{
							pos += 2;
							closed = true;
							break;
						}
						pos++;
					}
					if (!closed)
						pos = code.length();
					continue;
				}
			}
			if (c == '`')
			{ /* Skip Template Literal */
				pos++;
				while (pos < code.length())
				{
					if (code[pos] == '\\' && pos + 1 < code.length())
						pos += 2;
					else if (code[pos] == '$' && pos + 1 < code.length() &&
							 code[pos + 1] == '{')
					{
						pos += 2;
						int templ_balance = 1;
						while (pos < code.length() && templ_balance > 0)
						{ /* Skip nested structures inside ${} */
							if (code[pos] == '"' || code[pos] == '\'')
							{
								char q = code[pos++];
								while (pos < code.length())
								{
									if (code[pos] == '\\' && pos + 1 < code.length())
										pos += 2;
									else if (code[pos] == q)
									{
										pos++;
										break;
									} else
										pos++;
								}
								continue;
							}
							if (code[pos] == '`')
							{
								pos++;
								while (pos < code.length())
								{
									if (code[pos] == '\\' && pos + 1 < code.length())
										pos += 2;
									else if (code[pos] == '`')
									{
										pos++;
										break;
									} else
										pos++;
								}
								continue;
							}
							if (code[pos] == '{')
								templ_balance++;
							else if (code[pos] == '}')
								templ_balance--;
							pos++;
						}
						continue;
					} else if (code[pos] == '`')
					{
						pos++;
						break;
					} else
					{
						pos++;
					}
				}
				continue;
			}
			// --- End Skipping ---

			// --- Update Internal Balances (Relevant for seek_jsx_end) ---
			if (c == '{')
				internal_brace_balance++;
			else if (c == '}')
				internal_brace_balance--;
			else if (c == '(')
				internal_paren_balance++;
			else if (c == ')')
				internal_paren_balance--;

			// --- Check for End Condition ---
			if (seek_jsx_end)
			{
				// End Condition 1: Top-level semicolon
				if (c == ';' && internal_brace_balance == 0 && internal_paren_balance == 0)
				{
					pos++; // Consume the semicolon
					break; // Found end
				}
				// End Condition 2: Likely function/block closing brace
				if (c == '}' && internal_brace_balance < 0 && internal_paren_balance == 0)
				{
					// Correction: check internal_brace_balance < 0, as the
					// closing '}' brings the count below zero if it's the outer
					// function's brace. DO NOT consume the '}' - it belongs to
					// the outer JS scope.
					break; // Found likely end of containing block
				}
			} else
			{ // Balancing '(' or '{'
				if (c == start_char)
				{ // Needs to match the specific opener '('
					// or '{'
					balance++;
				} else if (c == end_char)
				{ // Needs to match the specific
					// closer ')' or '}'
					balance--;
					if (balance == 0)
					{
						pos++; // Consume the closing character ')' or '}'
						break; // Found matching end
					}
				}
			}

			pos++; // Consume the current character
		} // End while loop

		// Pop the IN_RETURN_BLOCK state from the stack
		if (!stateStack.empty() && stateStack.top() == LexerState::IN_RETURN_BLOCK)
		{
			stateStack.pop();
		} else
		{
			// This indicates a logic error if the state stack is corrupted
			// std::cerr << "Warning: Popping state but top was not
			// IN_RETURN_BLOCK at pos " << pos << std::endl;
		}

		// Ensure we didn't somehow go past the end
		if (pos > code.length())
			pos = code.length();

		// Return single token for the entire block
		size_t length = (pos >= block_start) ? (pos - block_start) : 0;
		return {TokenType::RETURN_BLOCK_CONTENT, block_start, length};
	}

	// Lex inside a template literal `...` (Handles state changes correctly)
	Token lexTemplateLiteral(const std::string &code,
							 size_t &pos,
							 std::stack<LexerState> &stateStack)
	{
		size_t start = pos;
		while (pos < code.length())
		{
			if (code[pos] == '\\' && pos + 1 < code.length())
			{
				pos += 2; // Skip escape sequence
			} else if (code[pos] == '$' && pos + 1 < code.length() && code[pos + 1] == '{')
			{
				// Found start of expression ${
				if (pos > start)
				{
					// Return the text part before the ${ first
					return {TokenType::TemplateString, start, pos - start};
				} else
				{
					// Start of expression right away
					pos += 2;
					stateStack.push(LexerState::JS); // Enter JS mode for the expression
					// The handling of '}' in lexJS needs to pop this state.
					return {TokenType::TemplateExprStart, start, 2};
				}
			} else if (code[pos] == '`')
			{
				// Found end of template literal
				if (pos > start)
				{
					// Emit the last text part *before* the closing `
					// Pop state *after* returning this token (or before if
					// returning final backtick)
					Token textToken = {TokenType::TemplateString, start, pos - start};
					// State popped by caller or next iteration detecting the `
					// token below
					return textToken;
				} else
				{
					// End of literal right away (empty literal `` or just
					// processed ${...})
					pos++;
					if (!stateStack.empty() &&
						stateStack.top() == LexerState::TEMPLATE_LITERAL)
					{
						stateStack.pop(); // Exit TEMPLATE_LITERAL state
					}
					return {TokenType::TemplateString,
							start,
							1}; // Token for the closing '`'
				}
			} else
			{
				pos++; // Consume literal character
			}
		}

		// Reached end of code within template literal (unclosed)
		if (!stateStack.empty() && stateStack.top() == LexerState::TEMPLATE_LITERAL)
		{
			stateStack.pop(); // Pop state if unclosed at EOF
		}
		if (pos > start)
		{
			return {TokenType::TemplateString, start, pos - start};
		} else
		{
			return {TokenType::Unknown, start, 0};
		}
	}

	ImVec4 getColorForTokenType(TokenType type) const
	{
		updateThemeColors();
		switch (type)
		{
		case TokenType::Keyword:
			return cachedColors.keyword;
		case TokenType::ReactHook:
			return cachedColors.reactHook;
		case TokenType::String:
		case TokenType::TemplateString:
			return cachedColors.string;
		case TokenType::Number:
			return cachedColors.number;
		case TokenType::Comment:
			return cachedColors.comment;
		case TokenType::Function:
			return cachedColors.function;
		case TokenType::ComponentName:
			return cachedColors.componentName;
		case TokenType::Operator:
		case TokenType::ArrowFunction:
		case TokenType::Parenthesis:
		case TokenType::Bracket:
		case TokenType::Colon:
		case TokenType::Comma:
		case TokenType::Dot:
			return cachedColors.operatorColor;
		case TokenType::Brace:
		case TokenType::TemplateExprStart:
		case TokenType::TemplateExprEnd:
			return cachedColors.operatorColor;
		case TokenType::RETURN_BLOCK_CONTENT:
			return cachedColors.returnBlock;
		case TokenType::Identifier:
		case TokenType::Whitespace:
		case TokenType::Unknown:
		default:
			return cachedColors.text;
		}
	}
};
} // namespace JsxLexer