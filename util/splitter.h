#pragma once
#include <imgui.h>

class Settings;

class Splitter
{
  public:
	explicit Splitter(Settings &settings);

	void renderSplitter(float padding, float availableWidth);

	static bool showSidebar;

  private:
	static constexpr float VISIBLE_WIDTH = 1.0f;
	static constexpr float HOVER_WIDTH = 6.0f;
	static constexpr float HOVER_EXPANSION = 3.0f;
	static constexpr float HOVER_DELAY = 0.15f;

	static constexpr ImU32 COLOR_BASE = IM_COL32(134, 134, 134, 140);
	static constexpr ImU32 COLOR_HOVER = IM_COL32(13, 110, 253, 255);
	static constexpr ImU32 COLOR_ACTIVE = IM_COL32(11, 94, 215, 255);

	Settings &settings;
	float mainHoverStartTime = -1.0f;
};
