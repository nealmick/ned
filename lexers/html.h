#pragma once
#include "../util/settings.h"
#include "imgui.h"
#include <iostream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

class Settings;
extern Settings gSettings;

namespace HtmlLexer {

enum class TokenType {
	Whitespace,
	Identifier,
	TagName,
	AttributeName,
	AttributeValue,
	String,
	Comment,
	ScriptContent,
	StyleContent,
	AngledBracket,
	Equals,
	Quote,
	Slash,
	DocType,
	EntityRef,
	Unknown,

	JsKeyword,
	JsFunction,
	JsString,
	JsNumber,
	JsComment,
	JsBrace,
	JsOperator,
	JsIdentifier,

	CssSelector,
	CssProperty,
	CssValue,
	CssBrace,
	CssColon,
	CssSemicolon,
	CssComment,
	CssNumber,
	CssUnit,
	CssColor,	  // For #hex values
	CssFunction,  // For things like rgb(), url()
	CssImportant, // For !important
	CssOperator,  // For @media, @import etc

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
		ImVec4 text;
		ImVec4 tagName;
		ImVec4 attributeName;
		ImVec4 attributeValue;
		ImVec4 string;
		ImVec4 comment;
		ImVec4 bracket;
		ImVec4 entityRef;
		// Add these for JavaScript
		ImVec4 keyword;	 // For JS keywords
		ImVec4 number;	 // For JS numbers
		ImVec4 function; // For JS functions
	};

	Lexer()
	{
		tags = {"html",
				"head",
				"body",
				"div",
				"span",
				"p",
				"a",
				"img",
				"script",
				"style",
				"link",
				"meta",
				"title"};

		attributes = {"class", "id", "href", "src", "type", "rel", "style", "onclick"};
	}

	void themeChanged() { colorsNeedUpdate = true; }

	std::vector<Token> tokenize(const std::string &code)
	{
		std::vector<Token> tokens;
		size_t pos = 0;
		bool inScript = false;
		bool inStyle = false;

		while (pos < code.length())
		{
			if (inScript)
			{
				tokens.push_back(lexScriptContent(code, pos));
				inScript = false;
				continue;
			}
			if (inStyle)
			{
				tokens.push_back(lexStyleContent(code, pos));
				inStyle = false;
				continue;
			}

			char c = code[pos];
			if (isWhitespace(c))
			{
				tokens.push_back({TokenType::Whitespace, pos, 1});
				pos++;
			} else if (c == '<')
			{
				if (pos + 3 < code.length() && code.substr(pos, 4) == "<!--")
				{
					tokens.push_back(lexComment(code, pos));
				} else
				{
					auto tag = lexTag(code, pos);
					tokens.push_back(tag);
					if (isScriptTag(code, tag))
						inScript = true;
					if (isStyleTag(code, tag))
						inStyle = true;

					// After tag name, look for attributes
					while (pos < code.length() && code[pos] != '>')
					{
						if (isWhitespace(code[pos]))
						{
							pos++;
						} else if (isAlpha(code[pos]))
						{
							tokens.push_back(lexAttribute(code, pos));
						} else if (code[pos] == '=')
						{
							tokens.push_back({TokenType::Equals, pos, 1});
							pos++;
						} else if (code[pos] == '"' || code[pos] == '\'')
						{
							tokens.push_back(lexString(code, pos));
						} else
						{
							pos++;
						}
					}
					if (pos < code.length() && code[pos] == '>')
					{
						tokens.push_back({TokenType::AngledBracket, pos, 1});
						pos++;
					}
				}
			} else if (c == '&')
			{
				// Handle entity references
				size_t start = pos;
				while (pos < code.length() && code[pos] != ';')
					pos++;
				if (pos < code.length())
					pos++; // Include the semicolon
				tokens.push_back({TokenType::EntityRef, start, pos - start});
			} else
			{
				// Plain text
				size_t start = pos;
				while (pos < code.length() && code[pos] != '<' && code[pos] != '&')
					pos++;
				tokens.push_back({TokenType::Identifier, start, pos - start});
			}
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
				if (token.type == TokenType::ScriptContent)
				{
					// Extract JavaScript content
					std::string jsContent = code.substr(token.start, token.length);

					// Tokenize the JavaScript content
					size_t jsPos = 0;
					while (jsPos < jsContent.length())
					{
						Token jsToken = lexJavaScript(jsContent, jsPos);

						// Apply color based on JavaScript token type
						ImVec4 color = getColorForTokenType(jsToken.type);
						for (size_t i = 0; i < jsToken.length; ++i)
						{
							size_t index = start_pos + token.start + jsToken.start + i;
							if (index < colors.size())
							{
								colors[index] = color;
							}
						}
					}
					continue;
				}

				if (token.type == TokenType::StyleContent)
				{
					// Extract CSS content
					std::string cssContent = code.substr(token.start, token.length);

					// Tokenize CSS content
					size_t cssPos = 0;
					while (cssPos < cssContent.length())
					{
						Token cssToken = lexCss(cssContent, cssPos);

						// Apply color based on CSS token type
						ImVec4 color = getColorForTokenType(cssToken.type);
						for (size_t i = 0; i < cssToken.length; ++i)
						{
							size_t index = start_pos + token.start + cssToken.start + i;
							if (index < colors.size())
							{
								colors[index] = color;
							}
						}
					}
					continue;
				}

				// Normal HTML token handling
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
			std::cerr << "Exception in HTML applyHighlighting: " << e.what() << std::endl;
		}
	}

	void forceColorUpdate() { colorsNeedUpdate = true; }

  private:
	std::unordered_set<std::string> tags;
	std::unordered_set<std::string> attributes;
	mutable ThemeColors cachedColors;
	mutable bool colorsNeedUpdate = true;

	ImVec4 getColorForTokenType(TokenType type) const
	{
		updateThemeColors();

		switch (type)
		{
		case TokenType::TagName:
			return cachedColors.tagName;
		case TokenType::AttributeName:
			return cachedColors.attributeName;
		case TokenType::AttributeValue:
			return cachedColors.attributeValue;
		case TokenType::String:
			return cachedColors.string;
		case TokenType::Comment:
			return cachedColors.comment;
		case TokenType::AngledBracket:
			return cachedColors.bracket;
		case TokenType::EntityRef:
			return cachedColors.entityRef;

		case TokenType::JsKeyword:
			return cachedColors.keyword;
		case TokenType::JsString:
			return cachedColors.string;
		case TokenType::JsNumber:
			return cachedColors.number;
		case TokenType::JsComment:
			return cachedColors.comment;
		case TokenType::JsFunction:
			return cachedColors.function;
		case TokenType::JsOperator:
			return ImVec4(cachedColors.text.x * 0.8f,
						  cachedColors.text.y * 0.8f,
						  cachedColors.text.z * 0.8f,
						  cachedColors.text.w);
		case TokenType::JsBrace:
			return cachedColors.bracket;
		case TokenType::JsIdentifier:
			return cachedColors.text;

		case TokenType::CssProperty:
			return cachedColors.keyword;
		case TokenType::CssValue:
			return cachedColors.text;
		case TokenType::CssNumber:
			return cachedColors.number;
		case TokenType::CssColor:
			return cachedColors.string;
		case TokenType::CssComment:
			return cachedColors.comment;
		case TokenType::CssBrace:
			return cachedColors.bracket;
		case TokenType::CssColon:
		case TokenType::CssSemicolon:
		case TokenType::CssOperator:
			return ImVec4(cachedColors.text.x * 0.8f,
						  cachedColors.text.y * 0.8f,
						  cachedColors.text.z * 0.8f,
						  cachedColors.text.w);
		case TokenType::CssImportant:
			return cachedColors.function;

		default:
			return cachedColors.text;
		}
	}

	void updateThemeColors() const
	{
		if (!colorsNeedUpdate)
			return;

		auto &theme = gSettings.getSettings()["themes"][gSettings.getCurrentTheme()];

		auto loadColor = [&theme](const char *key) -> ImVec4 {
			auto &c = theme[key];
			return ImVec4(c[0], c[1], c[2], c[3]);
		};

		cachedColors.text = loadColor("text");
		cachedColors.tagName = loadColor("keyword"); // Use keyword color for tags
		cachedColors.attributeName =
			loadColor("function"); // Use function color for attributes
		cachedColors.attributeValue = loadColor("string"); // Use string color for values
		cachedColors.string = loadColor("string");
		cachedColors.comment = loadColor("comment");
		cachedColors.bracket = loadColor("keyword");  // Use keyword color for brackets
		cachedColors.entityRef = loadColor("number"); // Use number color for entities

		// Cache JavaScript colors
		cachedColors.keyword = loadColor("keyword");
		cachedColors.number = loadColor("number");
		cachedColors.function = loadColor("function");

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

	bool isAlphaNumeric(char c) const
	{
		return isAlpha(c) || isDigit(c) || c == '_' || c == '-';
	}

	Token lexTag(const std::string &code, size_t &pos)
	{
		size_t start = pos++; // Skip opening
		bool isEndTag = pos < code.length() && code[pos] == '/';
		if (isEndTag)
			pos++;

		// Get tag name
		while (pos < code.length() && code[pos] != '>' && !isWhitespace(code[pos]))
		{
			pos++;
		}

		return {TokenType::TagName, start, pos - start};
	}

	Token lexAttribute(const std::string &code, size_t &pos)
	{
		size_t start = pos;
		while (pos < code.length() && isAlphaNumeric(code[pos]))
		{
			pos++;
		}
		return {TokenType::AttributeName, start, pos - start};
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
		pos += 4; // Skip <!--
		while (pos + 2 < code.length() &&
			   !(code[pos] == '-' && code[pos + 1] == '-' && code[pos + 2] == '>'))
		{
			pos++;
		}
		pos += 3; // Skip -->
		return {TokenType::Comment, start, pos - start};
	}

	bool isScriptTag(const std::string &code, const Token &token)
	{
		if (token.type != TokenType::TagName)
			return false;
		std::string tagName = code.substr(token.start + 1, token.length - 1);
		return tagName == "script";
	}

	bool isStyleTag(const std::string &code, const Token &token)
	{
		if (token.type != TokenType::TagName)
			return false;
		std::string tagName = code.substr(token.start + 1, token.length - 1);
		return tagName == "style";
	}

	Token lexScriptContent(const std::string &code, size_t &pos)
	{
		size_t start = pos;
		size_t contentStart = pos; // Mark where actual JS content begins

		// Find the end of script tag
		while (pos < code.length())
		{
			if (pos + 8 < code.length() && code.substr(pos, 9) == "</script>")
			{
				// Create a ScriptContent token that only includes the JS
				// content not including the closing tag
				return {TokenType::ScriptContent, contentStart, pos - contentStart};
			}
			pos++;
		}

		// If we didn't find the end tag, just return what we found
		return {TokenType::ScriptContent, contentStart, pos - contentStart};
	}
	Token lexStyleContent(const std::string &code, size_t &pos)
	{
		size_t start = pos;
		size_t contentStart = pos; // Mark where actual CSS content begins

		while (pos < code.length())
		{
			if (pos + 7 < code.length() && code.substr(pos, 8) == "</style>")
			{
				// Create a StyleContent token that only includes the CSS
				// content
				return {TokenType::StyleContent, contentStart, pos - contentStart};
			}
			pos++;
		}

		return {TokenType::StyleContent, contentStart, pos - contentStart};
	}

	std::unordered_set<std::string> jsKeywords = {
		"function", "var",		 "let",	  "const",	  "if",		"else",	  "for",
		"while",	"do",		 "break", "continue", "return", "class",  "new",
		"this",		"undefined", "null",  "true",	  "false",	"typeof", "instanceof"};

	Token lexJavaScript(const std::string &code, size_t &pos)
	{
		size_t start = pos; // Add this line to fix the error
		char c = code[pos];

		if (isWhitespace(c))
		{
			return {TokenType::Whitespace, pos++, 1};
		}

		if (c == '/' && pos + 1 < code.length())
		{
			if (code[pos + 1] == '/')
			{
				// Single line comment
				size_t start = pos;
				pos += 2;
				while (pos < code.length() && code[pos] != '\n')
					pos++;
				return {TokenType::JsComment, start, pos - start};
			}
			if (code[pos + 1] == '*')
			{
				// Multi-line comment
				size_t start = pos;
				pos += 2;
				while (pos + 1 < code.length() &&
					   !(code[pos] == '*' && code[pos + 1] == '/'))
					pos++;
				pos += 2;
				return {TokenType::JsComment, start, pos - start};
			}
		}

		if (c == '"' || c == '\'' || c == '`')
		{
			// String literals including template literals
			size_t start = pos++;
			char quote = c;
			while (pos < code.length() && code[pos] != quote)
			{
				if (code[pos] == '\\' && pos + 1 < code.length())
					pos++;
				pos++;
			}
			if (pos < code.length())
				pos++;
			return {TokenType::JsString, start, pos - start};
		}

		if (isDigit(c) || (c == '.' && pos + 1 < code.length() && isDigit(code[pos + 1])))
		{
			// Numbers including decimals
			size_t start = pos++;
			while (pos < code.length() && (isDigit(code[pos]) || code[pos] == '.'))
				pos++;
			return {TokenType::JsNumber, start, pos - start};
		}

		if (isAlpha(c) || c == '_' || c == '$')
		{
			// Keywords and identifiers
			size_t start = pos++;
			while (pos < code.length() &&
				   (isAlphaNumeric(code[pos]) || code[pos] == '_' || code[pos] == '$'))
				pos++;

			std::string word = code.substr(start, pos - start);
			if (jsKeywords.find(word) != jsKeywords.end())
			{
				return {TokenType::JsKeyword, start, pos - start};
			}

			// Look ahead for function calls
			size_t temp = pos;
			while (temp < code.length() && isWhitespace(code[temp]))
				temp++;
			if (temp < code.length() && code[temp] == '(')
			{
				return {TokenType::JsFunction, start, pos - start};
			}

			return {TokenType::JsIdentifier, start, pos - start};
		}

		// Single character tokens
		pos++;
		if (c == '{' || c == '}')
			return {TokenType::JsBrace, start, 1};

		// Operators
		return {TokenType::JsOperator, start, 1};
	}
	std::unordered_set<std::string> cssProperties = {
		// Existing properties
		"cursor",

		// Layout and Positioning
		"display",
		"position",
		"top",
		"left",
		"right",
		"bottom",
		"float",
		"clear",
		"visibility",
		"opacity",
		"z-index",
		"overflow",
		"clip",

		// Box Model
		"margin",
		"margin-top",
		"margin-right",
		"margin-bottom",
		"margin-left",
		"padding",
		"padding-top",
		"padding-right",
		"padding-bottom",
		"padding-left",
		"width",
		"height",
		"min-width",
		"min-height",
		"max-width",
		"max-height",
		"box-sizing",
		"box-shadow",

		// Border
		"border",
		"border-top",
		"border-right",
		"border-bottom",
		"border-left",
		"border-width",
		"border-style",
		"border-color",
		"border-radius",
		"outline",
		"outline-color",
		"outline-style",
		"outline-width",

		// Background
		"background",
		"background-color",
		"background-image",
		"background-repeat",
		"background-position",
		"background-size",
		"background-clip",
		"background-origin",
		"background-attachment",
		"backdrop-filter",

		// Typography
		"color",
		"font",
		"font-family",
		"font-size",
		"font-weight",
		"font-style",
		"text-align",
		"text-decoration",
		"line-height",
		"letter-spacing",
		"text-transform",
		"text-indent",
		"word-spacing",
		"white-space",
		"text-overflow",
		"word-break",
		"writing-mode",

		// Flexbox
		"flex",
		"flex-direction",
		"flex-wrap",
		"flex-flow",
		"justify-content",
		"align-items",
		"align-content",
		"align-self",
		"gap",
		"order",

		// Grid
		"grid",
		"grid-template",
		"grid-template-columns",
		"grid-template-rows",
		"grid-column",
		"grid-row",
		"grid-gap",
		"grid-area",
		"grid-template-areas",

		// Transforms and Animations
		"transform",
		"transform-origin",
		"transition",
		"animation",
		"animation-name",
		"animation-duration",
		"animation-timing-function",
		"animation-delay",
		"animation-iteration-count",
		"animation-direction",
		"animation-fill-mode",
		"animation-play-state",

		// Filters and Effects
		"filter",
		"backdrop-filter",
		"clip-path",
		"mask",
		"mask-image",

		// Miscellaneous
		"content",
		"cursor",
		"user-select",
		"pointer-events",
		"will-change",

		// Vendor Prefixes
		"-webkit-background-clip",
		"-moz-background-clip",
		"-webkit-text-fill-color",
		"-webkit-box-shadow",

		// CSS Custom Properties
		"--*"};

	Token lexCss(const std::string &code, size_t &pos)
	{
		size_t start = pos;
		char c = code[pos];

		if (isWhitespace(c))
		{
			return {TokenType::Whitespace, pos++, 1};
		}

		// CSS Comments (both single-line and multi-line)
		if (c == '/' && pos + 1 < code.length())
		{
			if (code[pos + 1] == '*')
			{
				size_t start = pos;
				pos += 2;
				while (pos + 1 < code.length() &&
					   !(code[pos] == '*' && code[pos + 1] == '/'))
					pos++;
				pos += 2;
				return {TokenType::CssComment, start, pos - start};
			} else if (code[pos + 1] == '/')
			{
				// Single-line comment
				size_t start = pos;
				pos += 2;
				while (pos < code.length() && code[pos] != '\n')
					pos++;
				return {TokenType::CssComment, start, pos - start};
			}
		}

		// Colors and Functions
		if (c == 'r' && pos + 3 < code.length() &&
			(code.substr(pos, 4) == "rgba" || code.substr(pos, 3) == "rgb"))
		{
			size_t start = pos;
			while (pos < code.length() && code[pos] != ')')
				pos++;
			if (pos < code.length())
				pos++; // Include closing parenthesis
			return {TokenType::CssFunction, start, pos - start};
		}

		// HSL/HSLA Functions
		if (c == 'h' && pos + 3 < code.length() &&
			(code.substr(pos, 4) == "hsla" || code.substr(pos, 3) == "hsl"))
		{
			size_t start = pos;
			while (pos < code.length() && code[pos] != ')')
				pos++;
			if (pos < code.length())
				pos++; // Include closing parenthesis
			return {TokenType::CssFunction, start, pos - start};
		}

		// URL Function
		if (c == 'u' && pos + 3 < code.length() && code.substr(pos, 4) == "url(")
		{
			size_t start = pos;
			while (pos < code.length() && code[pos] != ')')
				pos++;
			if (pos < code.length())
				pos++; // Include closing parenthesis
			return {TokenType::CssFunction, start, pos - start};
		}

		// Numbers and Units
		if (isDigit(c) || (c == '.' && pos + 1 < code.length() && isDigit(code[pos + 1])))
		{
			size_t numStart = pos;
			while (pos < code.length() && (isDigit(code[pos]) || code[pos] == '.'))
				pos++;

			// Check for units
			size_t unitStart = pos;
			while (pos < code.length() && isAlpha(code[pos]))
				pos++;

			if (pos > unitStart)
			{
				return {TokenType::CssNumber, numStart, pos - numStart};
			}
			return {TokenType::CssNumber, numStart, pos - numStart};
		}

		// Hex and RGB Colors
		if (c == '#')
		{
			pos++;
			while (pos < code.length() && ((code[pos] >= '0' && code[pos] <= '9') ||
										   (code[pos] >= 'a' && code[pos] <= 'f') ||
										   (code[pos] >= 'A' && code[pos] <= 'F')))
			{
				pos++;
			}
			return {TokenType::CssColor, start, pos - start};
		}

		// Identifiers, Properties, and Values
		if (isAlpha(c) || c == '-' || c == '_')
		{
			// Read the entire identifier
			while (pos < code.length() &&
				   (isAlphaNumeric(code[pos]) || code[pos] == '-' || code[pos] == '_'))
			{
				pos++;
			}

			std::string word = code.substr(start, pos - start);

			// Check for properties
			if (cssProperties.find(word) != cssProperties.end() ||
				(word.substr(0, 2) == "--"))
			{ // Custom properties
				return {TokenType::CssProperty, start, pos - start};
			}

			// Special values
			static const std::unordered_set<std::string> specialValues = {
				"inherit",	"initial",	"unset",		"none",	  "auto",
				"block",	"inline",	"inline-block", "flex",	  "grid",
				"absolute", "relative", "fixed",		"sticky", "bold",
				"normal",	"italic",	"center",		"left",	  "right"};

			if (specialValues.find(word) != specialValues.end())
			{
				return {TokenType::CssValue, start, pos - start};
			}

			// Fallback to generic value
			return {TokenType::CssValue, start, pos - start};
		}

		// Single character tokens
		pos++;
		switch (c)
		{
		case '{':
		case '}':
			return {TokenType::CssBrace, start, 1};
		case ':':
			return {TokenType::CssColon, start, 1};
		case ';':
			return {TokenType::CssSemicolon, start, 1};
		case '@':
			return {TokenType::CssOperator, start, 1};
		case '!':
			return {TokenType::CssImportant, start, 1};
		default:
			return {TokenType::CssValue, start, 1};
		}
	}
};

} // namespace HtmlLexer