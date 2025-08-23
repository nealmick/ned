#include "lsp_goto_def.h"
#include "../editor/editor.h"
#include "../files/files.h"
#include "../lsp/lsp_client.h"
#include <iostream>

// Global instance
LSPGotoDef gLSPGotoDef;

LSPGotoDef::LSPGotoDef() : show(false), pending(false) {}

LSPGotoDef::~LSPGotoDef() = default;

void LSPGotoDef::get()
{
	if (!gLSPClient.isInitialized())
		return;

	// Get current cursor position
	int row = gEditor.getLineFromPos(editor_state.cursor_index);
	int line_start = editor_state.editor_content_lines[row];
	int column = editor_state.cursor_index - line_start;

	// Set pending state
	pending = true;

	// TODO: Add request logic here later
	// For now just show placeholder
	gotoInfo = "Goto definition placeholder";
	pending = false;
	show = true;

	std::cout << "LSP GotoDef: Requested for line " << row << " column " << column
			  << std::endl;
}

void LSPGotoDef::render()
{
	if (!show)
		return;

	ImGui::SetNextWindowSize(ImVec2(300, 100), ImGuiCond_FirstUseEver);

	if (ImGui::Begin("Goto Definition", &show))
	{
		ImGui::Text("Rendering goto def");

		if (pending)
		{
			ImGui::Text("Loading goto definition...");
		} else if (gotoInfo.empty())
		{
			ImGui::Text("No goto definition available");
		} else
		{
			ImGui::TextWrapped("%s", gotoInfo.c_str());
		}

		if (ImGui::IsKeyPressed(ImGuiKey_Escape))
		{
			show = false;
		}
	}
	ImGui::End();
}