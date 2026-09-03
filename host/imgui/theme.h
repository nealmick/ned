/*
	File: host/imgui/theme.h
	Description: Applies the settings profile (background color, theme
	text, scrollbar chrome, dock/title-bar colors) to the ImGui style —
	the ImGui counterpart of host/qt/theme.h's QPalette.
*/

#pragma once

struct ImGuiStyle;

class Settings;

namespace NedImGuiTheme {

// Profile theme → ImGuiStyle: background + theme text + scrollbar sizing.
void applyProfile(ImGuiStyle &style, const Settings &settings);

// Standalone hosts only: match window/child/title-bar/tab colors to the
// theme background and hide the dock window-menu button.
void applyStandaloneBackground(ImGuiStyle &style, const Settings &settings);

} // namespace NedImGuiTheme
