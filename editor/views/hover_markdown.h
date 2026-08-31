#pragma once

/*
	Hover markdown subset parser (fences, rules, prose paragraphs).
	Pure — no ImGui, no editor types; unit-testable.
*/

#include <string>
#include <vector>

struct HoverMdBlock
{
	bool code = false;
	std::string language;
	std::string text;
};

// Split hover markdown into blocks. Fenced code carries its language; prose
// paragraphs are trimmed; `---` becomes a rule block ("---" text).
std::vector<HoverMdBlock> ParseHoverMarkdown(const std::string &src);

// Split into lines (no terminators; trailing '\r' dropped). Shared by the
// parser and the renderer's per-line walkers.
std::vector<std::string> SplitHoverLines(std::string_view s);
