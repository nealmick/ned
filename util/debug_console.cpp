// debug_console.cpp
#include "debug_console.h"
#include <iostream>

DebugConsole &gDebugConsole = DebugConsole::getInstance();

DebugConsole::DebugConsole()
	: isVisible(false), consoleBuf(*this, std::cout.rdbuf()) // Pass original buffer
	  ,
	  oldCoutBuf(nullptr)
{
	// Redirect cout to our buffer
	oldCoutBuf = std::cout.rdbuf();
	std::cout.rdbuf(&consoleBuf);
}
void DebugConsole::addLog(const std::string &message)
{
	logs.push_back(message); // Back to pushing at the end

	// Optional: Keep a maximum number of logs
	const size_t maxLogs = 1000;
	while (logs.size() > maxLogs)
	{
		logs.pop_front(); // Remove oldest logs if we exceed the limit
	}
}
void DebugConsole::render()
{
	if (!isVisible)
		return;

	ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.1f, 0.1f, 0.1f, 0.3f));
	ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 5.0f);

	ImGui::BeginChild("ScrollingRegion",
					  ImVec2(ImGui::GetWindowWidth() * 0.6f, 150),
					  true,
					  ImGuiWindowFlags_HorizontalScrollbar);

	// Iterate through logs from beginning to end (oldest first)
	auto it = logs.begin();
	auto end = logs.end();

	while (it != end)
	{
		std::string cleanLog = *it;
		ImVec4 textColor(1.0f, 1.0f, 1.0f, 1.0f); // Default white

		// Find the first color code
		if (cleanLog.find("\033[95m") != std::string::npos)
		{
			textColor = ImVec4(1.0f, 0.5f, 1.0f, 1.0f); // Bright Magenta
		} else if (cleanLog.find("\033[32m") != std::string::npos)
		{
			textColor = ImVec4(0.0f, 1.0f, 0.0f, 1.0f); // Green
		} else if (cleanLog.find("\033[35m") != std::string::npos)
		{
			textColor = ImVec4(1.0f, 0.0f, 1.0f, 1.0f); // Magenta
		}

		// Remove all ANSI escape sequences
		size_t pos;
		while ((pos = cleanLog.find("\033[")) != std::string::npos)
		{
			size_t end = cleanLog.find("m", pos);
			if (end != std::string::npos)
			{
				cleanLog.erase(pos, end - pos + 1);
			}
		}

		ImGui::PushStyleColor(ImGuiCol_Text, textColor);
		ImGui::TextUnformatted(cleanLog.c_str());
		ImGui::PopStyleColor();

		++it;
	}

	// Always scroll to bottom when new log appears
	ImGui::SetScrollHereY(1.0f);

	ImGui::EndChild();
	ImGui::PopStyleVar();
	ImGui::PopStyleColor();
}