#pragma once

/*
	Shared diagnostic severity presentation — gutter marks, squiggle color,
	tooltip labels. One table so the gutter, text view, and tooltip agree.
*/

#include "imgui.h"

// 1 error, 2 warning, 3 info, 4+ hint.
inline ImU32 DiagnosticSeverityMark(int severity)
{
	switch (severity)
	{
	case 2:
		return IM_COL32(210, 160, 50, 230);
	case 3:
		return IM_COL32(70, 140, 210, 230);
	default:
		if (severity >= 4)
			return IM_COL32(120, 160, 120, 220);
		return IM_COL32(220, 70, 70, 240);
	}
}

inline ImVec4 DiagnosticSeverityColor(int severity)
{
	const ImU32 mark = DiagnosticSeverityMark(severity);
	return ImGui::ColorConvertU32ToFloat4(mark);
}

inline const char *DiagnosticSeverityLabel(int severity)
{
	switch (severity)
	{
	case 2:
		return "Warning";
	case 3:
		return "Info";
	case 4:
		return "Hint";
	default:
		return "Error";
	}
}
