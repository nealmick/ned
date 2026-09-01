#include "clipboard_imgui.h"

#include "imgui.h"

void ImGuiClipboard::setText(const std::string &text)
{
	ImGui::SetClipboardText(text.c_str());
}

std::string ImGuiClipboard::text() const
{
	const char *clip = ImGui::GetClipboardText();
	return clip ? std::string(clip) : std::string();
}
