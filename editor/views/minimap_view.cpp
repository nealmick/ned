/*
	File: views/minimap_view.cpp
	Description: Density minimap — ImGui rects, click/drag/wheel.

	Density glyphs are expensive (lineInto + span walk + UTF-8 per char, every
	frame). Cache the strip's rect list keyed by doc version / highlight gen /
	visible line window; only rebuild when that key changes. Idle frames just
	replay verts. Slider still updates live with scroll.
*/
#include "minimap_view.h"
#include "../editor_state.h"
#include "../editor_view_state.h"
#include "../services/highlight/highlight_service.h"
#include "view_layout.h"
#include <algorithm>
#include <cmath>
#include <string>

namespace {

constexpr float kLineH = 2.0f, kCharW = 1.0f;
// Density rects read hotter than full glyphs — pull theme colors down a notch.
constexpr float kColorDim = 0.72f;
constexpr float kDotH = 1.5f;
constexpr float kPadX = 2.0f;

ImU32 dimInk(ImVec4 c)
{
	c.x *= kColorDim;
	c.y *= kColorDim;
	c.z *= kColorDim;
	return ImGui::ColorConvertFloat4ToU32(c);
}

struct Strip
{
	int lineCount = 0, start = 0, end = -1;
	float viewH = 0, maxScroll = 0, sliderTop = 0, sliderH = 0, ratio = 0;
};

Strip makeStrip(const EditorState &st, const ViewLayout &lay, float scrollY, float stripH)
{
	Strip s;
	s.lineCount = st.lineCount();
	const float elh = lay.lineHeight;
	if (s.lineCount <= 0 || stripH <= 1.0f || elh <= 0.0f)
		return s;
	scrollY = std::max(0.0f, scrollY);
	s.viewH = std::max(elh, std::max(0.0f, lay.size.y - lay.editorTopMargin));
	s.maxScroll = std::max(0.0f, std::max(lay.totalHeight, s.viewH) - s.viewH);
	s.sliderH = std::clamp(std::floor((s.viewH / elh) * kLineH), 4.0f, stripH);
	const float maxTop = std::min(
		stripH - s.sliderH, std::max(0.0f, float(s.lineCount) * kLineH - s.sliderH));
	s.ratio = s.maxScroll > 1.0f ? maxTop / s.maxScroll : 0.0f;
	s.sliderTop = std::clamp(scrollY * s.ratio, 0.0f, maxTop);
	const int fit = std::max(1, int(stripH / kLineH));
	if (s.lineCount <= fit)
	{
		s.end = s.lineCount - 1;
	} else
	{
		s.start = std::clamp(
			int(std::floor(scrollY / elh - s.sliderTop / kLineH)), 0, s.lineCount - fit);
		s.end = std::min(s.lineCount - 1, s.start + fit - 1);
	}
	return s;
}

} // namespace

void MinimapView::rebuildDensityCache(const CacheKey &key) const
{
	cacheDots_.clear();
	if (!state || !highlight || key.end < key.start || key.maxCols <= 0)
	{
		cacheKey_ = key;
		return;
	}

	// Rough upper bound: visible lines * columns (most cells empty).
	const int rows = key.end - key.start + 1;
	cacheDots_.reserve(static_cast<size_t>(rows * std::min(key.maxCols, 24)));

	static thread_local std::string line;
	for (int row = key.start; row <= key.end; ++row)
	{
		const float y0 = float(row - key.start) * kLineH;
		state->lineInto(row, line);
		const auto &spans = highlight->spansForLine(row);
		size_t sp = 0;
		for (int col = 0, i = 0; i < (int)line.size() && col < key.maxCols;)
		{
			const int byte = i;
			const unsigned char c = (unsigned char)line[i++];
			if ((c & 0xC0) == 0x80)
				continue;
			if (c == '\t')
			{
				col = std::min(key.maxCols, col + (4 - col % 4));
				continue;
			}
			if (c <= ' ')
			{
				++col;
				continue;
			}
			while (sp < spans.size() && spans[sp].end <= byte)
				++sp;
			ImU32 ink = key.defInk;
			if (sp < spans.size() && spans[sp].start <= byte)
				ink = dimInk(highlight->colorForSlot(spans[sp].slot));
			cacheDots_.push_back(Dot{kPadX + float(col++) * kCharW, y0, ink});
			while (i < (int)line.size() && ((unsigned char)line[i] & 0xC0) == 0x80)
				++i;
		}
	}
	cacheKey_ = key;
}

void MinimapView::replayDensity(ImDrawList *dl, ImVec2 origin) const
{
	const int n = static_cast<int>(cacheDots_.size());
	if (n <= 0 || !dl)
		return;

	// Batch into the window draw list — avoids per-dot AddRectFilled overhead.
	dl->PrimReserve(n * 6, n * 4);
	for (const Dot &d : cacheDots_)
	{
		const ImVec2 p0(origin.x + d.x, origin.y + d.y);
		const ImVec2 p1(p0.x + kCharW, p0.y + kDotH);
		dl->PrimRect(p0, p1, d.col);
	}
}

void MinimapView::interact(EditorViewState &view)
{
	if (!state || !layout || !layout->minimapVisible())
		return;
	const ImVec2 a = layout->minimapMin, b = layout->minimapMax;
	const float stripH = b.y - a.y;
	if (stripH <= 1.0f)
		return;
	auto req = [&](float y) { view.requestScroll(view.getScrollPosition().x, y); };

	if (dragging_)
	{
		if (!ImGui::IsMouseDown(ImGuiMouseButton_Left))
		{
			dragging_ = false;
			return;
		}
		if (dragRatio_ > 1e-6f)
			req(dragScroll0_ + (ImGui::GetIO().MousePos.y - dragY0_) / dragRatio_);
		return;
	}
	if (!ImGui::IsMouseHoveringRect(a, b, false))
		return;

	const Strip s = makeStrip(*state, *layout, view.getScrollPosition().y, stripH);
	if (ImGui::GetIO().MouseWheel != 0.0f && layout->lineHeight > 0.0f)
		req(view.getScrollPosition().y -
			ImGui::GetIO().MouseWheel * layout->lineHeight * 3.0f);

	if (ImGui::IsMouseClicked(ImGuiMouseButton_Left))
	{
		const float local = std::clamp(ImGui::GetIO().MousePos.y - a.y, 0.0f, stripH);
		const int line = (s.end < s.start)
							 ? 0
							 : std::clamp(s.start + int(local / kLineH), s.start, s.end);
		const float y = float(line) * layout->lineHeight - s.viewH * 0.5f;
		req(y);
		dragging_ = true;
		dragY0_ = ImGui::GetIO().MousePos.y;
		dragScroll0_ = y;
		dragRatio_ = s.ratio > 1e-6f ? s.ratio
									 : (s.maxScroll > 1.0f ? stripH / s.maxScroll : 0.0f);
	}
}

void MinimapView::draw(const EditorViewState &view) const
{
	if (!state || !highlight || !layout || !layout->minimapVisible())
		return;
	const float w = layout->minimapWidth, h = layout->minimapMax.y - layout->minimapMin.y;
	if (w <= 1.0f || h <= 1.0f)
		return;

	const ImVec2 a = layout->minimapMin;
	const Strip s = makeStrip(*state, *layout, view.getScrollPosition().y, h);
	if (s.end < s.start)
		return;

	ImGui::PushID(this);
	ImGui::SetCursorScreenPos(a);
	ImGui::InvisibleButton("##mm", ImVec2(w, h));
	ImDrawList *dl = ImGui::GetWindowDrawList();
	const int maxCols = std::max(1, int((w - 4.0f) / kCharW));
	const ImU32 defInk = dimInk(highlight->defaultTextColor());

	const CacheKey key{
		state->version, highlight->visualGeneration(), s.start, s.end, maxCols, defInk};
	if (!(key == cacheKey_))
		rebuildDensityCache(key);

	replayDensity(dl, a);

	if (s.sliderH > 0.0f)
	{
		const float sy0 = a.y + s.sliderTop, sy1 = std::min(a.y + h, sy0 + s.sliderH);
		dl->AddRectFilled(
			ImVec2(a.x, sy0), ImVec2(a.x + w, sy1), IM_COL32(180, 180, 220, 40));
		dl->AddRect(ImVec2(a.x + 0.5f, sy0),
					ImVec2(a.x + w - 0.5f, sy1),
					IM_COL32(220, 220, 255, 110));
	}
	ImGui::PopID();
}
