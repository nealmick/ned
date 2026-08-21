/*
	File: views/text_view.h
	Description: Draws document text, selection, whitespace guides, current line.
	Read-only: document, view state, highlight colors, layout metrics.
*/

#pragma once

#include "imgui.h"
#include <string>

class EditorState;
class EditorViewState;
class EditorHighlight;
struct ViewLayout;

class TextView
{
  public:
	TextView(const EditorState &document,
			 const EditorViewState &view,
			 const EditorHighlight &hl,
			 const ViewLayout &layoutMetrics)
		: state(&document), viewState(&view), highlight(&hl), layout(&layoutMetrics)
	{
	}

	void draw() const;

  private:
	const EditorState *state;
	const EditorViewState *viewState;
	const EditorHighlight *highlight;
	const ViewLayout *layout;

	static constexpr int TAB_SIZE = 4;
	static constexpr int VISIBLE_LINE_BUFFER = 2;
	static constexpr float CURRENT_LINE_X_OFFSET = 6.0f;
	static constexpr float WHITESPACE_GUIDE_Y_OFFSET = 2.0f;

	static const ImVec4 SELECTION_COLOR;
	static const ImVec4 CURRENT_LINE_COLOR;
	static const ImVec4 WHITESPACE_GUIDE_COLOR;

	static size_t advanceUtf8(const std::string &text, size_t index, size_t end);
	static float measureGlyphWidth(const char *start,
								   const char *end,
								   float draw_x,
								   float text_origin_x);

	void getVisibleLineRange(int &start_line, int &end_line) const;
	bool isSelected(int row, int col) const;
	void renderCurrentLineHighlight() const;
	// One rope copy per visible row: indent guides + glyphs + selection.
	void renderVisibleLines() const;
};
