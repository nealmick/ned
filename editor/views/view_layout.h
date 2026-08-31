/*
	File: views/view_layout.h
	Description: Per-frame layout metrics for the editor pane (view-layer POD,
	not a paint leaf). Owned by EditorFrame; read const by input, view state,
	paint views, and overlays.
*/

#pragma once

#include "imgui.h"

class WrapLayout;

struct ViewLayout
{
	// Outer "Editor" child window (screen space).
	ImVec2 panePos{};
	ImVec2 paneSize{};

	// Full content region under the title bar (gutter + document + minimap).
	float lineHeight = 0.0f;
	ImVec2 size{}; // GetContentRegionAvail at metrics time
	float totalHeight = 0.0f;
	float editorTopMargin = 0.0f;
	float textLeftMargin = 0.0f;
	ImVec2 textPos{}; // screen origin of first glyph in the document child
	bool rainbowMode = true;

	// Soft wrap (settings "word_wrap"). wrap != null ⇒ wrap mode this frame;
	// consumers map rows/coordinates through it instead of row * lineHeight.
	bool wordWrap = false;
	const WrapLayout *wrap = nullptr;

	// Minimap strip (0 width = hidden). Screen-space AABB set before input/draw.
	float minimapWidth = 0.0f;
	ImVec2 minimapMin{};
	ImVec2 minimapMax{};

	bool minimapVisible() const { return minimapWidth > 0.5f; }
};
