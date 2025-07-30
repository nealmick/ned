#pragma once

#include <cctype> // For tolower
#include <iostream>
#include <stack> // Potentially useful later, not used initially
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "../util/settings.h" // Ensure this path is correct
#include "imgui.h"			  // Ensure this path is correct

// Forward declare Settings and json as needed
class Settings;
extern Settings gSettings;
#include <lib/json.hpp> // Assuming settings.h includes this, otherwise add it here

namespace CSharpLexer {

enum class TokenType {
	Whitespace,
	Identifier,
	Keyword,
	BuiltInType,
	String,
	VerbatimString,
	InterpolatedString,
	CharLiteral,
	Number,
	Comment,
	XmlDocComment,
	Preprocessor,
	Operator,
	Parenthesis,
	Bracket,
	Brace,
	Semicolon,
	Comma,
	Dot,
	Colon,
	Unknown,
	MethodName,
	ClassName,
	AttributeName, // Use this one
	NamespaceName
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
		ImVec4 builtInType;
		ImVec4 string;
		ImVec4 number;
		ImVec4 comment;
		ImVec4 text;
		ImVec4 methodName;
		ImVec4 className;
		ImVec4 attributeName;
		ImVec4 preprocessor;
		ImVec4 operatorColor;
	};

	Lexer()
	{
		keywords = {
			"abstract",	 "as",		  "base",		"break",	"case",		  "catch",
			"checked",	 "class",	  "const",		"continue", "default",	  "delegate",
			"do",		 "else",	  "enum",		"event",	"explicit",	  "extern",
			"finally",	 "fixed",	  "for",		"foreach",	"goto",		  "if",
			"implicit",	 "in",		  "interface",	"internal", "is",		  "lock",
			"namespace", "new",		  "operator",	"out",		"override",	  "params",
			"private",	 "protected", "public",		"readonly", "ref",		  "return",
			"sealed",	 "sizeof",	  "stackalloc", "static",	"struct",	  "switch",
			"this",		 "throw",	  "try",		"typeof",	"unchecked",  "unsafe",
			"using",	 "virtual",	  "volatile",	"while",	"add",		  "alias",
			"ascending", "async",	  "await",		"by",		"descending", "dynamic",
			"equals",	 "from",	  "get",		"global",	"group",	  "into",
			"join",		 "let",		  "nameof",		"on",		"orderby",	  "partial",
			"remove",	 "select",	  "set",		"value",	"var",		  "when",
			"where",	 "yield",	  "unmanaged",	"nint",		"nuint",	  "notnull",
			"and",		 "or",		  "not",		"record",	"init",		  "with",
			"managed"};
		builtInTypes = {"bool",
						"byte",
						"sbyte",
						"char",
						"decimal",
						"double",
						"float",
						"int",
						"uint",
						"nint",
						"nuint",
						"long",
						"ulong",
						"short",
						"ushort",
						"object",
						"string",
						"void",
						"dynamic"};
		literals = {"true", "false", "null"};
		operators = {{">>=", TokenType::Operator}, {"<<=", TokenType::Operator},
					 {"==", TokenType::Operator},  {"!=", TokenType::Operator},
					 {">=", TokenType::Operator},  {"<=", TokenType::Operator},
					 {"&&", TokenType::Operator},  {"||", TokenType::Operator},
					 {"??", TokenType::Operator},  {"?.", TokenType::Operator},
					 {"=>", TokenType::Operator},  {"++", TokenType::Operator},
					 {"--", TokenType::Operator},  {"+=", TokenType::Operator},
					 {"-=", TokenType::Operator},  {"*=", TokenType::Operator},
					 {"/=", TokenType::Operator},  {"%=", TokenType::Operator},
					 {"&=", TokenType::Operator},  {"|=", TokenType::Operator},
					 {"^=", TokenType::Operator},  {"::", TokenType::Operator},
					 {"<<", TokenType::Operator},  {">>", TokenType::Operator},
					 {"+", TokenType::Operator},   {"-", TokenType::Operator},
					 {"*", TokenType::Operator},   {"/", TokenType::Operator},
					 {"%", TokenType::Operator},   {"=", TokenType::Operator},
					 {">", TokenType::Operator},   {"<", TokenType::Operator},
					 {"!", TokenType::Operator},   {"&", TokenType::Operator},
					 {"|", TokenType::Operator},   {"^", TokenType::Operator},
					 {"~", TokenType::Operator},   {"?", TokenType::Operator}};
		colorsNeedUpdate = true;
	}

	void themeChanged() { colorsNeedUpdate = true; }

	std::vector<Token> tokenize(const std::string &code)
	{
		std::vector<Token> tokens;
		size_t pos = 0;
		size_t lineStart = 0;
		size_t lastPos = 0;
		size_t maxIterations = code.length() * 2;
		size_t iterations = 0;

		try
		{
			while (pos < code.length() && iterations < maxIterations)
			{
				lastPos = pos;
				char current_char = code[pos];
				bool atLineStart = (pos == lineStart);

				// Update lineStart after potential newline in previous token
				if (!atLineStart && pos > 0 && code[pos - 1] == '\n')
				{
					lineStart = pos;
					atLineStart = true;
				}

				// Skip whitespace and update lineStart if newline is consumed
				size_t wsStart = pos;
				while (pos < code.length() && isWhitespace(code[pos]))
				{
					if (code[pos] == '\n')
						lineStart = pos + 1;
					pos++;
				}
				if (pos > wsStart)
				{
					tokens.push_back({TokenType::Whitespace, wsStart, pos - wsStart});
					atLineStart = (pos == lineStart); // Re-check if line start
													  // after whitespace
					if (pos >= code.length())
						break;
					current_char = code[pos];
				}

				// Line-start sensitive tokens (#, potentially attributes)
				if (atLineStart && current_char == '#')
				{
					tokens.push_back(lexPreprocessor(code, pos));
				} else if (current_char == '[' && isAttributeContext(code, pos, tokens))
				{
					tokens.push_back(lexAttribute(code, pos));
				}
				// Regular tokens
				else if (current_char == '/' && pos + 1 < code.length())
				{
					if (code[pos + 1] == '/')
					{
						if (pos + 2 < code.length() && code[pos + 2] == '/')
							tokens.push_back(lexXmlDocComment(code, pos));
						else
							tokens.push_back(lexSingleLineComment(code, pos));
					} else if (code[pos + 1] == '*')
					{
						tokens.push_back(lexMultiLineComment(code, pos));
					} else
					{
						tokens.push_back(lexOperatorOrPunctuation(code, pos));
					}
				} else if (current_char == '@' && pos + 1 < code.length() &&
						   code[pos + 1] == '"')
				{
					tokens.push_back(lexVerbatimString(code, pos));
				} else if (current_char == '$' && pos + 1 < code.length() &&
						   (code[pos + 1] == '"' ||
							(code[pos + 1] == '@' && pos + 2 < code.length() &&
							 code[pos + 2] == '"')))
				{
					tokens.push_back(lexInterpolatedString(code, pos));
				} else if (current_char == '@' && pos + 1 < code.length() &&
						   code[pos + 1] == '$' && pos + 2 < code.length() &&
						   code[pos + 2] == '"')
				{
					tokens.push_back(lexInterpolatedString(code, pos));
				} else if (current_char == '"')
				{
					tokens.push_back(lexString(code, pos));
				} else if (current_char == '\'')
				{
					tokens.push_back(lexCharLiteral(code, pos));
				} else if (isCSharpIdentifierStart(current_char) ||
						   (current_char == '@' && pos + 1 < code.length() &&
							isCSharpIdentifierStart(code[pos + 1])))
				{ // Handle regular and @
					// identifiers
					tokens.push_back(lexIdentifierKeywordOrType(code, pos, tokens));
				} else if (isDigit(current_char) ||
						   (current_char == '.' && pos + 1 < code.length() &&
							isDigit(code[pos + 1])))
				{
					tokens.push_back(lexNumber(code, pos));
				} else
				{
					tokens.push_back(lexOperatorOrPunctuation(code, pos));
				}

				// Safety check
				if (pos == lastPos && pos < code.length())
				{
					tokens.push_back({TokenType::Unknown, pos, 1});
					pos++;
				}
				iterations++;
			}
			if (iterations >= maxIterations)
			{
				std::cerr << "🔴 CSharp Tokenizer exceeded maximum iterations."
						  << std::endl;
			}
		} catch (const std::exception &e)
		{
			std::cerr << "🔴 Exception in CSharp tokenize: " << e.what() << std::endl;
		} catch (...)
		{
			std::cerr << "🔴 Unknown exception in CSharp tokenize" << std::endl;
		}
		return tokens;
	}

	void
	applyHighlighting(const std::string &code, std::vector<ImVec4> &colors, int start_pos)
	{
		try
		{
			if (colors.size() < code.length())
				colors.resize(code.length());
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
			std::cerr << "🔴 Exception in CSharp applyHighlighting: " << e.what()
					  << std::endl;
			if (!colorsNeedUpdate)
				std::fill(colors.begin() + start_pos, colors.end(), cachedColors.text);
			else
				std::fill(colors.begin() + start_pos,
						  colors.end(),
						  ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
		} catch (...)
		{
			std::cerr << "🔴 Unknown exception in CSharp applyHighlighting" << std::endl;
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
	std::unordered_set<std::string> builtInTypes;
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

	bool isAttributeContext(const std::string &code,
							size_t pos,
							const std::vector<Token> &tokens) const
	{
		if (pos == 0)
			return false;
		size_t scanPos = pos - 1;
		while (scanPos > 0 && isWhitespace(code[scanPos]))
			scanPos--;
		if (scanPos == 0 && isWhitespace(code[scanPos]))
			return true; // At start after whitespace
		char prevChar = code[scanPos];
		if (prevChar == ',' || prevChar == ')' || prevChar == ']' || prevChar == '{' ||
			prevChar == ';')
			return true; // After common separators/block ends
		int prevTokenIdx = findLastNonWhitespaceTokenIndex(tokens);
		if (prevTokenIdx == -1)
			return true; // Likely start of file or after only whitespace
		const auto &prevToken = tokens[prevTokenIdx];
		std::string prevWord = code.substr(prevToken.start, prevToken.length);
		static const std::unordered_set<std::string> contextKeywords = {"public",
																		"private",
																		"protected",
																		"internal",
																		"static",
																		"abstract",
																		"sealed",
																		"virtual",
																		"override",
																		"new",
																		"async",
																		"unsafe",
																		"class",
																		"struct",
																		"interface",
																		"enum",
																		"delegate",
																		"event",
																		"void"};
		if (prevToken.type == TokenType::Keyword && contextKeywords.count(prevWord))
			return true;
		if (prevToken.type == TokenType::BuiltInType ||
			prevToken.type == TokenType::ClassName)
			return true;
		if (prevToken.type == TokenType::Bracket && prevChar == ']')
			return true; // After another attribute
		return false;
	}

	void updateThemeColors() const
	{
		if (!colorsNeedUpdate)
			return;
		try
		{
			auto &theme = gSettings.getSettings()["themes"][gSettings.getCurrentTheme()];
			auto loadColor = [&theme](const char *key,
									  ImVec4 df =
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
					std::cerr << "Theme Error '" << key << "': " << e.what()
							  << ". Defaulting." << std::endl;
				} catch (const std::exception &e)
				{
					std::cerr << "Theme Error '" << key << "': " << e.what()
							  << ". Defaulting." << std::endl;
				}
				return df;
			};
			cachedColors.text = loadColor("text");
			cachedColors.keyword = loadColor("keyword", cachedColors.text);
			cachedColors.builtInType = loadColor("primitive_type", cachedColors.keyword);
			cachedColors.string = loadColor("string", cachedColors.text);
			cachedColors.number = loadColor("number", cachedColors.text);
			cachedColors.comment = loadColor("comment", ImVec4(0.5f, 0.5f, 0.5f, 1.0f));
			cachedColors.methodName = loadColor("function", cachedColors.text);
			cachedColors.className = loadColor("class_name", cachedColors.builtInType);
			cachedColors.attributeName = loadColor("decorator", cachedColors.keyword);
			cachedColors.preprocessor = loadColor("preprocessor", cachedColors.keyword);
			cachedColors.operatorColor = loadColor("operator", cachedColors.text);
			colorsNeedUpdate = false;
		} catch (const std::exception &e)
		{
			std::cerr << "🔴 Error updating CSharp theme colors: " << e.what()
					  << std::endl;
		}
	}
	bool isWhitespace(char c) const
	{
		return c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\f';
	}
	bool isAlpha(char c) const
	{
		return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
	}
	bool isDigit(char c) const { return c >= '0' && c <= '9'; }
	bool isCSharpIdentifierStart(char c) const { return isAlpha(c) || c == '_'; }
	bool isCSharpIdentifierPart(char c) const
	{
		return isAlpha(c) || isDigit(c) || c == '_';
	}
	bool isHexDigit(char c) const
	{
		return isDigit(c) || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
	}

	Token lexWhitespace(const std::string &code, size_t &pos)
	{
		size_t s = pos;
		while (pos < code.length() && isWhitespace(code[pos]))
			pos++;
		return {TokenType::Whitespace, s, pos - s};
	}
	Token lexSingleLineComment(const std::string &code, size_t &pos)
	{
		size_t s = pos;
		pos += 2;
		while (pos < code.length() && code[pos] != '\n')
			pos++;
		return {TokenType::Comment, s, pos - s};
	}
	Token lexXmlDocComment(const std::string &code, size_t &pos)
	{
		size_t s = pos;
		pos += 3;
		while (pos < code.length() && code[pos] != '\n')
			pos++;
		return {TokenType::XmlDocComment, s, pos - s};
	}
	Token lexMultiLineComment(const std::string &code, size_t &pos)
	{
		size_t s = pos;
		pos += 2;
		while (pos + 1 < code.length() && !(code[pos] == '*' && code[pos + 1] == '/'))
			pos++;
		if (pos + 1 < code.length())
			pos += 2;
		else
			pos = code.length();
		return {TokenType::Comment, s, pos - s};
	}
	Token lexString(const std::string &code, size_t &pos)
	{
		size_t s = pos;
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
		return {TokenType::String, s, pos - s};
	}
	Token lexVerbatimString(const std::string &code, size_t &pos)
	{
		size_t s = pos;
		pos += 2;
		while (pos < code.length())
		{
			if (code[pos] == '"')
			{
				if (pos + 1 < code.length() && code[pos + 1] == '"')
					pos += 2;
				else
				{
					pos++;
					break;
				}
			} else
				pos++;
		}
		return {TokenType::VerbatimString, s, pos - s};
	}
	Token lexInterpolatedString(const std::string &code, size_t &pos)
	{
		size_t s = pos;
		bool isV = false;
		if (code[pos] == '$')
			pos++;
		else if (code[pos] == '@')
		{
			pos++;
			isV = true;
			if (pos < code.length() && code[pos] == '$')
				pos++;
		}
		if (pos >= code.length() || code[pos] != '"')
			return {TokenType::Unknown, s, pos - s};
		pos++;
		while (pos < code.length())
		{
			if (isV)
			{
				if (code[pos] == '"')
				{
					if (pos + 1 < code.length() && code[pos + 1] == '"')
						pos += 2;
					else
					{
						pos++;
						break;
					}
				} else if (code[pos] == '{')
				{
					if (pos + 1 < code.length() && code[pos + 1] == '{')
						pos += 2;
					else
						pos++;
				} else if (code[pos] == '}')
				{
					if (pos + 1 < code.length() && code[pos + 1] == '}')
						pos += 2;
					else
						pos++;
				} else
					pos++;
			} else
			{
				if (code[pos] == '\\')
				{
					pos++;
					if (pos < code.length())
						pos++;
				} else if (code[pos] == '{')
				{
					if (pos + 1 < code.length() && code[pos + 1] == '{')
						pos += 2;
					else
					{
						pos++;
						int bL = 1;
						while (pos < code.length())
						{
							if (code[pos] == '"' || code[pos] == '\'')
							{
								size_t sS = pos;
								lexString(code, pos);
								pos = sS + (pos - sS);
								if (pos >= code.length())
									break;
							} else if (code[pos] == '{')
								bL++;
							else if (code[pos] == '}')
							{
								bL--;
								if (bL == 0)
								{
									pos++;
									break;
								}
							} else if (code[pos] == '\\')
								pos++;
							pos++;
						}
						if (bL > 0 || pos > code.length())
							break;
						continue;
					}
				} else if (code[pos] == '}')
				{
					if (pos + 1 < code.length() && code[pos + 1] == '}')
						pos += 2;
					else
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
		}
		return {TokenType::InterpolatedString, s, pos - s};
	}
	Token lexCharLiteral(const std::string &code, size_t &pos)
	{
		size_t s = pos;
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
		return {TokenType::CharLiteral, s, pos - s};
	}
	Token lexNumber(const std::string &code, size_t &pos)
	{
		size_t s = pos;
		bool isF = false;
		if (code[pos] == '0' && pos + 1 < code.length() &&
			(code[pos + 1] == 'x' || code[pos + 1] == 'X'))
		{
			pos += 2;
			while (pos < code.length() && (isHexDigit(code[pos]) || code[pos] == '_'))
				pos++;
		} else if (code[pos] == '0' && pos + 1 < code.length() &&
				   (code[pos + 1] == 'b' || code[pos + 1] == 'B'))
		{
			pos += 2;
			while (pos < code.length() &&
				   (code[pos] == '0' || code[pos] == '1' || code[pos] == '_'))
				pos++;
		} else
		{
			while (pos < code.length() && (isDigit(code[pos]) || code[pos] == '_'))
				pos++;
			if (pos < code.length() && code[pos] == '.')
			{
				if (pos + 1 < code.length() && isDigit(code[pos + 1]))
				{
					isF = true;
					pos++;
					while (pos < code.length() && (isDigit(code[pos]) || code[pos] == '_'))
						pos++;
				}
			}
			if (pos < code.length() && (code[pos] == 'e' || code[pos] == 'E'))
			{
				size_t eP = pos + 1;
				if (eP < code.length() && (code[eP] == '+' || code[eP] == '-'))
					eP++;
				if (eP < code.length() && isDigit(code[eP]))
				{
					isF = true;
					pos = eP;
					while (pos < code.length() && (isDigit(code[pos]) || code[pos] == '_'))
						pos++;
				}
			}
		}
		if (pos < code.length())
		{
			char sf = tolower(code[pos]);
			if (sf == 'f' || sf == 'm' || sf == 'd')
				pos++;
			else if (sf == 'u')
			{
				pos++;
				if (pos < code.length() && tolower(code[pos]) == 'l')
					pos++;
			} else if (sf == 'l')
			{
				pos++;
				if (pos < code.length() && tolower(code[pos]) == 'u')
					pos++;
			}
		}
		return {TokenType::Number, s, pos - s};
	}
	Token lexPreprocessor(const std::string &code, size_t &pos)
	{
		size_t s = pos;
		pos++;
		while (pos < code.length() && isAlpha(code[pos]))
			pos++;
		while (pos < code.length() && code[pos] != '\n')
		{
			if (code[pos] == '/' && pos + 1 < code.length() && code[pos + 1] == '/')
				break;
			pos++;
		}
		return {TokenType::Preprocessor, s, pos - s};
	}
	Token lexAttribute(const std::string &code, size_t &pos)
	{
		size_t s = pos;
		pos++;
		int bL = 1;
		while (pos < code.length() && bL > 0)
		{
			if (code[pos] == '"')
				lexString(code, pos);
			else if (code[pos] == '[')
				bL++;
			else if (code[pos] == ']')
				bL--;
			if (bL > 0)
				pos++;
		}
		if (pos < code.length() && bL == 0)
			pos++;
		return {TokenType::AttributeName, s, pos - s};
	} // <<< CORRECTED RETURN TYPE
	Token lexIdentifierKeywordOrType(const std::string &code,
									 size_t &pos,
									 const std::vector<Token> &tokens)
	{
		size_t s = pos;
		bool isV = false;
		if (code[pos] == '@')
		{
			isV = true;
			pos++;
		}
		if (!isCSharpIdentifierStart(code[pos]))
			return {TokenType::Unknown, s, pos - s + (isV ? 1 : 0)};
		pos++;
		while (pos < code.length() && isCSharpIdentifierPart(code[pos]))
			pos++;
		std::string w = code.substr(s + (isV ? 1 : 0), pos - s - (isV ? 1 : 0));
		if (w.empty() && !isV)
			return {TokenType::Unknown, s, 1};
		if (!isV && keywords.count(w))
			return {TokenType::Keyword, s, pos - s};
		if (!isV && builtInTypes.count(w))
			return {TokenType::BuiltInType, s, pos - s};
		if (!isV && literals.count(w))
			return {TokenType::Keyword, s, pos - s};
		size_t nW = pos;
		while (nW < code.length() && isWhitespace(code[nW]))
			nW++;
		TokenType infT = TokenType::Identifier;
		int pTI = findLastNonWhitespaceTokenIndex(tokens);
		if (pTI != -1)
		{
			const auto &t = tokens[pTI];
			std::string pW = "";
			if (t.length > 0)
				pW = code.substr(t.start, t.length);
			if (t.type == TokenType::Keyword)
			{
				if (pW == "class" || pW == "interface" || pW == "struct" ||
					pW == "enum" || pW == "delegate" || pW == "record" ||
					pW == "namespace")
					infT = TokenType::ClassName;
				else if (pW == "new")
					infT = TokenType::ClassName;
				else if (pW == "using" && nW < code.length() && code[nW] != ';' &&
						 code[nW] != '=')
					infT = TokenType::NamespaceName;
				else if (pW == "event")
					infT = TokenType::ClassName;
				else if (pW == "is" || pW == "as")
					infT = TokenType::ClassName;
			} else if (t.type == TokenType::Dot ||
					   (t.type == TokenType::Operator && pW == "::"))
			{
				if (nW < code.length() && code[nW] == '(')
					infT = TokenType::MethodName;
				else if (w.length() > 0 && isupper(w[0]))
					infT = TokenType::ClassName;
				else
					infT = TokenType::Identifier;
			} else if (t.type == TokenType::Bracket && code[t.start] == '[')
				infT = TokenType::AttributeName;
		}
		if (infT == TokenType::Identifier && nW < code.length())
		{
			if (code[nW] == '(')
				infT = TokenType::MethodName;
			else if (code[nW] == '<')
				infT = TokenType::MethodName;
			else if (code[nW] == '{' || code[nW] == ';' || code[nW] == '=')
			{
				if (w.length() > 0 && isupper(w[0]))
					infT = TokenType::ClassName;
			}
		}
		if (infT == TokenType::Identifier && w.length() > 0 && isupper(w[0]))
			infT = TokenType::ClassName;
		if (infT == TokenType::NamespaceName && isupper(w[0]))
			infT = TokenType::ClassName;
		return {infT, s, pos - s};
	}
	Token lexOperatorOrPunctuation(const std::string &code, size_t &pos)
	{
		size_t s = pos;
		for (int l = 3; l >= 1; --l)
			if (pos + l <= code.length())
			{
				std::string sb = code.substr(pos, l);
				if (operators.count(sb))
				{
					pos += l;
					return {operators.at(sb), s, (size_t)l};
				}
			}
		char c = code[pos];
		switch (c)
		{
		case '(':
		case ')':
			pos++;
			return {TokenType::Parenthesis, s, 1};
		case '[':
		case ']':
			pos++;
			return {TokenType::Bracket, s, 1};
		case '{':
		case '}':
			pos++;
			return {TokenType::Brace, s, 1};
		case ';':
			pos++;
			return {TokenType::Semicolon, s, 1};
		case ',':
			pos++;
			return {TokenType::Comma, s, 1};
		case '.':
			pos++;
			return {TokenType::Dot, s, 1};
		case ':':
			pos++;
			return {TokenType::Colon, s, 1};
		default:
			pos++;
			return {TokenType::Unknown, s, 1};
		}
	}

	ImVec4 getColorForTokenType(TokenType type) const
	{
		updateThemeColors();
		switch (type)
		{
		case TokenType::Keyword:
			return cachedColors.keyword;
		case TokenType::BuiltInType:
			return cachedColors.builtInType;
		case TokenType::ClassName:
		case TokenType::NamespaceName:
			return cachedColors.className;
		case TokenType::String:
		case TokenType::VerbatimString:
		case TokenType::InterpolatedString:
		case TokenType::CharLiteral:
			return cachedColors.string;
		case TokenType::Number:
			return cachedColors.number;
		case TokenType::Comment:
		case TokenType::XmlDocComment:
			return cachedColors.comment;
		case TokenType::MethodName:
			return cachedColors.methodName;
		case TokenType::AttributeName:
			return cachedColors.attributeName;
		case TokenType::Preprocessor:
			return cachedColors.preprocessor;
		case TokenType::Operator:
			return cachedColors.operatorColor;
		case TokenType::Parenthesis:
		case TokenType::Bracket:
		case TokenType::Brace:
		case TokenType::Semicolon:
		case TokenType::Comma:
		case TokenType::Dot:
		case TokenType::Colon:
			return cachedColors.text;
		default:
			return cachedColors.text;
		}
	}
};

} // namespace CSharpLexer