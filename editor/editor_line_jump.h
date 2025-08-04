/*
	editor/editor_line_jump.h
	This utility handles line changes, such as jumping to line 300...
	Use keybind cmd : to activate the number input box, type a number then
   press enter. The cursor jumps to the line number and scrolls the view to
   move show the cursor.
*/

#pragma once
#include "../files/files.h"
#include "../imgui/imgui.h"
#include "../util/close_popper.h"
#include "../util/keybinds.h"
#include "editor.h"
#include "editor_scroll.h"
#include "editor_types.h"

#include <string>

class LineJump
{
  private:
	char lineNumberBuffer[32] = "";
	bool justJumped = false;		  // Track if we just performed a jump
	bool wasKeyboardFocusSet = false; // Track if keyboard focus was set

  public:
	bool showLineJumpWindow = false;

	// Helper: Render window header (setup and title)
	inline void renderHeader()
	{
		// Window setup (size, position, flags)
		ImGui::SetNextWindowSize(ImVec2(400, 120), ImGuiCond_Always);

		// Center within the current editor window bounds (like LSP goto def)
		ImVec2 editorPanePos = ImGui::GetWindowPos();
		ImVec2 editorPaneSize = ImGui::GetWindowSize();

		// Position the popup within the editor pane bounds
		ImVec2 windowPos = ImVec2(editorPanePos.x + editorPaneSize.x * 0.5f - 200.0f,
								  editorPanePos.y + editorPaneSize.y * 0.35f - 60.0f);

		// Ensure the popup stays within the editor pane bounds
		if (windowPos.x < editorPanePos.x)
			windowPos.x = editorPanePos.x;
		if (windowPos.x + 400 > editorPanePos.x + editorPaneSize.x)
			windowPos.x = editorPanePos.x + editorPaneSize.x - 400;
		if (windowPos.y < editorPanePos.y)
			windowPos.y = editorPanePos.y;
		if (windowPos.y + 120 > editorPanePos.y + editorPaneSize.y)
			windowPos.y = editorPanePos.y + editorPaneSize.y - 120;

		ImGui::SetNextWindowPos(windowPos, ImGuiCond_Always);
		ImGuiWindowFlags windowFlags =
			ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
			ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar |
			ImGuiWindowFlags_NoScrollWithMouse;
		// Push window style (3 style vars, 3 style colors)
		ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 10.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(16.0f, 16.0f));
		// background
		ImGui::PushStyleColor(
			ImGuiCol_WindowBg,
			ImVec4(gSettings.getSettings()["backgroundColor"][0].get<float>() * .8,
				   gSettings.getSettings()["backgroundColor"][1].get<float>() * .8,
				   gSettings.getSettings()["backgroundColor"][2].get<float>() * .8,
				   1.0f));
		ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.3f, 0.3f, 0.3f, 1.0f));
		ImGui::PushStyleColor(
			ImGuiCol_FrameBg,
			ImVec4(gSettings.getSettings()["backgroundColor"][0].get<float>() * .8,
				   gSettings.getSettings()["backgroundColor"][1].get<float>() * .8,
				   gSettings.getSettings()["backgroundColor"][2].get<float>() * .8,
				   1.0f));

		ImGui::Begin("LineJump", nullptr, windowFlags);

		ImGui::TextUnformatted("Jump to line:");
		ImGui::Spacing();
		ImGui::Spacing();

		// Ensure keyboard focus is set on first render
		if (!wasKeyboardFocusSet)
		{
			ImGui::SetKeyboardFocusHere();
			wasKeyboardFocusSet = true;
		}
	}

	// Helper: Render the input box and force keyboard focus
	inline bool renderInput()
	{
		float inputWidth = ImGui::GetContentRegionAvail().x;
		ImGui::PushItemWidth(inputWidth);

		// Add border styling to match FileFinder
		ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(8, 8));

		// Match border and background colors from FileFinder
		ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.3f, 0.3f, 0.3f, 1.0f));
		ImGui::PushStyleColor(
			ImGuiCol_FrameBg,
			ImVec4(gSettings.getSettings()["backgroundColor"][0].get<float>() * 0.8f,
				   gSettings.getSettings()["backgroundColor"][1].get<float>() * 0.8f,
				   gSettings.getSettings()["backgroundColor"][2].get<float>() * 0.8f,
				   1.0f));

		// Force keyboard focus each frame so the input stays focused
		ImGui::SetKeyboardFocusHere();
		bool enterPressed = ImGui::InputText("##LineJumpInput",
											 lineNumberBuffer,
											 sizeof(lineNumberBuffer),
											 ImGuiInputTextFlags_CharsDecimal |
												 ImGuiInputTextFlags_EnterReturnsTrue);

		// Clean up style changes
		ImGui::PopStyleColor(2);
		ImGui::PopStyleVar(3);

		ImGui::PopItemWidth();
		return enterPressed;
	}

	// The main renderWindow() now calls the helpers - EXACTLY like file finder
	inline void renderLineJumpWindow()
	{
		// Toggle with keybind - EXACTLY like file finder
		bool main_key = ImGui::GetIO().KeyCtrl || ImGui::GetIO().KeySuper;
		bool shift_pressed = ImGui::GetIO().KeyShift;
		ImGuiKey line_jump_key = gKeybinds.getActionKey("line_jump_key");

		if (main_key && (ImGui::IsKeyPressed(line_jump_key, false) ||
						 (shift_pressed && ImGui::IsKeyPressed(line_jump_key, false))))
		{
			showLineJumpWindow = !showLineJumpWindow;
			if (showLineJumpWindow)
			{
				ClosePopper::closeAllExcept(ClosePopper::Type::LineJump);
				memset(lineNumberBuffer, 0, sizeof(lineNumberBuffer));
				wasKeyboardFocusSet = false;
			}
			editor_state.block_input = showLineJumpWindow;
			return;
		}

		if (showLineJumpWindow && ImGui::IsKeyPressed(ImGuiKey_Escape))
		{
			showLineJumpWindow = false;
			editor_state.block_input = false;
			memset(lineNumberBuffer, 0, sizeof(lineNumberBuffer));
			return;
		}

		if (!showLineJumpWindow)
			return;

		// Render header (window setup and title)
		renderHeader();

		// Render input; if Enter is pressed, perform the jump
		bool enterPressed = renderInput();
		if (enterPressed)
		{
			int lineNumber = std::atoi(lineNumberBuffer);
			jumpToLine(lineNumber - 1);
			showLineJumpWindow = false;
			editor_state.block_input = false;
			memset(lineNumberBuffer, 0, sizeof(lineNumberBuffer));
			justJumped = true;
			ImGui::GetIO().ClearInputCharacters();
			ImGui::End();
			ImGui::PopStyleColor(3);
			ImGui::PopStyleVar(3);
			return;
		}

		ImGui::Spacing();
		ImGui::Text("Type line number then Enter");

		ImGui::End();
		// Pop the window style colors and vars pushed in renderHeader()
		ImGui::PopStyleColor(3);
		ImGui::PopStyleVar(3);
	}

	inline void jumpToLine(int lineNumber)
	{
		if (lineNumber < 0)
			lineNumber = 0;
		if (lineNumber >= static_cast<int>(editor_state.editor_content_lines.size()))
		{
			lineNumber = static_cast<int>(editor_state.editor_content_lines.size()) - 1;
		}

		// Set cursor to the beginning of the requested line
		editor_state.cursor_index = editor_state.editor_content_lines[lineNumber];
		editor_state.selection_start = editor_state.cursor_index;
		editor_state.selection_end = editor_state.cursor_index;
		std::cout << "seting line number in linejump " << lineNumber << std::endl;
		editor_state.selection_active = false;

		editor_state.center_cursor_vertical = true;

		justJumped = true;
	}

	inline bool isWindowOpen() const { return showLineJumpWindow; }

	inline bool hasJustJumped() const
	{
		if (justJumped)
		{
			// Reset the flag when checked
			const_cast<LineJump *>(this)->justJumped = false;
			return true;
		}
		return false;
	}
};

extern LineJump gLineJump;