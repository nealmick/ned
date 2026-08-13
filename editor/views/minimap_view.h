/*
	File: views/minimap_view.h
	Description: Density strip + viewport slider.
*/
#pragma once

#include "imgui.h"
#include <cstdint>
#include <vector>

class EditorState;
class EditorViewState;
class EditorHighlight;
struct ViewLayout;

class MinimapView
{
  public:
	// 80px / 320px at a 20px font — width follows GetFontSize().
	static constexpr float kWidthFontMul = 4.0f;
	static constexpr float kMinPaneFontMul = 16.0f;

	MinimapView(const EditorState &document,
				const EditorHighlight &hl,
				const ViewLayout &layoutMetrics)
		: state(&document), highlight(&hl), layout(&layoutMetrics)
	{
	}

	void interact(EditorViewState &view);
	void draw(const EditorViewState &view) const;

  private:
	// One density pixel, positions relative to strip top-left (content origin).
	struct Dot
	{
		float x = 0.0f;
		float y = 0.0f;
		ImU32 col = 0;
	};

	// Key for the cached density strip (slider is always drawn live).
	struct CacheKey
	{
		int docVersion = -1;
		uint64_t hlGen = 0;
		int start = -1;
		int end = -1;
		int maxCols = -1;
		ImU32 defInk = 0;
		float fontSize = 0.0f;

		bool operator==(const CacheKey &o) const
		{
			return docVersion == o.docVersion && hlGen == o.hlGen && start == o.start &&
				   end == o.end && maxCols == o.maxCols && defInk == o.defInk &&
				   fontSize == o.fontSize;
		}
	};

	void rebuildDensityCache(const CacheKey &key) const;
	void replayDensity(ImDrawList *dl, ImVec2 origin) const;

	const EditorState *state;
	const EditorHighlight *highlight;
	const ViewLayout *layout;
	bool dragging_ = false;
	float dragY0_ = 0.0f, dragScroll0_ = 0.0f, dragRatio_ = 0.0f;

	// draw() is const (view API); cache is an implementation detail.
	mutable CacheKey cacheKey_{};
	mutable std::vector<Dot> cacheDots_;
};
