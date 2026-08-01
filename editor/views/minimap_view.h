/*
	File: views/minimap_view.h
	Description: Density strip + viewport slider.
*/
#pragma once

class EditorState;
class EditorViewState;
class EditorHighlight;
struct ViewLayout;

class MinimapView
{
  public:
	static constexpr float kWidth = 80.0f;
	static constexpr float kMinPaneWidth = 320.0f;

	MinimapView(const EditorState &document,
				const EditorHighlight &hl,
				const ViewLayout &layoutMetrics)
		: state(&document), highlight(&hl), layout(&layoutMetrics)
	{
	}

	void interact(EditorViewState &view);
	void draw(const EditorViewState &view) const;

  private:
	const EditorState *state;
	const EditorHighlight *highlight;
	const ViewLayout *layout;
	bool dragging_ = false;
	float dragY0_ = 0.0f, dragScroll0_ = 0.0f, dragRatio_ = 0.0f;
};
