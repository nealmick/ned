/*
	File: views/imgui/editor_utils.h
	Description: ImGui glyph metrics (font advance, column<->x) and the
	rainbow cursor color. Pure helpers live in text_columns.h.
*/

#pragma once

#include "../../util/text_columns.h"

#include "imgui.h"
#include <GLFW/glfw3.h> // For time functions
#include <cfloat>
#include <cmath>
#include <string>
#include <vector>

namespace EditorUtils {

inline ImVec4 getRainbowColor(float timeScale = 2.0f)
{
	static float sharedBlinkTime = 0.0f;
	static double lastUpdateTime = 0.0;

	// Update time only once per frame
	double currentTime = glfwGetTime();
	if (currentTime > lastUpdateTime)
	{
		sharedBlinkTime += static_cast<float>((currentTime - lastUpdateTime) * timeScale);
		lastUpdateTime = currentTime;
	}

	float t = sharedBlinkTime;
	float r = sin(t) * 0.5f + 0.5f;
	float g = sin(t + 2.0944f) * 0.5f + 0.5f; // 2.0944 is 2π/3
	float b = sin(t + 4.1888f) * 0.5f + 0.5f; // 4.1888 is 4π/3
	return ImVec4(r, g, b, 1.0f);
}

// CalcTextSizeA, not ImGui::CalcTextSize — the latter ceils every call
// and walks the caret to the right of AddText.
inline float glyphAdvance(const char *start, const char *end)
{
	ImFont *font = ImGui::GetFont();
	const float fs = ImGui::GetFontSize();
	if (end == start + 1)
	{
		const unsigned char c = static_cast<unsigned char>(*start);
		if (c < 128)
		{
			static ImFont *cachedFont = nullptr;
			static float cachedFs = 0.0f;
			static float adv[128];
			if (font != cachedFont || fs != cachedFs)
			{
				cachedFont = font;
				cachedFs = fs;
				char buf[2] = {0, 0};
				for (int i = 0; i < 128; ++i)
				{
					buf[0] = static_cast<char>(i);
					adv[i] = font->CalcTextSizeA(fs, FLT_MAX, 0.0f, buf, buf + 1).x;
				}
			}
			return adv[c];
		}
	}
	return font->CalcTextSizeA(fs, FLT_MAX, 0.0f, start, end).x;
}

inline float spaceWidth()
{
	static const char kSpace[] = " ";
	return glyphAdvance(kSpace, kSpace + 1);
}

// Width of one glyph (or tab) at drawX relative to text origin.
inline float measureGlyphWidth(const char *start,
							   const char *end,
							   float drawX,
							   float textOriginX,
							   int tabSize = kTabSize)
{
	if (*start == '\t')
	{
		const float spaceW = spaceWidth();
		const int column = static_cast<int>((drawX - textOriginX) / spaceW);
		return tabAdvanceWidth(spaceW, column, tabSize);
	}
	return glyphAdvance(start, end);
}

inline float lineColumnX(const std::string &line, int column, float originX = 0.0f)
{
	float x = originX;
	const int end = std::max(0, std::min(column, static_cast<int>(line.size())));
	for (int i = 0; i < end;)
	{
		const char *start = &line[i];
		const char *stop = line.data() + nextCharEnd(line, static_cast<size_t>(i));
		x += measureGlyphWidth(start, stop, x, originX);
		i = static_cast<int>(stop - line.data());
	}
	return x;
}

inline int columnAtX(const std::string &line, float clickX, float originX = 0.0f)
{
	if (line.empty())
		return 0;
	int best = 0;
	float bestDist = std::abs(clickX - originX);
	float x = originX;
	for (int i = 0; i < static_cast<int>(line.size());)
	{
		const char *start = &line[i];
		const char *stop = line.data() + nextCharEnd(line, static_cast<size_t>(i));
		x += measureGlyphWidth(start, stop, x, originX);
		const int next = static_cast<int>(stop - line.data());
		const float dist = std::abs(clickX - x);
		if (dist < bestDist)
		{
			bestDist = dist;
			best = next;
		}
		if (x >= clickX)
			break;
		i = next;
	}
	return best;
}

// x of byte column `to`, measured from byte column `from` with tab stops
// restarting at `from` (soft-wrap segments restart tab stops; do NOT compute
// this as lineColumnX(to) - lineColumnX(from) — absolute tab stops differ).
inline float columnsToX(const std::string &line, int from, int to)
{
	const int end = std::max(0, std::min(to, static_cast<int>(line.size())));
	float x = 0.0f;
	for (int i = std::max(0, from); i < end;)
	{
		const char *start = &line[i];
		const char *stop = line.data() + nextCharEnd(line, static_cast<size_t>(i));
		x += measureGlyphWidth(start, stop, x, 0.0f);
		i = static_cast<int>(stop - line.data());
	}
	return x;
}

} // namespace EditorUtils
