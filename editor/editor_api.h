/*
	File: editor_api.h
	Description: Sole external interface to the editor (files, LSP, shell).

	Friend of Editor — only type that may reach private subsystems.
	Caret moves and save go through EditorCommands.
	Document lifecycle is composed here (not on Commands).
*/

#pragma once

#include "editor_events.h"
#include "views/view_layout.h"
#include <string>

class Editor;

class EditorApi
{
  public:
	explicit EditorApi(Editor &editor);

	// --- Document lifecycle (orchestration; not Commands) ---
	// Save current, reset caret, cancel highlight — before reading a new path.
	void prepareLoad();
	// Install buffer at path: content, path, language, undo stack, re-highlight.
	void openDocument(const std::string &path, const std::string &raw);
	// Failed open: show message, clear path, clear highlight colors.
	void failOpen(const std::string &message);
	// Replace buffer only (no path). Prefer openDocument for real files.
	void setContent(const std::string &raw);
	// Per-editor project hooks (git). Undo load is ProjectUndo at the shell.
	void onProjectOpened(const std::string &root);

	// --- Document / caret queries (const reads) ---
	const std::string &path() const;
	bool hasPath() const;
	std::string text() const;
	int version() const;
	const std::string &languageId() const;
	void getCaret(int &row, int &column) const;

	// --- Navigation actions (via Commands) ---
	void resetCaret();
	void restoreCaret(int row, int column);
	void centerOn(int row, int column);
	void requestEnsureVisible();
	// Deferred center after layout exists (e.g. post file-load). View schedule, not setCursor.
	void requestCursorCenter(int row, int column);

	// --- Shell / overlays ---
	// Next editor paint: focus document child so keys/text work without a click.
	void requestFocus();
	void setBlockInput(bool blocked);
	bool isBlockInput() const;
	void dismissLineJump();
	void dismissFinder();
	// Mutual exclusion: emit so root closes shell overlays; dismiss editor siblings.
	// keep == None closes every overlay.
	void requestExclusiveOverlay(EditorEvents::DidRequestExclusiveOverlay::Keep keep);
	void closeAllOverlays();

	// --- Persist / theme / git ---
	void save();
	void forceColorUpdate();
	bool isFileModified(const std::string &filePath) const;

	// --- Layout (LSP UI / embedded popups) ---
	const ViewLayout &layout() const;
	float caretScreenX() const;

	// --- Events (composition root subscribes) ---
	EditorEvents &events();

  private:
	Editor &editor;
};
