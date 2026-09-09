/*
	File: views/imgui/caret_view.h
	Description: Draws the caret. Read-only view state + layout metrics.
*/

#pragma once

#include "../../platform/ned_types.h"
#include "imgui.h"

class EditorViewState;
struct ViewLayout;

class CaretView
{
  public:
	CaretView(const EditorViewState &view, const ViewLayout &layoutMetrics)
		: viewState(&view), layout(&layoutMetrics)
	{
	}

	void paint() const;

	// Screen X of caret given layout text origin (used by LSP hover placement).
	float caretScreenX(const NedVec2 &textPos) const;

  private:
	const EditorViewState *viewState;
	const ViewLayout *layout;
};
