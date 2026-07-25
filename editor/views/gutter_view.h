/*
	File: views/gutter_view.h
	Description: Line-number gutter; reads git line-edit status (const service leaf).
*/

#pragma once

#include "imgui.h"

class EditorState;
class EditorViewState;
class EditorGit;
struct ViewLayout;

class GutterView
{
  public:
	float lineNumberWidth = 0.0f;
	ImVec2 lineNumbersPos{};

	GutterView(const EditorState &document,
			   const EditorViewState &view,
			   const EditorGit &gitService,
			   const ViewLayout &layoutMetrics)
		: state(&document), viewState(&view), git(&gitService), layout(&layoutMetrics)
	{
	}

	void renderLineNumbers() const;
	ImVec2 createLineNumbersPanel();

  private:
	const EditorState *state;
	const EditorViewState *viewState;
	const EditorGit *git;
	const ViewLayout *layout;

	static constexpr ImU32 DEFAULT_LINE_NUMBER_COLOR = IM_COL32(128, 128, 128, 150);
	static constexpr ImU32 CURRENT_LINE_COLOR = IM_COL32(255, 255, 255, 255);
	static constexpr int LINE_NUMBER_BUFFER_SIZE = 32;

	void calculateSelectionLines(int &selectionStartLine, int &selectionEndLine) const;
	float calculateTextRightAlignedPosition(const char *text, float lineNumberWidth) const;
	float calculateRequiredLineNumberWidth() const;
};
