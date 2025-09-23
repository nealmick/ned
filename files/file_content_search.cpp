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
		editor_state.center_cursor_vertical = true;

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
		startPos = (lastFoundPos == 0) ? editor_state.fileContent.length() - 1
									   : lastFoundPos - 1;
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

	std::cout << "Searching backwards for '" << findText << "' starting from position "
			  << startPos;
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
		editor_state.center_cursor_vertical = true;

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

		// Check if there's an active selection and use it as search text
		if (editor_state.selection_active &&
			editor_state.selection_start != editor_state.selection_end)
		{
			int start =
				std::min(editor_state.selection_start, editor_state.selection_end);
			int end = std::max(editor_state.selection_start, editor_state.selection_end);
			findText = editor_state.fileContent.substr(start, end - start);
		}
		// If no selection, keep the existing findText (don't clear it)

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
	if (needsInputUnblock)
	{
		if (--unblockDelayFrames <= 0)
		{
			editor_state.block_input = false;
			needsInputUnblock = false;
		}
	} else
	{
		if (editor_state.active_find_box)
		{
			editor_state.block_input = true;
		}
	}

	// Only render if the find box is active.
	if (!editor_state.active_find_box)
	{
		findBoxRectSet = false; // Reset rect tracking when inactive
		return;
	}

	// Check for mouse click outside the find box area
	if (findBoxRectSet && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
	{
		if (!ImGui::IsMouseHoveringRect(findBoxRectMin, findBoxRectMax, true))
		{
			// Click occurred outside find box - close it
			editor_state.active_find_box = false;
			editor_state.block_input = false;
		}
	}

	// We'll declare this static here since it's used in both the UI and
	// keyboard shortcuts
	static bool ignoreCaseCheckbox = true;

	// Wrap entire find box in a group to get its bounding rect
	ImGui::BeginGroup();
	{
		// Reserve ~50% of available width for the input field.
		float availWidth = ImGui::GetContentRegionAvail().x;
		float inputWidth = availWidth * 0.5f;
		ImGui::SetNextItemWidth(inputWidth);

		// Render the input field in its own group.
		ImGui::BeginGroup();
		ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, Style::FRAME_ROUNDING);
		ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, Style::BORDER_SIZE);
		ImGui::PushStyleColor(
			ImGuiCol_FrameBg,
			ImVec4(gSettings.getSettings()["backgroundColor"][0].get<float>() * .8,
				   gSettings.getSettings()["backgroundColor"][1].get<float>() * .8,
				   gSettings.getSettings()["backgroundColor"][2].get<float>() * .8,
				   1.0f));
		ImGui::PushStyleColor(ImGuiCol_Border, Style::BORDER_COLOR);

		static char inputBuffer[256] = "";
		if (findBoxShouldFocus)
		{
			// Copy findText to input buffer when focusing
			strncpy(inputBuffer, findText.c_str(), sizeof(inputBuffer) - 1);
			inputBuffer[sizeof(inputBuffer) - 1] = '\0';
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

		// Show current match position and total matches if we have a search
		if (!findText.empty())
		{
			ImGui::SameLine();
			ImGui::Dummy(ImVec2(10, 0)); // 10 pixels of spacing.
			ImGui::SameLine();

			// Calculate current match position
			int currentMatch = -1;
			int totalMatches = 0;

			if (!findText.empty())
			{
				std::vector<size_t> positions = findAllOccurrences(ignoreCaseCheckbox);
				totalMatches = positions.size();

				if (lastFoundPos != std::string::npos)
				{
					for (int i = 0; i < totalMatches; i++)
					{
						if (positions[i] == lastFoundPos)
						{
							currentMatch = i + 1;
							break;
						}
					}
				}
			}

			// Display the match counter
			if (currentMatch == -1)
			{
				ImGui::Text("Not Found");
			} else if (totalMatches > 0)
			{
				ImGui::Text("%d/%d", currentMatch, totalMatches);
			}
		}

		ImGui::SameLine();
		ImGui::Dummy(ImVec2(10, 0)); // 10 pixels of spacing.
		ImGui::SameLine();

		ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, Style::FRAME_ROUNDING);
		ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, Style::BORDER_SIZE);
		ImGui::PushStyleColor(
			ImGuiCol_FrameBg,
			ImVec4(gSettings.getSettings()["backgroundColor"][0].get<float>() * .8,
				   gSettings.getSettings()["backgroundColor"][1].get<float>() * .8,
				   gSettings.getSettings()["backgroundColor"][2].get<float>() * .8,
				   1.0f));
		ImGui::PushStyleColor(ImGuiCol_Border, Style::BORDER_COLOR);

		ImGui::Checkbox("Case Insensitive", &ignoreCaseCheckbox);
		ImGui::PopStyleColor(2);
		ImGui::PopStyleVar(2);
	}
	ImGui::EndGroup(); // End entire find box group

	// Store bounding rect for click detection
	findBoxRectMin = ImGui::GetItemRectMin();
	findBoxRectMax = ImGui::GetItemRectMax();
	findBoxRectSet = true;

	handleFindBoxKeyboardShortcuts(ignoreCaseCheckbox);
}
std::vector<size_t> FileContentSearch::findAllOccurrences(bool ignoreCase)
{
	std::vector<size_t> positions;
	if (findText.empty())
		return positions;

	size_t startPos = 0;
	while (true)
	{
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

		if (foundPos == std::string::npos)
			break;

		positions.push_back(foundPos);
		startPos = foundPos + 1;

		if (startPos >= editor_state.fileContent.length())
			break;
	}

	return positions;
}
void FileContentSearch::handleFindBoxKeyboardShortcuts(bool ignoreCaseCheckbox)
{
	ImGuiIO &io = ImGui::GetIO();
	if (ImGui::IsKeyPressed(ImGuiKey_Enter, false))
	{
		if (io.KeyCtrl)
		{
			// Handle Ctrl+Enter - add multi-cursors
			std::vector<size_t> positions = findAllOccurrences(ignoreCaseCheckbox);
			if (!positions.empty())
			{
				// Clear existing cursors
				editor_state.multi_cursor_indices.clear();
				editor_state.multi_cursor_prefered_columns.clear();

				// Set main cursor to first occurrence
				editor_state.cursor_index = positions[0];
				editor_state.selection_start = 0;
				editor_state.selection_end = 0;
				editor_state.selection_active = false;
				gEditorCursor.calculateVisualColumn();

				// Add multi-cursors for remaining positions
				for (size_t i = 1; i < positions.size(); ++i)
				{
					int pos = positions[i];
					editor_state.multi_cursor_indices.push_back(pos);
					editor_state.multi_cursor_prefered_columns.push_back(
						EditorCursor::CalculateVisualColumnForPosition(
							pos,
							editor_state.fileContent,
							editor_state.editor_content_lines));
				}

				// Ensure visibility
				// editor_state.ensure_cursor_visible = {true, true};
				editor_state.center_cursor_vertical = true;

				std::cout << "Added " << positions.size() << " cursors\n";

				if (!positions.empty())
				{
					// Schedule input unblock
					needsInputUnblock = true;
					unblockDelayFrames = 2; // Wait 2 frames
					editor_state.active_find_box = false;
					// Keep block_input true for now
				}
			}
		} else if (io.KeyShift)
		{
			findPrevious(ignoreCaseCheckbox);
		} else
		{
			findNext(ignoreCaseCheckbox);
		}
	}

	if (ImGui::IsKeyPressed(ImGuiKey_Escape))
	{
		editor_state.active_find_box = false;
		editor_state.block_input = false;
	}
}