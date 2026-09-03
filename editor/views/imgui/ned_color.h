/*
	File: views/imgui/ned_color.h
	Description: NedColor <-> ImGui color conversions. The only place the
	ImGui backend should translate core colors.
*/

#pragma once

#include "../../platform/ned_types.h"
#include "imgui.h"

inline ImVec4 toImVec4(const NedColor &c) { return ImVec4(c.r, c.g, c.b, c.a); }

inline ImU32 toImU32(const NedColor &c)
{
	return ImGui::ColorConvertFloat4ToU32(toImVec4(c));
}

inline NedColor toNedColor(const ImVec4 &c) { return NedColor(c.x, c.y, c.z, c.w); }
