#pragma once

/*
	Hover markdown render runs — the backend-neutral span walks shared by the
	ImGui and Qt hover renderers. Pure logic: produces styled text runs per
	line; backends only map runs to draw calls / HTML. Keeps hover_markdown.h
	free of highlight types.
*/

#include "../services/highlight/tree_sitter.h"

#include <string_view>
#include <vector>

// Prose inline styles (HoverProseRuns).
enum class HoverRunStyle : uint8_t {
	Text, // plain paragraph text (default text color)
	Code, // `code` (theme String slot color)
	Bold, // **bold** (brightened default text color)
	Link, // [label](url) — the label only; the URL is dropped
};

struct HoverRun
{
	std::string_view text;
	HoverRunStyle style = HoverRunStyle::Text;
};

// Split one prose line into styled runs. Unterminated markers degrade to
// plain text. Runs reference `line` (no copies); an empty line yields no
// runs.
std::vector<HoverRun> HoverProseRuns(std::string_view line);

// One colored run of a highlighted code line. `themed` selects the theme
// slot color; otherwise the fallback text color applies.
struct HoverCodeRun
{
	std::string_view text;
	bool themed = false;
	ThemeSlot slot = ThemeSlot::Text;
};

// Split one code line into runs along the highlight spans (half-open byte
// ranges). Gaps between spans become fallback-colored runs.
std::vector<HoverCodeRun> HoverCodeRuns(std::string_view line,
										const LineColorSpans &spans);
