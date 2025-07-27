/*
File: scroll.cpp
Description: Scroll accumulator handling functions implementation
*/

#include "scroll.h"
#include <imgui.h>

void Scroll::handleScrollAccumulators(double &scrollXAccumulator,
									  double &scrollYAccumulator)
{
	// Handle scroll accumulators (this was removed but needed for scrolling)
	if (scrollXAccumulator != 0.0 || scrollYAccumulator != 0.0)
	{
		ImGui::GetIO().MouseWheel += scrollYAccumulator;  // Vertical
		ImGui::GetIO().MouseWheelH += scrollXAccumulator; // Horizontal
		scrollXAccumulator = 0.0;
		scrollYAccumulator = 0.0;
	}
}

void Scroll::scrollCallback(GLFWwindow *window,
							double xoffset,
							double yoffset,
							std::function<void(double, double)> updateAccumulators)
{
	updateAccumulators(xoffset * 0.2, yoffset * 0.2);
}