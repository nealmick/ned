#include "file_content_search.h"
#include "../util/close_popper.h"
#include <algorithm>
#include <iostream>

FileContentSearch gFileContentSearch;

FileContentSearch::FileContentSearch() {}

std::string FileContentSearch::toLower(const std::string &s)
{
	std::string result = s;
	std::transform(result.begin(), result.end(), result.begin(), [](unsigned char c) {
		return std::tolower(c);
	});
	return result;
}

void FileContentSearch::findNext(bool ignoreCase)
{
	if (findText.empty())
		return;

	size_t startPos;
	if (lastFoundPos == std::string::npos)
	{
		startPos = editor_state.cursor_index;
	} else
	{
		startPos = lastFoundPos + 1;
	}

	if (startPos >= editor_state.fileContent.length())
		startPos = 0; // Wrap around if at end

	size_t foundPos;
	if (ignoreCase)
	{
		std::string fileContentLower = toLower(editor_state.fileContent);
		std::string findTextLower = toLower(findText);
		foundPos = fileContentLower.find(findTextLower, startPos);
	} else
	{
		foundPos = editor_state.fileContent.find(findText, startPos);
	}

	std::cout << "Searching for '" << findText << "' starting from position " << startPos;
	if (ignoreCase)
		std::cout << " (case-insensitive)";
	std::cout << std::endl;

	if (foundPos == std::string::npos)
	{
		// Wrap around to the beginning
		if (ignoreCase)
		{
			std::string fileContentLower = toLower(editor_state.fileContent);
			std::string findTextLower = toLower(findText);
			foundPos = fileContentLower.find(findTextLower);
		} else
		{
			foundPos = editor_state.fileContent.find(findText);
		}
		std::cout << "Wrapped search to beginning" << std::endl;
	}

	if (foundPos != std::string::npos)
	{
		lastFoundPos = foundPos;
		editor_state.cursor_index = foundPos;
		editor_state.selection_start = foundPos;
		editor_state.selection_end = foundPos + findText.length();
		std::cout << "Found at position: " << foundPos
				  << ", cursor now at: " << editor_state.cursor_index << std::endl;
	} else
	{
		std::cout << "Not found" << std::endl;
	}
}

void FileContentSearch::findPrevious(bool ignoreCase)
{
	if (findText.empty())
		return;

	size_t startPos;
	if (lastFoundPos == std::string::npos)
	{
		startPos = editor_state.cursor_index;
	} else
	{
		startPos = (lastFoundPos == 0) ? editor_state.fileContent.length() - 1 : lastFoundPos - 1;
	}

	size_t foundPos;
	if (ignoreCase)
	{
		std::string fileContentLower = toLower(editor_state.fileContent);
		std::string findTextLower = toLower(findText);
		foundPos = fileContentLower.rfind(findTextLower, startPos);
	} else
	{
		foundPos = editor_state.fileContent.rfind(findText, startPos);
	}

	std::cout << "Searching backwards for '" << findText << "' starting from position " << startPos;
	if (ignoreCase)
		std::cout << " (case-insensitive)";
	std::cout << std::endl;

	if (foundPos == std::string::npos)
	{
		// Wrap around to the end
		if (ignoreCase)
		{
			std::string fileContentLower = toLower(editor_state.fileContent);
			std::string findTextLower = toLower(findText);
			foundPos = fileContentLower.rfind(findTextLower);
		} else
		{
			foundPos = editor_state.fileContent.rfind(findText);
		}
		std::cout << "Wrapped search to end" << std::endl;
	}

	if (foundPos != std::string::npos)
	{
		lastFoundPos = foundPos;
		editor_state.cursor_index = foundPos;
		editor_state.selection_start = foundPos;
		editor_state.selection_end = foundPos + findText.length();
		std::cout << "Found at position: " << foundPos
				  << ", cursor now at: " << editor_state.cursor_index << std::endl;
	} else
	{
		std::cout << "Not found" << std::endl;
	}
}

void FileContentSearch::handleFindBoxActivation()
{
	ImGuiIO &io = ImGui::GetIO();
	// If Cmd+F is pressed, activate the find box (do not toggle off here)
	if ((io.KeyCtrl || io.KeySuper) && ImGui::IsKeyPressed(ImGuiKey_F))
	{
		ClosePopper::closeAllExcept(ClosePopper::Type::LineJump);
		editor_state.active_find_box = true;
		editor_state.block_input = true;
		findText = "";
		findBoxShouldFocus = true; // force focus on activation
	}
	// If Escape is pressed, deactivate the find box.
	if (ImGui::IsKeyPressed(ImGuiKey_Escape))
	{
		editor_state.active_find_box = false;
		editor_state.block_input = false;
	}
}

void FileContentSearch::renderFindBox()
{
	// Only render if the find box is active.
	if (!editor_state.active_find_box)
		return;

	// Reserve ~70% of available width for the input field.
	float availWidth = ImGui::GetContentRegionAvail().x;
	float inputWidth = availWidth * 0.7f;
	ImGui::SetNextItemWidth(inputWidth);

	// Render the input field in its own group.
	ImGui::BeginGroup();
	ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, Style::FRAME_ROUNDING);
	ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, Style::BORDER_SIZE);
	ImGui::PushStyleColor(ImGuiCol_FrameBg, Style::FRAME_BG);
	ImGui::PushStyleColor(ImGuiCol_Border, Style::BORDER_COLOR);

	static char inputBuffer[256] = "";
	// Force focus on the input field on activation.
	if (findBoxShouldFocus)
	{
		ImGui::SetKeyboardFocusHere();
		findBoxShouldFocus = false;
	}
	ImGuiInputTextFlags flags =
		ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll;
	if (ImGui::InputText("##findbox", inputBuffer, sizeof(inputBuffer), flags))
	{
		findText = inputBuffer;
		lastFoundPos = std::string::npos;
	}
	ImGui::PopStyleColor(2);
	ImGui::PopStyleVar(2);
	ImGui::EndGroup();

	// Add horizontal spacing between the input field and the checkbox.
	ImGui::SameLine();
	ImGui::Dummy(ImVec2(10, 0)); // 10 pixels of spacing.
	ImGui::SameLine();

	// Render the checkbox.
	ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, Style::FRAME_ROUNDING);
	ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, Style::BORDER_SIZE);
	ImGui::PushStyleColor(ImGuiCol_FrameBg, Style::FRAME_BG);
	ImGui::PushStyleColor(ImGuiCol_Border, Style::BORDER_COLOR);

	static bool ignoreCaseCheckbox = false;
	ImGui::Checkbox("Ignore Case", &ignoreCaseCheckbox);
	ImGui::PopStyleColor(2);
	ImGui::PopStyleVar(2);

	handleFindBoxKeyboardShortcuts(ignoreCaseCheckbox);
}

void FileContentSearch::handleFindBoxKeyboardShortcuts(bool ignoreCaseCheckbox)
{
	ImGuiIO &io = ImGui::GetIO();
	if (ImGui::IsKeyPressed(ImGuiKey_Enter, false))
	{
		if (io.KeyShift)
		{
			std::cout << "Searching previous";
			if (ignoreCaseCheckbox)
				std::cout << " (case-insensitive)";
			std::cout << std::endl;
			findPrevious(ignoreCaseCheckbox);
		} else
		{
			std::cout << "Searching next";
			if (ignoreCaseCheckbox)
				std::cout << " (case-insensitive)";
			std::cout << std::endl;
			findNext(ignoreCaseCheckbox);
		}
	}

	if (ImGui::IsKeyPressed(ImGuiKey_Escape))
	{
		editor_state.active_find_box = false;
		editor_state.block_input = false;
	}
}