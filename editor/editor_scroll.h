#pragma once
#include "editor_types.h"
#include "imgui.h"
#include <string>

// Forward declarations
class Editor;
extern Editor gEditor;

class EditorScroll
{
  public:
	EditorScroll();
	~EditorScroll() = default;

	// Main scroll animation update function
	void updateScrollAnimation();

	// Mouse wheel handling
	void processMouseWheelForEditor();
	void processMouseWheelScrolling();

	// Cursor visibility functions
	ScrollChange ensureCursorVisible();

	void adjustScrollForCursorVisibility();

	// Direct scroll request handling
	void requestScroll(float x, float y)
	{
		requestedScrollX = x;
		requestedScrollY = y;
		hasScrollRequest = true;
	}

	bool handleScrollRequest(float &outScrollX, float &outScrollY)
	{
		if (hasScrollRequest)
		{
			outScrollX = requestedScrollX;
			outScrollY = requestedScrollY;
			hasScrollRequest = false;
			return true;
		}
		return false;
	}

	// New scroll position accessors (to replace direct access to EditorState)
	ImVec2 getScrollPosition() const { return scrollPos; }
	void setScrollPosition(const ImVec2 &pos) { scrollPos = pos; }
	void setScrollX(float x) { scrollX = x; }

	// Access methods for cursor visibility frames
	int getEnsureCursorVisibleFrames() const { return ensureCursorVisibleFrames; }
	void setEnsureCursorVisibleFrames(int frames) { ensureCursorVisibleFrames = frames; }
	void decrementEnsureCursorVisibleFrames()
	{
		if (ensureCursorVisibleFrames > 0)
			ensureCursorVisibleFrames--;
	}

	void handleCursorMovementScroll();
	void centerCursorVertically();

	// Add pending cursor centering state
	bool pending_cursor_centering = false;
	int pending_cursor_line = -1;
	int pending_cursor_char = -1;

	// Helpers
	float calculateCursorXPosition();

	// Helper that doesn't need direct access to Editor (to avoid circular
	// dependency)
	int getLineFromPosition(const std::vector<int> &line_starts, int pos);

	// Check if scroll animation is currently active
	bool isScrollAnimationActive() const;

	// Scroll state (moved from EditorState)
	ImVec2 scrollPos = ImVec2(0, 0);	  // Replaces state.scroll_pos
	float scrollX = 0.0f;				  // Replaces state.scroll_x
	int ensureCursorVisibleFrames = 0;	  // Replaces state.ensure_cursor_visible_frames
	ScrollAnimationState scrollAnimation; // Replaces state.scroll_animation

	// Direct scroll request state
	float requestedScrollX = 0;
	float requestedScrollY = 0;
	bool hasScrollRequest = false;

	// Extra state variables for bookmark/specific scrolling
	bool pendingBookmarkScroll = false;
	float pendingScrollX = 0.0f;
	float pendingScrollY = 0.0f;

  private:
};

// Global instance
extern EditorScroll gEditorScroll;