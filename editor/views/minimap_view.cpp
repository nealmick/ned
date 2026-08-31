/*
	File: views/minimap_view.cpp
	Description: Density minimap — ImGui rects, click/drag/wheel.

	Density glyphs are expensive (lineInto + span walk + UTF-8 per char).
	Cache merged same-color runs keyed by doc version / highlight gen /
	visible line window; only rebuild when that key changes. Idle/scroll
	frames replay a handful of rects. Slider still updates live with scroll.
*/
#include "minimap_view.h"
#include "../editor_state.h"
#include "../editor_view_state.h"
#include "../services/highlight/highlight_service.h"
#include "view_layout.h"
#include "wrap_layout.h"
#include <algorithm>
#include <cmath>
#include <string>

namespace {

// Density rects read hotter than full glyphs — pull theme colors down a notch.
constexpr float kColorDim = 0.72f;

// Designed at a 20px font: 2px rows, 1px columns, 1.5px dots, 2px pad.
struct Density
{
	float lineH;
	float charW;
	float dotH;
	float padX;
	float sliderMin;
};

Density densityFromFont()
{
	const float fs = std::max(1.0f, ImGui::GetFontSize());
	Density d;
	d.lineH = std::max(1.0f, fs * 0.1f);
	d.charW = std::max(1.0f, fs * 0.05f);
	d.dotH = std::max(1.0f, d.lineH * 0.75f);
	d.padX = std::max(1.0f, fs * 0.1f);
	d.sliderMin = std::max(4.0f, fs * 0.2f);
	return d;
}

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

Strip makeStrip(const EditorState &st,
				const ViewLayout &lay,
				float scrollY,
				float stripH,
				const Density &d)
{
	Strip s;
	s.lineCount = st.lineCount();
	const float elh = lay.lineHeight;
	if (s.lineCount <= 0 || stripH <= 1.0f || elh <= 0.0f)
		return s;
	scrollY = std::max(0.0f, scrollY);
	s.viewH = std::max(elh, std::max(0.0f, lay.size.y - lay.editorTopMargin));
	s.maxScroll = std::max(0.0f, std::max(lay.totalHeight, s.viewH) - s.viewH);
	s.sliderH = std::clamp(std::floor((s.viewH / elh) * d.lineH), d.sliderMin, stripH);
	const float maxTop = std::min(
		stripH - s.sliderH, std::max(0.0f, float(s.lineCount) * d.lineH - s.sliderH));
	s.ratio = s.maxScroll > 1.0f ? maxTop / s.maxScroll : 0.0f;
	s.sliderTop = std::clamp(scrollY * s.ratio, 0.0f, maxTop);
	const int fit = std::max(1, int(stripH / d.lineH));
	if (s.lineCount <= fit)
	{
		s.end = s.lineCount - 1;
	} else
	{
		// Wrapped rows are >1 visual line — map the top scroll position to a row.
		const int firstVisible = lay.wrap ? lay.wrap->yToRow(scrollY / elh).row
										  : int(std::floor(scrollY / elh));
		s.start =
			std::clamp(int(firstVisible - s.sliderTop / d.lineH), 0, s.lineCount - fit);
		s.end = std::min(s.lineCount - 1, s.start + fit - 1);
	}
	return s;
}

} // namespace

void MinimapView::rebuildDensityCache(const CacheKey &key) const
{
	cacheRuns_.clear();
	if (!state || !highlight || key.end < key.start || key.maxCols <= 0)
	{
		cacheKey_ = key;
		return;
	}

	const int rows = key.end - key.start + 1;
	cacheRuns_.reserve(static_cast<size_t>(rows * 8));

	const Density d = densityFromFont();
	constexpr size_t kSlotN = static_cast<size_t>(ThemeSlot::Count);
	ImU32 slotInk[kSlotN];
	for (size_t i = 0; i < kSlotN; ++i)
		slotInk[i] = dimInk(highlight->colorForSlot(static_cast<ThemeSlot>(i)));

	static thread_local std::string line;
	// Density only paints maxCols columns; 4 bytes/col covers UTF-8.
	const size_t byteCap = static_cast<size_t>(key.maxCols) * 4 + 8;
	for (int row = key.start; row <= key.end; ++row)
	{
		const float y0 = float(row - key.start) * d.lineH;
		state->lineInto(row, line, byteCap);
		const auto &spans = highlight->spansForLine(row);
		size_t sp = 0;
		int runStart = -1;
		ImU32 runInk = 0;
		auto flush = [&](int col) {
			if (runStart < 0 || col <= runStart)
			{
				runStart = -1;
				return;
			}
			cacheRuns_.push_back(Run{d.padX + float(runStart) * d.charW,
									 y0,
									 float(col - runStart) * d.charW,
									 d.dotH,
									 runInk});
			runStart = -1;
		};
		int col = 0;
		for (int i = 0; i < (int)line.size() && col < key.maxCols;)
		{
			const int byte = i;
			const unsigned char c = (unsigned char)line[i++];
			if ((c & 0xC0) == 0x80)
				continue;
			if (c == '\t')
			{
				flush(col);
				col = std::min(key.maxCols, col + (4 - col % 4));
				continue;
			}
			if (c <= ' ')
			{
				flush(col);
				++col;
				continue;
			}
			while (sp < spans.size() && spans[sp].end <= byte)
				++sp;
			ImU32 ink = key.defInk;
			if (sp < spans.size() && spans[sp].start <= byte)
			{
				const auto slot = static_cast<size_t>(spans[sp].slot);
				ink = slot < kSlotN ? slotInk[slot] : key.defInk;
			}
			if (runStart < 0 || ink != runInk)
			{
				flush(col);
				runStart = col;
				runInk = ink;
			}
			++col;
			while (i < (int)line.size() && ((unsigned char)line[i] & 0xC0) == 0x80)
				++i;
		}
		flush(col);
	}
	cacheKey_ = key;
}

void MinimapView::replayDensity(ImDrawList *dl, ImVec2 origin) const
{
	const int n = static_cast<int>(cacheRuns_.size());
	if (n <= 0 || !dl)
		return;

	dl->PrimReserve(n * 6, n * 4);
	for (const Run &r : cacheRuns_)
	{
		const ImVec2 p0(origin.x + r.x, origin.y + r.y);
		const ImVec2 p1(p0.x + r.w, p0.y + r.h);
		dl->PrimRect(p0, p1, r.col);
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

	const Density d = densityFromFont();
	const Strip s = makeStrip(*state, *layout, view.getScrollPosition().y, stripH, d);
	if (ImGui::GetIO().MouseWheel != 0.0f && layout->lineHeight > 0.0f)
		req(view.getScrollPosition().y -
			ImGui::GetIO().MouseWheel * layout->lineHeight * 3.0f);

	if (ImGui::IsMouseClicked(ImGuiMouseButton_Left))
	{
		const float local = std::clamp(ImGui::GetIO().MousePos.y - a.y, 0.0f, stripH);
		const int line = (s.end < s.start)
							 ? 0
							 : std::clamp(s.start + int(local / d.lineH), s.start, s.end);
		const int visualLine =
			layout->wrap ? layout->wrap->rowStartVisualLine(line) : line;
		const float y = float(visualLine) * layout->lineHeight - s.viewH * 0.5f;
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
	const Density d = densityFromFont();
	const Strip s = makeStrip(*state, *layout, view.getScrollPosition().y, h, d);
	if (s.end < s.start)
		return;

	ImGui::PushID(this);
	ImGui::SetCursorScreenPos(a);
	ImGui::InvisibleButton("##mm", ImVec2(w, h));
	ImDrawList *dl = ImGui::GetWindowDrawList();
	const int maxCols = std::max(1, int((w - d.padX * 2.0f) / d.charW));
	const ImU32 defInk = dimInk(highlight->defaultTextColor());

	const CacheKey key{state->version,
					   highlight->visualGeneration(),
					   s.start,
					   s.end,
					   maxCols,
					   defInk,
					   ImGui::GetFontSize()};
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
