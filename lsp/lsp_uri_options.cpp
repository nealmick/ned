#include "lsp_uri_options.h"
#include "../util/settings.h"
#include "imgui.h"
#include "lsp_includes.h"
#include <algorithm>

// Global instance
LSPUriOptions gLSPUriOptions;

LSPUriOptions::LSPUriOptions() : selectedIndex(0), isEmbedded(false) {}

LSPUriOptions::~LSPUriOptions() {}

void LSPUriOptions::render(const std::string &title,
						   const std::vector<std::map<std::string, std::string>> &options,
						   bool &show)
{
	if (!show)
	{
		return;
	}

	// Don't auto-close on empty - let calling class manage this
	// The window will show "No results available" if options is empty

	// Store current data
	currentTitle = title;
	currentOptions = options;

	// Reset selection if options changed
	if (selectedIndex >= options.size())
		selectedIndex = 0;

	// Block editor input while popup is shown
	editor_state.block_input = true;

	// Height calculations
	float itemHeight = ImGui::GetTextLineHeightWithSpacing();
	float padding = 16.0f;
	float separatorHeight = ImGui::GetTextLineHeight() * 0.4f;
	float titleHeight = itemHeight + separatorHeight + 4.0f;
	float footerHeight = itemHeight + padding;
	float contentHeight = itemHeight * std::max(options.size(), size_t(1));
	float totalHeight = titleHeight + contentHeight + footerHeight + padding * 2;
	const float maxHeight = ImGui::GetIO().DisplaySize.y * 0.5f;

	float desiredWidth = 600.0f;
	desiredWidth = std::max(desiredWidth, 500.0f);
	ImVec2 windowSize(desiredWidth,
					  std::min(totalHeight, maxHeight) +
						  (options.size() <= 1 ? 10.0f : 25.0f));
	windowSize.x = std::min(windowSize.x, ImGui::GetIO().DisplaySize.x * 0.9f);

	ImVec2 windowPos;
	if (isEmbedded)
	{
		// In embedded mode, position relative to the editor pane bounds
		ImVec2 editorPanePos = ImGui::GetWindowPos();
		ImVec2 editorPaneSize = ImGui::GetWindowSize();

		windowPos =
			ImVec2(editorPanePos.x + editorPaneSize.x * 0.5f - windowSize.x * 0.5f,
				   editorPanePos.y + editorPaneSize.y * 0.35f - windowSize.y * 0.5f);

		// Ensure the popup stays within the editor pane bounds
		if (windowPos.x < editorPanePos.x)
			windowPos.x = editorPanePos.x;
		if (windowPos.x + windowSize.x > editorPanePos.x + editorPaneSize.x)
			windowPos.x = editorPanePos.x + editorPaneSize.x - windowSize.x;
		if (windowPos.y < editorPanePos.y)
			windowPos.y = editorPanePos.y;
		if (windowPos.y + windowSize.y > editorPanePos.y + editorPaneSize.y)
			windowPos.y = editorPanePos.y + editorPaneSize.y - windowSize.y;
	} else
	{
		// In standalone mode, center on screen
		windowPos = ImVec2(ImGui::GetIO().DisplaySize.x * 0.5f - windowSize.x * 0.5f,
						   ImGui::GetIO().DisplaySize.y * 0.35f - windowSize.y * 0.5f);
	}

	ImGui::SetNextWindowPos(windowPos, ImGuiCond_Always);
	ImGui::SetNextWindowSize(windowSize, ImGuiCond_Always);

	ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoTitleBar |
								   ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
								   ImGuiWindowFlags_NoScrollbar;

	// Style setup
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(padding, padding));
	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 10.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8.0f, 8.0f));

	// Theme colors from settings
	extern Settings gSettings;
	ImVec4 windowBg =
		ImVec4(gSettings.getSettings()["backgroundColor"][0].get<float>() * 0.8f,
			   gSettings.getSettings()["backgroundColor"][1].get<float>() * 0.8f,
			   gSettings.getSettings()["backgroundColor"][2].get<float>() * 0.8f,
			   1.0f);
	ImGui::PushStyleColor(ImGuiCol_WindowBg, windowBg);
	ImGui::PushStyleColor(ImGuiCol_ChildBg,
						  windowBg); // Match child background to window background
	ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.3f, 0.3f, 0.3f, 1.0f));
	ImGui::PushStyleColor(ImGuiCol_FrameBg, windowBg);
	ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(1.0f, 0.1f, 0.7f, 0.3f));
	ImGui::PushStyleColor(
		ImGuiCol_HeaderHovered,
		ImVec4(0.0f, 0.0f, 0.0f, 0.0f)); // Default transparent hover (overridden per-item)
	ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(1.0f, 0.1f, 0.7f, 0.5f));

	if (ImGui::Begin("##LSPUriOptions", nullptr, windowFlags))
	{
		// Handle click outside window
		if (ImGui::IsMouseClicked(ImGuiMouseButton_Left))
		{
			ImVec2 mousePos = ImGui::GetMousePos();
			ImVec2 currentWindowPos = ImGui::GetWindowPos();
			ImVec2 currentWindowSize = ImGui::GetWindowSize();
			if (!ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByPopup) &&
				(mousePos.x < currentWindowPos.x ||
				 mousePos.x > currentWindowPos.x + currentWindowSize.x ||
				 mousePos.y < currentWindowPos.y ||
				 mousePos.y > currentWindowPos.y + currentWindowSize.y))
			{
				show = false;
				editor_state.block_input = false;
			}
		}

		// Handle escape key
		if (ImGui::IsKeyPressed(ImGuiKey_Escape))
		{
			show = false;
			editor_state.block_input = false;
		}

		// Fixed header
		ImGui::BeginChild("##Header", ImVec2(0, titleHeight), false);
		ImGui::Text("%s (%zu)", title.c_str(), options.size());
		ImGui::Separator();
		ImGui::EndChild();

		if (options.empty())
		{
			ImGui::Text("No results available");
		} else
		{
			// Scrollable content area
			float contentAvailableHeight =
				windowSize.y - titleHeight - footerHeight - padding * 2;
			ImGui::BeginChild("##ContentScroll",
							  ImVec2(0, contentAvailableHeight),
							  false,
							  ImGuiWindowFlags_HorizontalScrollbar |
								  ImGuiWindowFlags_AlwaysVerticalScrollbar);

			// Keyboard navigation
			if (!ImGui::IsAnyItemActive())
			{
				if (ImGui::IsKeyPressed(ImGuiKey_UpArrow))
				{
					selectedIndex =
						(selectedIndex > 0) ? selectedIndex - 1 : options.size() - 1;
					ImGui::SetScrollHereY(0.0f);
				}
				if (ImGui::IsKeyPressed(ImGuiKey_DownArrow))
				{
					selectedIndex = (selectedIndex + 1) % options.size();
					ImGui::SetScrollHereY(1.0f);
				}
			}

			// List items
			for (size_t i = 0; i < options.size(); ++i)
			{
				const auto &option = options[i];
				bool is_selected = (selectedIndex == i);

				// Format filename
				std::string filename = option.at("file");
				size_t lastSlash = filename.find_last_of("/\\");
				if (lastSlash != std::string::npos)
				{
					filename = filename.substr(lastSlash + 1);
				}
				std::string label =
					filename + ":" + option.at("row") + ":" + option.at("col");

				// For selected items, override hover color to maintain selection visibility
				if (is_selected)
				{
					ImGui::PushStyleColor(
						ImGuiCol_HeaderHovered,
						ImVec4(1.0f,
							   0.1f,
							   0.7f,
							   0.3f)); // Same as selection color to avoid color change
				} else
				{
					ImGui::PushStyleColor(
						ImGuiCol_HeaderHovered,
						ImVec4(0.0f, 0.0f, 0.0f, 0.0f)); // Transparent for non-selected
				}

				if (ImGui::Selectable(label.c_str(),
									  is_selected,
									  ImGuiSelectableFlags_AllowDoubleClick |
										  ImGuiSelectableFlags_SpanAllColumns |
										  ImGuiSelectableFlags_DontClosePopups))
				{
					selectedIndex = i;
					if (ImGui::IsMouseDoubleClicked(0))
					{
						handleSelection();
						show = false;
						editor_state.block_input = false;
					}
				}

				// Always pop the hover style color we pushed
				ImGui::PopStyleColor();

				if (is_selected && (ImGui::IsKeyPressed(ImGuiKey_UpArrow) ||
									ImGui::IsKeyPressed(ImGuiKey_DownArrow)))
				{
					ImGui::SetScrollHereY();
				}
				if (is_selected && ImGui::IsWindowAppearing())
				{
					ImGui::SetScrollHereY();
				}
			}

			ImGui::EndChild(); // End content scroll area
		}

		// Fixed footer
		ImGui::BeginChild("##Footer", ImVec2(0, footerHeight), false);
		ImGui::Separator();
		ImGui::Text("Up/Down Enter");
		ImGui::EndChild();

		// Handle Enter key
		if ((ImGui::IsKeyPressed(ImGuiKey_Enter) ||
			 ImGui::IsKeyPressed(ImGuiKey_KeypadEnter)) &&
			!options.empty())
		{
			handleSelection();
			show = false;
			editor_state.block_input = false;
		}

		ImGui::End(); // End main window
	} else
	{
		if (show)
		{
			show = false;
			editor_state.block_input = false;
		}
	}

	// Cleanup
	ImGui::PopStyleColor(7); // Original 7 colors (WindowBg, ChildBg, Border, FrameBg,
							 // Header, HeaderHovered, HeaderActive)
	ImGui::PopStyleVar(4);

	if (!show)
	{
		editor_state.block_input = false;
		selectedIndex = 0; // Reset selection when popup closes
	}
}

void LSPUriOptions::handleSelection()
{
	if (selectedIndex >= currentOptions.size())
		return;

	const auto &selected = currentOptions[selectedIndex];
	std::string filePath = selected.at("file");
	int line = std::stoi(selected.at("row")) - 1; // Convert back to 0-based
	int col = std::stoi(selected.at("col")) - 1;  // Convert back to 0-based

	std::cout << "Selected option at " << filePath << " line " << (line + 1) << std::endl;

	// Use the file loading logic from the old system

	if (filePath != gFileExplorer.currentFile)
	{
		gFileExplorer.loadFileContent(filePath, [line, col]() {
			gEditorScroll.pending_cursor_centering = true;
			gEditorScroll.pending_cursor_line = line;
			gEditorScroll.pending_cursor_char = col;
		});
	} else
	{
		// Same file - just move cursor
		int index = 0;
		int currentLine = 0;
		const std::string &content = editor_state.fileContent;

		while (currentLine < line && index < content.length())
		{
			if (content[index] == '\n')
				currentLine++;
			index++;
		}

		index += col;
		index = std::min(index, static_cast<int>(content.length()));

		editor_state.cursor_index = index;
		editor_state.center_cursor_vertical = true;
		gEditorScroll.centerCursorVertically();
	}
}