#include "lsp_uri_options.h"
#include "../../../editor/editor_api.h"
#include "../../../editor/util/doc_path.h"
#include "../../../editor/views/imgui/editor_surface.h"
#include "../../../files/files.h"
#include "../../../lsp/lsp_goto.h"
#include "../../../lsp/lsp_includes.h"
#include "../../../util/settings.h"
#include "imgui.h"
#include <algorithm>

LSPUriOptions::LSPUriOptions(EditorApi &api,
							 EditorSurface &surface,
							 FileExplorer &fileExplorer,
							 Settings &settings)
	: api(&api), surface(&surface), fileExplorer(&fileExplorer), settings(&settings)
{
}

LSPUriOptions::~LSPUriOptions() {}

void LSPUriOptions::present(const std::string &title,
							const std::vector<LSPLocation> &options,
							bool &show)
{
	if (!show || !api || !fileExplorer || !settings)
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
	api->setBlockInput(true);

	// Height calculations
	const float fs = ImGui::GetFontSize();
	float itemHeight = ImGui::GetTextLineHeightWithSpacing();
	float padding = fs * 0.8f;
	float separatorHeight = ImGui::GetTextLineHeight() * 0.4f;
	float titleHeight = itemHeight + separatorHeight + fs * 0.2f;
	float footerHeight = itemHeight + padding;
	float contentHeight = itemHeight * std::max(options.size(), size_t(1));
	float totalHeight = titleHeight + contentHeight + footerHeight + padding * 2;
	const float maxHeight = ImGui::GetIO().DisplaySize.y * 0.5f;

	const float desiredWidth = fs * 30.0f;
	ImVec2 windowSize(desiredWidth,
					  std::min(totalHeight, maxHeight) +
						  (options.size() <= 1 ? fs * 0.5f : fs * 1.25f));
	windowSize.x = std::min(windowSize.x, ImGui::GetIO().DisplaySize.x * 0.9f);

	ImVec2 windowPos;
	if (settings && settings->isEmbedded)
	{
		const NedVec2 panePos = (surface ? surface->layout().panePos : NedVec2{});
		const NedVec2 paneSize = (surface ? surface->layout().paneSize : NedVec2{});

		windowPos = ImVec2(panePos.x + paneSize.x * 0.5f - windowSize.x * 0.5f,
						   panePos.y + paneSize.y * 0.35f - windowSize.y * 0.5f);

		if (windowPos.x < panePos.x)
			windowPos.x = panePos.x;
		if (windowPos.x + windowSize.x > panePos.x + paneSize.x)
			windowPos.x = panePos.x + paneSize.x - windowSize.x;
		if (windowPos.y < panePos.y)
			windowPos.y = panePos.y;
		if (windowPos.y + windowSize.y > panePos.y + paneSize.y)
			windowPos.y = panePos.y + paneSize.y - windowSize.y;
	} else
	{
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
	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, fs * 0.5f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(fs * 0.4f, fs * 0.4f));

	// Theme colors from settings (guarded like the dashboard: a profile
	// without backgroundColor must not throw every frame).
	ImVec4 windowBg = ImVec4(0.1f, 0.1f, 0.1f, 1.0f);
	if (settings->settings.contains("backgroundColor") &&
		settings->settings["backgroundColor"].is_array() &&
		settings->settings["backgroundColor"].size() >= 3)
	{
		const auto &bg = settings->settings["backgroundColor"];
		windowBg = ImVec4(bg[0].get<float>() * 0.8f,
						  bg[1].get<float>() * 0.8f,
						  bg[2].get<float>() * 0.8f,
						  1.0f);
	}
	ImGui::PushStyleColor(ImGuiCol_WindowBg, windowBg);
	ImGui::PushStyleColor(ImGuiCol_ChildBg,
						  windowBg); // Match child background to window background
	ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.3f, 0.3f, 0.3f, 1.0f));
	ImGui::PushStyleColor(ImGuiCol_FrameBg, windowBg);
	ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(1.0f, 0.1f, 0.7f, 0.3f));
	ImGui::PushStyleColor(
		ImGuiCol_HeaderHovered,
		ImVec4(
			0.0f, 0.0f, 0.0f, 0.0f)); // Default transparent hover (overridden per-item)
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
				api->setBlockInput(false);
			}
		}

		// Handle escape key
		if (ImGui::IsKeyPressed(ImGuiKey_Escape))
		{
			show = false;
			api->setBlockInput(false);
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
				std::string label = lsp_locations::locationLabel(option);

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
						commit();
						show = false;
						api->setBlockInput(false);
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
			commit();
			show = false;
			api->setBlockInput(false);
		}

		ImGui::End(); // End main window
	} else
	{
		if (show)
		{
			show = false;
			api->setBlockInput(false);
		}
	}

	// Cleanup
	ImGui::PopStyleColor(7); // Original 7 colors (WindowBg, ChildBg, Border, FrameBg,
							 // Header, HeaderHovered, HeaderActive)
	ImGui::PopStyleVar(4);

	if (!show)
	{
		api->setBlockInput(false);
		selectedIndex = 0; // Reset selection when popup closes
	}
}

void LSPUriOptions::commit()
{
	if (selectedIndex >= currentOptions.size())
		return;

	const LSPLocation &selected = currentOptions[selectedIndex];

	// The jump runs against the DESTINATION document — after the target file
	// is loaded, so the UTF-16 column converts against its buffer.
	auto jump = [this, selected]() { lspJumpToLocation(*api, selected); };

	// clangd canonicalizes paths; the open editor may hold a different
	// spelling of the same file — normalize before deciding "same document"
	// (a mismatch would open a duplicate tab instead of jumping).
	if (DocPath::normalize(selected.file) != DocPath::normalize(api->path()))
	{
		fileExplorer->loadFileContent(selected.file, jump);
	} else
	{
		jump();
	}
}
