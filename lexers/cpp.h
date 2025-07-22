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
	Unknown,
	Function,		 // Function names
	ClassName,		 // Class names
	MemberVar,		 // Class member variables
	MemberFunc,		 // Class member functions
	MacroDefinition, // #define macros
	TypeName,		 // Built-in and user-defined types
	NamespaceKw,	 // namespace keyword specifically
	TemplateParam,	 // Template parameters
	ScopeOperator,

};

struct Context
{
	bool inClassDef = false;
	bool inFuncDef = false;
	bool inTemplate = false;
	std::vector<std::string> knownClasses;
	std::vector<std::string> knownFunctions;
	std::unordered_set<std::string> knownTypes = {"int",
												  "char",
												  "bool",
												  "float",
												  "double",
												  "void",
												  "size_t",
												  "std::string",
												  "vector",
												  "map",
												  "set",
												  "string",
												  "array",
												  "unique_ptr",
												  "shared_ptr"};
};

struct Token
{
	TokenType type;
	size_t start;
	size_t length;
};

class Lexer
{
  public:
	// Theme colors cache
	struct ThemeColors
	{
		ImVec4 keyword;
		ImVec4 string;
		ImVec4 number;
		ImVec4 comment;
		ImVec4 text;

		ImVec4 function;
		ImVec4 className;
		ImVec4 memberVar;
		ImVec4 memberFunc;
		ImVec4 type;
		ImVec4 macro;
	};

	Lexer()
	{
		keywords = {"auto",			"break",	 "case",	 "char",
					"const",		"continue",	 "default",	 "do",
					"double",		"else",		 "enum",	 "extern",
					"float",		"for",		 "goto",	 "if",
					"int",			"long",		 "register", "return",
					"short",		"signed",	 "sizeof",	 "static",
					"struct",		"switch",	 "typedef",	 "union",
					"unsigned",		"void",		 "volatile", "while",
					"class",		"namespace", "try",		 "catch",
					"throw",		"new",		 "delete",	 "public",
					"private",		"protected", "virtual",	 "friend",
					"inline",		"template",	 "typename", "using",
					"bool",			"true",		 "false",	 "nullptr",
					"and",			"or",		 "not",		 "xor",
					"and_eq",		"or_eq",	 "not_eq",	 "xor_eq",
					"bitand",		"bitor",	 "compl",	 "constexpr",
					"decltype",		"mutable",	 "noexcept", "static_assert",
					"thread_local", "alignas",	 "alignof",	 "char16_t",
					"char32_t",		"export",	 "explicit", "final",
					"override",		"operator",	 "this"};

		operators = {{"+", TokenType::Operator},  {"-", TokenType::Operator},
					 {"*", TokenType::Operator},  {"/", TokenType::Operator},
					 {"%", TokenType::Operator},  {"=", TokenType::Operator},
					 {"==", TokenType::Operator}, {"!=", TokenType::Operator},
					 {">", TokenType::Operator},  {"<", TokenType::Operator},
					 {">=", TokenType::Operator}, {"<=", TokenType::Operator},
					 {"&&", TokenType::Operator}, {"||", TokenType::Operator},
					 {"!", TokenType::Operator},  {"&", TokenType::Operator},
					 {"|", TokenType::Operator},  {"^", TokenType::Operator},
					 {"~", TokenType::Operator},  {"<<", TokenType::Operator},
					 {">>", TokenType::Operator}, {"++", TokenType::Operator},
					 {"--", TokenType::Operator}, {"->", TokenType::Operator},
					 {".*", TokenType::Operator}, {"->*", TokenType::Operator},
					 {"::", TokenType::Operator}};
	}
	void themeChanged() { colorsNeedUpdate = true; }
	std::vector<Token> tokenize(const std::string &code)
	{
		std::cout << "Inside C++ tokenizer.." << std::endl;
		std::vector<Token> tokens;
		size_t pos = 0;
		size_t lastPos = 0;
		size_t maxIterations = code.length() * 2;
		size_t iterations = 0;

		try
		{
			std::cout << "Starting C++ tokenization loop" << std::endl;
			while (pos < code.length() && iterations < maxIterations)
			{
				lastPos = pos;
				if (isWhitespace(code[pos]))
				{
					tokens.push_back({TokenType::Whitespace, pos, 1});
					pos++;
				} else if (code[pos] == '#')
				{
					tokens.push_back(lexPreprocessor(code, pos));
				} else if (isAlpha(code[pos]) || code[pos] == '_')
				{
					tokens.push_back(lexIdentifierOrKeyword(code, pos));
				} else if (isDigit(code[pos]))
				{
					tokens.push_back(lexNumber(code, pos));
				} else if (code[pos] == '/' && pos + 1 < code.length() &&
						   (code[pos + 1] == '/' || code[pos + 1] == '*'))
				{
					tokens.push_back(lexComment(code, pos));
				} else if (code[pos] == '"' || code[pos] == '\'')
				{
					tokens.push_back(lexString(code, pos));
				} else
				{
					tokens.push_back(lexOperatorOrPunctuation(code, pos));
				}

				if (pos == lastPos)
				{
					// std::cerr << "C++ Tokenizer stuck at position " << pos <<
					// ", character: '" << code[pos] << "'" << std::endl;
					pos++;
				}

				iterations++;
			}

			if (iterations >= maxIterations)
			{
				std::cerr << "C++ Tokenizer exceeded maximum iterations. "
							 "Possible infinite loop."
						  << std::endl;
			}

			std::cout << "Finished C++ tokenization loop" << std::endl;
		} catch (const std::exception &e)
		{
			std::cerr << "🔴 Exception in C++ tokenize: " << e.what() << std::endl;
		} catch (...)
		{
			std::cerr << "🔴 Unknown exception in C++ tokenize" << std::endl;
		}

		std::cout << "Exiting C++ tokenizer, tokens size: " << tokens.size() << std::endl;
		return tokens;
	}
	void
	applyHighlighting(const std::string &code, std::vector<ImVec4> &colors, int start_pos)
	{
		std::cout << "Entering C++ applyHighlighting, code length: " << code.length()
				  << ", colors size: " << colors.size() << ", start_pos: " << start_pos
				  << std::endl;
		try
		{
			std::vector<Token> tokens = tokenize(code.substr(start_pos));
			std::cout << "After tokenize, tokens size: " << tokens.size() << std::endl;

			int colorChanges = 0;
			for (const auto &token : tokens)
			{
				ImVec4 color = getColorForTokenType(token.type);
				for (size_t i = 0; i < token.length; ++i)
				{
					size_t index = start_pos + token.start + i;
					if (index < colors.size())
					{
						colors[index] = color;
						colorChanges++;
					}
				}
			}
			std::cout << "Exiting C++ applyHighlighting, changed " << colorChanges
					  << " color values" << std::endl;
		} catch (const std::exception &e)
		{
			std::cerr << "🔴 Exception in C++ applyHighlighting: " << e.what()
					  << std::endl;
			std::fill(colors.begin() + start_pos,
					  colors.end(),
					  ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
		} catch (...)
		{
			std::cerr << "🔴 Unknown exception in C++ applyHighlighting" << std::endl;
			std::fill(colors.begin() + start_pos,
					  colors.end(),
					  ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
		}
	}
	void forceColorUpdate() { colorsNeedUpdate = true; }

  private:
	std::unordered_set<std::string> keywords;
	std::unordered_map<std::string, TokenType> operators;
	mutable ThemeColors cachedColors;
	mutable bool colorsNeedUpdate = true;
	void updateThemeColors() const
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
		cachedColors.function = loadColor("function"); // Cache the function color

		colorsNeedUpdate = false;
	}

	bool isWhitespace(char c) const
	{
		return c == ' ' || c == '\t' || c == '\n' || c == '\r';
	}
	bool isAlpha(char c) const
	{
		return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
	}
	bool isDigit(char c) const { return c >= '0' && c <= '9'; }
	bool isAlphaNumeric(char c) const { return isAlpha(c) || isDigit(c) || c == '_'; }

	size_t skipWhitespace(const std::string &code, size_t pos) const
	{
		while (pos < code.length() && isWhitespace(code[pos]))
		{
			pos++;
		}
		return pos;
	}
	Token lexIdentifierOrKeyword(const std::string &code, size_t &pos)
	{
		size_t start = pos;
		while (pos < code.length() && isAlphaNumeric(code[pos]))
			pos++;
		std::string word = code.substr(start, pos - start);

		// Look ahead for function detection
		size_t next = skipWhitespace(code, pos);
		bool isFunction = next < code.length() && code[next] == '(';

		// Basic types that should be highlighted
		static std::unordered_set<std::string> basicTypes = {"void",
															 "bool",
															 "char",
															 "int",
															 "float",
															 "double",
															 "long",
															 "int8_t",
															 "int16_t",
															 "int32_t",
															 "int64_t",
															 "uint8_t",
															 "uint16_t",
															 "uint32_t",
															 "uint64_t",
															 "size_t",
															 "wchar_t"};

		if (keywords.find(word) != keywords.end())
		{
			return {TokenType::Keyword, start, pos - start};
		}

		if (basicTypes.find(word) != basicTypes.end())
		{
			return {TokenType::Keyword, start, pos - start};
		}

		if (isFunction && word.find("::") == std::string::npos)
		{ // Only highlight non-scoped functions
			return {TokenType::Function, start, pos - start};
		}

		return {TokenType::Identifier, start, pos - start};
	}

	Token lexNumber(const std::string &code, size_t &pos)
	{
		size_t start = pos;
		while (pos < code.length() &&
			   (isDigit(code[pos]) || code[pos] == '.' || code[pos] == 'e' ||
				code[pos] == 'E' || code[pos] == '+' || code[pos] == '-' ||
				code[pos] == 'f' || code[pos] == 'F' || code[pos] == 'l' ||
				code[pos] == 'L' || code[pos] == 'u' || code[pos] == 'U'))
		{
			pos++;
		}
		return {TokenType::Number, start, pos - start};
	}

	Token lexString(const std::string &code, size_t &pos)
	{
		size_t start = pos;
		char quote = code[pos++];
		while (pos < code.length() && code[pos] != quote)
		{
			if (code[pos] == '\\' && pos + 1 < code.length())
				pos++;
			pos++;
		}
		if (pos < code.length())
			pos++;
		return {TokenType::String, start, pos - start};
	}

	Token lexComment(const std::string &code, size_t &pos)
	{
		size_t start = pos;
		if (code[pos + 1] == '/')
		{
			pos += 2;
			while (pos < code.length() && code[pos] != '\n')
				pos++;
		} else
		{
			pos += 2;
			while (pos + 1 < code.length() && !(code[pos] == '*' && code[pos + 1] == '/'))
				pos++;
			if (pos + 1 < code.length())
				pos += 2;
		}
		return {TokenType::Comment, start, pos - start};
	}

	Token lexPreprocessor(const std::string &code, size_t &pos)
	{
		size_t start = pos;
		while (pos < code.length() && code[pos] != '\n')
		{
			if (code[pos] == '\\' && pos + 1 < code.length() && code[pos + 1] == '\n')
			{
				pos += 2;
			} else
			{
				pos++;
			}
		}
		return {TokenType::Preprocessor, start, pos - start};
	}
	Token lexOperatorOrPunctuation(const std::string &code, size_t &pos)
	{
		size_t start = pos;
		char c = code[pos];

		// Special handling for :: operator
		if (pos + 1 < code.length() && code[pos] == ':' && code[pos + 1] == ':')
		{
			pos += 2;
			return {TokenType::ScopeOperator, start, 2}; // New token type for ::
		}

		// Handle single-character tokens
		if (c == '(' || c == ')')
			return {TokenType::Parenthesis, start, 1};
		if (c == '[' || c == ']')
			return {TokenType::Bracket, start, 1};
		if (c == '{' || c == '}')
			return {TokenType::Brace, start, 1};
		if (c == ';')
			return {TokenType::Semicolon, start, 1};
		if (c == ',')
			return {TokenType::Comma, start, 1};
		if (c == '.')
			return {TokenType::Dot, start, 1};

		// Handle multi-character operators
		std::string op;
		while (pos < code.length() && !isAlphaNumeric(code[pos]) &&
			   !isWhitespace(code[pos]))
		{
			op += code[pos];
			if (operators.find(op) != operators.end())
			{
				pos++;
				return {TokenType::Operator, start, op.length()};
			}
			pos++;
		}

		// If we didn't find a known operator, treat it as a single-character
		// unknown token
		if (op.empty())
		{
			pos++;
			return {TokenType::Unknown, start, 1};
		}

		// We found some unknown multi-character operator
		return {TokenType::Unknown, start, op.length()};
	}

	ImVec4 getColorForTokenType(TokenType type) const
	{
		updateThemeColors();

		switch (type)
		{
		case TokenType::Keyword:
			return cachedColors.keyword;

		case TokenType::String:
			return cachedColors.string;

		case TokenType::Number:
			return cachedColors.number;

		case TokenType::Comment:
			return cachedColors.comment;

		case TokenType::Function:
			return cachedColors.function;

		case TokenType::ScopeOperator: // Highlight :: in function color
			return cachedColors.function;

		case TokenType::Operator:
			return ImVec4(cachedColors.text.x * 0.8f,
						  cachedColors.text.y * 0.8f,
						  cachedColors.text.z * 0.8f,
						  cachedColors.text.w);

		default:
			return cachedColors.text;
		}
	}
};

} // namespace CppLexer