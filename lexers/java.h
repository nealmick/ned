#pragma once

#include <iostream>
#include <stack> // Might be useful later
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "../util/settings.h" // Ensure this path is correct
#include "imgui.h"			  // Ensure this path is correct

// Forward declare Settings and json as needed
class Settings;
extern Settings gSettings;
#include <lib/json.hpp>

namespace JavaLexer {

enum class TokenType {
	Whitespace,
	Identifier,
	Keyword,
	PrimitiveType,
	String,
	CharLiteral,
	Number,
	Comment,
	Operator,
	Parenthesis,
	Bracket,
	Brace,
	Semicolon,
	Comma,
	Dot,
	Unknown,
	MethodName,
	ClassName,
	Annotation
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
	struct ThemeColors
	{
		ImVec4 keyword;
		ImVec4 primitiveType;
		ImVec4 string;
		ImVec4 number;
		ImVec4 comment;
		ImVec4 text;
		ImVec4 methodName;
		ImVec4 className;
		ImVec4 annotation;
		ImVec4 operatorColor;
	};

	Lexer()
	{
		keywords = {"abstract",	  "assert",	   "break",		 "case",		 "catch",
					"class",	  "const",	   "continue",	 "default",		 "do",
					"else",		  "enum",	   "extends",	 "final",		 "finally",
					"for",		  "goto",	   "if",		 "implements",	 "import",
					"instanceof", "interface", "native",	 "new",			 "package",
					"private",	  "protected", "public",	 "return",		 "static",
					"strictfp",	  "super",	   "switch",	 "synchronized", "this",
					"throw",	  "throws",	   "transient",	 "try",			 "volatile",
					"while",	  "exports",   "module",	 "non-sealed",	 "open",
					"opens",	  "permits",   "provides",	 "record",		 "requires",
					"sealed",	  "to",		   "transitive", "uses",		 "var",
					"when",		  "yield"};
		primitiveTypes = {
			"boolean", "byte", "char", "short", "int", "long", "float", "double", "void"};
		literals = {"true", "false", "null"};
		operators = {{"<<=", TokenType::Operator},	{">>=", TokenType::Operator},
					 {">>>=", TokenType::Operator}, {"==", TokenType::Operator},
					 {"!=", TokenType::Operator},	{">=", TokenType::Operator},
					 {"<=", TokenType::Operator},	{"&&", TokenType::Operator},
					 {"||", TokenType::Operator},	{"++", TokenType::Operator},
					 {"--", TokenType::Operator},	{"+=", TokenType::Operator},
					 {"-=", TokenType::Operator},	{"*=", TokenType::Operator},
					 {"/=", TokenType::Operator},	{"%=", TokenType::Operator},
					 {"&=", TokenType::Operator},	{"|=", TokenType::Operator},
					 {"^=", TokenType::Operator},	{"<<", TokenType::Operator},
					 {">>", TokenType::Operator},	{">>>", TokenType::Operator},
					 {"->", TokenType::Operator},	{"::", TokenType::Operator},
					 {"+", TokenType::Operator},	{"-", TokenType::Operator},
					 {"*", TokenType::Operator},	{"/", TokenType::Operator},
					 {"%", TokenType::Operator},	{"=", TokenType::Operator},
					 {">", TokenType::Operator},	{"<", TokenType::Operator},
					 {"!", TokenType::Operator},	{"&", TokenType::Operator},
					 {"|", TokenType::Operator},	{"^", TokenType::Operator},
					 {"~", TokenType::Operator},	{"?", TokenType::Operator},
					 {":", TokenType::Operator}};
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
				char current_char = code[pos];

				if (isWhitespace(current_char))
				{
					tokens.push_back(lexWhitespace(code, pos));
				} else if (current_char == '/' && pos + 1 < code.length())
				{
					if (code[pos + 1] == '/')
						tokens.push_back(lexSingleLineComment(code, pos));
					else if (code[pos + 1] == '*')
						tokens.push_back(lexMultiLineComment(code, pos));
					else
						tokens.push_back(lexOperatorOrPunctuation(code, pos));
				} else if (current_char == '"')
				{
					tokens.push_back(lexString(code, pos));
				} else if (current_char == '\'')
				{
					tokens.push_back(lexCharLiteral(code, pos));
				} else if (isJavaIdentifierStart(current_char))
				{
					tokens.push_back(lexIdentifierKeywordOrType(code, pos, tokens));
				} else if (isDigit(current_char) ||
						   (current_char == '.' && pos + 1 < code.length() &&
							isDigit(code[pos + 1])))
				{
					tokens.push_back(lexNumber(code, pos));
				} else if (current_char == '@')
				{
					tokens.push_back(lexAnnotation(code, pos));
				} else
				{
					tokens.push_back(lexOperatorOrPunctuation(code, pos));
				}

				if (pos == lastPos && pos < code.length())
				{
					tokens.push_back({TokenType::Unknown, pos, 1});
					pos++;
				}
				iterations++;
			}
			if (iterations >= maxIterations)
			{
				std::cerr << "🔴 Java Tokenizer exceeded maximum iterations."
						  << std::endl;
			}
		} catch (const std::exception &e)
		{
			std::cerr << "🔴 Exception in Java tokenize: " << e.what() << std::endl;
		} catch (...)
		{
			std::cerr << "🔴 Unknown exception in Java tokenize" << std::endl;
		}
		return tokens;
	}

	void
	applyHighlighting(const std::string &code, std::vector<ImVec4> &colors, int start_pos)
	{
		try
		{
			if (colors.size() < code.length())
			{
				colors.resize(code.length());
			}
			std::string subCode = code.substr(start_pos);
			std::vector<Token> tokens = tokenize(subCode);

			for (const auto &token : tokens)
			{
				if (token.length == 0)
					continue;
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
						colors[i] = color;
					else
						break;
				}
			}
		} catch (const std::exception &e)
		{
			std::cerr << "🔴 Exception in Java applyHighlighting: " << e.what()
					  << std::endl;
			if (!colorsNeedUpdate)
				std::fill(colors.begin() + start_pos, colors.end(), cachedColors.text);
			else
				std::fill(colors.begin() + start_pos,
						  colors.end(),
						  ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
		} catch (...)
		{
			std::cerr << "🔴 Unknown exception in Java applyHighlighting" << std::endl;
			if (!colorsNeedUpdate)
				std::fill(colors.begin() + start_pos, colors.end(), cachedColors.text);
			else
				std::fill(colors.begin() + start_pos,
						  colors.end(),
						  ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
		}
	}

	void forceColorUpdate() { colorsNeedUpdate = true; }

  private:
	std::unordered_set<std::string> keywords;
	std::unordered_set<std::string> primitiveTypes;
	std::unordered_set<std::string> literals;
	std::unordered_map<std::string, TokenType> operators;
	mutable ThemeColors cachedColors;
	mutable bool colorsNeedUpdate = true;

	int findLastNonWhitespaceTokenIndex(const std::vector<Token> &tokens) const
	{
		for (int i = static_cast<int>(tokens.size()) - 1; i >= 0; --i)
			if (tokens[i].type != TokenType::Whitespace)
				return i;
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
							return ImVec4(c[0].get<float>(),
										  c[1].get<float>(),
										  c[2].get<float>(),
										  c[3].get<float>());
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
			cachedColors.primitiveType =
				loadColor("primitive_type", cachedColors.keyword);
			cachedColors.string = loadColor("string", cachedColors.text);
			cachedColors.number = loadColor("number", cachedColors.text);
			cachedColors.comment = loadColor("comment", ImVec4(0.5f, 0.5f, 0.5f, 1.0f));
			cachedColors.methodName = loadColor("function", cachedColors.text);
			cachedColors.className =
				loadColor("class_name",
						  cachedColors.primitiveType); // <<< CORRECTED DEFAULT
			cachedColors.annotation = loadColor("decorator", cachedColors.keyword);
			cachedColors.operatorColor = loadColor("operator", cachedColors.text);
			colorsNeedUpdate = false;
		} catch (const std::exception &e)
		{
			std::cerr << "🔴 Error updating Java theme colors: " << e.what() << std::endl;
		}
	}

	// --- Basic Character Checks ---
	bool isWhitespace(char c) const
	{
		return c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\f';
	}
	bool isAlpha(char c) const
	{
		return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
	}
	bool isDigit(char c) const { return c >= '0' && c <= '9'; }
	bool isJavaIdentifierStart(char c) const
	{
		return isAlpha(c) || c == '_' || c == '$';
	}
	bool isJavaIdentifierPart(char c) const
	{
		return (isAlpha(c) || isDigit(c)) || c == '_' || c == '$';
	} // <<< CORRECTED
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
		pos += 2;
		while (pos < code.length() && code[pos] != '\n')
			pos++;
		return {TokenType::Comment, start, pos - start};
	}
	Token lexMultiLineComment(const std::string &code, size_t &pos)
	{
		size_t start = pos;
		pos += 2;
		while (pos + 1 < code.length() && !(code[pos] == '*' && code[pos + 1] == '/'))
			pos++;
		if (pos + 1 < code.length())
			pos += 2;
		else
			pos = code.length();
		return {TokenType::Comment, start, pos - start};
	}
	Token lexString(const std::string &code, size_t &pos)
	{
		size_t start = pos;
		pos++;
		while (pos < code.length())
		{
			if (code[pos] == '\\')
			{
				pos++;
				if (pos < code.length())
					pos++;
			} else if (code[pos] == '"')
			{
				pos++;
				break;
			} else if (code[pos] == '\n')
				break;
			else
				pos++;
		}
		return {TokenType::String, start, pos - start};
	}
	Token lexCharLiteral(const std::string &code, size_t &pos)
	{
		size_t start = pos;
		pos++;
		if (pos < code.length())
		{
			if (code[pos] == '\\')
			{
				pos++;
				if (pos < code.length())
					pos++;
			} else if (code[pos] != '\'')
				pos++;
		}
		if (pos < code.length() && code[pos] == '\'')
			pos++;
		return {TokenType::CharLiteral, start, pos - start};
	}

	// <<< CORRECTED lexNumber FUNCTION >>>
	Token lexNumber(const std::string &code, size_t &pos)
	{
		size_t start = pos;
		bool isHex = false;
		bool isBinary = false;

		// Handle 0x (Hex) and 0b (Binary) prefixes
		if (code[pos] == '0' && pos + 1 < code.length())
		{
			if (code[pos + 1] == 'x' || code[pos + 1] == 'X')
			{
				isHex = true;
				pos += 2;
				while (pos < code.length() && (isHexDigit(code[pos]) || code[pos] == '_'))
					pos++;
			} else if (code[pos + 1] == 'b' || code[pos + 1] == 'B')
			{
				isBinary = true;
				pos += 2;
				while (pos < code.length() &&
					   (code[pos] == '0' || code[pos] == '1' || code[pos] == '_'))
					pos++;
			}
		}

		bool potentialFloatChars = false; // Track if float-specific chars appear
		if (!isHex && !isBinary)
		{
			// Consume digits
			while (pos < code.length() && (isDigit(code[pos]) || code[pos] == '_'))
				pos++;
			// Check for floating point
			if (pos < code.length() && code[pos] == '.')
			{
				if (pos + 1 < code.length() && isDigit(code[pos + 1]))
				{
					potentialFloatChars = true;
					pos++;
					while (pos < code.length() && (isDigit(code[pos]) || code[pos] == '_'))
						pos++;
				}
			}
			// Check for exponent
			if (pos < code.length() && (code[pos] == 'e' || code[pos] == 'E'))
			{
				size_t expPos = pos + 1;
				if (expPos < code.length() && (code[expPos] == '+' || code[expPos] == '-'))
					expPos++;
				if (expPos < code.length() && isDigit(code[expPos]))
				{
					potentialFloatChars = true;
					pos = expPos;
					while (pos < code.length() && (isDigit(code[pos]) || code[pos] == '_'))
						pos++;
				}
			}
			// Check for Float/Double suffix
			if (pos < code.length())
			{
				if (code[pos] == 'f' || code[pos] == 'F')
				{
					potentialFloatChars = true;
					pos++;
				} else if (code[pos] == 'd' || code[pos] == 'D')
				{
					potentialFloatChars = true;
					pos++;
				}
			}
		}

		// Check for Long suffix (only if no float chars seen)
		if (!potentialFloatChars && pos < code.length() &&
			(code[pos] == 'l' || code[pos] == 'L'))
		{
			if (isHex || isBinary)
				pos++; // Hex/Binary can have L
			else
			{ // Check if Decimal/Octal was truly integer
				bool trulyInteger = true;
				for (size_t i = start; i < pos; ++i)
					if (code[i] == '.' || code[i] == 'e' || code[i] == 'E')
					{
						trulyInteger = false;
						break;
					}
				if (trulyInteger)
					pos++;
			}
		}
		return {TokenType::Number, start, pos - start};
	}

	Token lexIdentifierKeywordOrType(const std::string &code,
									 size_t &pos,
									 const std::vector<Token> &tokens)
	{
		size_t start = pos;
		if (!isJavaIdentifierStart(code[pos]))
			return {TokenType::Unknown, start, 1};
		pos++;
		while (pos < code.length() && isJavaIdentifierPart(code[pos]))
			pos++;
		std::string word = code.substr(start, pos - start);
		if (word.empty())
			return {TokenType::Unknown, start, 0};
		if (keywords.count(word))
			return {TokenType::Keyword, start, pos - start};
		if (primitiveTypes.count(word))
			return {TokenType::PrimitiveType, start, pos - start};
		if (literals.count(word))
			return {TokenType::Keyword, start, pos - start};
		size_t nextNonWs = pos;
		while (nextNonWs < code.length() && isWhitespace(code[nextNonWs]))
			nextNonWs++;
		TokenType inferredType = TokenType::Identifier;
		int prevTokenIdx = findLastNonWhitespaceTokenIndex(tokens);
		if (prevTokenIdx != -1)
		{
			const auto &t = tokens[prevTokenIdx];
			std::string pW = "";
			if (t.length > 0)
				pW = code.substr(t.start, t.length);
			if (t.type == TokenType::Keyword)
			{
				if (pW == "class" || pW == "interface" || pW == "enum" || pW == "record")
					inferredType = TokenType::ClassName;
				else if (pW == "new")
					inferredType = TokenType::ClassName;
				else if (pW == "extends" || pW == "implements" || pW == "throws")
					inferredType = TokenType::ClassName;
			} else if (t.type == TokenType::Dot)
			{
				if (nextNonWs < code.length() && code[nextNonWs] == '(')
					inferredType = TokenType::MethodName;
				else if (word.length() > 0 && isupper(word[0]))
					inferredType = TokenType::ClassName;
			}
		}
		if (inferredType == TokenType::Identifier && nextNonWs < code.length())
		{
			if (code[nextNonWs] == '(')
				inferredType = TokenType::MethodName;
			else if (word.length() > 0 && isupper(word[0]))
				inferredType = TokenType::ClassName;
		}
		return {inferredType, start, pos - start};
	}
	Token lexAnnotation(const std::string &code, size_t &pos)
	{
		size_t start = pos;
		pos++;
		if (pos < code.length() && isJavaIdentifierStart(code[pos]))
		{
			pos++;
			while (pos < code.length() &&
				   (isJavaIdentifierPart(code[pos]) || code[pos] == '.'))
				pos++;
		}
		return {TokenType::Annotation, start, pos - start};
	}
	Token lexOperatorOrPunctuation(const std::string &code, size_t &pos)
	{
		size_t start = pos;
		for (int len = 3; len >= 1; --len)
			if (pos + len <= code.length())
			{
				std::string sub = code.substr(pos, len);
				if (operators.count(sub))
				{
					pos += len;
					return {operators.at(sub), start, (size_t)len};
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
			return {TokenType::Brace, start, 1};
		case ';':
			pos++;
			return {TokenType::Semicolon, start, 1};
		case ',':
			pos++;
			return {TokenType::Comma, start, 1};
		case '.':
			pos++;
			return {TokenType::Dot, start, 1};
		default:
			pos++;
			return {TokenType::Unknown, start, 1};
		}
	}

	ImVec4 getColorForTokenType(TokenType type) const
	{
		updateThemeColors();
		switch (type)
		{
		case TokenType::Keyword:
			return cachedColors.keyword;
		case TokenType::PrimitiveType:
			return cachedColors.primitiveType;
		case TokenType::ClassName:
			return cachedColors.className;
		case TokenType::String:
		case TokenType::CharLiteral:
			return cachedColors.string;
		case TokenType::Number:
			return cachedColors.number;
		case TokenType::Comment:
			return cachedColors.comment;
		case TokenType::MethodName:
			return cachedColors.methodName;
		case TokenType::Annotation:
			return cachedColors.annotation;
		case TokenType::Operator:
			return cachedColors.operatorColor;
		case TokenType::Parenthesis:
		case TokenType::Bracket:
		case TokenType::Brace:
		case TokenType::Semicolon:
		case TokenType::Comma:
		case TokenType::Dot:
			return cachedColors.text;
		default:
			return cachedColors.text;
		}
	}
}; // End class Lexer

} // namespace JavaLexer