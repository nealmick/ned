#include "splitter.h"
#include "settings.h"
#include <algorithm>

bool Splitter::showSidebar = true;

Splitter::Splitter(Settings &settings) : settings(settings) {}

void Splitter::renderSplitter(float padding, float availableWidth)
{
	ImGui::SameLine(0, 0);

	ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(0, 0, 0, 0));
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, IM_COL32(0, 0, 0, 0));
	ImGui::PushStyleColor(ImGuiCol_ButtonActive, IM_COL32(0, 0, 0, 0));
	ImGui::Button("##vsplitter_left", ImVec2(HOVER_WIDTH, -1));

	const bool is_hovered = ImGui::IsItemHovered();
	const bool is_active = ImGui::IsItemActive();
	bool visual_hover = false;

	if (is_hovered && !is_active)
	{
		if (mainHoverStartTime < 0)
			mainHoverStartTime = static_cast<float>(ImGui::GetTime());
		visual_hover = (ImGui::GetTime() - mainHoverStartTime) >= HOVER_DELAY;
	} else
	{
		mainHoverStartTime = -1.0f;
	}

	ImVec2 min = ImGui::GetItemRectMin();
	ImVec2 max = ImGui::GetItemRectMax();
	const float width = (visual_hover || is_active) ? HOVER_EXPANSION : VISIBLE_WIDTH;
	min.x += (HOVER_WIDTH - width) * 0.5f;
	max.x = min.x + width;

	ImU32 color = COLOR_BASE;
	if (is_active)
		color = COLOR_ACTIVE;
	else if (visual_hover)
		color = COLOR_HOVER;
	ImGui::GetWindowDrawList()->AddRectFilled(min, max, color);

	if (is_active)
	{
		const float mouse_x = ImGui::GetMousePos().x - ImGui::GetWindowPos().x;
		const float new_split = std::clamp(
			(mouse_x - padding * 2) / (availableWidth - padding * 4 - 6), 0.1f, 0.9f);
		settings.settings["splitPos"] = new_split;
	}

	ImGui::PopStyleColor(3);
}
