/*
	File: views/view_layout.h
	Description: Per-frame layout metrics for the editor pane (view-layer POD,
	not a paint leaf). Owned by EditorFrame; read const by input, view state,
	paint views, and overlays.
*/

#pragma once

#include "imgui.h"

struct ViewLayout
{
	// Outer "Editor" child window (screen space). Updated each frame by Editor.
	ImVec2 panePos{};
	ImVec2 paneSize{};

	// Content-region metrics updated in setupEditorDisplay (previous-frame
	// scroll geometry is OK for hit-testing and draw).
	float lineHeight = 0.0f;
	ImVec2 size{}; // ImGui content region available for text+gutter
	float totalHeight = 0.0f;
	float editorTopMargin = 0.0f;
	float textLeftMargin = 0.0f;
	ImVec2 textPos{}; // screen origin of first glyph
	bool rainbowMode = true;
};
