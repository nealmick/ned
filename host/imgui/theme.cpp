#include "theme.h"

#include "../../../util/settings.h"

#include "imgui.h"

#include <algorithm>
#include <string>

namespace NedImGuiTheme {

void applyProfile(ImGuiStyle &style, const Settings &settings)
{
	if (!settings.settings.is_object())
		return;

	// Standalone only: embedded mode leaves WindowBg/ChildBg to the host style.
	if (!settings.isEmbedded && settings.settings.contains("backgroundColor") &&
		settings.settings["backgroundColor"].is_array() &&
		settings.settings["backgroundColor"].size() >= 4)
	{
		const auto &bg = settings.settings["backgroundColor"];
		style.Colors[ImGuiCol_WindowBg] = ImVec4(bg[0].get<float>(),
												 bg[1].get<float>(),
												 bg[2].get<float>(),
												 bg[3].get<float>());
	}

	const std::string theme = settings.settings.value("theme", std::string("default"));
	if (!settings.settings.contains("themes") ||
		!settings.settings["themes"].is_object() ||
		!settings.settings["themes"].contains(theme) ||
		!settings.settings["themes"][theme].contains("text"))
		return;

	const auto &textColor = settings.settings["themes"][theme]["text"];
	if (!textColor.is_array() || textColor.size() < 4)
		return;
	ImVec4 textCol(textColor[0].get<float>(),
				   textColor[1].get<float>(),
				   textColor[2].get<float>(),
				   textColor[3].get<float>());
	style.Colors[ImGuiCol_Text] = textCol;
	style.Colors[ImGuiCol_TextDisabled] =
		ImVec4(textCol.x * 0.6f, textCol.y * 0.6f, textCol.z * 0.6f, textCol.w);
	style.Colors[ImGuiCol_TextSelectedBg] = ImVec4(1.0f, 0.1f, 0.7f, 0.3f);

	if (!settings.isEmbedded)
	{
		const float dpi = std::max(1.0f, style.FontScaleDpi);
		style.ScrollbarSize = 30.0f * dpi;
		style.Colors[ImGuiCol_ScrollbarBg] = ImVec4(0, 0, 0, 0);
		style.Colors[ImGuiCol_ScrollbarGrab] = ImVec4(0, 0, 0, 0);
		style.Colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0, 0, 0, 0);
		style.Colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0, 0, 0, 0);
	} else
	{
		style.Colors[ImGuiCol_ScrollbarBg] = ImVec4(0, 0, 0, 0);
	}
	style.ScaleAllSizes(1.0f);
}

void applyStandaloneBackground(ImGuiStyle &style, const Settings &settings)
{
	// Embedded hosts keep the host ImGui theme for window/child backgrounds.
	// Standalone: match window, child, and editor tab bar to theme background.
	if (!settings.isEmbedded && settings.settings.contains("backgroundColor") &&
		settings.settings["backgroundColor"].is_array() &&
		settings.settings["backgroundColor"].size() >= 3)
	{
		const auto &bg = settings.settings["backgroundColor"];
		const float a =
			settings.settings["backgroundColor"].size() >= 4 ? bg[3].get<float>() : 1.0f;
		const ImVec4 bgCol(bg[0].get<float>(), bg[1].get<float>(), bg[2].get<float>(), a);
		const ImVec4 bgOpaque(
			bg[0].get<float>(), bg[1].get<float>(), bg[2].get<float>(), 1.0f);
		style.Colors[ImGuiCol_ChildBg] = bgOpaque;
		style.Colors[ImGuiCol_WindowBg] = bgCol;
		// Dock title-bar tabs: no fill by default; slight highlight only on hover.
		const ImVec4 tabNone(0.0f, 0.0f, 0.0f, 0.0f);
		style.Colors[ImGuiCol_Tab] = tabNone;
		style.Colors[ImGuiCol_TabSelected] = tabNone;
		style.Colors[ImGuiCol_TabDimmed] = tabNone;
		style.Colors[ImGuiCol_TabDimmedSelected] = tabNone;
		// Soft lift from text color so hover reads on light and dark themes.
		const ImVec4 &text = style.Colors[ImGuiCol_Text];
		style.Colors[ImGuiCol_TabHovered] = ImVec4(text.x, text.y, text.z, 0.12f);
		// Window/tab close (X) draws ButtonHovered / ButtonActive as its fill.
		style.Colors[ImGuiCol_Button] = ImVec4(text.x, text.y, text.z, 0.12f);
		style.Colors[ImGuiCol_ButtonHovered] = ImVec4(text.x, text.y, text.z, 0.18f);
		style.Colors[ImGuiCol_ButtonActive] = ImVec4(text.x, text.y, text.z, 0.30f);
		// Hide selected-tab overline so active doesn't look different.
		style.Colors[ImGuiCol_TabSelectedOverline] = tabNone;
		style.Colors[ImGuiCol_TabDimmedSelectedOverline] = tabNone;
		// Window / dock title bars (active, inactive, collapsed) match theme bg.
		style.Colors[ImGuiCol_TitleBg] = bgOpaque;
		style.Colors[ImGuiCol_TitleBgActive] = bgOpaque;
		style.Colors[ImGuiCol_TitleBgCollapsed] = bgOpaque;
		style.Colors[ImGuiCol_MenuBarBg] = bgOpaque;
		style.Colors[ImGuiCol_DockingEmptyBg] = bgOpaque;
		// Hide the dock title-bar / tab-bar window-menu (tab list) button.
		// See https://github.com/ocornut/imgui/issues/4880
		style.WindowMenuButtonPosition = ImGuiDir_None;
	}
}

} // namespace NedImGuiTheme
