/*
	File: views/qt/diagnostic_style.h
	Description: Shared diagnostic severity presentation (backend-neutral
	table in services/diagnostics/diagnostic_colors.h), converted to QColor
	so the gutter, text view, and tooltip agree. Parallel of
	views/imgui/diagnostic_style.h (ImU32/ImVec4 mapping).
*/

#pragma once

#include "../../services/diagnostics/diagnostic_colors.h"

#include <QColor>

// 1 error, 2 warning, 3 info, 4+ hint.
inline QColor DiagnosticSeverityColorQ(int severity)
{
	const DiagnosticSeverityRGB c = DiagnosticSeverityColor(severity);
	return QColor::fromRgbF(c.r, c.g, c.b, c.a);
}
