#include "editor_surface.h"

#include "../../editor.h"
#include "../wrap_layout.h"
#include "editor_utils.h"

EditorSurface::EditorSurface(Editor &editor, Icons &icons)
	: editor(editor),
	  input(editor.commands, editor.viewState, editor.state, editor.projectUndo),
	  frame(editor.state,
			editor.viewState,
			input,
			editor.settings,
			editor.git,
			editor.highlight,
			icons),
	  lineJump(editor.commands, input, editor.settings, editor.api),
	  finder(editor.state,
			 editor.viewState,
			 editor.commands,
			 input,
			 editor.settings,
			 editor.api)
{
	// Backend glyph metrics for the shared wrap layout (ImFont advances).
	WrapLayout::setGlyphWidthFn(
		[](const char *s, const char *e) { return EditorUtils::glyphAdvance(s, e); });
	WrapLayout::setSpaceWidthFn(
		[](const char *, const char *) { return EditorUtils::spaceWidth(); });

	// Dirty-span fan-out to the view caches (what the composition root used
	// to wire directly into the frame it owned).
	editor.api.events().subscribeDidEdit([this](const EditorEvents::DidEdit &e) {
		frame.noteContentEdit(e.firstRow, e.lastRow);
	});

	// Overlay mutual exclusion: the api emits, the surface owns the
	// editor-side overlays and dismisses the siblings not being kept.
	editor.api.events().subscribeDidRequestExclusiveOverlay(
		[this](const EditorEvents::DidRequestExclusiveOverlay &e) {
			using Keep = EditorEvents::DidRequestExclusiveOverlay::Keep;
			if (e.keep != Keep::LineJump)
				lineJump.dismiss();
			if (e.keep != Keep::Find)
				finder.dismiss();
		});
}

void EditorSurface::closeOverlays()
{
	lineJump.dismiss();
	finder.dismiss();
}

void EditorSurface::render(ImFont *font, float editorWidth)
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
	editor.highlight.poll();
	editor.save.poll();
	editor.git.poll();

	// Document reload (setContent) resets state.version to 0 — the only
	// direction it can decrease. Full-invalidate the width/wrap caches.
	if (editor.state.version < seenVersion)
		frame.invalidateContentWidth();
	seenVersion = editor.state.version;

	finder.update();
	lineJump.update();
	frame.run(font);

	ImGui::EndChild();

	ImGui::PopStyleColor();
	ImGui::PopStyleVar(2);
}
