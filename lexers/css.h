#pragma once

#include <algorithm> // Required for std::transform
#include <cctype>	 // For isalpha, isdigit, isxdigit, isalnum, tolower
#include <iostream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "../util/settings.h" // Ensure this path is correct
#include "imgui.h"			  // Ensure this path is correct

// Forward declare Settings and json as needed
class Settings;
extern Settings gSettings;
#include <lib/json.hpp> // Assuming settings.h includes this

namespace CssLexer {

enum class TokenType {
	Whitespace,
	Comment,
	AtRule,
	SelectorTag,
	SelectorId,
	SelectorClass,
	SelectorAttribute,
	SelectorPseudo,
	PropertyName,
	PropertyValueKeyword,
	PropertyValueNumber,
	PropertyValueString,
	PropertyValueColor,
	PropertyValueFunction,
	Punctuation,
	Unknown
};

struct Token
{
	TokenType type;
	size_t start;
	size_t length;
};

enum class LexerState { TopLevel, InDeclarationBlock };

class Lexer
{
  public:
	// Theme colors cache - Simplified like Python's
	struct ThemeColors
	{
		ImVec4 keyword; // Used for @rules, pseudo-classes, maybe important
						// keywords
		ImVec4 string; // Used for strings, maybe attribute selectors
		ImVec4 number; // Used for numbers, maybe colors
		ImVec4 comment;
		ImVec4 text;	  // Default, punctuation, unknown
		ImVec4 function;  // Used for function calls, maybe IDs
		ImVec4 type;	  // Used for tag selectors, property names
		ImVec4 className; // Used for .class selectors
						  // We can map CSS types to these common categories
	};

	Lexer()
	{
		// Properties (lowercase)
		properties = {"align-content",
					  "align-items",
					  "align-self",
					  "all",
					  "animation",
					  "animation-delay",
					  "animation-direction",
					  "animation-duration",
					  "animation-fill-mode",
					  "animation-iteration-count",
					  "animation-name",
					  "animation-play-state",
					  "animation-timing-function",
					  "backdrop-filter",
					  "backface-visibility",
					  "background",
					  "background-attachment",
					  "background-blend-mode",
					  "background-clip",
					  "background-color",
					  "background-image",
					  "background-origin",
					  "background-position",
					  "background-repeat",
					  "background-size",
					  "border",
					  "border-bottom",
					  "border-bottom-color",
					  "border-bottom-left-radius",
					  "border-bottom-right-radius",
					  "border-bottom-style",
					  "border-bottom-width",
					  "border-collapse",
					  "border-color",
					  "border-image",
					  "border-image-outset",
					  "border-image-repeat",
					  "border-image-slice",
					  "border-image-source",
					  "border-image-width",
					  "border-left",
					  "border-left-color",
					  "border-left-style",
					  "border-left-width",
					  "border-radius",
					  "border-right",
					  "border-right-color",
					  "border-right-style",
					  "border-right-width",
					  "border-spacing",
					  "border-style",
					  "border-top",
					  "border-top-color",
					  "border-top-left-radius",
					  "border-top-right-radius",
					  "border-top-style",
					  "border-top-width",
					  "border-width",
					  "bottom",
					  "box-decoration-break",
					  "box-shadow",
					  "box-sizing",
					  "break-after",
					  "break-before",
					  "break-inside",
					  "caption-side",
					  "caret-color",
					  "clear",
					  "clip",
					  "clip-path",
					  "color",
					  "column-count",
					  "column-fill",
					  "column-gap",
					  "column-rule",
					  "column-rule-color",
					  "column-rule-style",
					  "column-rule-width",
					  "column-span",
					  "column-width",
					  "columns",
					  "content",
					  "counter-increment",
					  "counter-reset",
					  "cursor",
					  "direction",
					  "display",
					  "empty-cells",
					  "filter",
					  "flex",
					  "flex-basis",
					  "flex-direction",
					  "flex-flow",
					  "flex-grow",
					  "flex-shrink",
					  "flex-wrap",
					  "float",
					  "font",
					  "font-family",
					  "font-feature-settings",
					  "font-kerning",
					  "font-language-override",
					  "font-size",
					  "font-size-adjust",
					  "font-stretch",
					  "font-style",
					  "font-synthesis",
					  "font-variant",
					  "font-variant-alternates",
					  "font-variant-caps",
					  "font-variant-east-asian",
					  "font-variant-ligatures",
					  "font-variant-numeric",
					  "font-variant-position",
					  "font-weight",
					  "gap",
					  "grid",
					  "grid-area",
					  "grid-auto-columns",
					  "grid-auto-flow",
					  "grid-auto-rows",
					  "grid-column",
					  "grid-column-end",
					  "grid-column-gap",
					  "grid-column-start",
					  "grid-gap",
					  "grid-row",
					  "grid-row-end",
					  "grid-row-gap",
					  "grid-row-start",
					  "grid-template",
					  "grid-template-areas",
					  "grid-template-columns",
					  "grid-template-rows",
					  "hanging-punctuation",
					  "height",
					  "hyphens",
					  "image-rendering",
					  "isolation",
					  "justify-content",
					  "justify-items",
					  "justify-self",
					  "left",
					  "letter-spacing",
					  "line-break",
					  "line-height",
					  "list-style",
					  "list-style-image",
					  "list-style-position",
					  "list-style-type",
					  "margin",
					  "margin-bottom",
					  "margin-left",
					  "margin-right",
					  "margin-top",
					  "mask",
					  "mask-clip",
					  "mask-composite",
					  "mask-image",
					  "mask-mode",
					  "mask-origin",
					  "mask-position",
					  "mask-repeat",
					  "mask-size",
					  "mask-type",
					  "max-height",
					  "max-width",
					  "min-height",
					  "min-width",
					  "mix-blend-mode",
					  "object-fit",
					  "object-position",
					  "opacity",
					  "order",
					  "orphans",
					  "outline",
					  "outline-color",
					  "outline-offset",
					  "outline-style",
					  "outline-width",
					  "overflow",
					  "overflow-wrap",
					  "overflow-x",
					  "overflow-y",
					  "padding",
					  "padding-bottom",
					  "padding-left",
					  "padding-right",
					  "padding-top",
					  "page-break-after",
					  "page-break-before",
					  "page-break-inside",
					  "perspective",
					  "perspective-origin",
					  "pointer-events",
					  "position",
					  "quotes",
					  "resize",
					  "right",
					  "row-gap",
					  "scroll-behavior",
					  "tab-size",
					  "table-layout",
					  "text-align",
					  "text-align-last",
					  "text-combine-upright",
					  "text-decoration",
					  "text-decoration-color",
					  "text-decoration-line",
					  "text-decoration-skip-ink",
					  "text-decoration-style",
					  "text-decoration-thickness",
					  "text-emphasis",
					  "text-emphasis-color",
					  "text-emphasis-position",
					  "text-emphasis-style",
					  "text-indent",
					  "text-justify",
					  "text-orientation",
					  "text-overflow",
					  "text-rendering",
					  "text-shadow",
					  "text-transform",
					  "text-underline-offset",
					  "text-underline-position",
					  "top",
					  "transform",
					  "transform-box",
					  "transform-origin",
					  "transform-style",
					  "transition",
					  "transition-delay",
					  "transition-duration",
					  "transition-property",
					  "transition-timing-function",
					  "unicode-bidi",
					  "user-select",
					  "vertical-align",
					  "visibility",
					  "white-space",
					  "widows",
					  "width",
					  "word-break",
					  "word-spacing",
					  "word-wrap",
					  "writing-mode",
					  "z-index"};
		// Value keywords (lowercase)
		valueKeywords = {"auto",
						 "inherit",
						 "initial",
						 "unset",
						 "revert",
						 "none",
						 "hidden",
						 "visible",
						 "solid",
						 "dashed",
						 "dotted",
						 "double",
						 "groove",
						 "ridge",
						 "inset",
						 "outset",
						 "block",
						 "inline",
						 "inline-block",
						 "flex",
						 "grid",
						 "table",
						 "table-row",
						 "table-cell",
						 "absolute",
						 "relative",
						 "fixed",
						 "static",
						 "sticky",
						 "center",
						 "left",
						 "right",
						 "top",
						 "bottom",
						 "start",
						 "end",
						 "justify",
						 "stretch",
						 "normal",
						 "bold",
						 "italic",
						 "underline",
						 "overline",
						 "line-through",
						 "uppercase",
						 "lowercase",
						 "capitalize",
						 "pointer",
						 "default",
						 "move",
						 "not-allowed",
						 "wait",
						 "help",
						 "crosshair",
						 "text",
						 "vertical-text",
						 "alias",
						 "copy",
						 "no-drop",
						 "grab",
						 "grabbing",
						 "all-scroll",
						 "col-resize",
						 "row-resize",
						 "n-resize",
						 "e-resize",
						 "s-resize",
						 "w-resize",
						 "ne-resize",
						 "nw-resize",
						 "se-resize",
						 "sw-resize",
						 "ew-resize",
						 "ns-resize",
						 "nesw-resize",
						 "nwse-resize",
						 "zoom-in",
						 "zoom-out",
						 "transparent",
						 "currentcolor",
						 "aliceblue",
						 "antiquewhite",
						 "aqua",
						 "aquamarine",
						 "azure",
						 "beige",
						 "bisque",
						 "black",
						 "blanchedalmond",
						 "blue",
						 "blueviolet",
						 "brown",
						 "burlywood",
						 "cadetblue",
						 "chartreuse",
						 "chocolate",
						 "coral",
						 "cornflowerblue",
						 "cornsilk",
						 "crimson",
						 "cyan",
						 "darkblue",
						 "darkcyan",
						 "darkgoldenrod",
						 "darkgray",
						 "darkgreen",
						 "darkgrey",
						 "darkkhaki",
						 "darkmagenta",
						 "darkolivegreen",
						 "darkorange",
						 "darkorchid",
						 "darkred",
						 "darksalmon",
						 "darkseagreen",
						 "darkslateblue",
						 "darkslategray",
						 "darkslategrey",
						 "darkturquoise",
						 "darkviolet",
						 "deeppink",
						 "deepskyblue",
						 "dimgray",
						 "dimgrey",
						 "dodgerblue",
						 "firebrick",
						 "floralwhite",
						 "forestgreen",
						 "fuchsia",
						 "gainsboro",
						 "ghostwhite",
						 "gold",
						 "goldenrod",
						 "gray",
						 "green",
						 "greenyellow",
						 "grey",
						 "honeydew",
						 "hotpink",
						 "indianred",
						 "indigo",
						 "ivory",
						 "khaki",
						 "lavender",
						 "lavenderblush",
						 "lawngreen",
						 "lemonchiffon",
						 "lightblue",
						 "lightcoral",
						 "lightcyan",
						 "lightgoldenrodyellow",
						 "lightgray",
						 "lightgreen",
						 "lightgrey",
						 "lightpink",
						 "lightsalmon",
						 "lightseagreen",
						 "lightskyblue",
						 "lightslategray",
						 "lightslategrey",
						 "lightsteelblue",
						 "lightyellow",
						 "lime",
						 "limegreen",
						 "linen",
						 "magenta",
						 "maroon",
						 "mediumaquamarine",
						 "mediumblue",
						 "mediumorchid",
						 "mediumpurple",
						 "mediumseagreen",
						 "mediumslateblue",
						 "mediumspringgreen",
						 "mediumturquoise",
						 "mediumvioletred",
						 "midnightblue",
						 "mintcream",
						 "mistyrose",
						 "moccasin",
						 "navajowhite",
						 "navy",
						 "oldlace",
						 "olive",
						 "olivedrab",
						 "orange",
						 "orangered",
						 "orchid",
						 "palegoldenrod",
						 "palegreen",
						 "paleturquoise",
						 "palevioletred",
						 "papayawhip",
						 "peachpuff",
						 "peru",
						 "pink",
						 "plum",
						 "powderblue",
						 "purple",
						 "rebeccapurple",
						 "red",
						 "rosybrown",
						 "royalblue",
						 "saddlebrown",
						 "salmon",
						 "sandybrown",
						 "seagreen",
						 "seashell",
						 "sienna",
						 "silver",
						 "skyblue",
						 "slateblue",
						 "slategray",
						 "slategrey",
						 "snow",
						 "springgreen",
						 "steelblue",
						 "tan",
						 "teal",
						 "thistle",
						 "tomato",
						 "turquoise",
						 "violet",
						 "wheat",
						 "white",
						 "whitesmoke",
						 "yellow",
						 "yellowgreen"};

		colorsNeedUpdate = true;
		currentState = LexerState::TopLevel;
	}

	void themeChanged() { colorsNeedUpdate = true; }

	// --- PUBLIC METHODS ---

	std::vector<Token> tokenize(const std::string &code)
	{
		std::vector<Token> tokens;
		size_t pos = 0;
		size_t lastPos = 0;
		size_t maxIterations = code.length() * 2;
		size_t iterations = 0;

		currentState = LexerState::TopLevel; // Reset state

		try
		{
			while (pos < code.length() && iterations < maxIterations)
			{
				lastPos = pos;
				char current_char = code[pos];

				if (isWhitespace(current_char))
				{
					tokens.push_back(lexWhitespace(code, pos));
				} else if (current_char == '/' && pos + 1 < code.length() &&
						   code[pos + 1] == '*')
				{
					tokens.push_back(lexComment(code, pos));
				}
				// --- State-Based Parsing ---
				else if (currentState == LexerState::TopLevel)
				{
					if (current_char == '@')
					{
						tokens.push_back(lexAtRule(code, pos));
					} else if (current_char == '{')
					{
						tokens.push_back({TokenType::Punctuation, pos++, 1});
						currentState = LexerState::InDeclarationBlock;
					} else if (current_char == '}')
					{ // Stray closing brace?
						tokens.push_back({TokenType::Punctuation, pos++, 1});
					} else
					{
						tokens.push_back(lexSelectorToken(code, pos));
					}
				} else if (currentState == LexerState::InDeclarationBlock)
				{
					if (current_char == '}')
					{
						tokens.push_back({TokenType::Punctuation, pos++, 1});
						currentState = LexerState::TopLevel;
					} else if (current_char == ':')
					{
						tokens.push_back({TokenType::Punctuation, pos++, 1});
					} else if (current_char == ';')
					{
						tokens.push_back({TokenType::Punctuation, pos++, 1});
					} else if (current_char == '!' && pos + 9 < code.length() &&
							   code.substr(pos, 10) == "!important")
					{
						tokens.push_back({TokenType::PropertyValueKeyword, pos, 10});
						pos += 10;
					} else if (isIdentStartChar(current_char))
					{
						TokenType inferredType = TokenType::Unknown;
						int prevTokenIdx = findLastNonWhitespaceTokenIndex(tokens);
						if (prevTokenIdx == -1 ||
							(tokens[prevTokenIdx].type == TokenType::Punctuation &&
							 (code[tokens[prevTokenIdx].start] == '{' ||
							  code[tokens[prevTokenIdx].start] == ';')))
						{
							inferredType = TokenType::PropertyName;
						} else
						{
							inferredType =
								TokenType::PropertyValueKeyword; // Assume value
																 // otherwise
						}

						if (inferredType == TokenType::PropertyName)
						{
							tokens.push_back(lexPropertyName(code, pos));
						} else
						{
							tokens.push_back(lexPropertyValue(code, pos));
						}
					} else
					{
						tokens.push_back(lexPropertyValue(code, pos));
					}
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
				std::cerr << "🔴 CSS Tokenizer exceeded maximum iterations." << std::endl;
		} catch (const std::exception &e)
		{
			std::cerr << "🔴 Exception in CSS tokenize: " << e.what() << std::endl;
		} catch (...)
		{
			std::cerr << "🔴 Unknown exception in CSS tokenize" << std::endl;
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
			std::cerr << "🔴 Exception in CSS applyHighlighting: " << e.what()
					  << std::endl;
			if (!colorsNeedUpdate)
				std::fill(colors.begin() + start_pos, colors.end(), cachedColors.text);
			else
				std::fill(colors.begin() + start_pos,
						  colors.end(),
						  ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
		} catch (...)
		{
			std::cerr << "🔴 Unknown exception in CSS applyHighlighting" << std::endl;
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
	// --- Member Variables ---
	std::unordered_set<std::string> properties;
	std::unordered_set<std::string> valueKeywords;
	mutable ThemeColors cachedColors;
	mutable bool colorsNeedUpdate = true;
	LexerState currentState;

	// --- Helper Functions --- (Defined before use)

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

	// Using simplified theme loading like Python example
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
			// Load colors into simplified struct
			cachedColors.keyword = loadColor("keyword");
			cachedColors.string = loadColor("string");
			cachedColors.number = loadColor("number"); // <<< NOW DEFINED
			cachedColors.comment = loadColor("comment");
			cachedColors.text = loadColor("text");
			cachedColors.function = loadColor("function");
			cachedColors.type =
				loadColor("type", cachedColors.keyword); // Default type to keyword color
			cachedColors.className =
				loadColor("class_name",
						  cachedColors.type); // Default class to type color

			colorsNeedUpdate = false;
		} catch (const std::exception &e)
		{
			std::cerr << "🔴 Error updating CSS theme colors: " << e.what() << std::endl;
		}
	}

	bool isWhitespace(char c) const
	{
		return c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\f';
	}
	bool isIdentStartChar(char c) const
	{
		return std::isalpha(static_cast<unsigned char>(c)) || c == '_' || c == '-';
	}
	bool isIdentChar(char c) const
	{
		return std::isalnum(static_cast<unsigned char>(c)) || c == '_' || c == '-';
	}
	bool isHexDigit(char c) const { return std::isxdigit(static_cast<unsigned char>(c)); }
	bool isDigit(char c) const { return std::isdigit(static_cast<unsigned char>(c)); }

	Token lexWhitespace(const std::string &code, size_t &pos)
	{
		size_t s = pos;
		while (pos < code.length() && isWhitespace(code[pos]))
			pos++;
		return {TokenType::Whitespace, s, pos - s};
	}
	Token lexComment(const std::string &code, size_t &pos)
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
	Token lexAtRule(const std::string &code, size_t &pos)
	{
		size_t s = pos;
		pos++;
		while (pos < code.length() && isIdentChar(code[pos]))
			pos++;
		return {TokenType::AtRule, s, pos - s};
	}

	Token lexSelectorToken(const std::string &code, size_t &pos)
	{
		size_t s = pos;
		char c = code[pos];
		if (c == '#')
		{
			pos++;
			while (pos < code.length() && isIdentChar(code[pos]))
				pos++;
			return {TokenType::SelectorId, s, pos - s};
		}
		if (c == '.')
		{
			pos++;
			while (pos < code.length() && isIdentChar(code[pos]))
				pos++;
			return {TokenType::SelectorClass, s, pos - s};
		}
		if (c == '[')
		{
			pos++;
			while (pos < code.length() && code[pos] != ']')
				pos++;
			if (pos < code.length())
				pos++;
			return {TokenType::SelectorAttribute, s, pos - s};
		}
		if (c == ':')
		{
			pos++;
			if (pos < code.length() && code[pos] == ':')
				pos++;
			while (pos < code.length() && isIdentChar(code[pos]))
				pos++;
			if (pos < code.length() && code[pos] == '(')
			{
				int pL = 1;
				pos++;
				while (pos < code.length() && pL > 0)
				{
					if (code[pos] == '(')
						pL++;
					else if (code[pos] == ')')
						pL--;
					if (pL > 0)
						pos++;
				}
				if (pos < code.length() && pL == 0)
					pos++;
			}
			return {TokenType::SelectorPseudo, s, pos - s};
		}
		if (c == '>' || c == '+' || c == '~' || c == '*')
		{
			pos++;
			return {TokenType::Punctuation, s, 1};
		}
		if (isIdentStartChar(c))
		{
			while (pos < code.length() && isIdentChar(code[pos]))
				pos++;
			return {TokenType::SelectorTag, s, pos - s};
		}
		pos++;
		return {TokenType::Punctuation, s, 1};
	}

	Token lexPropertyName(const std::string &code, size_t &pos)
	{
		size_t s = pos;
		if (code[pos] == '-')
			pos++;
		while (pos < code.length() && isIdentChar(code[pos]))
			pos++;
		std::string prop = code.substr(s, pos - s);
		std::string lowerProp = prop;
		std::transform(lowerProp.begin(), lowerProp.end(), lowerProp.begin(), ::tolower);
		return {TokenType::PropertyName, s, pos - s};
	}

	Token lexPropertyValue(const std::string &code, size_t &pos)
	{
		size_t s = pos;
		char c = code[pos];
		if (c == '"' || c == '\'')
		{
			char q = code[pos++];
			while (pos < code.length())
			{
				if (code[pos] == '\\')
				{
					pos++;
					if (pos < code.length())
						pos++;
				} else if (code[pos] == q)
				{
					pos++;
					break;
				} else if (code[pos] == '\n')
					break;
				else
					pos++;
			}
			return {TokenType::PropertyValueString, s, pos - s};
		}
		if (c == '#')
		{
			pos++;
			while (pos < code.length() && isHexDigit(code[pos]))
				pos++;
			size_t l = pos - s;
			if (l == 4 || l == 7 || l == 9)
				return {TokenType::PropertyValueColor, s, l};
			else
				return {TokenType::Unknown, s, l};
		}
		if (isDigit(c) ||
			(c == '.' && pos + 1 < code.length() && isDigit(code[pos + 1])) ||
			(c == '-' && pos + 1 < code.length() &&
			 (isDigit(code[pos + 1]) || code[pos + 1] == '.')))
		{
			if (c == '-')
				pos++;
			while (pos < code.length() && isDigit(code[pos]))
				pos++;
			if (pos < code.length() && code[pos] == '.')
			{
				pos++;
				while (pos < code.length() && isDigit(code[pos]))
					pos++;
			}
			while (pos < code.length() &&
				   std::isalpha(static_cast<unsigned char>(code[pos])))
				pos++;
			if (pos < code.length() && code[pos] == '%')
				pos++;
			return {TokenType::PropertyValueNumber, s, pos - s};
		}
		if (isIdentStartChar(c))
		{
			while (pos < code.length() && isIdentChar(code[pos]))
				pos++;
			if (pos < code.length() && code[pos] == '(')
			{
				pos++;
				int pL = 1;
				while (pos < code.length() && pL > 0)
				{
					if (code[pos] == '(')
						pL++;
					else if (code[pos] == ')')
						pL--;
					if (pL > 0)
						pos++;
				}
				if (pos < code.length() && pL == 0)
					pos++;
				std::string fN = code.substr(s, pos - s);
				std::string lFN = fN;
				std::transform(lFN.begin(), lFN.end(), lFN.begin(), ::tolower);
				if (lFN.rfind("rgb(", 0) == 0 || lFN.rfind("rgba(", 0) == 0 ||
					lFN.rfind("hsl(", 0) == 0 || lFN.rfind("hsla(", 0) == 0)
					return {TokenType::PropertyValueColor, s, pos - s};
				else
					return {TokenType::PropertyValueFunction, s, pos - s};
			} else
			{
				std::string w = code.substr(s, pos - s);
				std::string lW = w;
				std::transform(lW.begin(), lW.end(), lW.begin(), ::tolower);
				if (valueKeywords.count(lW))
					return {TokenType::PropertyValueKeyword, s, pos - s};
				else
					return {TokenType::PropertyValueKeyword, s, pos - s};
			}
		}
		pos++;
		if (c == ',' || c == '/' || c == '(' || c == ')' || c == '*')
			return {TokenType::Punctuation, s, 1};
		return {TokenType::Unknown, s, 1};
	}

	// Map CSS tokens to simplified theme colors
	ImVec4 getColorForTokenType(TokenType type) const
	{
		updateThemeColors();
		switch (type)
		{
		case TokenType::Comment:
			return cachedColors.comment;
		case TokenType::PropertyValueString:
		case TokenType::SelectorAttribute:
			return cachedColors.string;
		case TokenType::PropertyValueNumber:
		case TokenType::PropertyValueColor:
		case TokenType::PropertyValueKeyword: // <<< NOW MAPPED TO NUMBER COLOR
			return cachedColors.number;
		case TokenType::AtRule:
		case TokenType::SelectorPseudo:
			return cachedColors.keyword;
		case TokenType::SelectorId:
		case TokenType::PropertyValueFunction:
			return cachedColors.function;
		case TokenType::SelectorTag:
		case TokenType::PropertyName:
			return cachedColors.type;
		case TokenType::SelectorClass:
			return cachedColors.className;
		case TokenType::Punctuation:
		case TokenType::Whitespace:
		case TokenType::Unknown:
		default:
			return cachedColors.text;
		}
	}

}; // End class Lexer

} // namespace CssLexer