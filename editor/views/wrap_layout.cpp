/*
	File: views/wrap_layout.cpp
	Description: Soft-wrap segment cache implementation.
*/

#include "wrap_layout.h"
#include "../editor_state.h"
#include "../util/editor_utils.h"

#include <algorithm>
#include <cfloat>
#include <cmath>

namespace {

// UTF-8 glyph boundaries (mirrors TextView::advanceUtf8).
size_t advanceUtf8(const std::string &text, size_t index)
{
	++index;
	while (index < text.size() && (static_cast<unsigned char>(text[index]) & 0xC0) == 0x80)
		++index;
	return index;
}

} // namespace

void WrapLayout::noteEdit(int lo, int hi)
{
	if (!valid)
		return;
	if (lo > hi)
		std::swap(lo, hi);
	if (dirtyHi < dirtyLo)
	{
		dirtyLo = lo;
		dirtyHi = hi;
	} else
	{
		dirtyLo = std::min(dirtyLo, lo);
		dirtyHi = std::max(dirtyHi, hi);
	}
}

void WrapLayout::ensure(const EditorState &state, float wrapWidth)
{
	const float fs = ImGui::GetFontSize();
	const int n = state.lineCount();

	// Width hysteresis: a full re-wrap is O(document); rebuild only when the
	// width moved by more than ~half a glyph (sub-glyph drift during a resize
	// drag just shifts wrap points imperceptibly instead of re-measuring).
	const float widthEps = std::max(2.0f, fs * 0.5f);

	if (!valid || std::abs(width - wrapWidth) > widthEps || fontKey != fs || lines != n)
	{
		breaks.assign(static_cast<size_t>(std::max(n, 0)), {});
		for (int i = 0; i < n; ++i)
			wrapRow(state.line(i), wrapWidth, breaks[static_cast<size_t>(i)]);
		lines = n;
		width = wrapWidth;
		fontKey = fs;
		valid = true;
		dirtyHi = -1;
		dirtyLo = 0;
		rebuildCumulative();
		return;
	}

	if (dirtyHi >= dirtyLo && n > 0)
	{
		const int lo = std::clamp(dirtyLo, 0, n - 1);
		const int hi = std::clamp(dirtyHi, 0, n - 1);
		for (int i = lo; i <= hi; ++i)
			wrapRow(state.line(i), wrapWidth, breaks[static_cast<size_t>(i)]);
		dirtyHi = -1;
		dirtyLo = 0;
		rebuildCumulative();
	}
}

// Greedy word wrap: break after the latest space that keeps the segment within
// limit; hard-break runs (long tokens / URLs) at the exact width. Each segment
// measures x from 0, so tab stops restart per segment (flush-left continuation).
void WrapLayout::wrapRow(const std::string &line, float limit, std::vector<int> &out) const
{
	out.clear();
	if (line.empty())
		return;

	const float spaceW = EditorUtils::SpaceWidth();
	auto glyphWidth = [&](size_t pos, float drawX) {
		const char *s = &line[pos];
		const char *e = s + 1;
		if (*s == '\t')
			return EditorUtils::TabAdvanceWidth(spaceW, static_cast<int>(drawX / spaceW));
		if ((static_cast<unsigned char>(*s) & 0x80) != 0)
		{
			while (e < line.data() + line.size() &&
				   (static_cast<unsigned char>(*e) & 0xC0) == 0x80)
				++e;
		}
		return EditorUtils::GlyphAdvance(s, e);
	};

	float x = 0.0f;
	int segStart = 0;
	int lastBreak = -1; // byte column just after the latest candidate space
	size_t i = 0;
	while (i < line.size())
	{
		const float w = glyphWidth(i, x);
		if (x + w > limit && static_cast<int>(i) > segStart)
		{
			const int brk = (lastBreak > segStart) ? lastBreak : static_cast<int>(i);
			out.push_back(brk);
			segStart = brk;
			x = 0.0f;
			for (size_t j = static_cast<size_t>(brk); j < i;)
			{
				x += glyphWidth(j, x);
				j = advanceUtf8(line, j);
			}
			lastBreak = -1;
			continue; // re-process the overflowing glyph in the new segment
		}
		if (line[i] == ' ')
			lastBreak = static_cast<int>(i) + 1;
		x += w;
		i = advanceUtf8(line, i);
	}
}

void WrapLayout::rebuildCumulative()
{
	rowStart.assign(static_cast<size_t>(lines) + 1, 0);
	int running = 0;
	for (int i = 0; i < lines; ++i)
	{
		rowStart[static_cast<size_t>(i)] = running;
		running += 1 + static_cast<int>(breaks[static_cast<size_t>(i)].size());
	}
	rowStart[static_cast<size_t>(lines)] = running;
}

int WrapLayout::visualLineCount(int row) const
{
	if (row < 0 || row >= lines)
		return 1;
	return 1 + static_cast<int>(breaks[static_cast<size_t>(row)].size());
}

int WrapLayout::rowStartVisualLine(int row) const
{
	if (row < 0)
		return 0;
	if (row >= lines)
		return totalVisualLines();
	return rowStart[static_cast<size_t>(row)];
}

WrapLayout::Hit WrapLayout::yToRow(float visualLine) const
{
	Hit hit;
	if (lines <= 0)
		return hit;
	const int total = totalVisualLines();
	const int v = std::clamp(static_cast<int>(std::floor(visualLine)), 0, total - 1);

	// Largest row whose start visual line is <= v (rowStart is ascending).
	int lo = 0, hi = lines - 1;
	while (lo < hi)
	{
		const int mid = (lo + hi + 1) / 2;
		if (rowStart[static_cast<size_t>(mid)] <= v)
			lo = mid;
		else
			hi = mid - 1;
	}
	hit.row = lo;
	hit.segment = v - rowStart[static_cast<size_t>(lo)];
	return hit;
}

int WrapLayout::segmentCount(int row) const
{
	if (row < 0 || row >= lines)
		return 1;
	return 1 + static_cast<int>(breaks[static_cast<size_t>(row)].size());
}

int WrapLayout::segmentStartColumn(int row, int segment) const
{
	if (segment <= 0 || row < 0 || row >= lines)
		return 0;
	const std::vector<int> &b = breaks[static_cast<size_t>(row)];
	if (segment > static_cast<int>(b.size()))
		return INT_MAX;
	return b[static_cast<size_t>(segment - 1)];
}

int WrapLayout::segmentOf(int row, int column) const
{
	if (row < 0 || row >= lines)
		return 0;
	const std::vector<int> &b = breaks[static_cast<size_t>(row)];
	return static_cast<int>(std::upper_bound(b.begin(), b.end(), column) - b.begin());
}

// x of a byte column inside its segment. Measured from the segment start with
// tab stops restarting there (EditorUtils::ColumnsToX) — matches how wrapRow
// and TextView measure/render segments. Never a LineColumnX difference:
// absolute tab stops from column 0 would misplace tab-indented segments.
float WrapLayout::columnX(const std::string &line, int row, int column) const
{
	const int seg = segmentOf(row, column);
	return EditorUtils::ColumnsToX(line, segmentStartColumn(row, seg), column);
}

int WrapLayout::columnAt(const std::string &line, int row, int segment, float xRel) const
{
	const int start = segmentStartColumn(row, segment);
	const int end = segment + 1 < segmentCount(row) ? segmentStartColumn(row, segment + 1)
													: static_cast<int>(line.size());
	if (line.empty() || start >= static_cast<int>(line.size()))
		return start;

	// Nearest-glyph match within the segment (same rule as ColumnAtX, but with
	// tab stops restarting at the segment start).
	int best = start;
	float bestDist = std::abs(xRel);
	float x = 0.0f;
	for (int i = start; i < end;)
	{
		const char *s = &line[i];
		const char *e = s + 1;
		if (*s != '\t' && (static_cast<unsigned char>(*s) & 0x80) != 0)
		{
			while (e < line.data() + line.size() &&
				   (static_cast<unsigned char>(*e) & 0xC0) == 0x80)
				++e;
		}
		x += EditorUtils::MeasureGlyphWidth(s, e, x, 0.0f);
		const int next = static_cast<int>(e - line.data());
		const float dist = std::abs(xRel - x);
		if (dist < bestDist)
		{
			bestDist = dist;
			best = next;
		}
		if (x >= xRel)
			break;
		i = next;
	}
	return std::clamp(best, start, end);
}
