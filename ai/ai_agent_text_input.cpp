#include "ai_agent_text_input.h"
#include "../files/files.h" // for gFileExplorer
#include "../lib/json.hpp"
#include "editor/editor.h" // for editor_state
#include "util/settings.h"
#include <cctype>
#include <cstring>
#include <imgui.h>
#include <imgui_internal.h>
#include <iostream>

using json = nlohmann::json;

AIAgentTextInput::AIAgentTextInput()
	: inputBuffer(nullptr), bufferSize(0), shouldRestoreFocus(false)
{
}

void AIAgentTextInput::setInputBuffer(char *buffer, size_t size)
{
	inputBuffer = buffer;
	bufferSize = size;
}

void AIAgentTextInput::setSendMessageCallback(
	std::function<void(const char *, bool)> callback)
{
	sendMessageCallback = callback;
}

void AIAgentTextInput::setIsProcessingCallback(std::function<bool()> callback)
{
	isProcessingCallback = callback;
}

void AIAgentTextInput::setNotificationCallback(
	std::function<void(const char *, float)> callback)
{
	notificationCallback = callback;
}

void AIAgentTextInput::setClearConversationCallback(std::function<void()> callback)
{
	clearConversationCallback = callback;
}

void AIAgentTextInput::setToggleHistoryCallback(std::function<void()> callback)
{
	toggleHistoryCallback = callback;
}

void AIAgentTextInput::setBlockInputCallback(std::function<void(bool)> callback)
{
	blockInputCallback = callback;
}

void AIAgentTextInput::setStopRequestCallback(std::function<void()> callback)
{
	stopRequestCallback = callback;
}

void AIAgentTextInput::render(const ImVec2 &textBoxSize,
							  float textBoxWidth,
							  float horizontalPadding)
{
	// Apply custom styles
	ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(8, 8));
	ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.3f, 0.3f, 0.3f, 1.0f));

	// Safe access to backgroundColor with null checks
	auto &bgColor = gSettings.getSettings()["backgroundColor"];
	float bgR = (bgColor.is_array() && bgColor.size() > 0 && !bgColor[0].is_null())
					? bgColor[0].get<float>()
					: 0.1f;
	float bgG = (bgColor.is_array() && bgColor.size() > 1 && !bgColor[1].is_null())
					? bgColor[1].get<float>()
					: 0.1f;
	float bgB = (bgColor.is_array() && bgColor.size() > 2 && !bgColor[2].is_null())
					? bgColor[2].get<float>()
					: 0.1f;

	ImGui::PushStyleColor(ImGuiCol_FrameBg,
						  ImVec4(bgR * 0.8f, bgG * 0.8f, bgB * 0.8f, 1.0f));

	renderInputBox(textBoxSize, textBoxWidth);
	ImGui::Spacing();
	renderButtons(textBoxWidth);

	ImGui::PopStyleColor(2);
	ImGui::PopStyleVar(3);
}

void AIAgentTextInput::renderInputBox(const ImVec2 &textBoxSize, float textBoxWidth)
{
	bool inputActive =
		ImGui::InputTextMultiline("##AIAgentInput", inputBuffer, bufferSize, textBoxSize);

	// Restore focus if needed
	if (shouldRestoreFocus)
	{
		ImGui::SetKeyboardFocusHere(-1);
		shouldRestoreFocus = false;
	}

	// Check focus state immediately after the input widget
	bool isFocused = ImGui::IsItemActive();

	handleInputLogic();
	renderHintText(textBoxWidth);

	ImGuiContext &g = *ImGui::GetCurrentContext();
	ImGuiWindow *window = g.CurrentWindow;
	ImGuiID id = window->GetID("##AIAgentInput");
	ImGuiInputTextState *state = ImGui::GetInputTextState(id);

	bool did_wrap = false;
	if (isFocused && state)
	{
		// Check if we need to wrap the text
		did_wrap = HandleWordBreakAndWrap(
			inputBuffer, bufferSize, state, textBoxWidth - 10.0f, textBoxWidth);
	}

	if (isFocused && (ImGui::IsItemEdited() || did_wrap))
	{
		// Auto-scroll to bottom when text is edited or wrapped
		ImGui::SetScrollHereY(1.0f);
	}

	static bool wasFocused = false;
	if (isFocused != wasFocused)
	{
		if (blockInputCallback)
		{
			blockInputCallback(isFocused);
		}
		wasFocused = isFocused;
	}
	if (isFocused && blockInputCallback)
	{
		blockInputCallback(true);
	}
}

void AIAgentTextInput::renderButtons(float textBoxWidth)
{
	// Style the Send button to match the input box
	ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(8, 8));
	ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.3f, 0.3f, 0.3f, 1.0f));

	// Safe access to backgroundColor with null checks
	auto &bgColor = gSettings.getSettings()["backgroundColor"];
	float bgR = (bgColor.is_array() && bgColor.size() > 0 && !bgColor[0].is_null())
					? bgColor[0].get<float>()
					: 0.1f;
	float bgG = (bgColor.is_array() && bgColor.size() > 1 && !bgColor[1].is_null())
					? bgColor[1].get<float>()
					: 0.1f;
	float bgB = (bgColor.is_array() && bgColor.size() > 2 && !bgColor[2].is_null())
					? bgColor[2].get<float>()
					: 0.1f;

	ImGui::PushStyleColor(ImGuiCol_Button,
						  ImVec4(bgR * 0.8f, bgG * 0.8f, bgB * 0.8f, 1.0f));
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered,
						  ImVec4(bgR * 0.95f, bgG * 0.95f, bgB * 0.95f, 1.0f));
	ImGui::PushStyleColor(ImGuiCol_ButtonActive,
						  ImVec4(bgR * 0.7f, bgG * 0.7f, bgB * 0.7f, 1.0f));

	// Calculate button size based on font size
	ImVec2 textSize = ImGui::CalcTextSize("Send");
	ImVec2 buttonSize = ImVec2(textSize.x + 16.0f,
							   0); // Add padding for comfortable button size

	if (ImGui::Button("Send", buttonSize))
	{
		// Check if there's an ongoing conversation
		if (isProcessingCallback && isProcessingCallback())
		{
			if (notificationCallback)
			{
				notificationCallback("Ongoing Conversation \nplease wait", 3.0f);
			}
		} else
		{
			const char *str = inputBuffer;
			// Skip leading whitespace
			while (*str && isspace((unsigned char)*str))
			{
				str++;
			}

			if (*str == '\0')
			{
				// String is empty or contains only whitespace
				if (notificationCallback)
				{
					notificationCallback("No prompt provided", 3.0f);
				}
			} else
			{
				if (sendMessageCallback)
				{
					sendMessageCallback(inputBuffer, false);
				}
				inputBuffer[0] = '\0';
			}
		}
	}

	ImGui::SameLine();

	// New button
	ImVec2 newTextSize = ImGui::CalcTextSize("New");
	ImVec2 newButtonSize = ImVec2(newTextSize.x + 16.0f, 0);

	if (ImGui::Button("New", newButtonSize))
	{
		// Check if there's an ongoing conversation
		if (isProcessingCallback && isProcessingCallback())
		{
			if (notificationCallback)
			{
				notificationCallback("Ongoing Conversation \nplease wait", 3.0f);
			}
		} else
		{
			// Clear current conversation and start new chat
			if (clearConversationCallback)
			{
				clearConversationCallback();
			}
			if (notificationCallback)
			{
				notificationCallback("New chat started", 2.0f);
			}
		}
	}

	ImGui::SameLine();

	// History button
	ImVec2 historyTextSize = ImGui::CalcTextSize("History");
	ImVec2 historyButtonSize = ImVec2(historyTextSize.x + 16.0f, 0);

	if (ImGui::Button("History", historyButtonSize))
	{
		// Toggle history window
		if (toggleHistoryCallback)
		{
			toggleHistoryCallback();
		}
	}

	// Add spinner next to History button if there's an active message
	if (isProcessingCallback && isProcessingCallback())
	{
		ImGui::SameLine();

		// Get the position after the History button
		ImVec2 spinnerPos = ImGui::GetCursorScreenPos();
		spinnerPos.x += 16.0f; // More padding to the right of the button

		// Get the actual button height to center the spinner properly
		ImVec2 buttonMin = ImGui::GetItemRectMin();
		ImVec2 buttonMax = ImGui::GetItemRectMax();
		float buttonHeight = buttonMax.y - buttonMin.y;
		spinnerPos.y =
			buttonMin.y + buttonHeight * 0.5f; // Center vertically with the button

		// Get current time for animation
		float time = (float)ImGui::GetTime();

		// Check if mouse is hovering over the spinner area
		ImVec2 mousePos = ImGui::GetMousePos();
		float fontSize = ImGui::GetFontSize();
		float spinnerSize =
			fontSize * 0.8f; // Make spinner slightly smaller than font size
		ImRect spinnerRect(spinnerPos.x - spinnerSize * 0.5f,
						   spinnerPos.y - spinnerSize * 0.5f,
						   spinnerPos.x + spinnerSize * 0.5f,
						   spinnerPos.y + spinnerSize * 0.5f);

		bool isHovered = spinnerRect.Contains(mousePos);

		if (isHovered)
		{
			// Show close icon when hovering
			ImTextureID closeIcon = gFileExplorer.getIcon("close-mac-hover");
			ImVec2 iconSize = ImVec2(
				fontSize * 0.8f,
				fontSize * 0.8f); // Make close button slightly smaller than font size
			ImVec2 iconPos = ImVec2(spinnerPos.x - iconSize.x * 0.5f,
									spinnerPos.y - iconSize.y * 0.5f);

			// Draw the close icon
			ImGui::GetWindowDrawList()->AddImage(closeIcon,
												 iconPos,
												 ImVec2(iconPos.x + iconSize.x,
														iconPos.y + iconSize.y));

			// Handle click to stop the request
			if (ImGui::IsMouseClicked(ImGuiMouseButton_Left))
			{
				if (stopRequestCallback)
				{
					stopRequestCallback();
				}
			}
		} else
		{
			// Render the spinner
			renderSpinner(spinnerPos, spinnerSize, time);
		}
	}

	ImGui::PopStyleColor(4);
	ImGui::PopStyleVar(3);
}

void AIAgentTextInput::handleInputLogic()
{
	// Simple Enter key detection
	static bool enterPressed = false;
	bool isFocused = ImGui::IsItemActive();

	if (isFocused && ImGui::IsKeyDown(ImGuiKey_Enter) && !enterPressed)
	{
		enterPressed = true;
		ImGuiIO &io = ImGui::GetIO();
		if (!io.KeyShift)
		{
			// Check if there's an ongoing conversation
			if (isProcessingCallback && isProcessingCallback())
			{
				if (notificationCallback)
				{
					notificationCallback("Ongoing Conversation\nplease wait", 3.0f);
				}
			} else
			{
				const char *str = inputBuffer;
				// Skip leading whitespace
				while (*str && isspace((unsigned char)*str))
				{
					str++;
				}

				if (*str == '\0')
				{
					// String is empty or contains only whitespace
					if (notificationCallback)
					{
						notificationCallback("No prompt provided", 3.0f);
					}
				} else
				{
					if (sendMessageCallback)
					{
						sendMessageCallback(inputBuffer, false);
					}
					// Clear the input buffer and reset ImGui state
					inputBuffer[0] = '\0';
					ImGui::ClearActiveID();
					// Set flag to restore focus on next frame
					shouldRestoreFocus = true;
				}
			}
		}
	} else if (!ImGui::IsKeyDown(ImGuiKey_Enter))
	{
		enterPressed = false;
	}
}

void AIAgentTextInput::renderHintText(float textBoxWidth)
{
	bool isFocused = ImGui::IsItemActive();

	// Draw hint if empty and not focused
	if (inputBuffer[0] == '\0' && !isFocused)
	{
		ImVec2 pos = ImGui::GetItemRectMin();
		ImVec2 padding = ImGui::GetStyle().FramePadding;
		pos.x += padding.x + 2.0f;
		pos.y += padding.y;

		// Calculate available width for hint text
		float availableWidth = textBoxWidth - padding.x * 2.0f -
							   4.0f; // Account for padding and small buffer

		// Choose appropriate hint text based on font size and available width
		const char *hintText = "prompt here";
		float fontSize = ImGui::GetFontSize();
		ImVec2 textSize = ImGui::CalcTextSize(hintText);

		// If the text is too wide for the container, use a shorter version
		if (textSize.x > availableWidth)
		{
			if (fontSize > 20.0f)
			{
				hintText = "prompt..."; // Very short for large fonts
			} else if (textSize.x > availableWidth * 0.8f)
			{
				hintText = "prompt"; // Shorter version for medium fonts
			}
		}

		// Recalculate text size with the potentially shorter text
		textSize = ImGui::CalcTextSize(hintText);

		// Only draw if the text actually fits
		if (textSize.x <= availableWidth)
		{
			ImGui::GetWindowDrawList()->AddText(pos,
												ImGui::GetColorU32(ImGuiCol_TextDisabled),
												hintText);
		}
	}
}

// Static helper function for word breaking and line wrapping
bool AIAgentTextInput::HandleWordBreakAndWrap(char *inputBuffer,
											  size_t bufferSize,
											  ImGuiInputTextState *state,
											  float max_width,
											  float text_box_x)
{
	// TODO: Re-implement word break and wrap functionality for new ImGui version
	// The ImGuiInputTextState structure has changed significantly in ImGui 1.92+
	// This functionality needs to be reworked to use the new text input system

	// For now, return false to disable word wrapping
	// This can be re-implemented later when the new text input system is understood
	return false;
}

void AIAgentTextInput::renderSpinner(const ImVec2 &position, float size, float time)
{
	// Draw a simple rotating spinner using ImGui primitives
	const int numSegments = 8;
	const float angleStep = 2.0f * 3.14159f / numSegments;
	const float rotationSpeed = 2.0f; // rotations per second
	const float currentAngle = time * rotationSpeed * 2.0f * 3.14159f;

	ImDrawList *drawList = ImGui::GetWindowDrawList();
	ImVec2 center = position;

	for (int i = 0; i < numSegments; ++i)
	{
		float angle = currentAngle + i * angleStep;
		float alpha = 1.0f - (float)i / numSegments;

		ImVec2 start = ImVec2(center.x + cos(angle) * size * 0.3f,
							  center.y + sin(angle) * size * 0.3f);
		ImVec2 end = ImVec2(center.x + cos(angle) * size * 0.6f,
							center.y + sin(angle) * size * 0.6f);

		ImU32 color = ImGui::GetColorU32(ImVec4(0.7f, 0.7f, 0.7f, alpha));
		drawList->AddLine(start, end, color, 2.0f);
	}
}