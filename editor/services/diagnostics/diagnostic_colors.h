#pragma once

/*
	Shared diagnostic severity presentation — gutter marks, squiggle color,
	tooltip labels. One table so every backend's gutter, text view, and
	tooltip agree. Values are plain floats; backend views convert to their
	color type (ImGui: views/imgui/diagnostic_style.h).
*/

struct DiagnosticSeverityRGB
{
	float r = 1.0f;
	float g = 0.0f;
	float b = 0.0f;
	float a = 1.0f;
};

// 1 error, 2 warning, 3 info, 4+ hint.
inline DiagnosticSeverityRGB DiagnosticSeverityColor(int severity)
{
	switch (severity)
	{
	case 2:
		return {210.0f / 255.0f, 160.0f / 255.0f, 50.0f / 255.0f, 230.0f / 255.0f};
	case 3:
		return {70.0f / 255.0f, 140.0f / 255.0f, 210.0f / 255.0f, 230.0f / 255.0f};
	default:
		if (severity >= 4)
			return {120.0f / 255.0f, 160.0f / 255.0f, 120.0f / 255.0f, 220.0f / 255.0f};
		return {220.0f / 255.0f, 70.0f / 255.0f, 70.0f / 255.0f, 240.0f / 255.0f};
	}
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
