/*
	File: views/imgui/ned_key_imgui.h
	Description: NedKey <-> ImGuiKey conversion. The only place the ImGui
	backend translates keybind keys.
*/

#pragma once

#include "../../platform/ned_key.h"
#include "imgui.h"

inline ImGuiKey imguiKeyFromNed(NedKey key)
{
	if (key == NedKey::None)
		return ImGuiKey_None;
	const int k = static_cast<int>(key);
	const int a = static_cast<int>(NedKey::A);
	if (k >= a && k <= static_cast<int>(NedKey::Z))
		return static_cast<ImGuiKey>(ImGuiKey_A + (k - a));
	if (k >= static_cast<int>(NedKey::D0) && k <= static_cast<int>(NedKey::D9))
		return static_cast<ImGuiKey>(ImGuiKey_0 + (k - static_cast<int>(NedKey::D0)));

	switch (key)
	{
	case NedKey::Space:
		return ImGuiKey_Space;
	case NedKey::Enter:
		return ImGuiKey_Enter;
	case NedKey::Escape:
		return ImGuiKey_Escape;
	case NedKey::Tab:
		return ImGuiKey_Tab;
	case NedKey::Backspace:
		return ImGuiKey_Backspace;
	case NedKey::Delete:
		return ImGuiKey_Delete;
	case NedKey::Insert:
		return ImGuiKey_Insert;
	case NedKey::UpArrow:
		return ImGuiKey_UpArrow;
	case NedKey::DownArrow:
		return ImGuiKey_DownArrow;
	case NedKey::LeftArrow:
		return ImGuiKey_LeftArrow;
	case NedKey::RightArrow:
		return ImGuiKey_RightArrow;
	case NedKey::Home:
		return ImGuiKey_Home;
	case NedKey::End:
		return ImGuiKey_End;
	case NedKey::PageUp:
		return ImGuiKey_PageUp;
	case NedKey::PageDown:
		return ImGuiKey_PageDown;
	case NedKey::LeftCtrl:
		return ImGuiKey_LeftCtrl;
	case NedKey::RightCtrl:
		return ImGuiKey_RightCtrl;
	case NedKey::LeftShift:
		return ImGuiKey_LeftShift;
	case NedKey::RightShift:
		return ImGuiKey_RightShift;
	case NedKey::LeftAlt:
		return ImGuiKey_LeftAlt;
	case NedKey::RightAlt:
		return ImGuiKey_RightAlt;
	case NedKey::LeftSuper:
		return ImGuiKey_LeftSuper;
	case NedKey::RightSuper:
		return ImGuiKey_RightSuper;
	case NedKey::F1:
		return ImGuiKey_F1;
	case NedKey::F2:
		return ImGuiKey_F2;
	case NedKey::F3:
		return ImGuiKey_F3;
	case NedKey::F4:
		return ImGuiKey_F4;
	case NedKey::F5:
		return ImGuiKey_F5;
	case NedKey::F6:
		return ImGuiKey_F6;
	case NedKey::F7:
		return ImGuiKey_F7;
	case NedKey::F8:
		return ImGuiKey_F8;
	case NedKey::F9:
		return ImGuiKey_F9;
	case NedKey::F10:
		return ImGuiKey_F10;
	case NedKey::F11:
		return ImGuiKey_F11;
	case NedKey::F12:
		return ImGuiKey_F12;
	case NedKey::Apostrophe:
		return ImGuiKey_Apostrophe;
	case NedKey::Comma:
		return ImGuiKey_Comma;
	case NedKey::Minus:
		return ImGuiKey_Minus;
	case NedKey::Period:
		return ImGuiKey_Period;
	case NedKey::Slash:
		return ImGuiKey_Slash;
	case NedKey::Semicolon:
		return ImGuiKey_Semicolon;
	case NedKey::Equal:
		return ImGuiKey_Equal;
	case NedKey::LeftBracket:
		return ImGuiKey_LeftBracket;
	case NedKey::Backslash:
		return ImGuiKey_Backslash;
	case NedKey::RightBracket:
		return ImGuiKey_RightBracket;
	case NedKey::GraveAccent:
		return ImGuiKey_GraveAccent;
	default:
		break; // letters/digits handled above; None/Count fall through
	}
	return ImGuiKey_None;
}
