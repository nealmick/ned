/*
	editor/views/imgui/line_jump.h
	Go-to-line overlay (keybind → number input → jump + center cursor).
*/

#pragma once
#include "../../../util/keybinds.h"
#include "../../../util/settings.h"
#include "imgui.h"

#include <string>

class EditorApi;
class EditorInput;
class EditorCommands;
class Settings;

class LineJump
{
  public:
	bool showLineJumpWindow = false;

	LineJump(EditorCommands &cmds,
			 EditorInput &editorInput,
			 Settings &appSettings,
			 EditorApi &editorApi)
		: commands(&cmds), input(&editorInput), settings(&appSettings), api(&editorApi)
	{
	}

	// Helper: Render window header (setup and title)
	void renderHeader();

	// Helper: Render the input box and force keyboard focus
	bool renderInput();

	// Per-frame: keys, block input, UI if open. Call before frame.run().
	void update();

	void jumpToLine(int lineNumber);
	void dismiss();

  private:
	EditorCommands *commands;
	EditorInput *input;
	Settings *settings;
	EditorApi *api;
	char lineNumberBuffer[32] = "";
	bool wasKeyboardFocusSet = false;
};
