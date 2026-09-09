/*
	File: editor_surface.h
	Description: ImGui presentation wrapper around a core Editor. Owns the
	frame, input and overlays (line-jump / find) that used to live on Editor,
	plus the presentation queries that used to live on EditorApi (layout,
	caret pixel pos, tooltip arbitration, hover trigger, diagnostics). The
	Editor itself stays toolkit-neutral (ned_core).
*/

#pragma once
#include "imgui.h"

#include "../../editor_api.h"
#include "../view_layout.h"
#include "editor_frame.h"
#include "editor_input.h"
#include "find_bar.h"
#include "line_jump.h"

struct ImFont;
class Icons;
class LSPDiagnostics;

class EditorSurface
{
  public:
	EditorSurface(Editor &editor, Icons &icons);

	// One full pass (what Editor::renderEditor used to do): service ticks,
	// overlays, document frame. editorWidth <= 0 → fill the current window/
	// region (docked panel mode); positive → legacy side-by-side layout.
	void render(ImFont *font, float editorWidth);

	// --- Presentation queries (moved off EditorApi; ImGui views use them) ---
	const ViewLayout &layout() const { return frame.layout; }
	float caretScreenX() const
	{
		return frame.caretView.caretScreenX(frame.layout.textPos);
	}
	// One shared ImGui tooltip per frame; first claimer owns it.
	bool claimTooltip() { return frame.claimTooltip(); }
	const HoverTrigger::Info &hoverInfo() const { return frame.hoverInfo(); }
	// True when this frame carried a dismissal signal (key/click/scroll/block).
	bool hoverDismissed() const { return frame.hoverDismissed(); }
	void setDiagnostics(const LSPDiagnostics *store) { frame.setDiagnostics(store); }

	// Dismiss the editor-owned overlays (line-jump + find). The exclusive-
	// overlay event drives this automatically; the shell can also force it.
	void closeOverlays();

	EditorInput input;
	EditorFrame frame;
	LineJump lineJump;
	FindBar finder;

  private:
	Editor &editor;

	// state.version only ever increases — except a document (re)load resets
	// it to 0. That reset is the signal to full-invalidate view caches
	// (content width, wrap) the way Editor::setContent used to do directly.
	int seenVersion = 0;
};
