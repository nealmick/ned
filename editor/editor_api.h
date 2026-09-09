/*
	File: editor_api.h
	Description: Sole external interface to the editor (files, LSP, shell).

	Friend of Editor — only type that may reach private subsystems.
	Caret moves and save go through EditorCommands.
	Document lifecycle is composed here (not on Commands).
	Toolkit-neutral: presentation queries (layout, caret pixel position,
	tooltip arbitration, hover trigger, diagnostics binding) live on the
	per-backend surfaces (ImGui: views/imgui/editor_surface.h).
*/

#pragma once

#include "editor_events.h"
#include "platform/lsp_editor.h"
#include "platform/ned_types.h"
#include "services/highlight/capture_map.h"
#include <string>

class Editor;

class EditorApi : public LSPEditor
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
	// Per-editor project hooks (git). Undo load is ProjectUndo at the shell.
	void onProjectOpened(const std::string &root);

	// --- Document / caret queries (const reads) ---
	// The six below override the LSPEditor seam (lsp core path), as does
	// requestCursorCenter further down.
	const std::string &path() const override;
	bool hasPath() const;
	std::string text() const;
	std::string line(int row) const override;
	int version() const;
	const std::string &languageId() const override;
	NedColor defaultTextColor() const override;
	NedColor syntaxColor(ThemeSlot slot) const override;
	void getCaret(int &row, int &column) const override;

	// --- Navigation actions (via Commands) ---
	void resetCaret();
	void requestEnsureVisible();
	// Deferred center after layout exists (e.g. post file-load). View schedule, not setCursor.
	void requestCursorCenter(int row, int column) override;

	// --- Shell / overlays ---
	// Next editor paint: focus document child so keys/text work without a click.
	void requestFocus();
	void setBlockInput(bool blocked);
	bool isBlockInput() const;
	// Mutual exclusion: emit so root closes shell overlays; the backend
	// surface dismisses the editor-owned siblings (line-jump / find).
	// keep == None closes every overlay.
	void requestExclusiveOverlay(EditorEvents::DidRequestExclusiveOverlay::Keep keep);
	void closeAllOverlays();

	// --- Persist / theme / git ---
	void save();
	void forceColorUpdate();
	bool isFileModified(const std::string &filePath) const;

	// --- Events (composition root subscribes) ---
	EditorEvents &events();

  private:
	Editor &editor;
};
