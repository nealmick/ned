/*
File: ui_settings.h
Description: UI settings management utilities
*/

#pragma once

#include "util/settings.h"

class UISettings
{
  public:
	// Load and manage sidebar visibility settings
	static void loadSidebarSettings();

	// Load and manage agent pane visibility settings
	static void loadAgentPaneSettings();

	// Adjust agent split position based on sidebar visibility
	static void adjustAgentSplitPosition();

	// Getter methods for global variables
	static bool isSidebarVisible();
	static bool isAgentPaneVisible();
	static float getAgentSplitPos();

  public:
	// Global variables (externs)
	static bool showSidebar;
	static bool showAgentPane;
	static float agentSplitPos;
};