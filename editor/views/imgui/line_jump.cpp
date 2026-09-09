#include "line_jump.h"
#include "../../editor_api.h"
#include "../../editor_commands.h"
#include "../../editor_events.h"
#include "editor_input.h"
#include "ned_key.h"
#include <cstdio>
#include <cstring>

void LineJump::dismiss()
{
	showLineJumpWindow = false;
	wasKeyboardFocusSet = false;
	memset(lineNumberBuffer, 0, sizeof(lineNumberBuffer));
}

// Helper: Render window header (setup and title)
void LineJump::renderHeader()
{
	const float fs = ImGui::GetFontSize();
	const ImVec2 windowSize(fs * 20.0f, fs * 6.0f);
	ImGui::SetNextWindowSize(windowSize, ImGuiCond_Always);

	// Host calls update() inside the Editor child — use current window.
	const ImVec2 panePos = ImGui::GetWindowPos();
	const ImVec2 paneSize = ImGui::GetWindowSize();

	ImVec2 windowPos = ImVec2(panePos.x + paneSize.x * 0.5f - windowSize.x * 0.5f,
							  panePos.y + paneSize.y * 0.35f - windowSize.y * 0.5f);

	if (windowPos.x < panePos.x)
		windowPos.x = panePos.x;
	if (windowPos.x + windowSize.x > panePos.x + paneSize.x)
		windowPos.x = panePos.x + paneSize.x - windowSize.x;
	if (windowPos.y < panePos.y)
		windowPos.y = panePos.y;
	if (windowPos.y + windowSize.y > panePos.y + paneSize.y)
		windowPos.y = panePos.y + paneSize.y - windowSize.y;

	ImGui::SetNextWindowPos(windowPos, ImGuiCond_Always);
	ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoTitleBar |
								   ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
								   ImGuiWindowFlags_NoScrollbar |
								   ImGuiWindowFlags_NoScrollWithMouse;
	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, fs * 0.5f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(fs * 0.8f, fs * 0.8f));
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

	// Unique id per LineJump instance (multi-tab / side-by-side docks).
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
bool LineJump::renderInput()
{
	float inputWidth = ImGui::GetContentRegionAvail().x;
	ImGui::PushItemWidth(inputWidth);

	const float fs = ImGui::GetFontSize();
	ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, fs * 0.2f);
	ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(fs * 0.4f, fs * 0.4f));

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
	std::snprintf(
		inputId, sizeof(inputId), "##LineJumpInput_%p", static_cast<const void *>(this));
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

void LineJump::update()
{
	// Only the focused editor host may open/toggle (multi-tab: avoid every
	// LineJump handling the same Cmd+; and spawning duplicate windows).
	const bool hostFocused = ImGui::IsWindowFocused(ImGuiFocusedFlags_ChildWindows) ||
							 ImGui::IsWindowFocused(0) ||
							 ImGui::IsWindowFocused(ImGuiFocusedFlags_RootWindow);

	bool main_key = ImGui::GetIO().KeyCtrl || ImGui::GetIO().KeySuper;
	bool shift_pressed = ImGui::GetIO().KeyShift;
	const ImGuiKey line_jump_key =
		imguiKeyFromNed(settings->keybinds.getActionKey("line_jump_key"));

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

void LineJump::jumpToLine(int lineNumber)
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
