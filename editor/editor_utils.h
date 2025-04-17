#pragma once
#include "imgui.h"
#include <GLFW/glfw3.h> // For time functions
#include <algorithm>
#include <cmath>
#include <vector>

namespace EditorUtils {
// Helper function for rainbow colors that maintains its own synchronized time
inline ImVec4 GetRainbowColor(float timeScale = 2.0f)
{
	// Use a static float to track time
	static float sharedBlinkTime = 0.0f;
	static double lastUpdateTime = 0.0;

	// Update time only once per frame
	double currentTime = glfwGetTime();
	if (currentTime > lastUpdateTime)
	{
		sharedBlinkTime += (currentTime - lastUpdateTime) * timeScale;
		lastUpdateTime = currentTime;
	}

	float t = sharedBlinkTime;
	float r = sin(t) * 0.5f + 0.5f;
	float g = sin(t + 2.0944f) * 0.5f + 0.5f; // 2.0944 is 2π/3
	float b = sin(t + 4.1888f) * 0.5f + 0.5f; // 4.1888 is 4π/3
	return ImVec4(r, g, b, 1.0f);
}

// Helper method to calculate the line number from cursor position
inline int GetLineFromPosition(const std::vector<int> &line_starts, int content_index)
{
	auto it = std::upper_bound(line_starts.begin(), line_starts.end(), content_index);
	return std::distance(line_starts.begin(), it) - 1;
}

} // namespace EditorUtils
