#include "clipboard.h"

#include "imgui.h"

void Clipboard::setText(const std::string &text)
{
	ImGui::SetClipboardText(text.c_str());
}

std::string Clipboard::text() const
{
	const char *clip = ImGui::GetClipboardText();
	return clip ? std::string(clip) : std::string();
}
