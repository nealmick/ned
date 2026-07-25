#include "editor_line_jump.h"
#include "../editor_api.h"
#include "../editor_commands.h"
#include "../editor_events.h"
#include "../editor_input.h"
#include <cstring>
#include <iostream>

void EditorLineJump::dismiss()
{
	showLineJumpWindow = false;
	wasKeyboardFocusSet = false;
	memset(lineNumberBuffer, 0, sizeof(lineNumberBuffer));
}

void EditorLineJump::update()
{
	// Only the focused editor host may open/toggle (multi-tab: avoid every
	// EditorLineJump handling the same Cmd+; and spawning duplicate windows).
	const bool hostFocused = ImGui::IsWindowFocused(ImGuiFocusedFlags_ChildWindows) ||
							 ImGui::IsWindowFocused(0) ||
							 ImGui::IsWindowFocused(ImGuiFocusedFlags_RootWindow);

	bool main_key = ImGui::GetIO().KeyCtrl || ImGui::GetIO().KeySuper;
	bool shift_pressed = ImGui::GetIO().KeyShift;
	ImGuiKey line_jump_key = settings->keybinds.getActionKey("line_jump_key");

	if (hostFocused && main_key &&
		(ImGui::IsKeyPressed(line_jump_key, false) ||
		 (shift_pressed && ImGui::IsKeyPressed(line_jump_key, false))))
	{
		showLineJumpWindow = !showLineJumpWindow;
		if (showLineJumpWindow)
		{
			if (api)
				api->requestExclusiveOverlay(
					EditorEvents::DidRequestExclusiveOverlay::Keep::LineJump);
			memset(lineNumberBuffer, 0, sizeof(lineNumberBuffer));
			wasKeyboardFocusSet = false;
		}
		if (api)
			api->setBlockInput(showLineJumpWindow);
		return;
	}

	if (showLineJumpWindow && ImGui::IsKeyPressed(ImGuiKey_Escape))
	{
		showLineJumpWindow = false;
		if (api)
			api->setBlockInput(false);
		memset(lineNumberBuffer, 0, sizeof(lineNumberBuffer));
		wasKeyboardFocusSet = false;
		return;
	}

	if (!showLineJumpWindow)
		return;

	renderHeader();

	bool enterPressed = renderInput();
	if (enterPressed)
	{
		int lineNumber = std::atoi(lineNumberBuffer);
		jumpToLine(lineNumber - 1);
		showLineJumpWindow = false;
		if (api)
			api->setBlockInput(false);
		memset(lineNumberBuffer, 0, sizeof(lineNumberBuffer));
		wasKeyboardFocusSet = false;
		ImGui::GetIO()
			.ClearInputKeys(); // clears text input queue (ClearInputCharacters removed)
		ImGui::End();
		ImGui::PopStyleColor(3);
		ImGui::PopStyleVar(3);
		return;
	}

	ImGui::Spacing();
	ImGui::Text("Type line number then Enter");

	ImGui::End();
	ImGui::PopStyleColor(3);
	ImGui::PopStyleVar(3);
}

void EditorLineJump::jumpToLine(int lineNumber)
{
	if (!commands)
		return;
	commands->goToLine(lineNumber);
	// Same Enter must not insert a newline in the document this frame.
	if (input)
		input->suppressNextEnter = true;
	// Ensure the document child scrolls this frame (center may run before layout).
	if (api)
		api->requestEnsureVisible();
}
