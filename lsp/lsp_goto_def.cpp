#include "lsp_goto_def.h"
#include "../editor/editor.h"			// Access to gEditor, editor_state
#include "../editor/editor_line_jump.h" // Access to gEditorScroll
#include "editor_scroll.h"
#include "files.h"	 // Access to gFileExplorer
#include <algorithm> // For std::min, std::max
#include <cstdio>
#include <iostream>
#include <sstream>
#include <string>

using json = nlohmann::json;

LSPGotoDef gLSPGotoDef;

LSPGotoDef::LSPGotoDef()
	: currentRequestId(2000), showDefinitionOptions(false), selectedDefinitionIndex(0)
{
}
LSPGotoDef::~LSPGotoDef() = default;

bool LSPGotoDef::gotoDefinition(const std::string &filePath, int line, int character)
{
	if (!gLSPManager.isInitialized())
	{
		std::cout << "\033[31mLSP GotoDef:\033[0m Not initialized" << std::endl;
		return false;
	}

	if (!gLSPManager.selectAdapterForFile(filePath))
	{
		std::cout << "\033[31mLSP GotoDef:\033[0m No LSP adapter available for file: " << filePath
				  << std::endl;
		return false;
	}

	int requestId = getNextRequestId();
	std::cout << "\033[35mLSP GotoDef:\033[0m Requesting definition at line " << line << ", char "
			  << character << " (ID: " << requestId << ")" << std::endl;

	std::string request = std::string(R"({
            "jsonrpc": "2.0",
            "id": )" + std::to_string(requestId) +
									  R"(,
            "method": "textDocument/definition",
            "params": {
                "textDocument": {
                    "uri": "file://)" +
									  filePath +
									  R"("
                },
                "position": {
                    "line": )" + std::to_string(line) +
									  R"(,
                    "character": )" + std::to_string(character) +
									  R"(
                }
            }
        })");

	if (!gLSPManager.sendRequest(request))
	{
		std::cout << "\033[31mLSP GotoDef:\033[0m Failed to send request" << std::endl;
		return false;
	}

	const int MAX_ATTEMPTS = 15;
	for (int attempt = 0; attempt < MAX_ATTEMPTS; attempt++)
	{
		std::cout << "\033[35mLSP GotoDef:\033[0m Waiting for definition "
					 "response (attempt "
				  << (attempt + 1) << ")" << std::endl;

		int contentLength = 0;
		std::string response = gLSPManager.readResponse(&contentLength);

		if (response.empty())
		{
			std::cout << "\033[31mLSP GotoDef:\033[0m Empty response received" << std::endl;
			if (attempt == MAX_ATTEMPTS - 1)
			{
				std::cout << "\033[31mLSP GotoDef:\033[0m Timeout waiting for "
							 "response."
						  << std::endl;
				return false;
			}
			continue;
		}

		if (response.find("\"id\":" + std::to_string(requestId)) != std::string::npos)
		{
			std::cout << "\033[32mLSP GotoDef:\033[0m Received response for "
						 "request ID "
					  << requestId << std::endl;

			parseDefinitionResponse(response); // Pass the raw response

			if (showDefinitionOptions)
			{
				return true;
			} else if (response.find("\"error\":") != std::string::npos)
			{
				std::cout << "\033[31mLSP GotoDef:\033[0m Error reported in "
							 "server response."
						  << std::endl;
				return false; // Error from server
			} else
			{
				std::cout << "\033[33mLSP GotoDef:\033[0m No definition "
							 "locations found or parsed from the response."
						  << std::endl;
				return true;
			}
		}

		std::cout << "\033[33mLSP GotoDef:\033[0m Received unrelated response. "
					 "Continuing..."
				  << std::endl;
	}

	std::cout << "\033[31mLSP GotoDef:\033[0m Exceeded maximum attempts "
				 "waiting for response ID "
			  << requestId << std::endl;
	return false;
}

void LSPGotoDef::parseDefinitionResponse(const std::string &response)
{

	definitionLocations.clear();   // Clear previous results
	showDefinitionOptions = false; // Assume no options initially

	try
	{
		json j = json::parse(response);

		if (!j.contains("result"))
		{
			if (j.contains("error"))
			{
				std::cout << "\033[31mLSP GotoDef Parse:\033[0m Response "
							 "contains an error object."
						  << std::endl;
			} else
			{
				std::cout << "\033[31mLSP GotoDef Parse:\033[0m Response "
							 "missing 'result' key."
						  << std::endl;
			}
			return;
		}

		const auto &result = j["result"];

		if (result.is_null())
		{
			std::cout << "\033[32mLSP GotoDef Parse:\033[0m 'result' is null. "
						 "No definition found."
					  << std::endl;
		} else if (result.is_object() && result.contains("uri") && result.contains("range"))
		{
			std::cout << "\033[32mLSP GotoDef Parse:\033[0m Found single "
						 "'result' object (Location)."
					  << std::endl;
			json results_array = json::array({result});
			parseDefinitionArray(results_array); // Call helper to parse
		} else if (result.is_array())
		{
			std::cout << "\033[32mLSP GotoDef Parse:\033[0m Found 'result' "
						 "array with "
					  << result.size() << " items." << std::endl;
			if (result.empty())
			{
				std::cout << "\033[33mLSP GotoDef Parse:\033[0m Result array "
							 "is empty."
						  << std::endl;
			} else
			{
				parseDefinitionArray(result); // Call helper to parse the array
			}
		} else
		{
			std::cout << "\033[31mLSP GotoDef Parse:\033[0m 'result' key "
						 "contains unexpected data type: "
					  << result.type_name() << std::endl;
		}
	} catch (json::parse_error &e)
	{
		std::cerr << "\033[31mLSP GotoDef Parse:\033[0m JSON parsing error: " << e.what() << '\n'
				  << "Exception id: " << e.id << std::endl;
		definitionLocations.clear(); // Ensure list is empty on error
		showDefinitionOptions = false;
		return; // Stop processing on parse error
	} catch (json::exception &e)
	{
		std::cerr << "\033[31mLSP GotoDef Parse:\033[0m JSON exception: " << e.what() << '\n'
				  << "Exception id: " << e.id << std::endl;
		definitionLocations.clear();
		showDefinitionOptions = false;
		return;
	}

	// Update state based on whether locations were found AFTER parsing
	if (!definitionLocations.empty())
	{
		std::cout << "\033[32mLSP GotoDef Parse:\033[0m Finished Parsing. Found "
				  << definitionLocations.size() << " definition location(s)." << std::endl;
		showDefinitionOptions = true; // Set flag to true ONLY if locations were added
		selectedDefinitionIndex = 0;  // Reset selection
	} else
	{
		// This means result was null, empty array, or parsing failed for all
		// items.
		std::cout << "\033[33mLSP GotoDef Parse:\033[0m Finished Parsing. No "
					 "valid definition locations were successfully extracted "
					 "or none were found."
				  << std::endl;
		// showDefinitionOptions remains false
	}
}

// --- Helper function to parse array items (Location or LocationLink) ---
// --- ADDED IMPLEMENTATION ---
void LSPGotoDef::parseDefinitionArray(const json &results_array)
{
	for (const auto &item : results_array)
	{
		if (!item.is_object())
		{
			std::cout << "\033[33mLSP GotoDef Parse:\033[0m Skipping "
						 "non-object item in results array."
					  << std::endl;
			continue;
		}

		std::string uri = "";
		int startLine = -1, startChar = -1;
		int endLine = -1, endChar = -1;
		bool parsed_successfully = false;

		// Try parsing as LocationLink first (more specific keys)
		if (item.contains("targetUri") && item.contains("targetRange"))
		{
			if (item["targetUri"].is_string() && item["targetRange"].is_object())
			{
				std::string fullUri = item["targetUri"].get<std::string>();
				if (fullUri.rfind("file://", 0) == 0)
					uri = fullUri.substr(7);
				else
					uri = fullUri;

				const auto &range_json = item["targetRange"];
				// Check the *inner* structure of targetRange as well
				if (range_json.is_object() && range_json.contains("start") &&
					range_json["start"].is_object())
				{
					startLine = range_json["start"].value("line", -1);
					startChar = range_json["start"].value("character", -1);

					if (range_json.contains("end") && range_json["end"].is_object())
					{
						endLine = range_json["end"].value("line", -1);
						endChar = range_json["end"].value("character", -1);
					}
					// Consider parse successful if start is valid
					parsed_successfully = (!uri.empty() && startLine != -1 && startChar != -1);
					if (parsed_successfully)
						std::cout << "\033[35mLSP GotoDef Parse:\033[0m Parsed "
									 "as LocationLink."
								  << std::endl;
				}
			}
		}
		// Else, try parsing as regular Location
		else if (item.contains("uri") && item.contains("range"))
		{
			if (item["uri"].is_string() && item["range"].is_object())
			{
				std::string fullUri = item["uri"].get<std::string>();
				if (fullUri.rfind("file://", 0) == 0)
					uri = fullUri.substr(7);
				else
					uri = fullUri;

				const auto &range_json = item["range"];
				// Check the *inner* structure of range as well
				if (range_json.is_object() && range_json.contains("start") &&
					range_json["start"].is_object())
				{
					startLine = range_json["start"].value("line", -1);
					startChar = range_json["start"].value("character", -1);

					if (range_json.contains("end") && range_json["end"].is_object())
					{
						endLine = range_json["end"].value("line", -1);
						endChar = range_json["end"].value("character", -1);
					}
					// Consider parse successful if start is valid
					parsed_successfully = (!uri.empty() && startLine != -1 && startChar != -1);
					if (parsed_successfully)
						std::cout << "\033[35mLSP GotoDef Parse:\033[0m Parsed "
									 "as Location."
								  << std::endl;
				}
			}
		}

		// Add if parsing was successful for either type
		if (parsed_successfully)
		{
			std::cout << "\033[32mLSP GotoDef Parse:\033[0m Adding Location: " << uri
					  << " [Line: " << startLine + 1 << " Char: " << startChar + 1 << "]"
					  << std::endl;
			definitionLocations.push_back({uri, startLine, startChar, endLine, endChar});
		} else
		{
			// Only log if it was an object but didn't match expected structures
			if (item.is_object())
			{
				std::cout << "\033[33mLSP GotoDef Parse:\033[0m Skipping item - "
							 "failed to parse as Location or LocationLink. Content: "
						  << item.dump() << std::endl;
			}
		}
	} // End for loop
}

bool LSPGotoDef::hasDefinitionOptions() const
{
	return showDefinitionOptions && !definitionLocations.empty();
}

void LSPGotoDef::renderDefinitionOptions()
{
	if (!hasDefinitionOptions())
	{ // Combined check using the method
		editor_state.block_input = false;
		return;
	}

	editor_state.block_input = true;

	float itemHeight = ImGui::GetTextLineHeightWithSpacing();
	float padding = 16.0f;
	float titleHeight = itemHeight + 4.0f;
	float footerHeight = itemHeight + padding;
	float contentHeight = itemHeight * definitionLocations.size();
	float totalHeight = titleHeight + contentHeight + footerHeight + padding * 2;

	totalHeight = std::min(totalHeight, ImGui::GetIO().DisplaySize.y * 0.6f);

	float desiredWidth = 600.0f;
	ImVec2 windowSize(desiredWidth, totalHeight);
	windowSize.x = std::min(windowSize.x, ImGui::GetIO().DisplaySize.x * 0.9f);

	ImVec2 windowPos(ImGui::GetIO().DisplaySize.x * 0.5f - windowSize.x * 0.5f,
					 ImGui::GetIO().DisplaySize.y * 0.35f - windowSize.y * 0.5f);

	ImGui::SetNextWindowPos(windowPos, ImGuiCond_Always);
	ImGui::SetNextWindowSize(windowSize, ImGuiCond_Always);

	ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
								   ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar |
								   ImGuiWindowFlags_NoMouseInputs;
	float availableContentHeight = totalHeight - titleHeight - footerHeight - padding * 2;
	if (contentHeight > availableContentHeight)
	{
		windowFlags &= ~ImGuiWindowFlags_NoScrollbar; // Enable scrollbar if needed
	}

	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(padding, padding));
	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 10.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8.0f, 8.0f));
	ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(gSettings.getSettings()["backgroundColor"][0].get<float>()* .8,
			   gSettings.getSettings()["backgroundColor"][1].get<float>()* .8,
			   gSettings.getSettings()["backgroundColor"][2].get<float>()* .8,
			   1.0f));
	ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.3f, 0.3f, 0.3f, 1.0f));
	ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.15f, 0.15f, 0.15f, 1.0f)); // Unused?
	ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(1.0f, 0.1f, 0.7f, 0.3f));
	ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(1.0f, 0.1f, 0.7f, 0.4f));
	ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(1.0f, 0.1f, 0.7f, 0.5f));

	if (ImGui::Begin("##DefinitionOptions", nullptr, windowFlags))
	{ // Use nullptr for p_open

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
				showDefinitionOptions = false;
				editor_state.block_input = false;
			}
			else
			{
				showDefinitionOptions = false;
				editor_state.block_input = false;
			}
		}
		if (ImGui::IsKeyPressed(ImGuiKey_Escape))
		{
			showDefinitionOptions = false;
			editor_state.block_input = false;
		}

		ImGui::Text(
						   "Go to Definition (%zu)",
						   definitionLocations.size());
		ImGui::Separator();

		bool useChildWindow = !(windowFlags & ImGuiWindowFlags_NoScrollbar);
		if (useChildWindow)
		{
			ImGui::BeginChild("##DefListScroll",
							  ImVec2(0, availableContentHeight),
							  false,
							  ImGuiWindowFlags_HorizontalScrollbar |
								  ImGuiWindowFlags_NoMouseInputs);
		}

		if (!ImGui::IsAnyItemActive())
		{
			if (ImGui::IsKeyPressed(ImGuiKey_UpArrow))
			{
				selectedDefinitionIndex = (selectedDefinitionIndex > 0)
											  ? selectedDefinitionIndex - 1
											  : definitionLocations.size() - 1;
				if (useChildWindow)
					ImGui::SetScrollHereY(0.0f);
			}
			if (ImGui::IsKeyPressed(ImGuiKey_DownArrow))
			{
				selectedDefinitionIndex =
					(selectedDefinitionIndex + 1) % definitionLocations.size();
				if (useChildWindow)
					ImGui::SetScrollHereY(1.0f);
			}
		}

		for (size_t i = 0; i < definitionLocations.size(); i++)
		{
			const auto &loc = definitionLocations[i];
			bool is_selected = (selectedDefinitionIndex == i);

			std::string filename = loc.uri;
			size_t lastSlash = filename.find_last_of("/\\");
			if (lastSlash != std::string::npos)
			{
				filename = filename.substr(lastSlash + 1);
			}
			std::string label = filename + ":" + std::to_string(loc.startLine + 1) + ":" +
								std::to_string(loc.startChar + 1);

			if (ImGui::Selectable(label.c_str(),
								  is_selected,
								  ImGuiSelectableFlags_AllowDoubleClick |
									  ImGuiSelectableFlags_SpanAllColumns))
			{
				selectedDefinitionIndex = i;
				if (ImGui::IsMouseDoubleClicked(0))
				{
					goto handle_enter_key_def; // Use unique label
				}
			}

			if (is_selected &&
				(ImGui::IsKeyPressed(ImGuiKey_UpArrow) || ImGui::IsKeyPressed(ImGuiKey_DownArrow)))
			{
				ImGui::SetScrollHereY();
			}
			if (is_selected && ImGui::IsWindowAppearing())
			{
				ImGui::SetScrollHereY();
			}
		}

		if (useChildWindow)
		{
			ImGui::EndChild();
		}

		ImGui::Separator();
		ImGui::Text("Up/Down Enter");

		if (ImGui::IsKeyPressed(ImGuiKey_Enter) || ImGui::IsKeyPressed(ImGuiKey_KeypadEnter))
		{
		handle_enter_key_def: // Unique label
			if (selectedDefinitionIndex >= 0 &&
				selectedDefinitionIndex < definitionLocations.size())
			{ // Check bounds
				const auto &selected = definitionLocations[selectedDefinitionIndex];
				std::cout << "Selected definition at " << selected.uri << " line "
						  << (selected.startLine + 1) << " char " << (selected.startChar + 1)
						  << std::endl;

				if (selected.uri != gFileExplorer.currentFile)
				{
					gFileExplorer.loadFileContent(selected.uri, [selected]() {
						gEditorScroll.pending_cursor_centering = true;
						gEditorScroll.pending_cursor_line = selected.startLine;
						gEditorScroll.pending_cursor_char = selected.startChar;
					});
				} else {
					int index = 0;
					int currentLine = 0;
					std::cout << "Calculating cursor position..." << std::endl;

					while (currentLine < selected.startLine &&
						   index < editor_state.fileContent.length())
					{
						if (editor_state.fileContent[index] == '\n')
						{
							currentLine++;
						}
						index++;
					}

					index += selected.startChar;
					index = std::min(index, (int)editor_state.fileContent.length());
					editor_state.cursor_index = index;
					editor_state.center_cursor_vertical = true;
					gEditorScroll.centerCursorVertically();
				}
				showDefinitionOptions = false;
				editor_state.block_input = false;
			}
		} else if (ImGui::IsKeyPressed(ImGuiKey_Escape))
		{ 
			showDefinitionOptions = false;
			editor_state.block_input = false;
		}

		ImGui::End();
	} else
	{
		// If Begin returned false
		if (showDefinitionOptions)
		{
			showDefinitionOptions = false;
			editor_state.block_input = false;
		}
	}

	ImGui::PopStyleColor(6);
	ImGui::PopStyleVar(4);

	// Final check
	if (!showDefinitionOptions)
	{
		editor_state.block_input = false;
	}
}