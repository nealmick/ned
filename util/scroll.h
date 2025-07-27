/*
File: scroll.h
Description: Scroll accumulator handling functions
*/

#pragma once

#include <GLFW/glfw3.h>
#include <functional>

class Scroll
{
  public:
	// Scroll accumulator handling
	static void handleScrollAccumulators(double &scrollXAccumulator,
										 double &scrollYAccumulator);

	// Scroll callback that takes a function to update the accumulators
	static void scrollCallback(GLFWwindow *window,
							   double xoffset,
							   double yoffset,
							   std::function<void(double, double)> updateAccumulators);
};