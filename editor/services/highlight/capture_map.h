/*
	Capture name → theme slot + paint priority (nvim-style hi-link layer).

	Queries emit rich names (@function.method, @keyword.import, …).
	Themes only have ~15 colors; this table collapses the vocabulary.
*/
#pragma once

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <string_view>
#include <utility>
#include <vector>

// Theme JSON keys collapsed to a dense slot. Spans store the slot; paint
// looks up RGB so a theme switch never re-queries.
enum class ThemeSlot : uint8_t {
	Text = 0,
	Comment,
	Keyword,
	String,
	Number,
	Function,
	Type,
	Variable,
	Parameter,
	Property,
	Constant,
	Operator,
	Punctuation,
	Special,
	Count
};

// Index matches ThemeSlot. JSON keys under themes.<name>.
inline constexpr const char *kThemeKeys[] = {
	"text",
	"comment",
	"keyword",
	"string",
	"number",
	"function",
	"type",
	"variable",
	"parameter",
	"property",
	"constant",
	"operator",
	"punctuation",
	"special",
};
static_assert(sizeof(kThemeKeys) / sizeof(kThemeKeys[0]) ==
			  static_cast<unsigned>(ThemeSlot::Count));

// nvim (interpolation) @none — hole in parent @string, not a color.
inline bool isNoneCapture(std::string_view n) { return n == "none"; }

inline bool isStringCapture(std::string_view n)
{
	return n == "string" || (n.size() > 7 && n.compare(0, 7, "string.") == 0);
}

// Clip [start,end) by sorted/merged hole ranges.
inline std::vector<std::pair<uint32_t, uint32_t>> subtractRanges(
	uint32_t start, uint32_t end, const std::vector<std::pair<uint32_t, uint32_t>> &holes)
{
	std::vector<std::pair<uint32_t, uint32_t>> out;
	if (start >= end)
		return out;
	uint32_t cur = start;
	for (const auto &[hs, he] : holes)
	{
		if (he <= cur)
			continue;
		if (hs >= end)
			break;
		if (hs > cur)
			out.push_back({cur, std::min(hs, end)});
		cur = std::max(cur, he);
		if (cur >= end)
			return out;
	}
	if (cur < end)
		out.push_back({cur, end});
	return out;
}

// First matching rule wins. prefix=true → name starts with `pat`.
// prio: higher wins on overlap; -1 = @none hole; <=10 = skip paint.
struct CaptureRule
{
	const char *pat;
	bool prefix;
	ThemeSlot slot;
	int prio;
};

// Order is load-bearing (specific before general).
inline constexpr CaptureRule kCaptureRules[] = {
	// holes / skip
	{"none", false, ThemeSlot::Text, -1},
	{"default", false, ThemeSlot::Text, 10},
	{"text", false, ThemeSlot::Text, 10},
	{"conceal", false, ThemeSlot::Text, 10},
	{"spell", false, ThemeSlot::Text, 10},
	{"nospell", false, ThemeSlot::Text, 10},
	{"embedded", false, ThemeSlot::Text, 10},
	{"markup.", true, ThemeSlot::Text, 10},
	{"diff.", true, ThemeSlot::Text, 10},

	// comments / keywords / strings / numbers
	{"comment", false, ThemeSlot::Comment, 100},
	{"comment.", true, ThemeSlot::Comment, 100},
	{"keyword.function", false, ThemeSlot::Keyword, 100},
	{"keyword", false, ThemeSlot::Keyword, 100},
	{"keyword.", true, ThemeSlot::Keyword, 100},
	{"boolean", false, ThemeSlot::Keyword, 100},
	{"string.escape", false, ThemeSlot::Special, 100},
	{"string.escape.", true, ThemeSlot::Special, 100},
	{"escape", false, ThemeSlot::Special, 100},
	{"string", false, ThemeSlot::String, 100},
	{"string.", true, ThemeSlot::String, 100},
	{"character", false, ThemeSlot::String, 100},
	{"character.", true, ThemeSlot::String, 100},
	{"number", false, ThemeSlot::Number, 100},
	{"number.", true, ThemeSlot::Number, 100},
	{"float", false, ThemeSlot::Number, 100},

	// macros before function.*
	{"function.macro", false, ThemeSlot::Function, 110},
	{"function.macro.", true, ThemeSlot::Function, 110},
	{"macro", false, ThemeSlot::Function, 110},
	{"function.builtin", false, ThemeSlot::Special, 105},
	{"function", false, ThemeSlot::Function, 95},
	{"function.", true, ThemeSlot::Function, 95},
	{"method", false, ThemeSlot::Function, 95},
	{"constructor", false, ThemeSlot::Type, 95},

	// types / modules
	{"type.builtin", false, ThemeSlot::Special, 105},
	{"type", false, ThemeSlot::Type, 90},
	{"type.", true, ThemeSlot::Type, 90},
	{"tag", false, ThemeSlot::Type, 90},
	{"tag.", true, ThemeSlot::Type, 90},
	{"module.builtin", false, ThemeSlot::Special, 105},
	{"module", false, ThemeSlot::Type, 90},
	{"module.", true, ThemeSlot::Type, 90},
	{"namespace.builtin", false, ThemeSlot::Special, 105},
	{"namespace", false, ThemeSlot::Type, 90},
	{"label", false, ThemeSlot::Type, 90},
	{"label.", true, ThemeSlot::Type, 90},

	// variables / members / params / builtins
	{"variable.parameter", false, ThemeSlot::Parameter, 75},
	{"variable.parameter.", true, ThemeSlot::Parameter, 75},
	{"parameter", false, ThemeSlot::Parameter, 75},
	{"variable.member", false, ThemeSlot::Property, 70},
	{"variable.builtin", false, ThemeSlot::Special, 105},
	{"constant.builtin", false, ThemeSlot::Special, 105},
	{"property", false, ThemeSlot::Property, 70},
	{"property.", true, ThemeSlot::Property, 70},
	{"field", false, ThemeSlot::Property, 70},
	{"attribute", false, ThemeSlot::Property, 70},
	{"attribute.", true, ThemeSlot::Property, 70},
	{"constant", false, ThemeSlot::Constant, 90},
	{"constant.", true, ThemeSlot::Constant, 90},
	{"variable", false, ThemeSlot::Variable, 40},
	{"variable.", true, ThemeSlot::Variable, 40},

	// ops / punct / misc
	{"operator", false, ThemeSlot::Operator, 100},
	{"punctuation", false, ThemeSlot::Punctuation, 50},
	{"punctuation.", true, ThemeSlot::Punctuation, 50},
	{"delimiter", false, ThemeSlot::Punctuation, 50},
	{"delimiter.", true, ThemeSlot::Punctuation, 50},
	{"special", false, ThemeSlot::Special, 100},
	{"hook", false, ThemeSlot::Special, 100},
	{"preprocessor", false, ThemeSlot::Special, 100},
	{"preproc", false, ThemeSlot::Special, 100},
	{"preproc.", true, ThemeSlot::Special, 100},
	{"define", false, ThemeSlot::Special, 100},
	{"include", false, ThemeSlot::Special, 100},
};

inline const CaptureRule *matchCaptureRule(std::string_view name)
{
	for (const CaptureRule &r : kCaptureRules)
	{
		const size_t n = std::strlen(r.pat);
		if (r.prefix)
		{
			if (name.size() >= n && name.compare(0, n, r.pat) == 0)
				return &r;
		} else if (name == r.pat)
			return &r;
	}
	return nullptr;
}

inline int capturePriority(std::string_view name)
{
	if (const CaptureRule *r = matchCaptureRule(name))
		return r->prio;
	return 30;
}

inline ThemeSlot themeSlotForCapture(std::string_view name)
{
	if (const CaptureRule *r = matchCaptureRule(name))
		return r->slot;
	return ThemeSlot::Text;
}

// Theme JSON key under themes.<name>. Never null.
inline const char *themeKeyForCapture(std::string_view name)
{
	return kThemeKeys[static_cast<uint8_t>(themeSlotForCapture(name))];
}

inline ThemeSlot themeSlotForKey(std::string_view key)
{
	for (uint8_t i = 0; i < static_cast<uint8_t>(ThemeSlot::Count); ++i)
	{
		if (key == kThemeKeys[i])
			return static_cast<ThemeSlot>(i);
	}
	return ThemeSlot::Text;
}
