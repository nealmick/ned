/*
	File: editor.cpp
	Description: Thin composition root — graph bound entirely in the member init list.
	Backend-neutral: glyph metrics for the shared wrap layout are wired by
	each UI backend (ImGui: views/imgui/editor_surface; Qt: views/qt/editor_frame).
*/

#include "editor.h"

#include "../util/project_undo.h"
#include "../util/settings.h"

Editor::Editor(Settings &settings, std::string &projectRoot, ProjectUndo &projectUndo)
	: settings(settings),
	  projectRoot(projectRoot),
	  projectUndo(projectUndo),
	  api(*this),
	  operations(state),
	  viewState(state),
	  save(state, events),
	  commands(state, viewState, operations, projectUndo, events, save),
	  highlight(state, operations, &settings),
	  git(state, projectRoot, settings)
{
	events.clear();

	events.subscribeDidEdit([this](const EditorEvents::DidEdit &e) {
		highlight.highlightContent();
		save.onDidEdit();
		git.onDidEdit(e.firstRow, e.lastRow);
	});
}

void Editor::setContent(const std::string &raw)
{
	state.setFromString(raw);
	operations.clearPending();
	operations.bumpGeneration();
	highlight.resetForDocument(static_cast<size_t>(state.lineCount()));
	// View caches (content width, wrap) are invalidated by the backend
	// surface: it detects the version reset (reload) and full-invalidates.
}
