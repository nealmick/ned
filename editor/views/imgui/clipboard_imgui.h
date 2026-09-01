/*
	File: views/imgui/clipboard_imgui.h
	Description: IClipboard backed by ImGui's platform clipboard.
*/

#pragma once

#include "../../platform/clipboard.h"

class ImGuiClipboard : public IClipboard
{
  public:
	void setText(const std::string &text) override;
	std::string text() const override;
};
