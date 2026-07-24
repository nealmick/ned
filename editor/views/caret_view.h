/*
	File: views/caret_view.h
	Description: Draws the caret. Read-only view state + layout metrics.
*/

#pragma once

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

	void draw() const;

	// Screen X of caret given layout text origin (used by LSP hover placement).
	float caretScreenX(const ImVec2 &textPos) const;

  private:
	const EditorViewState *viewState;
	const ViewLayout *layout;
};
