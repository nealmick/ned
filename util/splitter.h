/*
File: splitter.h
Description: Splitter rendering and interaction logic for NED text editor.
*/
#pragma once

#include <imgui.h>

class Splitter
{
  public:
	Splitter();
	~Splitter();

	// Render the main splitter (between file explorer and editor)
	void renderSplitter(float padding, float availableWidth);

	// Render the agent splitter (between editor and agent pane)
	void renderAgentSplitter(float padding, float availableWidth, bool sidebarVisible);

	// UI Settings methods (moved from UISettings class)
	static void loadSidebarSettings();
	static void loadAgentPaneSettings();
	static void adjustAgentSplitPosition();
	static bool isSidebarVisible();
	static bool isAgentPaneVisible();
	static float getAgentSplitPos();

  public:
	// Global variables (moved from UISettings)
	static bool showSidebar;
	static bool showAgentPane;
	static float agentSplitPos;

  private:
	// Interaction settings
	static constexpr float VISIBLE_WIDTH = 1.0f;   // Rendered width at rest
	static constexpr float HOVER_WIDTH = 6.0f;	   // Invisible hitbox size
	static constexpr float HOVER_EXPANSION = 3.0f; // Expanded visual width
	static constexpr float HOVER_DELAY = 0.15f;	   // Seconds before hover effect

	// Color constants
	static constexpr ImU32 COLOR_BASE = IM_COL32(134, 134, 134, 140);
	static constexpr ImU32 COLOR_HOVER = IM_COL32(13, 110, 253, 255);
	static constexpr ImU32 COLOR_ACTIVE = IM_COL32(11, 94, 215, 255);

	// Splitter width constant
	static constexpr float AGENT_SPLITTER_WIDTH = 6.0f;

	// Utility functions
	static float clamp(float value, float min, float max);
	void drawSplitterVisual(const ImVec2 &min, const ImVec2 &max, ImU32 color);
	ImU32 determineSplitterColor(bool is_hovered, bool is_active, bool visual_hover);

	// State tracking for hover effects (separate for each splitter)
	float main_hover_start_time = -1.0f;
	float agent_hover_start_time = -1.0f;

	// State tracking for agent splitter dragging
	bool dragging = false;
	float dragOffset = 0.0f;
};