#include "editor_api.h"
#include "editor.h"
#include "util/project_undo.h"

EditorApi::EditorApi(Editor &editor) : editor(editor) {}

// ---------------------------------------------------------------------------
// Document lifecycle — compose services; caret via Commands.
// ---------------------------------------------------------------------------

void EditorApi::prepareLoad()
{
	editor.commands.save();
	resetCaret();
	editor.highlight.cancelHighlighting();
}

void EditorApi::openDocument(const std::string &path, const std::string &raw)
{
	editor.setContent(raw);
	editor.state.path = path;
	editor.state.languageId = EditorState::languageIdFromPath(path);
	resetCaret();
	editor.projectUndo.ensureFile(path);
	editor.highlight.highlightContent();
	editor.git.onDocumentOpened();
}

void EditorApi::failOpen(const std::string &message)
{
	editor.setContent(message);
	editor.state.path.clear();
	editor.state.languageId.clear();
	editor.highlight.clear();
}

void EditorApi::onProjectOpened(const std::string &root)
{
	// Undo is project-scoped (ProjectUndo::loadProject) — load once at the shell.
	(void)root;
	editor.git.init();
}

// ---------------------------------------------------------------------------
// Queries
// ---------------------------------------------------------------------------

const std::string &EditorApi::path() const { return editor.state.path; }

bool EditorApi::hasPath() const { return !editor.state.path.empty(); }

std::string EditorApi::text() const { return editor.state.join(); }

std::string EditorApi::line(int row) const { return editor.state.line(row); }

int EditorApi::version() const { return editor.state.version; }

const std::string &EditorApi::languageId() const { return editor.state.languageId; }

NedColor EditorApi::defaultTextColor() const
{
	return editor.highlight.defaultTextColor();
}

NedColor EditorApi::syntaxColor(ThemeSlot slot) const
{
	return editor.highlight.colorForSlot(slot);
}

void EditorApi::getCaret(int &row, int &column) const
{
	row = editor.viewState.row;
	column = editor.viewState.column;
}

// ---------------------------------------------------------------------------
// Navigation — same path as input / find / line-jump.
// ---------------------------------------------------------------------------

void EditorApi::resetCaret()
{
	editor.commands.setCursor(0, 0, false, EditorCommands::CursorReveal::ensure);
}

void EditorApi::requestEnsureVisible() { editor.commands.requestEnsureVisible(); }

void EditorApi::requestCursorCenter(int row, int column)
{
	// Deferred until updateScroll has layout (post file-load goto).
	editor.viewState.requestCursorCenter(row, column);
}

// ---------------------------------------------------------------------------
// Shell / overlays
// ---------------------------------------------------------------------------

void EditorApi::requestFocus() { editor.viewState.requestFocus = true; }

void EditorApi::setBlockInput(bool blocked) { editor.viewState.blockInput = blocked; }

bool EditorApi::isBlockInput() const { return editor.viewState.blockInput; }

void EditorApi::requestExclusiveOverlay(EditorEvents::DidRequestExclusiveOverlay::Keep keep)
{
	// The backend surface (owner of line-jump / find) subscribes and dismisses
	// the editor-side siblings not being kept; the shell closes its overlays.
	editor.events.emitDidRequestExclusiveOverlay({keep});
}

void EditorApi::closeAllOverlays()
{
	requestExclusiveOverlay(EditorEvents::DidRequestExclusiveOverlay::Keep::None);
}

// ---------------------------------------------------------------------------
// Persist / theme / git / events
// ---------------------------------------------------------------------------

void EditorApi::save() { editor.commands.save(); }

void EditorApi::forceColorUpdate() { editor.highlight.forceColorUpdate(); }

bool EditorApi::isFileModified(const std::string &filePath) const
{
	return editor.git.isFileModified(filePath);
}

EditorEvents &EditorApi::events() { return editor.events; }
