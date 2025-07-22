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

namespace PythonLexer {
enum class TokenType {
	Whitespace,
	Identifier,
	Keyword,
	String,
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

	// Add new token types
	Function,	  // For function names
	ClassName,	  // For class names
	Decorator,	  // For @decorators
	SelfParam,	  // For 'self' parameter
	BuiltinType,  // For built-in types like 'int', 'str', etc.
	ScopeOperator // For . in method calls
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
		ImVec4 function; // Add this
	};

	Lexer()
	{
		keywords = {"and",	"as",		"assert", "break",	"class", "continue", "def",
					"del",	"elif",		"else",	  "except", "False", "finally",	 "for",
					"from", "global",	"if",	  "import", "in",	 "is",		 "lambda",
					"None", "nonlocal", "not",	  "or",		"pass",	 "raise",	 "return",
					"True", "try",		"while",  "with",	"yield"};

		// Add built-in types
		builtinTypes = {"int",
						"str",
						"float",
						"bool",
						"list",
						"dict",
						"set",
						"tuple",
						"bytes",
						"object",
						"BaseException",
						"Exception"};

		operators = {{"+", TokenType::Operator},
					 {"-", TokenType::Operator},
					 {"*", TokenType::Operator},
					 {"/", TokenType::Operator},
					 {"//", TokenType::Operator},
					 {"%", TokenType::Operator},
					 {"**", TokenType::Operator},
					 {"=", TokenType::Operator},
					 {"==", TokenType::Operator},
					 {"!=", TokenType::Operator},
					 {">", TokenType::Operator},
					 {"<", TokenType::Operator},
					 {">=", TokenType::Operator},
					 {"<=", TokenType::Operator},
					 {"and", TokenType::Operator},
					 {"or", TokenType::Operator},
					 {"not", TokenType::Operator},
					 {"in", TokenType::Operator},
					 {"is", TokenType::Operator}};
		colorsNeedUpdate = true;
	}

	void themeChanged() { colorsNeedUpdate = true; }

	std::vector<Token> tokenize(const std::string &code)
	{
		std::vector<Token> tokens;
		size_t pos = 0;
		size_t lastPos = 0;
		size_t maxIterations = code.length() * 2;
		size_t iterations = 0;

		try
		{
			while (pos < code.length() && iterations < maxIterations)
			{
				lastPos = pos;
				if (isWhitespace(code[pos]))
				{
					tokens.push_back({TokenType::Whitespace, pos, 1});
					pos++;
				} else if (isAlpha(code[pos]) || code[pos] == '_')
				{
					tokens.push_back(lexIdentifierOrKeyword(code, pos));
				} else if (isDigit(code[pos]))
				{
					tokens.push_back(lexNumber(code, pos));
				} else if (code[pos] == '#')
				{
					tokens.push_back(lexComment(code, pos));
				} else if (code[pos] == '"' || code[pos] == '\'')
				{
					tokens.push_back(lexString(code, pos));
				} else
				{
					tokens.push_back(lexOperator(code, pos));
				}

				if (pos == lastPos)
				{
					// std::cerr << "Tokenizer stuck at position " << pos << ",
					// character: '" << code[pos] << "'" << std::endl;
					pos++;
				}

				iterations++;
			}

			if (iterations >= maxIterations)
			{
				std::cerr << "🔴 Tokenizer exceeded maximum iterations." << std::endl;
			}
		} catch (const std::exception &e)
		{
			std::cerr << "🔴 Exception in tokenize: " << e.what() << std::endl;
		} catch (...)
		{
			std::cerr << "🔴 Unknown exception in tokenize" << std::endl;
		}

		return tokens;
	}

	void
	applyHighlighting(const std::string &code, std::vector<ImVec4> &colors, int start_pos)
	{
		try
		{
			std::vector<Token> tokens = tokenize(code);

			for (const auto &token : tokens)
			{
				ImVec4 color = getColorForTokenType(token.type);
				for (size_t i = 0; i < token.length; ++i)
				{
					size_t index = start_pos + token.start + i;
					if (index < colors.size())
					{
						colors[index] = color;
					}
				}
			}
		} catch (const std::exception &e)
		{
			std::cerr << "🔴 Exception in applyHighlighting: " << e.what() << std::endl;
		}
	}
	void forceColorUpdate() { colorsNeedUpdate = true; }

  private:
	std::unordered_set<std::string> keywords;
	std::unordered_set<std::string> builtinTypes; // Add this
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
		cachedColors.function = loadColor("function"); // Add this

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

	Token lexIdentifierOrKeyword(const std::string &code, size_t &pos)
	{
		size_t start = pos;
		while (pos < code.length() && isAlphaNumeric(code[pos]))
			pos++;
		std::string word = code.substr(start, pos - start);

		// Look ahead for function detection
		size_t next = pos;
		while (next < code.length() && isWhitespace(code[next]))
			next++;
		bool isFunction = next < code.length() && code[next] == '(';

		// Check for decorators
		if (start > 0 && code[start - 1] == '@')
		{
			return {TokenType::Decorator, start, pos - start};
		}

		// Check priority in this order
		if (keywords.find(word) != keywords.end())
		{
			if (word == "class")
			{
				return {TokenType::ClassName, start, pos - start};
			}
			return {TokenType::Keyword, start, pos - start};
		}

		if (builtinTypes.find(word) != builtinTypes.end())
		{
			return {TokenType::BuiltinType, start, pos - start};
		}

		if (word == "self")
		{
			return {TokenType::SelfParam, start, pos - start};
		}

		if (isFunction)
		{
			return {TokenType::Function, start, pos - start};
		}

		return {TokenType::Identifier, start, pos - start};
	}

	Token lexNumber(const std::string &code, size_t &pos)
	{
		size_t start = pos;
		while (pos < code.length() && (isDigit(code[pos]) || code[pos] == '.'))
			pos++;
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
		while (pos < code.length() && code[pos] != '\n')
			pos++;
		return {TokenType::Comment, start, pos - start};
	}

	Token lexOperator(const std::string &code, size_t &pos)
	{
		size_t start = pos;
		if (code[pos] == '(' || code[pos] == ')')
			return {TokenType::Parenthesis, start, 1};
		if (code[pos] == '[' || code[pos] == ']')
			return {TokenType::Bracket, start, 1};
		if (code[pos] == '{' || code[pos] == '}')
			return {TokenType::Brace, start, 1};
		if (code[pos] == ':')
			return {TokenType::Colon, start, 1};
		if (code[pos] == ',')
			return {TokenType::Comma, start, 1};
		if (code[pos] == '.')
			return {TokenType::Dot, start, 1};

		std::string op;
		while (pos < code.length() && !isAlphaNumeric(code[pos]) &&
			   !isWhitespace(code[pos]))
		{
			op += code[pos++];
			if (operators.find(op) != operators.end())
				break;
		}

		if (op.empty())
		{
			op = code[pos++];
		}

		return {operators.find(op) != operators.end() ? TokenType::Operator
													  : TokenType::Unknown,
				start,
				op.length()};
	}

	ImVec4 getColorForTokenType(TokenType type) const
	{
		updateThemeColors();

		switch (type)
		{
		case TokenType::Keyword:
		case TokenType::BuiltinType:
			return cachedColors.keyword;

		case TokenType::String:
			return cachedColors.string;

		case TokenType::Number:
			return cachedColors.number;

		case TokenType::Comment:
			return cachedColors.comment;

		case TokenType::Function:
			return cachedColors.function;

		case TokenType::ClassName:
			return cachedColors.keyword;

		case TokenType::Decorator:
			return cachedColors.function; // Use function color for decorators

		case TokenType::ScopeOperator:
			return cachedColors.function; // Use function color for .

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

} // namespace PythonLexer
