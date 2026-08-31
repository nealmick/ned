/*
	File: editor_input.h
	Description: Keyboard + mouse → EditorCommands. No document mutation logic.
	Overlays do not live here: they set viewState.blockInput / suppressNextEnter
	and handle their own keys.
*/

#pragma once

#include "imgui.h"
#include <utility>

class EditorCommands;
class EditorState;
class EditorViewState;
class ProjectUndo;
struct ViewLayout;

class EditorInput
{
  public:
	// Set by overlays that confirm with Enter so the same key does not insert a newline.
	bool suppressNextEnter = false;

	EditorInput(EditorCommands &cmds,
				EditorViewState &view,
				EditorState &document,
				ProjectUndo &undo)
		: commands(&cmds), viewState(&view), state(&document), projectUndo(&undo)
	{
	}

	// Wired by EditorFrame (owns layout).
	void setLayout(const ViewLayout &layoutMetrics) { layout = &layoutMetrics; }

	// Per-frame: mouse (if hovered) + keyboard when not blocked.
	void process();

  private:
	EditorCommands *commands;
	EditorViewState *viewState;
	EditorState *state;
	ProjectUndo *projectUndo;
	const ViewLayout *layout = nullptr;

	// Mouse drag state
	bool isDragging = false;
	int anchorRow = -1;
	int anchorColumn = -1;

	// Right-click context menu (opened at the click position).
	static constexpr const char *kContextMenuId = "ned_editor_context_menu";

	void processMouse();
	void renderContextMenu();
	void processKeyboard();
	void processTextInput();
	void processCharacterInput();
	void processPrimaryShortcuts();

	void handleMouseClick(int row, int column);
	void handleMouseDrag(int row, int column);
	void handleMouseRelease();
	std::pair<int, int> rowColFromMouse() const;
};
