/*
File: splitter.cpp
Description: Splitter rendering and interaction logic implementation for NED text editor.
*/

#include "splitter.h"
#include "keybinds.h"
#include "settings.h"

// External dependencies
extern Settings gSettings;
extern bool showAgentPane;

Splitter::Splitter() {}

Splitter::~Splitter() {}

float Splitter::clamp(float value, float min, float max)
{
	if (value < min)
		return min;
	if (value > max)
		return max;
	return value;
}

void Splitter::drawSplitterVisual(const ImVec2 &min, const ImVec2 &max, ImU32 color)
{
	ImGui::GetWindowDrawList()->AddRectFilled(min, max, color);
}

ImU32 Splitter::determineSplitterColor(bool is_hovered, bool is_active, bool visual_hover)
{
	if (is_active)
	{
		return COLOR_ACTIVE;
	} else if (visual_hover)
	{
		return COLOR_HOVER;
	}
	return COLOR_BASE;
}

void Splitter::renderSplitter(float padding, float availableWidth)
{
	ImGui::SameLine(0, 0);

	ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(0, 0, 0, 0));
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, IM_COL32(0, 0, 0, 0));
	ImGui::PushStyleColor(ImGuiCol_ButtonActive, IM_COL32(0, 0, 0, 0));

	// Create invisible button with larger hitbox
	ImGui::Button("##vsplitter_left", ImVec2(HOVER_WIDTH, -1));

	// Hover delay logic
	bool visual_hover = false;
	const bool is_hovered = ImGui::IsItemHovered();
	const bool is_active = ImGui::IsItemActive();

	if (is_hovered && !is_active)
	{
		if (main_hover_start_time < 0)
		{
			main_hover_start_time = ImGui::GetTime();
		}
		float elapsed = ImGui::GetTime() - main_hover_start_time;
		visual_hover = elapsed >= HOVER_DELAY;
	} else
	{
		main_hover_start_time = -1.0f;
	}

	// Calculate dimensions
	ImVec2 min = ImGui::GetItemRectMin();
	ImVec2 max = ImGui::GetItemRectMax();
	const float width = (visual_hover || is_active) ? HOVER_EXPANSION : VISIBLE_WIDTH;

	// Center the visible splitter in the hover zone
	min.x += (HOVER_WIDTH - width) * 0.5f;
	max.x = min.x + width;

	// Determine color and draw
	ImU32 current_color = determineSplitterColor(is_hovered, is_active, visual_hover);
	drawSplitterVisual(min, max, current_color);

	// Handle dragging
	if (is_active)
	{
		const float mouse_x = ImGui::GetMousePos().x - ImGui::GetWindowPos().x;
		float new_split = (mouse_x - padding * 2) / (availableWidth - padding * 4 - 6);
		// Clamp so that editor is always at least 10% of availableWidth
		float rightSplit = gSettings.getAgentSplitPos();
		bool agentPaneVisible = showAgentPane;
		float maxLeftSplit = agentPaneVisible ? (0.9f - rightSplit) : 0.9f;
		new_split = clamp(new_split, 0.1f, maxLeftSplit);
		gSettings.setSplitPos(new_split);
	}

	ImGui::PopStyleColor(3);
}

void Splitter::renderAgentSplitter(float padding,
								   float availableWidth,
								   bool sidebarVisible)
{
	ImGui::SameLine(0, 0);

	ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(0, 0, 0, 0));
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, IM_COL32(0, 0, 0, 0));
	ImGui::PushStyleColor(ImGuiCol_ButtonActive, IM_COL32(0, 0, 0, 0));

	// Create invisible button with larger hitbox
	ImGui::Button("##vsplitter_right", ImVec2(HOVER_WIDTH, -1));

	// Hover delay logic
	bool visual_hover = false;
	const bool is_hovered = ImGui::IsItemHovered();
	const bool is_active = ImGui::IsItemActive();

	if (is_hovered && !is_active)
	{
		if (agent_hover_start_time < 0)
		{
			agent_hover_start_time = ImGui::GetTime();
		}
		float elapsed = ImGui::GetTime() - agent_hover_start_time;
		visual_hover = elapsed >= HOVER_DELAY;
	} else
	{
		agent_hover_start_time = -1.0f;
	}

	// Calculate dimensions
	ImVec2 min = ImGui::GetItemRectMin();
	ImVec2 max = ImGui::GetItemRectMax();
	const float width = (visual_hover || is_active) ? HOVER_EXPANSION : VISIBLE_WIDTH;

	// Center the visible splitter in the hover zone
	min.x += (HOVER_WIDTH - width) * 0.5f;
	max.x = min.x + width;

	// Determine color and draw
	ImU32 current_color = determineSplitterColor(is_hovered, is_active, visual_hover);
	drawSplitterVisual(min, max, current_color);

	// Drag logic
	float rightSplit = gSettings.getAgentSplitPos();
	float splitterX;
	if (sidebarVisible)
	{
		splitterX = availableWidth - (availableWidth * rightSplit) - AGENT_SPLITTER_WIDTH;
	} else
	{
		splitterX = availableWidth * rightSplit;
	}

	if (ImGui::IsItemActive() && !dragging)
	{
		dragging = true;
		dragOffset = ImGui::GetMousePos().x - (ImGui::GetWindowPos().x + splitterX);
	}
	if (!ImGui::IsItemActive() && dragging)
	{
		dragging = false;
		gSettings.saveSettings();
	}
	if (dragging)
	{
		float mouseX = ImGui::GetMousePos().x - ImGui::GetWindowPos().x - dragOffset;
		float new_split;
		float leftSplit = gSettings.getSplitPos();
		float maxRightSplit = sidebarVisible ? (0.85f - leftSplit) : 0.85f;
		if (sidebarVisible)
		{
			new_split =
				clamp((availableWidth - mouseX - AGENT_SPLITTER_WIDTH) / availableWidth,
					  0.15f,
					  maxRightSplit);
		} else
		{
			new_split = clamp(mouseX / availableWidth, 0.15f, maxRightSplit);
		}
		gSettings.setAgentSplitPos(new_split);
	}

	ImGui::PopStyleColor(3);
}