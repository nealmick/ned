/*
	File: editor_frame.h
	Description: Document presentation frame — title bar, layout metrics,
	ImGui child, input, scroll, text/gutter/caret views.
*/

#pragma once
#include "imgui.h"

#include "views/caret_view.h"
#include "views/gutter_view.h"
#include "views/text_view.h"
#include "views/title_bar_view.h"
#include "views/view_layout.h"

struct ImFont;
class EditorViewState;
class EditorInput;
class EditorState;
class EditorApi;
class EditorGit;
class EditorHighlight;
class Icons;
class Settings;

class EditorFrame
{
  public:
	// Per-frame metrics; consumers hold const ViewLayout*.
	ViewLayout layout;

	TitleBarView titleBar;
	TextView textView;
	GutterView gutter;
	CaretView caret;

	EditorFrame(EditorState &document,
				EditorViewState &view,
				EditorInput &editorInput,
				Settings &appSettings,
				EditorGit &gitService,
				EditorHighlight &hl,
				Icons &iconSet,
				EditorApi &api);

	// Full document pass: title bar → layout → focus → input → scroll → draw.
	void run(ImFont *font);

	// Longest-line scroll width (view cache). Composition root only.
	void invalidateContentWidth() { widthFull = true; }
	void noteContentEdit(int firstRow, int lastRow);

  private:
	EditorViewState *viewState;
	EditorInput *input;
	EditorState *state;
	Settings *settings;

	static constexpr const char *EDITOR_CHILD_ID = "##editor";
	static constexpr int LINE_NUMBER_DIGITS = 4;
	static constexpr float LINE_NUMBER_PAD = 8.0f;
	static constexpr float EDITOR_TOP_MARGIN = 2.0f;
	static constexpr float TEXT_LEFT_MARGIN = 7.0f;
	static constexpr float SCROLL_WIDTH_FONT_MUL = 10.0f;
	static constexpr float GIT_CHANGES_MIN_WIDTH = 250.0f;

	void drawTitleBar(ImFont *font);
	void updateLayoutMetrics();
	void beginDocumentChild();
	void updateFocusPolicy();
	void drawDocument();
	float contentWidth();

	// Longest-line width cache (widthDirtyHi < widthDirtyLo => no pending dirty).
	// widthLongest < 0 means which row holds widthMax is unknown; widthMax may be a
	// slight overestimate (safe for horizontal scroll).
	bool widthFull = true;
	float widthMax = 0.0f, widthPad = 0.0f, widthFont = 0.0f;
	int widthLongest = -1, widthLines = 0, widthDirtyLo = 0, widthDirtyHi = -1;

	void recomputeWidthPad();
	void shiftLongestForLineDelta(int dirtyLo, int lineDelta);

	// Per-frame instance (must NOT be static — multi-tab embed has many frames).
	bool wasEditorFocused = false;
};
