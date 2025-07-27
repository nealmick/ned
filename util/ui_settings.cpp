/*
File: ui_settings.cpp
Description: UI settings management implementation
*/

#include "ui_settings.h"
#include "util/settings.h"
#include <iostream>

// Global externals
extern Settings gSettings;

// Global variables
bool UISettings::showSidebar = true;
bool UISettings::showAgentPane = true;
float UISettings::agentSplitPos = 0.75f;

void UISettings::loadSidebarSettings()
{
	showSidebar = gSettings.getSettings().value("sidebar_visible", true);
}

void UISettings::loadAgentPaneSettings()
{
	showAgentPane = gSettings.getSettings().value("agent_pane_visible", true);
}

void UISettings::adjustAgentSplitPosition()
{
	// Adjust agent split position if sidebar is hidden (first load only)
	if (!gSettings.isAgentSplitPosProcessed() && !showSidebar)
	{
		float currentAgentSplitPos = gSettings.getAgentSplitPos();
		float originalValue = currentAgentSplitPos;
		// When sidebar is hidden, the agent split position represents editor width,
		// not agent pane width. So we need to invert it: 1 - saved_agent_position
		float newAgentSplitPos = 1.0f - currentAgentSplitPos;
		gSettings.setAgentSplitPos(newAgentSplitPos);
		std::cout << "[Ned] Adjusted agent_split_pos from " << originalValue << " to "
				  << newAgentSplitPos << " (sidebar hidden) on first load" << std::endl;
	}
}

bool UISettings::isSidebarVisible() { return showSidebar; }

bool UISettings::isAgentPaneVisible() { return showAgentPane; }

float UISettings::getAgentSplitPos() { return agentSplitPos; }