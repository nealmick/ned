#include "lsp_goto_ref.h"
#include "../.build/lib/lsp-framework/generated/lsp/messages.h"
#include "../.build/lib/lsp-framework/generated/lsp/types.h"
#include "../editor/editor.h"
#include "../files/files.h"
#include "../lib/lsp-framework/lsp/messagehandler.h"
#include "../lsp/lsp_client.h"
#include <chrono>
#include <iostream>
#include <thread>

// Global instance
LSPGotoRef gLSPGotoRef;

LSPGotoRef::LSPGotoRef() : show(false), pending(false) {}

LSPGotoRef::~LSPGotoRef() = default;

void LSPGotoRef::get()
{
	if (!gLSPClient.isInitialized())
		return;

	// Get current cursor position
	int row = gEditor.getLineFromPos(editor_state.cursor_index);
	int line_start = editor_state.editor_content_lines[row];
	int column = editor_state.cursor_index - line_start;

	// Set pending state
	pending = true;
	references.clear();
	show = true;

	// Call the request function
	request(row,
			column,
			[this](const std::vector<std::map<std::string, std::string>> &results) {
				references = results;
				pending = false;

				// Print the structured results object
				printResponse(results);
			});
}

void LSPGotoRef::request(
	int line,
	int character,
	std::function<void(const std::vector<std::map<std::string, std::string>> &)> callback)
{
	try
	{
		// Create request parameters
		lsp::ReferenceParams params;
		params.textDocument.uri = lsp::FileUri::fromPath(gFileExplorer.currentFile);
		params.position.line = line;
		params.position.character = character;
		params.context.includeDeclaration =
			false; // Don't include the definition, just references

		// Send request and handle response asynchronously
		auto response =
			gLSPClient.getMessageHandler()
				->sendRequest<lsp::requests::TextDocument_References>(std::move(params));

		asyncResponse = std::make_shared<std::future<void>>(
			std::async(std::launch::async,
					   [this, response = std::move(response), callback]() mutable {
						   try
						   {
							   auto result = response.result.get();
							   auto locations = processResponse(result);
							   callback(locations);
						   } catch (const std::exception &e)
						   {
							   // Pass empty vector on error
							   std::vector<std::map<std::string, std::string>> empty;
							   callback(empty);
						   }
					   }));

		// Initial callback with empty vector to indicate request sent
		std::vector<std::map<std::string, std::string>> empty;
		callback(empty);

	} catch (const std::exception &e)
	{
		// Pass empty vector on error
		std::vector<std::map<std::string, std::string>> empty;
		callback(empty);
	}
}

std::vector<std::map<std::string, std::string>>
LSPGotoRef::processResponse(const lsp::TextDocument_ReferencesResult &result)
{
	std::vector<std::map<std::string, std::string>> results;

	if (result.isNull())
		return results; // Return empty vector

	auto locations = result.value();
	if (locations.empty())
		return results; // Return empty vector

	for (const auto &loc : locations)
	{
		std::map<std::string, std::string> entry;
		entry["file"] = std::string(loc.uri.path());
		entry["row"] = std::to_string(loc.range.start.line + 1); // Convert to 1-based
		entry["col"] =
			std::to_string(loc.range.start.character + 1); // Convert to 1-based
		results.push_back(entry);
	}

	return results;
}

void LSPGotoRef::printResponse(
	const std::vector<std::map<std::string, std::string>> &results)
{
	std::cout << "LSP GotoRef: [" << std::endl;
	for (const auto &result : results)
	{
		std::cout << "  {";
		for (const auto &pair : result)
		{
			std::cout << pair.first << ": " << pair.second << ", ";
		}
		std::cout << "}" << std::endl;
	}
	std::cout << "]" << std::endl;
}

void LSPGotoRef::render()
{
	if (!show)
		return;

	ImGui::SetNextWindowSize(ImVec2(300, 100), ImGuiCond_FirstUseEver);

	if (ImGui::Begin("Goto References", &show))
	{
		ImGui::Text("Rendering goto references");

		if (pending)
		{
			ImGui::Text("Loading references...");
		} else if (references.empty())
		{
			ImGui::Text("No references available");
		} else
		{
			for (size_t i = 0; i < references.size(); ++i)
			{
				const auto &result = references[i];
				std::string display = std::to_string(i + 1) + ". " + result.at("file") +
									  " (line " + result.at("row") + ", col " +
									  result.at("col") + ")";
				ImGui::TextWrapped("%s", display.c_str());
			}
		}

		if (ImGui::IsKeyPressed(ImGuiKey_Escape))
		{
			show = false;
		}
	}
	ImGui::End();
}