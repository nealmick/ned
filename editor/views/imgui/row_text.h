/*
	File: views/imgui/row_text.h
	Description: Per-row UTF-8 text measurement for the ImGui renderer —
	glyph advance walking + tab-aware measuring (Qt counterpart: the
	tab-expanded cell model in views/qt/row_text.h).
*/

#pragma once

#include <cstddef>
#include <string>

namespace RowText {
// Advance one UTF-8 glyph (skips continuation bytes).
size_t advanceUtf8(const std::string &text, size_t index, size_t end);

// Draw-position-aware width of one glyph (tabs rebase at draw_x).
float measureGlyphWidth(const char *start,
						const char *end,
						float draw_x,
						float text_origin_x);
} // namespace RowText
