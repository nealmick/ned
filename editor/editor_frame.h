/*
	File: editor_frame.h
	Description: Document presentation frame — title bar, layout metrics,
	ImGui child, input, scroll, text/gutter/caret views.
*/

#pragma once
#include "imgui.h"

#include "views/caret_view.h"
#include "views/gutter_view.h"
#include "views/hover_tooltip.h"
#include "views/hover_trigger.h"
#include "views/minimap_view.h"
#include "views/text_view.h"
#include "views/title_bar_view.h"
#include "views/view_layout.h"

struct ImFont;
class EditorViewState;
class EditorInput;
class EditorState;
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
	MinimapView minimap;
	CaretView caret;

	EditorFrame(EditorState &document,
				EditorViewState &view,
				EditorInput &editorInput,
				Settings &appSettings,
				EditorGit &gitService,
				EditorHighlight &hl,
				Icons &iconSet);

	void setDiagnostics(const class LSPDiagnostics *store);

	// One shared ImGui tooltip per frame; first claimer (gutter marks, squiggle
	// hover, symbol hover) owns it. Exposed to the shell via EditorApi.
	bool claimTooltip() { return tooltipArbiter.claim(); }

	// Transient hover trigger (VSCode-style, shared by all hover consumers).
	// Updated each run(); views and the shell read the frozen target.
	const HoverTrigger::Info &hoverInfo() const { return hoverTrigger.info(); }
	// True when this frame carried a dismissal signal (key/click/scroll/block).
	bool hoverDismissed() const { return frameDismissed; }

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
	static constexpr float SCROLL_WIDTH_FONT_MUL = 10.0f;

	void drawTitleBar(ImFont *font);
	void updateLayoutMetrics();
	void beginDocumentChild();
	void updateFocusPolicy();
	void drawDocument();
	// Zone/row/column under the mouse right now (Text or Gutter), for the trigger.
	HoverTrigger::Target hoverHitTest() const;
	void updateHoverTrigger();
	bool frameDismissed = false;
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
	TooltipArbiter tooltipArbiter;
	HoverTrigger hoverTrigger;
	ImVec2 lastMousePos{0.0f, 0.0f};
	ImVec2 lastScroll{0.0f, 0.0f};
};
