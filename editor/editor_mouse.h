#pragma once
#include "editor_types.h"
#include "imgui.h"
#include <string>
#include <vector>

// Forward declarations
class Editor;
extern Editor gEditor;

class EditorMouse
{
  public:
	EditorMouse();
	~EditorMouse() = default;

	// Main mouse handling function
	void handleMouseInput();

	// Character position calculation from mouse coordinates
	int getCharIndexFromCoords();

	// Context menu handler
	void handleContextMenu();

  private:
	// Selection state
	bool is_dragging;
	int anchor_pos;
	bool show_context_menu;
	ImVec2 context_menu_pos;

	// Handle different mouse actions
	void handleMouseClick(int char_index);
	void handleMouseDrag(int char_index);
	void handleMouseRelease();
};

// Global instance
extern EditorMouse gEditorMouse;