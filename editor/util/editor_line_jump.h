/*
	editor/util/editor_line_jump.h
	Go-to-line overlay (keybind → number input → jump + center cursor).
*/

#pragma once
#include "../../util/keybinds.h"
#include "../../util/settings.h"
#include "imgui.h"

#include <cstdio>
#include <string>

class EditorApi;
class EditorInput;
class EditorCommands;
class Settings;

class EditorLineJump
{
  public:
	bool showLineJumpWindow = false;

	EditorLineJump(EditorCommands &cmds,
				   EditorInput &editorInput,
				   Settings &appSettings,
				   EditorApi &editorApi)
		: commands(&cmds), input(&editorInput), settings(&appSettings), api(&editorApi)
	{
	}

	// Helper: Render window header (setup and title)
	inline void renderHeader()
	{
		// Window setup (size, position, flags)
		ImGui::SetNextWindowSize(ImVec2(400, 120), ImGuiCond_Always);

		// Host calls update() inside the Editor child — use current window.
		const ImVec2 panePos = ImGui::GetWindowPos();
		const ImVec2 paneSize = ImGui::GetWindowSize();

		ImVec2 windowPos = ImVec2(panePos.x + paneSize.x * 0.5f - 200.0f,
								  panePos.y + paneSize.y * 0.35f - 60.0f);

		if (windowPos.x < panePos.x)
			windowPos.x = panePos.x;
		if (windowPos.x + 400 > panePos.x + paneSize.x)
			windowPos.x = panePos.x + paneSize.x - 400;
		if (windowPos.y < panePos.y)
			windowPos.y = panePos.y;
		if (windowPos.y + 120 > panePos.y + paneSize.y)
			windowPos.y = panePos.y + paneSize.y - 120;

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
			ImVec4(settings->settings["backgroundColor"][0].get<float>() * .8,
				   settings->settings["backgroundColor"][1].get<float>() * .8,
				   settings->settings["backgroundColor"][2].get<float>() * .8,
				   1.0f));
		ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.3f, 0.3f, 0.3f, 1.0f));
		ImGui::PushStyleColor(
			ImGuiCol_FrameBg,
			ImVec4(settings->settings["backgroundColor"][0].get<float>() * .8,
				   settings->settings["backgroundColor"][1].get<float>() * .8,
				   settings->settings["backgroundColor"][2].get<float>() * .8,
				   1.0f));

		// Unique id per EditorLineJump instance (multi-tab / side-by-side docks).
		char winId[64];
		std::snprintf(
			winId, sizeof(winId), "LineJump###lj_%p", static_cast<const void *>(this));
		ImGui::Begin(winId, nullptr, windowFlags);

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
			ImVec4(settings->settings["backgroundColor"][0].get<float>() * 0.8f,
				   settings->settings["backgroundColor"][1].get<float>() * 0.8f,
				   settings->settings["backgroundColor"][2].get<float>() * 0.8f,
				   1.0f));

		// Force keyboard focus each frame so the input stays focused
		ImGui::SetKeyboardFocusHere();
		char inputId[64];
		std::snprintf(inputId,
					  sizeof(inputId),
					  "##LineJumpInput_%p",
					  static_cast<const void *>(this));
		bool enterPressed = ImGui::InputText(
			inputId,
			lineNumberBuffer,
			sizeof(lineNumberBuffer),
			ImGuiInputTextFlags_CharsDecimal | ImGuiInputTextFlags_EnterReturnsTrue);

		// Clean up style changes
		ImGui::PopStyleColor(2);
		ImGui::PopStyleVar(3);

		ImGui::PopItemWidth();
		return enterPressed;
	}

	// Per-frame: keys, block input, UI if open. Call before frame.run().
	void update();

	void jumpToLine(int lineNumber);
	void dismiss();

  private:
	EditorCommands *commands;
	EditorInput *input;
	Settings *settings;
	EditorApi *api;
	char lineNumberBuffer[32] = "";
	bool wasKeyboardFocusSet = false;
};
