/*
	File: views/qt/clipboard.h
	Description: IClipboard backed by the Qt platform clipboard
	(QClipboard), mirroring views/imgui/clipboard for the ImGui backend.
*/

#pragma once

#include "../../platform/clipboard.h"

class Clipboard : public IClipboard
{
  public:
	void setText(const std::string &text) override;
	std::string text() const override;
};
