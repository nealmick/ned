/*
	File: editor.cpp
	Description: Thin composition root — graph bound entirely in the member init list.
*/

#include "editor.h"

#include "../util/icons.h"
#include "../util/project_undo.h"
#include "../util/settings.h"

Editor::Editor(Settings &settings,
			   std::string &projectRoot,
			   Icons &icons,
			   ProjectUndo &projectUndo)
	: settings(settings),
	  projectRoot(projectRoot),
	  icons(icons),
	  projectUndo(projectUndo),
	  api(*this),
	  operations(state),
	  viewState(state),
	  save(state, events),
	  commands(state, viewState, operations, projectUndo, events, save),
	  input(commands, viewState, state, projectUndo),
	  highlight(state, operations, settings),
	  git(state, projectRoot, settings),
	  frame(state, viewState, input, settings, git, highlight, icons, api),
	  lineJump(commands, input, settings, api),
	  finder(state, viewState, commands, input, settings, api)
{
	events.clear();

	events.subscribeDidEdit([this](const EditorEvents::DidEdit &e) {
		highlight.highlightContent();
		save.onDidEdit();
		git.onDidEdit(e.firstRow, e.lastRow);
		frame.noteContentEdit(e.firstRow, e.lastRow);
	});
}

void Editor::setContent(const std::string &raw)
{
	state.setFromString(raw);
	operations.clearPending();
	operations.bumpGeneration();
	highlight.resetForDocument(static_cast<size_t>(state.lineCount()));
	frame.invalidateContentWidth();
}

void Editor::renderEditor(ImFont *font, float editorWidth)
{
	// editorWidth <= 0 → fill the current window/region (docked panel mode).
	// Positive width → legacy side-by-side layout next to the file explorer.
	const bool fill = editorWidth <= 0.0f;
	if (!fill)
		ImGui::SameLine(0, 0);

	ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 0.0f);
	// Bordered children use WindowPadding; zero it so the title bar is flush
	// under the dock tab strip (no extra gap from default 8px padding).
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
	ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.2f, 0.2f, 0.2f, 0.0f));

	const ImVec2 childSize = fill ? ImVec2(0.0f, 0.0f) : ImVec2(editorWidth, -1.0f);
	ImGui::BeginChild("Editor", childSize, true);

	// Main-thread service ticks (before paint).
	highlight.poll();
	save.poll();
	git.poll();

	finder.update();
	lineJump.update();
	frame.run(font);

	ImGui::EndChild();

	ImGui::PopStyleColor();
	ImGui::PopStyleVar(2);
}
