#include "lsp_symbol_info.h"
#include "lsp_includes.h"

// Global instance
LSPSymbolInfo gLSPSymbolInfo;

LSPSymbolInfo::LSPSymbolInfo() : show(false), pending(false) {}

LSPSymbolInfo::~LSPSymbolInfo() = default;

void LSPSymbolInfo::get()
{
	if (!gLSPClient.isInitialized())
		return;

	// Get current cursor position
	int row = gEditor.getLineFromPos(editor_state.cursor_index);
	int line_start = editor_state.editor_content_lines[row];
	int column = editor_state.cursor_index - line_start;

	// Set pending state
	pending = true;

	// Request symbol info
	request(row, column, [this](const std::string &result) {
		symbolInfo = result;
		pending = false;
		if (symbolInfo != "No hover info")
		{
			show = true;
		}
	});
}

void LSPSymbolInfo::request(int line,
							int character,
							std::function<void(const std::string &)> callback)
{
	try
	{
		// Create the hover request parameters
		lsp::HoverParams params;
		params.textDocument.uri = lsp::FileUri::fromPath(gFileExplorer.currentFile);
		params.position.line = line;
		params.position.character = character;

		gLSPClient.getMessageHandler()->sendRequest<lsp::requests::TextDocument_Hover>(
			std::move(params),
			[callback](auto &&result) {
				if (!result.isNull())
				{
					auto &contents = result.value().contents;
					if (std::holds_alternative<lsp::MarkupContent>(contents))
					{
						callback(std::get<lsp::MarkupContent>(contents).value);
					} else
					{
						callback("Got hover data, but failed to process");
					}
				} else
				{
					callback("No hover info");
				}
			},
			[callback](auto &error) { callback("LSP error"); });

	} catch (const std::exception &e)
	{
		callback("Error sending hover request: " + std::string(e.what()));
	}
}

void LSPSymbolInfo::render()
{
	if (!show)
		return;

	// Calculate cursor position in screen space
	int cursor_line = gEditor.getLineFromPos(editor_state.cursor_index);
	float cursor_x = gEditorCursor.getCursorXPosition(editor_state.text_pos,
													  editor_state.fileContent,
													  editor_state.cursor_index);

	// Get the actual screen position of the text cursor
	ImVec2 cursor_screen_pos = editor_state.text_pos;
	cursor_screen_pos.x = cursor_x;
	cursor_screen_pos.y += cursor_line * editor_state.line_height;

	// Set initial display position relative to cursor
	ImVec2 displayPosition = cursor_screen_pos;
	displayPosition.y += editor_state.line_height + 5.0f; // Position below cursor line
	displayPosition.x += 5.0f;							  // Small horizontal offset

	// Width configuration
	const float min_width = 450.0f;
	const float max_width = 650.0f;
	const float screen_padding = 20.0f;
	const float text_wrap_padding = 30.0f;

	// Set initial window position and size constraints
	ImGui::SetNextWindowPos(displayPosition, ImGuiCond_Appearing);
	ImGui::SetNextWindowSizeConstraints(ImVec2(min_width, 0), // Minimum size
										ImVec2(max_width,
											   FLT_MAX) // Maximum size
	);

	// Style settings
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(14, 10));
	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 6.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8, 6));
	ImGui::PushStyleColor(
		ImGuiCol_WindowBg,
		ImVec4(gSettings.getSettings()["backgroundColor"][0].get<float>() * .8,
			   gSettings.getSettings()["backgroundColor"][1].get<float>() * .8,
			   gSettings.getSettings()["backgroundColor"][2].get<float>() * .8,
			   1.0f));
	ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.25f, 0.25f, 0.25f, 1.0f));

	if (ImGui::Begin("SymbolInfoWindow",
					 &show,
					 ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse |
						 ImGuiWindowFlags_AlwaysAutoResize |
						 ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoMove |
						 ImGuiWindowFlags_NoNav))
	{
		// Get current window dimensions
		ImVec2 window_pos = ImGui::GetWindowPos();
		ImVec2 window_size = ImGui::GetWindowSize();
		ImVec2 work_size = ImGui::GetMainViewport()->WorkSize;

		// Calculate available space with new width considerations
		ImVec2 clamped_pos = window_pos;
		const float right_bound = work_size.x - window_size.x - screen_padding;
		const float bottom_bound = work_size.y - window_size.y - screen_padding;

		// Horizontal clamping
		if (window_pos.x < screen_padding)
		{
			clamped_pos.x = screen_padding;
		} else if (window_pos.x > right_bound)
		{
			clamped_pos.x = right_bound;
		}

		// Vertical clamping with flip to above cursor if needed
		if (window_pos.y < screen_padding)
		{
			clamped_pos.y = screen_padding;
		} else if (window_pos.y > bottom_bound)
		{
			// If window would be cutoff at bottom, display above cursor
			clamped_pos.y = cursor_screen_pos.y - window_size.y - 15.0f;
			// Ensure we don't go above the screen
			clamped_pos.y = std::max(clamped_pos.y, screen_padding);
		}

		// Compare ImVec2 components individually
		if (clamped_pos.x != window_pos.x || clamped_pos.y != window_pos.y)
		{
			ImGui::SetWindowPos(clamped_pos);
			// Update window size after position change
			window_size = ImGui::GetWindowSize();
		}

		// Force reasonable minimum width
		if (window_size.x < min_width)
		{
			ImGui::SetWindowSize(ImVec2(min_width, window_size.y));
		}

		// Focus handling
		if (ImGui::IsWindowAppearing())
		{
			ImGui::SetWindowFocus();
		}

		// Dismissal interactions
		if (ImGui::IsMouseClicked(ImGuiMouseButton_Left))
		{
			if (!ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByPopup))
			{
				show = false;
			}
		}

		if (ImGui::IsKeyPressed(ImGuiKey_Escape))
		{
			show = false;
		}

		// Content rendering
		ImGui::PushTextWrapPos(max_width - text_wrap_padding);

		if (pending)
		{
			ImGui::Text("Loading symbol info...");
		} else if (symbolInfo.empty())
		{
			ImGui::Text("No symbol info available");
		} else
		{
			// Display the hover content with proper wrapping
			ImGui::TextWrapped("%s", symbolInfo.c_str());
		}

		ImGui::PopTextWrapPos();

		ImGui::End();
	}

	// Cleanup
	ImGui::PopStyleColor(2);
	ImGui::PopStyleVar(3);
}
