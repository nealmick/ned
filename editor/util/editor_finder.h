#pragma once

#include "imgui.h"
#include <string>
#include <vector>

class EditorApi;
class EditorState;
class EditorViewState;
class EditorCommands;
class EditorInput;
class Settings;

// In-buffer find (Cmd/Ctrl+F). One host call per frame: update().
class EditorFinder
{
  public:
	EditorFinder(EditorState &document,
				 EditorViewState &view,
				 EditorCommands &cmds,
				 EditorInput &editorInput,
				 Settings &appSettings,
				 EditorApi &editorApi)
		: state(&document),
		  viewState(&view),
		  commands(&cmds),
		  input(&editorInput),
		  settings(&appSettings),
		  api(&editorApi)
	{
	}

	bool active = false;

	// Per-frame: keys, viewState.blockInput, UI if open. Call before frame.run()
	// so document input sees an up-to-date block.
	void update();

	// Force closed (e.g. exclusive-overlay request).
	void dismiss();

  private:
	EditorState *state;
	EditorViewState *viewState;
	EditorCommands *commands;
	EditorInput *input;
	Settings *settings;
	EditorApi *api;

	static constexpr size_t INPUT_CAP = 256;

	struct Match
	{
		int row = 0;
		int column = 0;
	};

	std::string findText;
	char inputBuffer[INPUT_CAP] = {};
	bool ignoreCase = true;
	bool shouldFocus = false;

	// Reused across rebuildMatches to avoid per-line allocations.
	std::string lineScratch;
	std::string hayScratch;

	std::vector<Match> matches;
	bool matchesDirty = true;
	int matchIndex = -1;

	ImVec2 boxMin{}, boxMax{};
	bool boxRectValid = false;
	// After close, keep block one more frame so Escape does not hit the doc.
	bool releaseBlockNextFrame = false;

	void open();
	void close();
	void syncInputBlock();
	void pollOpenCloseKeys();
	void draw();

	void setQuery(const std::string &query);
	void rebuildMatches();
	void selectMatch(int index, bool highlight);
	void stepMatch(int direction);
	// Ctrl/Cmd+Enter: selection on every match; primary near previous caret.
	void selectAllMatches();
	void handleEnterShortcuts();
};
