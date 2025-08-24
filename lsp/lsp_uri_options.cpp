#include "lsp_uri_options.h"
#include "imgui.h"
#include <iostream>

// Global instance
LSPUriOptions gLSPUriOptions;

LSPUriOptions::LSPUriOptions() {}

LSPUriOptions::~LSPUriOptions() {}

void LSPUriOptions::render(const std::string &title,
						   const std::vector<std::map<std::string, std::string>> &options,
						   bool &show)
{
	if (!show)
		return;

	ImGui::SetNextWindowSize(ImVec2(300, 100), ImGuiCond_FirstUseEver);

	if (ImGui::Begin(title.c_str(), &show))
	{
		if (options.empty())
		{
			ImGui::Text("No results available");
		} else
		{
			for (size_t i = 0; i < options.size(); ++i)
			{
				const auto &result = options[i];
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