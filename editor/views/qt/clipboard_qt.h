/*
	File: views/qt/clipboard_qt.h
	Description: IClipboard backed by the Qt platform clipboard
	(QClipboard), mirroring clipboard_imgui for the ImGui backend.
*/

#pragma once

#include "../../platform/clipboard.h"

class QtClipboard : public IClipboard
{
  public:
	void setText(const std::string &text) override;
	std::string text() const override;
};
