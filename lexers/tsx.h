#pragma once

#include <iostream>
#include <stack> // Needed for JSX tag balancing (optional but good) and state management
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "../util/settings.h" // Ensure this path is correct for your project

#include "imgui.h" // Ensure this path is correct for your project

// Forward declare Settings to avoid circular dependency if needed,
// but ensure gSettings definition is available at link time.
class Settings;
extern Settings gSettings;

// Forward declare json if needed (likely included via settings.h)
#include <lib/json.hpp> // Assuming settings.h includes this, otherwise add it here

namespace TsxLexer {

enum class TokenType {
	Whitespace,
	Identifier,
	Keyword,		 // TS keywords like if, for, class, let, const, etc.
	Type,			 // TS types like string, number, boolean, any, custom types,
					 // interfaces
	String,			 // "string", 'string'
	TemplateLiteral, // `template ${expr}`
	Number,
	Comment,	  // // comment, /* comment */
	JsxComment,	  // {/* jsx comment */}
	Preprocessor, // Not typical in TS/JS, but keep for potential future use or
				  // directives? (Less likely needed)
	Operator,	  // +, -, *, /, =, =>, ===, !==, ?, ??, etc.
	Parenthesis,  // ()
	Bracket,	  // []
	Brace,		  // {} (used for blocks and JSX expressions)
	Semicolon,
	Comma,
	Dot,   // Member access
	Colon, // Type annotations, object literals
	Unknown,
	Function,	  // Function names
	ClassName,	  // Class names
	Decorator,	  // @decorator
	JsxTag,		  // <, >, /, =, /> within JSX tags (includes self-closing/fragment
				  // end)
	JsxTagName,	  // Component or HTML tag name (e.g., div, MyComponent)
	JsxAttribute, // Attribute name (e.g., className, onClick)
	JsxText,	  // Text content within JSX elements
	RegexLiteral, // /regex/flags
	OptionalChain // ?. operator
};

struct Token
{
	TokenType type;
	size_t start;
	size_t length;
};

// Simple state for JSX parsing
enum class LexerState {
	Default,	 // Parsing standard TS code
	InJsxTag,	 // Inside a tag definition <...>
	InJsxContent // Between tags >...<
};

class Lexer
{
  public:
	// Theme colors cache
	struct ThemeColors
	{
		ImVec4 keyword;
		ImVec4 type; // Added for TS types
		ImVec4 string;
		ImVec4 number;
		ImVec4 comment;
		ImVec4 text;
		ImVec4 function;
		ImVec4 className; // Often same as type or keyword
		ImVec4 decorator;
		ImVec4 jsxTag;
		ImVec4 jsxAttribute;
		ImVec4 jsxText;		  // Often same as default text
		ImVec4 operatorColor; // Using a separate name to avoid conflict
		ImVec4 templateLiteral;
		ImVec4 regexLiteral;
	};

	Lexer()
	{
		// Combined TypeScript & JavaScript Keywords
		keywords = {"abstract",
					"as",
					"async",
					"await",
					"break",
					"case",
					"catch",
					"class",
					"const",
					"continue",
					"debugger",
					"default",
					"delete",
					"do",
					"else",
					"enum",
					"export",
					"extends",
					"false",
					"finally",
					"for",
					"from",
					"function",
					"if",
					"implements",
					"import",
					"in",
					"instanceof",
					"interface",
					"let",
					"new",
					"null",
					"package", // reserved
					"private",
					"protected",
					"public",
					"return",
					"static",
					"super",
					"switch",
					"this",
					"throw",
					"true",
					"try",
					"typeof",
					"var",
					"void",
					"while",
					"with", // reserved in strict mode
					"yield",
					// TS Specific
					"any",
					"boolean",
					"constructor",
					"declare",
					"get",
					"module", // legacy
					"namespace",
					"never",
					"readonly",
					"require", // legacy
					"number",
					"set",
					"string",
					"symbol",
					"type",
					"undefined", // Also a value
					"unique",
					"unknown",
					"accessor",
					"asserts",
					"infer",
					"is",
					"keyof",
					"out",
					"override",
					// Common Contextual Keywords (often highlighted)
					"useState",
					"useEffect",
					"useContext",
					"useReducer",
					"useCallback",
					"useMemo",
					"useRef",
					"useImperativeHandle",
					"useLayoutEffect",
					"useDebugValue"};

		// Basic built-in types (others identified contextually)
		builtinTypes = {"any",
						"boolean",
						"number",
						"string",
						"symbol",
						"void",
						"null",		 // Also a value
						"undefined", // Also a keyword/value
						"never",
						"object",
						"unknown",
						"bigint",
						// Common DOM/React types (add more as needed)
						"ReactElement",
						"JSX.Element",
						"ReactNode",
						"ChangeEvent",
						"MouseEvent",
						"KeyboardEvent",
						"CSSProperties",
						"HTMLElement",
						"HTMLDivElement", // etc.
						"Promise",
						"Array",
						"Map",
						"Set",
						"Date"};

		// Operators (including TS specific ones)
		operators = {
			// Length 3
			{"===", TokenType::Operator},
			{"!==", TokenType::Operator},
			{"**=", TokenType::Operator},
			// Length 2
			{"==", TokenType::Operator},
			{"!=", TokenType::Operator},
			{">=", TokenType::Operator},
			{"<=", TokenType::Operator},
			{"&&", TokenType::Operator},
			{"||", TokenType::Operator},
			{"??", TokenType::Operator},
			{"?.", TokenType::OptionalChain},
			{"=>", TokenType::Operator},
			{"++", TokenType::Operator},
			{"--", TokenType::Operator},
			{"+=", TokenType::Operator},
			{"-=", TokenType::Operator},
			{"*=", TokenType::Operator},
			{"/=", TokenType::Operator},
			{"%=", TokenType::Operator},
			{"&=", TokenType::Operator},
			{"|=", TokenType::Operator},
			{"^=", TokenType::Operator},
			{"<<=", TokenType::Operator},
			{">>=", TokenType::Operator}, // Add missing shift assignments
			{"<<", TokenType::Operator},
			{">>", TokenType::Operator}, // {">>>", TokenType::Operator} // Add
										 // >>> if needed
			{"**", TokenType::Operator},
			// Length 1
			{"+", TokenType::Operator},
			{"-", TokenType::Operator},
			{"*", TokenType::Operator},
			{"/", TokenType::Operator},
			{"%", TokenType::Operator},
			{"=", TokenType::Operator},
			{">", TokenType::Operator},
			{"<", TokenType::Operator},
			{"!", TokenType::Operator},
			{"&", TokenType::Operator},
			{"|", TokenType::Operator},
			{"^", TokenType::Operator},
			{"~", TokenType::Operator},
			{"?", TokenType::Operator} // Ternary conditional
		};

		colorsNeedUpdate = true;
		currentState = LexerState::Default;
	}

	void themeChanged() { colorsNeedUpdate = true; }

	std::vector<Token> tokenize(const std::string &code)
	{
		// std::cout << "Inside TSX tokenizer.." << std::endl;
		std::vector<Token> tokens;
		size_t pos = 0;
		size_t lastPos = 0;
		size_t maxIterations =
			code.length() * 4; // Increased slightly due to state complexity
		size_t iterations = 0;

		// Reset state for each tokenization pass
		currentState = LexerState::Default;
		jsxTagStack = {}; // Optional: Reset JSX tag stack if used
		while (!stateStack.empty())
			stateStack.pop(); // <<< CLEAR THE STATE STACK

		try
		{
			// std::cout << "Starting TSX tokenization loop" << std::endl;
			while (pos < code.length() && iterations < maxIterations)
			{
				lastPos = pos;
				char current_char = code[pos];

				// --- State-Based Parsing ---
				if (currentState == LexerState::Default)
				{
					// --- Standard TypeScript/JavaScript Parsing ---
					if (isWhitespace(current_char))
					{
						tokens.push_back(lexWhitespace(code, pos));
					} else if (current_char == '/' && pos + 1 < code.length())
					{
						// Handle comments and regex first
						if (code[pos + 1] == '/')
						{
							tokens.push_back(lexSingleLineComment(code, pos));
						} else if (code[pos + 1] == '*')
						{
							tokens.push_back(lexMultiLineComment(code, pos));
						} else if (isRegexStart(code, pos, tokens))
						{ // Check if it's a
							// regex literal
							tokens.push_back(lexRegexLiteral(code, pos));
						} else
						{ // Assume division operator
							tokens.push_back(lexOperatorOrPunctuation(
								code, pos)); // Use the unified function
						}
					} else if (current_char == '`')
					{
						tokens.push_back(lexTemplateLiteral(code, pos));
					} else if (current_char == '"' || current_char == '\'')
					{
						tokens.push_back(lexString(code, pos));
					} else if (isAlpha(current_char) || current_char == '_' ||
							   current_char == '$')
					{ // Identifiers can start with $, _
						tokens.push_back(lexIdentifierKeywordOrType(code, pos, tokens));
					} else if (isDigit(current_char) ||
							   (current_char == '.' && pos + 1 < code.length() &&
								isDigit(code[pos + 1])))
					{
						tokens.push_back(lexNumber(code, pos));
					} else if (current_char == '@')
					{
						tokens.push_back(lexDecorator(code, pos));
					}
					// --- Handle '}' specifically to return from embedded
					// expression ---
					else if (current_char == '}')
					{ // Potential end of embedded JSX expression
						tokens.push_back({TokenType::Brace, pos++, 1});
						if (!stateStack.empty())
						{
							currentState = stateStack.top(); // Restore previous JSX state
							stateStack.pop();
						}
						// If stack was empty, it's just a normal brace, stay in
						// Default state.
					}
					// --- JSX ENTRY POINT / Less-Than Operator ---
					else if (current_char == '<')
					{
						// Check if next char could reasonably be part of a
						// tag/fragment/comment
						if (pos + 1 < code.length() &&
							(isAlpha(code[pos + 1]) || code[pos + 1] == '/' ||
							 code[pos + 1] == '>' ||
							 (code[pos + 1] == '!' && pos + 3 < code.length() &&
							  code[pos + 2] == '-' && code[pos + 3] == '-')))
						{ // Added check for <!-- comment start

							// Check for HTML/XML comment <!-- ... --> which
							// isn't standard JSX but might appear
							if (code[pos + 1] == '!' && code[pos + 2] == '-' &&
								code[pos + 3] == '-')
							{
								// Treat as a multi-line comment for
								// highlighting purposes Need a specific
								// lexer or adapt multi-line? For now, let's
								// treat it as unknown or handle simply
								tokens.push_back(
									lexHtmlComment(code,
												   pos)); // Need to add lexHtmlComment
														  // helper
								continue;				  // Skip rest of loop iteration
							}

							// Context check: Is '<' acting as JSX start or
							// less-than operator?
							bool isJsxContext = false;
							int lastTokenIdx = findLastNonWhitespaceTokenIndex(tokens);

							if (lastTokenIdx != -1)
							{
								const auto &prevToken = tokens[lastTokenIdx];
								char prevChar = (prevToken.length > 0 &&
												 prevToken.start < code.length())
													? code[prevToken.start]
													: '\0';

								// Contexts expecting JSX
								if (prevToken.type == TokenType::Keyword)
								{
									std::string prevWord =
										code.substr(prevToken.start, prevToken.length);
									if (prevWord == "return" || prevWord == "yield" ||
										prevWord == "case" || prevWord == "default" ||
										prevWord == "throw" || prevWord == "export")
										isJsxContext = true;
								} else if ((prevToken.type == TokenType::Parenthesis &&
											prevChar == '(') ||
										   (prevToken.type == TokenType::Bracket &&
											prevChar == '[') ||
										   (prevToken.type == TokenType::Comma) ||
										   (prevToken.type == TokenType::Colon) ||
										   (prevToken.type == TokenType::Semicolon))
									isJsxContext = true;
								else if (prevToken.type == TokenType::Operator)
								{
									std::string opStr =
										code.substr(prevToken.start, prevToken.length);
									if (opStr != "++" && opStr != "--")
										isJsxContext = true;
								}
								// Add check for JSX directly inside braces
								// (e.g. array map)
								else if (prevToken.type == TokenType::Brace &&
										 prevChar == '{')
								{
									// If the opening brace didn't come
									// from JSX itself, it might start
									// an object where JSX isn't allowed
									// directly. However, map(() => {
									// return (<...>) }) is common. This
									// needs more sophisticated context
									// tracking (parser) for 100%
									// accuracy. A reasonable heuristic:
									// allow JSX after '{' if it's
									// likely inside function
									// body/expression context. For
									// simplicity, let's allow it but be
									// aware of potential FPs.
									isJsxContext = true;
								}

								// Contexts likely meaning Less-Than
								// (Override)
								if (prevToken.type == TokenType::Identifier ||
									prevToken.type == TokenType::Number ||
									prevToken.type == TokenType::String ||
									prevToken.type == TokenType::TemplateLiteral ||
									prevToken.type == TokenType::RegexLiteral ||
									(prevToken.type == TokenType::Parenthesis &&
									 prevChar == ')') ||
									(prevToken.type == TokenType::Bracket &&
									 prevChar == ']') ||
									(prevToken.type == TokenType::Brace &&
									 prevChar == '}') ||
									(prevToken.type == TokenType::Keyword &&
									 (code.substr(prevToken.start, prevToken.length) ==
										  "type" ||
									  code.substr(prevToken.start, prevToken.length) ==
										  "interface")) ||
									(prevToken.type == TokenType::Operator &&
									 (code.substr(prevToken.start, prevToken.length) ==
										  "++" ||
									  code.substr(prevToken.start, prevToken.length) ==
										  "--")))
								{
									isJsxContext = false;
								}
							} else
							{
								// Start of file - could be JSX fragment <>
								// or <Component/> directly
								// exported/rendered
								isJsxContext = true; // Be more lenient at
													 // start of file
							}

							if (isJsxContext)
							{
								// --- Enter JSX Mode ---
								currentState = LexerState::InJsxTag;
								tokens.push_back(
									{TokenType::JsxTag, pos, 1}); // Tokenize '<'
								pos++;

								if (pos < code.length())
								{
									if (code[pos] == '>')
									{ // Fragment <>
										tokens.push_back({TokenType::JsxTag, pos, 1});
										pos++;
										currentState = LexerState::InJsxContent;
									} else if (code[pos] == '/')
									{ // Closing tag </
										tokens.push_back(
											{TokenType::JsxTag, pos, 1}); // Tokenize '/'
										pos++;
									}
								}
							} else
							{ // Treat as less-than operator
								tokens.push_back(lexOperatorOrPunctuation(code, pos));
							}
						} else
						{ // Treat as less-than operator
							tokens.push_back(lexOperatorOrPunctuation(code, pos));
						}
					} // --- END of '<' handling ---
					else
					{ // Handle other characters (operators, punctuation
						// not handled above)
						tokens.push_back(lexOperatorOrPunctuation(code, pos));
					}
				}
				// --- END of Default State Logic ---

				else if (currentState == LexerState::InJsxTag)
				{
					// --- Parsing Inside JSX Tag Definition <...> ---
					if (isWhitespace(current_char))
					{
						tokens.push_back(lexWhitespace(code, pos));
					} else if (isAlpha(current_char) || current_char == '_')
					{ // TagName or AttributeName
						size_t start = pos;
						// Include dot for member components
						while (pos < code.length() &&
							   (isAlphaNumeric(code[pos]) || code[pos] == '-' ||
								code[pos] == ':' || code[pos] == '.'))
						{
							pos++;
						}
						std::string word = code.substr(start, pos - start);
						TokenType determinedType = TokenType::JsxAttribute;
						int lastNonWsIdx = findLastNonWhitespaceTokenIndex(tokens);
						if (lastNonWsIdx != -1)
						{
							const auto &prevToken = tokens[lastNonWsIdx];
							if (prevToken.type == TokenType::JsxTag &&
								(code[prevToken.start] == '<' ||
								 code[prevToken.start] == '/'))
							{
								determinedType = TokenType::JsxTagName;
							}
						} else if (tokens.empty())
						{
							determinedType = TokenType::JsxTagName;
						}
						tokens.push_back({determinedType, start, pos - start});
					} else if (current_char == '=')
					{
						tokens.push_back({TokenType::JsxTag, pos++, 1});
					} else if (current_char == '"' || current_char == '\'')
					{
						tokens.push_back(lexString(code, pos));
					} else if (current_char == '{')
					{ // Start of embedded expression
						tokens.push_back({TokenType::Brace, pos++, 1});
						stateStack.push(
							LexerState::InJsxTag);			// <<< PUSH originating state
						currentState = LexerState::Default; // Switch back to parse
															// TS expression
					} else if (current_char == '/')
					{ // Potential self-closing tag />
						if (pos + 1 < code.length() && code[pos + 1] == '>')
						{
							tokens.push_back(
								{TokenType::JsxTag, pos, 2}); // Tokenize '/>'
							pos += 2;
							// Finished tag, return to previous context (could
							// be Content or Default)
							if (!stateStack.empty())
							{
								currentState = stateStack.top(); // Could be InJsxContent
																 // if nested
								stateStack.pop();
							} else
							{
								currentState = LexerState::Default; // Assume end of
																	// top-level JSX
																	// block
							}
						} else
						{
							tokens.push_back({TokenType::Unknown,
											  pos++,
											  1}); // Treat '/' alone as unknown here
						}
					} else if (current_char == '>')
					{ // End of opening tag >
						tokens.push_back({TokenType::JsxTag, pos++, 1});
						currentState =
							LexerState::InJsxContent; // Move to content parsing
					} else
					{
						tokens.push_back({TokenType::Unknown, pos++, 1});
					}
				}
				// --- END of InJsxTag State Logic ---

				else if (currentState == LexerState::InJsxContent)
				{
					// --- Parsing Between JSX Tags >...< ---
					if (current_char == '<')
					{ // Start of a new tag or closing tag
						if (pos + 2 < code.length() && code[pos + 1] == '/' &&
							code[pos + 2] == '>')
						{ // Closing fragment </>
							tokens.push_back({TokenType::JsxTag, pos, 3});
							pos += 3;
							// End of fragment, return to previous context if
							// nested
							if (!stateStack.empty())
							{
								currentState = stateStack.top();
								stateStack.pop();
							} else
							{
								currentState = LexerState::Default; // Assume end of
																	// top-level JSX
																	// block
							}
						} else if (pos + 3 < code.length() && code[pos + 1] == '!' &&
								   code[pos + 2] == '-' && code[pos + 3] == '-')
						{ // HTML Comment <!--
							tokens.push_back(lexHtmlComment(code, pos));
							// Stay in InJsxContent after comment
						} else
						{ // Start of opening/closing tag
							tokens.push_back(
								{TokenType::JsxTag, pos++, 1}); // Tokenize '<'
							currentState = LexerState::InJsxTag;
							stateStack.push(
								LexerState::InJsxContent); // <<< PUSH
														   // originating state
														   // (before entering
														   // new tag)
							if (pos < code.length() && code[pos] == '/')
							{ // Handle closing tag </ immediately
								tokens.push_back(
									{TokenType::JsxTag, pos++, 1}); // Tokenize '/'
							}
						}
					} else if (current_char == '{')
					{ // Start of embedded expression or JSX
						// comment
						if (pos + 3 < code.length() && code[pos + 1] == '/' &&
							code[pos + 2] == '*')
						{
							tokens.push_back(lexJsxComment(code, pos));
							// Stay in InJsxContent after JSX comment
						} else
						{
							tokens.push_back({TokenType::Brace, pos++, 1});
							stateStack.push(
								LexerState::InJsxContent);		// <<< PUSH
																// originating state
							currentState = LexerState::Default; // Switch to parse TS
																// expression
						}
					} else
					{ // Treat as JsxText (anything not < or {)
						tokens.push_back(lexJsxText(code, pos));
					}
				}
				// --- END of InJsxContent State Logic ---

				// Safety check: If position hasn't advanced, increment to avoid
				// infinite loop
				if (pos == lastPos && pos < code.length())
				{
					// std::cerr << "TSX Tokenizer stuck at position " << pos <<
					// ", character: '" << code[pos] << "', state: " <<
					// (int)currentState << ". Advancing..." << std::endl;
					tokens.push_back({TokenType::Unknown, pos, 1});
					pos++;
				}

				iterations++;
			} // End while loop

			if (iterations >= maxIterations)
			{
				std::cerr << "🔴 TSX Tokenizer exceeded maximum iterations. "
							 "Possible infinite loop."
						  << std::endl;
			}

			// std::cout << "Finished TSX tokenization loop" << std::endl;
		} catch (const std::exception &e)
		{
			std::cerr << "🔴 Exception in TSX tokenize: " << e.what() << std::endl;
		} catch (...)
		{
			std::cerr << "🔴 Unknown exception in TSX tokenize" << std::endl;
		}

		// std::cout << "Exiting TSX tokenizer, tokens size: " << tokens.size()
		// << std::endl;
		return tokens;
	}

	void
	applyHighlighting(const std::string &code, std::vector<ImVec4> &colors, int start_pos)
	{
		// std::cout << "Entering TSX applyHighlighting, code length: " <<
		// code.length() << ", colors size: " << colors.size() << ", start_pos:
		// " << start_pos << std::endl;
		try
		{
			if (colors.size() < code.length())
			{
				colors.resize(code.length());
			}

			std::string subCode = code.substr(start_pos);
			std::vector<Token> tokens = tokenize(subCode);

			int colorChanges = 0;
			for (const auto &token : tokens)
			{
				if (token.length == 0)
					continue; // Skip zero-length tokens from lexJsxText edge
							  // case

				ImVec4 color = getColorForTokenType(token.type);
				size_t globalStart = start_pos + token.start;
				size_t globalEnd = globalStart + token.length;

				if (globalEnd > colors.size())
				{
					globalEnd = colors.size();
					if (globalStart >= globalEnd)
						continue;
				}

				for (size_t i = globalStart; i < globalEnd; ++i)
				{
					if (i < colors.size())
					{
						colors[i] = color;
						colorChanges++;
					} else
					{
						break;
					}
				}
			}
			// std::cout << "Exiting TSX applyHighlighting, changed " <<
			// colorChanges << " color values" << std::endl;
		} catch (const std::exception &e)
		{
			std::cerr << "🔴 Exception in TSX applyHighlighting: " << e.what()
					  << std::endl;
			if (!colorsNeedUpdate)
			{ // <<< CORRECTED VARIABLE NAME
				std::fill(colors.begin() + start_pos, colors.end(), cachedColors.text);
			} else
			{
				std::fill(colors.begin() + start_pos,
						  colors.end(),
						  ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
			}
		} catch (...)
		{
			std::cerr << "🔴 Unknown exception in TSX applyHighlighting" << std::endl;
			if (!colorsNeedUpdate)
			{ // <<< CORRECTED VARIABLE NAME
				std::fill(colors.begin() + start_pos, colors.end(), cachedColors.text);
			} else
			{
				std::fill(colors.begin() + start_pos,
						  colors.end(),
						  ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
			}
		}
	}

	void forceColorUpdate() { colorsNeedUpdate = true; }

  private:
	// --- Member Variables ---
	std::unordered_set<std::string> keywords;
	std::unordered_set<std::string> builtinTypes;
	std::unordered_map<std::string, TokenType> operators;
	mutable ThemeColors cachedColors;
	mutable bool colorsNeedUpdate = true;
	LexerState currentState;
	std::stack<std::string> jsxTagStack; // Optional
	std::stack<LexerState> stateStack;	 // <<< STATE STACK FOR JSX/TS CONTEXT

	// --- Helper Function ---
	int findLastNonWhitespaceTokenIndex(const std::vector<Token> &tokens) const
	{
		for (int i = static_cast<int>(tokens.size()) - 1; i >= 0; --i)
		{
			if (tokens[i].type != TokenType::Whitespace)
			{
				return i;
			}
		}
		return -1;
	}

	void updateThemeColors() const
	{
		if (!colorsNeedUpdate)
			return;
		try
		{
			auto &theme = gSettings.getSettings()["themes"][gSettings.getCurrentTheme()];
			auto loadColor = [&theme](const char *key,
									  ImVec4 defaultColor =
										  ImVec4(1.0f, 1.0f, 1.0f, 1.0f)) -> ImVec4 {
				try
				{
					if (theme.contains(key))
					{
						auto &c = theme[key];
						if (c.is_array() && c.size() == 4)
						{
							return ImVec4(c[0].get<float>(),
										  c[1].get<float>(),
										  c[2].get<float>(),
										  c[3].get<float>());
						}
					}
				} catch (const nlohmann::json::exception &e)
				{
					std::cerr << "Error loading theme color '" << key
							  << "' (JSON error): " << e.what() << ". Using default."
							  << std::endl;
				} catch (const std::exception &e)
				{
					std::cerr << "Error loading theme color '" << key << "': " << e.what()
							  << ". Using default." << std::endl;
				}
				return defaultColor;
			};
			cachedColors.text = loadColor("text");
			cachedColors.keyword = loadColor("keyword", cachedColors.text);
			cachedColors.type = loadColor("type", cachedColors.keyword);
			cachedColors.string = loadColor("string", cachedColors.text);
			cachedColors.number = loadColor("number", cachedColors.text);
			cachedColors.comment = loadColor("comment", ImVec4(0.5f, 0.5f, 0.5f, 1.0f));
			cachedColors.function = loadColor("function", cachedColors.text);
			cachedColors.className = loadColor("class_name", cachedColors.type);
			cachedColors.decorator = loadColor("decorator", cachedColors.function);
			cachedColors.jsxTag = loadColor("jsx_tag", cachedColors.keyword);
			cachedColors.jsxAttribute = loadColor("jsx_attribute", cachedColors.text);
			cachedColors.jsxText = loadColor("jsx_text", cachedColors.text);
			cachedColors.operatorColor = loadColor("operator", cachedColors.text);
			cachedColors.templateLiteral =
				loadColor("template_literal", cachedColors.string);
			cachedColors.regexLiteral = loadColor("regex_literal", cachedColors.string);
			colorsNeedUpdate = false;
		} catch (const std::exception &e)
		{
			std::cerr << "🔴 Error updating theme colors: " << e.what() << std::endl;
		}
	}

	// --- Basic Character Checks ---
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
		return isAlpha(c) || isDigit(c) || c == '_';
	} // Allow _
	bool isHexDigit(char c) const
	{
		return isDigit(c) || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
	}
	bool isOctalDigit(char c) const { return c >= '0' && c <= '7'; }

	// --- Lexing Helper Functions ---

	Token lexWhitespace(const std::string &code, size_t &pos)
	{
		size_t start = pos;
		while (pos < code.length() && isWhitespace(code[pos]))
			pos++;
		return {TokenType::Whitespace, start, pos - start};
	}

	Token lexSingleLineComment(const std::string &code, size_t &pos)
	{
		size_t start = pos;
		pos += 2; // Skip '//'
		while (pos < code.length() && code[pos] != '\n')
			pos++;
		return {TokenType::Comment, start, pos - start};
	}

	Token lexMultiLineComment(const std::string &code, size_t &pos)
	{
		size_t start = pos;
		pos += 2; // Skip '/*'
		while (pos + 1 < code.length() && !(code[pos] == '*' && code[pos + 1] == '/'))
			pos++;
		if (pos + 1 < code.length())
			pos += 2; // Skip '*/'
		else
			pos = code.length(); // Unterminated
		return {TokenType::Comment, start, pos - start};
	}

	Token lexJsxComment(const std::string &code, size_t &pos)
	{
		size_t start = pos;
		pos += 3; // Skip '{/*'
		while (pos + 2 < code.length() &&
			   !(code[pos] == '*' && code[pos + 1] == '/' && code[pos + 2] == '}'))
			pos++;
		if (pos + 2 < code.length())
			pos += 3; // Skip '*/}'
		else
			pos = code.length(); // Unterminated
		return {TokenType::JsxComment, start, pos - start};
	}

	// Added helper for <!-- HTML Comments --> (Treat as normal comment)
	Token lexHtmlComment(const std::string &code, size_t &pos)
	{
		size_t start = pos;
		pos += 4; // Skip <!--
		while (pos + 2 < code.length() &&
			   !(code[pos] == '-' && code[pos + 1] == '-' && code[pos + 2] == '>'))
		{
			pos++;
		}
		if (pos + 2 < code.length())
			pos += 3; // Skip -->
		else
			pos = code.length();						 // Unterminated
		return {TokenType::Comment, start, pos - start}; // Use standard Comment type
	}

	Token lexString(const std::string &code, size_t &pos)
	{
		size_t start = pos;
		char quote = code[pos++];
		while (pos < code.length())
		{
			if (code[pos] == '\\')
			{
				pos++;
				if (pos < code.length())
					pos++;
			} else if (code[pos] == quote)
			{
				pos++;
				break;
			} else
			{
				pos++;
			}
		}
		return {TokenType::String, start, pos - start};
	}

	Token lexTemplateLiteral(const std::string &code, size_t &pos)
	{
		size_t start = pos;
		pos++; // Skip `
		while (pos < code.length())
		{
			if (code[pos] == '\\')
			{
				pos++;
				if (pos < code.length())
					pos++;
			} else if (code[pos] == '$' && pos + 1 < code.length() && code[pos + 1] == '{')
			{
				pos += 2; // Skip ${
				int braceLevel = 1;
				// Rudimentary skipping of expression - doesn't handle nested
				// templates/comments/strings properly
				while (pos < code.length())
				{
					if (code[pos] == '{')
						braceLevel++;
					else if (code[pos] == '}')
					{
						braceLevel--;
						if (braceLevel == 0)
						{
							pos++;
							break;
						}
					} else if (code[pos] == '\\')
					{
						pos++;
					} // Skip escaped chars
					  // Naive skip
					pos++;
				}
			} else if (code[pos] == '`')
			{
				pos++;
				break;
			} else
			{
				pos++;
			}
		}
		return {TokenType::TemplateLiteral, start, pos - start};
	}

	Token lexNumber(const std::string &code, size_t &pos)
	{
		size_t start = pos;
		// Prefixes (0x, 0o, 0b)
		if (code[pos] == '0' && pos + 1 < code.length())
		{
			if (code[pos + 1] == 'x' || code[pos + 1] == 'X')
			{
				pos += 2;
				while (pos < code.length() && isHexDigit(code[pos]))
					pos++;
				return {TokenType::Number, start, pos - start};
			} else if (code[pos + 1] == 'o' || code[pos + 1] == 'O')
			{
				pos += 2;
				while (pos < code.length() && isOctalDigit(code[pos]))
					pos++;
				return {TokenType::Number, start, pos - start};
			} else if (code[pos + 1] == 'b' || code[pos + 1] == 'B')
			{
				pos += 2;
				while (pos < code.length() && (code[pos] == '0' || code[pos] == '1'))
					pos++;
				return {TokenType::Number, start, pos - start};
			}
		}
		// Decimal & Float
		while (pos < code.length() && isDigit(code[pos]))
			pos++;
		if (pos < code.length() && code[pos] == '.')
		{
			if (pos + 1 >= code.length() || !isDigit(code[pos + 1]))
			{ /* Just a dot */
			} else
			{
				pos++;
				while (pos < code.length() && isDigit(code[pos]))
					pos++;
			}
		}
		// Exponent
		if (pos < code.length() && (code[pos] == 'e' || code[pos] == 'E'))
		{
			size_t exponentPos = pos + 1;
			if (exponentPos < code.length() &&
				(code[exponentPos] == '+' || code[exponentPos] == '-'))
				exponentPos++;
			if (exponentPos < code.length() && isDigit(code[exponentPos]))
			{
				pos = exponentPos;
				while (pos < code.length() && isDigit(code[pos]))
					pos++;
			}
		}
		// BigInt 'n'
		if (pos < code.length() && code[pos] == 'n')
		{
			bool isInteger = true;
			for (size_t i = start; i < pos; ++i)
				if (code[i] == '.' || code[i] == 'e' || code[i] == 'E')
				{
					isInteger = false;
					break;
				}
			if (isInteger)
				pos++;
		}
		return {TokenType::Number, start, pos - start};
	}

	bool isRegexStart(const std::string &code,
					  size_t pos,
					  const std::vector<Token> &tokens) const
	{
		if (pos == 0)
			return false;
		int prevTokenIdx = findLastNonWhitespaceTokenIndex(tokens);
		if (prevTokenIdx == -1)
			return true; // Assume regex if first non-whitespace

		const auto &prevToken = tokens[prevTokenIdx];
		char prevChar = (prevToken.length > 0 && prevToken.start < code.length())
							? code[prevToken.start]
							: '\0';

		// Contexts likely DIVISION
		if (prevToken.type == TokenType::Identifier ||
			prevToken.type == TokenType::Number ||
			(prevToken.type == TokenType::Parenthesis && prevChar == ')') ||
			(prevToken.type == TokenType::Bracket && prevChar == ']') ||
			(prevToken.type == TokenType::Brace && prevChar == '}') ||
			(prevToken.type == TokenType::Operator &&
			 (code.substr(prevToken.start, prevToken.length) == "++" ||
			  code.substr(prevToken.start, prevToken.length) == "--")))
		{
			return false;
		}

		// Contexts likely REGEX
		if (prevToken.type == TokenType::Keyword)
		{
			std::string kw = code.substr(prevToken.start, prevToken.length);
			if (kw == "return" || kw == "yield" || kw == "case" || kw == "throw" ||
				kw == "new" || kw == "await" || kw == "delete" || kw == "typeof" ||
				kw == "void")
				return true;
		}
		if ((prevToken.type == TokenType::Parenthesis && prevChar == '(') ||
			(prevToken.type == TokenType::Bracket && prevChar == '[') ||
			(prevToken.type == TokenType::Brace && prevChar == '{') ||
			(prevToken.type == TokenType::Comma) ||
			(prevToken.type == TokenType::Colon) ||
			(prevToken.type == TokenType::Semicolon))
		{
			return true;
		}
		if (prevToken.type == TokenType::Operator)
		{
			std::string opStr = code.substr(prevToken.start, prevToken.length);
			if (opStr != "++" && opStr != "--")
			{
				return true;
			}
		}

		// Structural check if ambiguous
		if (pos + 1 < code.length() && (code[pos + 1] == '*' || code[pos + 1] == '/'))
			return false; // Comment
		size_t endSlashPos = pos + 1;
		bool escaped = false;
		bool inCharClass = false;
		while (endSlashPos < code.length())
		{
			char c = code[endSlashPos];
			if (escaped)
				escaped = false;
			else if (c == '\\')
				escaped = true;
			else if (inCharClass)
			{
				if (c == ']')
					inCharClass = false;
			} else if (c == '[')
				inCharClass = true;
			else if (c == '/')
				return true;
			else if (c == '\n')
				return false;
			endSlashPos++;
		}
		return false; // No closing slash or context was division
	}

	Token lexRegexLiteral(const std::string &code, size_t &pos)
	{
		size_t start = pos;
		pos++;
		bool escaped = false;
		bool inCharClass = false;
		while (pos < code.length())
		{
			char c = code[pos];
			if (escaped)
				escaped = false;
			else if (c == '\\')
				escaped = true;
			else if (inCharClass)
			{
				if (c == ']')
					inCharClass = false;
			} else if (c == '[')
				inCharClass = true;
			else if (c == '/')
			{
				pos++;
				break;
			} else if (c == '\n')
				break;
			pos++;
		}
		while (pos < code.length() && isAlpha(code[pos]))
			pos++; // Flags
		return {TokenType::RegexLiteral, start, pos - start};
	}

	Token lexIdentifierKeywordOrType(const std::string &code,
									 size_t &pos,
									 const std::vector<Token> &tokens)
	{
		size_t start = pos;
		while (pos < code.length() && (isAlphaNumeric(code[pos]) || code[pos] == '$'))
			pos++;
		std::string word = code.substr(start, pos - start);
		if (word.empty())
			return {TokenType::Unknown, start, 0}; // Should not happen

		// 1. Keywords
		if (keywords.count(word))
		{
			if (word == "class" || word == "interface" || word == "enum" ||
				word == "type" || word == "namespace")
			{
				size_t namePos = pos;
				while (namePos < code.length() && isWhitespace(code[namePos]))
					namePos++;
				if (namePos < code.length() &&
					(isAlpha(code[namePos]) || code[namePos] == '_' ||
					 code[namePos] == '$'))
					return {TokenType::Keyword, start, pos - start};
			} else
				return {TokenType::Keyword, start, pos - start};
		}

		// 2. Builtin Types
		if (builtinTypes.count(word))
			return {TokenType::Type, start, pos - start};

		// 3. Contextual Checks
		size_t nextNonWs = pos;
		while (nextNonWs < code.length() && isWhitespace(code[nextNonWs]))
			nextNonWs++;
		TokenType inferredType = TokenType::Identifier;
		int prevTokenIdx = findLastNonWhitespaceTokenIndex(tokens);
		if (prevTokenIdx != -1)
		{
			const auto &prevToken = tokens[prevTokenIdx];
			std::string prevWord = "";
			if (prevToken.length > 0)
				prevWord = code.substr(prevToken.start, prevToken.length);
			if (prevToken.type == TokenType::Keyword)
			{
				if (prevWord == "class" || prevWord == "interface" ||
					prevWord == "enum" || prevWord == "namespace")
					inferredType = TokenType::ClassName;
				else if (prevWord == "type")
					inferredType = TokenType::Type;
				else if (prevWord == "function" || prevWord == "get" || prevWord == "set")
					inferredType = TokenType::Function;
				else if (prevWord == "const" || prevWord == "let" || prevWord == "var" ||
						 prevWord == "import" || prevWord == "export")
				{
					if (nextNonWs < code.length() &&
						(code[nextNonWs] == '(' || code[nextNonWs] == '<'))
						inferredType = TokenType::Function;
				} else if (prevWord == "extends" || prevWord == "implements")
					inferredType = TokenType::Type;
			} else if (prevToken.type == TokenType::Colon)
				inferredType = TokenType::Type;
		}

		// Check Following Character
		if (inferredType == TokenType::Identifier && nextNonWs < code.length())
		{
			bool isUpper = word.length() > 0 && isAlpha(word[0]) && isupper(word[0]);
			if (code[nextNonWs] == '(')
				inferredType = isUpper ? TokenType::ClassName : TokenType::Function;
			else if (code[nextNonWs] == '.')
				inferredType = isUpper ? TokenType::ClassName : TokenType::Identifier;
			else if (code[nextNonWs] == '<')
				inferredType = isUpper ? TokenType::Type : TokenType::Function;
		}

		// Final Uppercase Check
		if (inferredType == TokenType::Identifier && word.length() > 0 &&
			isAlpha(word[0]) && isupper(word[0]))
			inferredType = TokenType::Type;
		return {inferredType, start, pos - start};
	}

	Token lexDecorator(const std::string &code, size_t &pos)
	{
		size_t start = pos;
		pos++;
		while (pos < code.length() && (isAlphaNumeric(code[pos]) || code[pos] == '$'))
			pos++;
		if (pos < code.length() && code[pos] == '.')
		{
			pos++;
			while (pos < code.length() && (isAlphaNumeric(code[pos]) || code[pos] == '$'))
				pos++;
		}
		return {TokenType::Decorator, start, pos - start};
	}

	Token lexOperatorOrPunctuation(const std::string &code, size_t &pos)
	{
		size_t start = pos;
		for (int len = 3; len >= 1; --len)
		{
			if (pos + len <= code.length())
			{
				std::string sub = code.substr(pos, len);
				if (operators.count(sub))
				{
					pos += len;
					return {operators.at(sub), start, (size_t)len};
				}
			}
		}
		char c = code[pos];
		switch (c)
		{
		case '(':
		case ')':
			pos++;
			return {TokenType::Parenthesis, start, 1};
		case '[':
		case ']':
			pos++;
			return {TokenType::Bracket, start, 1};
		case '{':
		case '}':
			pos++;
			return {TokenType::Brace, start, 1}; // State handled in main loop
		case ';':
			pos++;
			return {TokenType::Semicolon, start, 1};
		case ',':
			pos++;
			return {TokenType::Comma, start, 1};
		case '.':
			pos++;
			return {TokenType::Dot, start, 1}; // Number dot handled earlier
		case ':':
			pos++;
			return {TokenType::Colon, start, 1};
		default:
			pos++;
			return {TokenType::Unknown, start, 1};
		}
	}

	Token lexJsxText(const std::string &code, size_t &pos)
	{
		size_t start = pos;
		while (pos < code.length())
		{
			if (code[pos] == '<' || code[pos] == '{')
				break;
			pos++;
		}
		if (start == pos)
			return {TokenType::Unknown, start, 0}; // Return 0 length if nothing found
		bool onlyWhitespace = true;
		for (size_t i = start; i < pos; ++i)
			if (!isWhitespace(code[i]))
			{
				onlyWhitespace = false;
				break;
			}
		return {onlyWhitespace ? TokenType::Whitespace : TokenType::JsxText,
				start,
				pos - start};
	}

	ImVec4 getColorForTokenType(TokenType type) const
	{
		updateThemeColors();
		switch (type)
		{
		case TokenType::Keyword:
			return cachedColors.keyword;
		case TokenType::Type:
			return cachedColors.type;
		case TokenType::ClassName:
			return cachedColors.className;
		case TokenType::String:
			return cachedColors.string;
		case TokenType::TemplateLiteral:
			return cachedColors.templateLiteral;
		case TokenType::RegexLiteral:
			return cachedColors.regexLiteral;
		case TokenType::Number:
			return cachedColors.number;
		case TokenType::Comment:
		case TokenType::JsxComment:
			return cachedColors.comment;
		case TokenType::Function:
			return cachedColors.function;
		case TokenType::Decorator:
			return cachedColors.decorator;
		case TokenType::Operator:
		case TokenType::OptionalChain:
			return cachedColors.operatorColor;
		case TokenType::JsxTag:
			return cachedColors.jsxTag;
		case TokenType::JsxTagName:
			return cachedColors.className; // Reuse class color for tags
		case TokenType::JsxAttribute:
			return cachedColors.jsxAttribute;
		case TokenType::JsxText:
			return cachedColors.jsxText;
		case TokenType::Parenthesis:
		case TokenType::Bracket:
		case TokenType::Brace:
		case TokenType::Semicolon:
		case TokenType::Comma:
		case TokenType::Dot:
		case TokenType::Colon:
			return cachedColors.text; // Or slightly dimmed?
		case TokenType::Whitespace:
		case TokenType::Identifier:
		case TokenType::Unknown:
		case TokenType::Preprocessor:
		default:
			return cachedColors.text;
		}
	}
}; // End class Lexer

} // namespace TsxLexer