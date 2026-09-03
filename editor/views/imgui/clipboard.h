/*
	File: views/imgui/clipboard.h
	Description: IClipboard backed by ImGui's platform clipboard.
*/

#pragma once

#include "../../platform/clipboard.h"

class Clipboard : public IClipboard
{
  public:
	void setText(const std::string &text) override;
	std::string text() const override;
};
