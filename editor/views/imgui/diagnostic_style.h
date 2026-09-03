#pragma once

/*
	Shared diagnostic severity presentation (backend-neutral table in
	services/diagnostics/diagnostic_colors.h), converted to ImGui types so the
	gutter, text view, and tooltip agree.
*/

#include "../../services/diagnostics/diagnostic_colors.h"

#include "imgui.h"

// 1 error, 2 warning, 3 info, 4+ hint.
inline ImU32 DiagnosticSeverityMark(int severity)
{
	// Round-to-nearest like ImGui's IM_F32_TO_INT8_SAT — truncation let
	// exact channels drift by one (210/255 * 255 floored to 209).
	const DiagnosticSeverityRGB c = DiagnosticSeverityColor(severity);
	const auto to8 = [](float v) { return static_cast<int>(v * 255.0f + 0.5f); };
	return IM_COL32(to8(c.r), to8(c.g), to8(c.b), to8(c.a));
}

inline ImVec4 DiagnosticSeverityVec4(int severity)
{
	const DiagnosticSeverityRGB c = DiagnosticSeverityColor(severity);
	return ImVec4(c.r, c.g, c.b, c.a);
}
