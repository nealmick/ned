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
	const char *key;
	int prio;
};

// Order is load-bearing (specific before general).
inline constexpr CaptureRule kCaptureRules[] = {
	// holes / skip
	{"none", false, "text", -1},
	{"default", false, "text", 10},
	{"text", false, "text", 10},
	{"conceal", false, "text", 10},
	{"spell", false, "text", 10},
	{"nospell", false, "text", 10},
	{"embedded", false, "text", 10},
	{"markup.", true, "text", 10},
	{"diff.", true, "text", 10},

	// comments / keywords / strings / numbers
	{"comment", false, "comment", 100},
	{"comment.", true, "comment", 100},
	{"keyword.function", false, "keyword", 100},
	{"keyword", false, "keyword", 100},
	{"keyword.", true, "keyword", 100},
	{"boolean", false, "keyword", 100},
	{"string.escape", false, "special", 100},
	{"string.escape.", true, "special", 100},
	{"escape", false, "special", 100},
	{"string", false, "string", 100},
	{"string.", true, "string", 100},
	{"character", false, "string", 100},
	{"character.", true, "string", 100},
	{"number", false, "number", 100},
	{"number.", true, "number", 100},
	{"float", false, "number", 100},

	// macros before function.*
	{"function.macro", false, "function", 110},
	{"function.macro.", true, "function", 110},
	{"macro", false, "function", 110},
	{"function.builtin", false, "special", 105},
	{"function", false, "function", 95},
	{"function.", true, "function", 95},
	{"method", false, "function", 95},
	{"constructor", false, "type", 95},

	// types / modules
	{"type.builtin", false, "special", 105},
	{"type", false, "type", 90},
	{"type.", true, "type", 90},
	{"tag", false, "type", 90},
	{"tag.", true, "type", 90},
	{"module.builtin", false, "special", 105},
	{"module", false, "type", 90},
	{"module.", true, "type", 90},
	{"namespace.builtin", false, "special", 105},
	{"namespace", false, "type", 90},
	{"label", false, "type", 90},
	{"label.", true, "type", 90},

	// variables / members / params / builtins
	{"variable.parameter", false, "parameter", 75},
	{"variable.parameter.", true, "parameter", 75},
	{"parameter", false, "parameter", 75},
	{"variable.member", false, "property", 70},
	{"variable.builtin", false, "special", 105},
	{"constant.builtin", false, "special", 105},
	{"property", false, "property", 70},
	{"property.", true, "property", 70},
	{"field", false, "property", 70},
	{"attribute", false, "property", 70},
	{"attribute.", true, "property", 70},
	{"constant", false, "constant", 90},
	{"constant.", true, "constant", 90},
	{"variable", false, "variable", 40},
	{"variable.", true, "variable", 40},

	// ops / punct / misc
	{"operator", false, "operator", 100},
	{"punctuation", false, "punctuation", 50},
	{"punctuation.", true, "punctuation", 50},
	{"delimiter", false, "punctuation", 50},
	{"delimiter.", true, "punctuation", 50},
	{"special", false, "special", 100},
	{"hook", false, "special", 100},
	{"preprocessor", false, "special", 100},
	{"preproc", false, "special", 100},
	{"preproc.", true, "special", 100},
	{"define", false, "special", 100},
	{"include", false, "special", 100},
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

// Theme JSON key under themes.<name>. Never null.
inline const char *themeKeyForCapture(std::string_view name)
{
	if (const CaptureRule *r = matchCaptureRule(name))
		return r->key;
	return "text";
}
