/*
File: init.h
Description: Initialization class moved from ned.cpp
*/

#pragma once

#include <GLFW/glfw3.h>

class Init
{
  public:
	static void initializeResources();
	static void initializeImGui(GLFWwindow *window);
};