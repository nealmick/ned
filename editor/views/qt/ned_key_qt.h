/*
	File: views/qt/ned_key_qt.h
	Description: NedKey -> Qt key conversion. The only place the Qt backend
	translates keybind keys (sibling of views/imgui/ned_key_imgui.h).
*/

#pragma once

#include "../../platform/ned_key.h"
#include <Qt>

inline int qtKeyFromNed(NedKey key)
{
	if (key == NedKey::None)
		return Qt::Key_unknown;
	const int k = static_cast<int>(key);
	const int a = static_cast<int>(NedKey::A);
	if (k >= a && k <= static_cast<int>(NedKey::Z))
		return Qt::Key_A + (k - a);
	if (k >= static_cast<int>(NedKey::D0) && k <= static_cast<int>(NedKey::D9))
		return Qt::Key_0 + (k - static_cast<int>(NedKey::D0));

	switch (key)
	{
	case NedKey::Space:
		return Qt::Key_Space;
	case NedKey::Enter:
		return Qt::Key_Return;
	case NedKey::Escape:
		return Qt::Key_Escape;
	case NedKey::Tab:
		return Qt::Key_Tab;
	case NedKey::Backspace:
		return Qt::Key_Backspace;
	case NedKey::Delete:
		return Qt::Key_Delete;
	case NedKey::Insert:
		return Qt::Key_Insert;
	case NedKey::UpArrow:
		return Qt::Key_Up;
	case NedKey::DownArrow:
		return Qt::Key_Down;
	case NedKey::LeftArrow:
		return Qt::Key_Left;
	case NedKey::RightArrow:
		return Qt::Key_Right;
	case NedKey::Home:
		return Qt::Key_Home;
	case NedKey::End:
		return Qt::Key_End;
	case NedKey::PageUp:
		return Qt::Key_PageUp;
	case NedKey::PageDown:
		return Qt::Key_PageDown;
	case NedKey::LeftCtrl:
		return Qt::Key_Control;
	case NedKey::RightCtrl:
		return Qt::Key_Control;
	case NedKey::LeftShift:
		return Qt::Key_Shift;
	case NedKey::RightShift:
		return Qt::Key_Shift;
	case NedKey::LeftAlt:
		return Qt::Key_Alt;
	case NedKey::RightAlt:
		return Qt::Key_Alt;
	case NedKey::LeftSuper:
		return Qt::Key_Meta;
	case NedKey::RightSuper:
		return Qt::Key_Meta;
	case NedKey::F1:
		return Qt::Key_F1;
	case NedKey::F2:
		return Qt::Key_F2;
	case NedKey::F3:
		return Qt::Key_F3;
	case NedKey::F4:
		return Qt::Key_F4;
	case NedKey::F5:
		return Qt::Key_F5;
	case NedKey::F6:
		return Qt::Key_F6;
	case NedKey::F7:
		return Qt::Key_F7;
	case NedKey::F8:
		return Qt::Key_F8;
	case NedKey::F9:
		return Qt::Key_F9;
	case NedKey::F10:
		return Qt::Key_F10;
	case NedKey::F11:
		return Qt::Key_F11;
	case NedKey::F12:
		return Qt::Key_F12;
	case NedKey::Apostrophe:
		return Qt::Key_Apostrophe;
	case NedKey::Comma:
		return Qt::Key_Comma;
	case NedKey::Minus:
		return Qt::Key_Minus;
	case NedKey::Period:
		return Qt::Key_Period;
	case NedKey::Slash:
		return Qt::Key_Slash;
	case NedKey::Semicolon:
		return Qt::Key_Semicolon;
	case NedKey::Equal:
		return Qt::Key_Equal;
	case NedKey::LeftBracket:
		return Qt::Key_BracketLeft;
	case NedKey::Backslash:
		return Qt::Key_Backslash;
	case NedKey::RightBracket:
		return Qt::Key_BracketRight;
	case NedKey::GraveAccent:
		return Qt::Key_QuoteLeft;
	default:
		break; // letters/digits handled above; None/Count fall through
	}
	return Qt::Key_unknown;
}
