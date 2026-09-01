#include "ned_key.h"

#include <algorithm>
#include <iostream>

NedKey stringToNedKey(const std::string &keyString)
{
	std::string key = keyString;
	std::transform(key.begin(), key.end(), key.begin(), ::tolower);

	if (key.length() == 1)
	{
		const char c = key[0];
		if (c >= 'a' && c <= 'z')
			return static_cast<NedKey>(static_cast<int>(NedKey::A) + (c - 'a'));
		if (c >= '0' && c <= '9')
			return static_cast<NedKey>(static_cast<int>(NedKey::D0) + (c - '0'));
	}

	if (key == "space" || key == "spacebar")
		return NedKey::Space;
	if (key == "enter" || key == "return")
		return NedKey::Enter;
	if (key == "escape" || key == "esc")
		return NedKey::Escape;
	if (key == "tab")
		return NedKey::Tab;
	if (key == "backspace")
		return NedKey::Backspace;
	if (key == "delete" || key == "del")
		return NedKey::Delete;
	if (key == "insert" || key == "ins")
		return NedKey::Insert;
	if (key == "up")
		return NedKey::UpArrow;
	if (key == "down")
		return NedKey::DownArrow;
	if (key == "left")
		return NedKey::LeftArrow;
	if (key == "right")
		return NedKey::RightArrow;
	if (key == "home")
		return NedKey::Home;
	if (key == "end")
		return NedKey::End;
	if (key == "pageup" || key == "pgup")
		return NedKey::PageUp;
	if (key == "pagedown" || key == "pgdn")
		return NedKey::PageDown;
	if (key == "leftctrl" || key == "lctrl")
		return NedKey::LeftCtrl;
	if (key == "rightctrl" || key == "rctrl")
		return NedKey::RightCtrl;
	if (key == "leftshift" || key == "lshift")
		return NedKey::LeftShift;
	if (key == "rightshift" || key == "rshift")
		return NedKey::RightShift;
	if (key == "leftalt" || key == "lalt")
		return NedKey::LeftAlt;
	if (key == "rightalt" || key == "ralt")
		return NedKey::RightAlt;
	if (key == "leftsuper" || key == "lsuper" || key == "cmd" || key == "command" ||
		key == "win" || key == "windows")
		return NedKey::LeftSuper;
	if (key == "rightsuper" || key == "rsuper")
		return NedKey::RightSuper;
	if (key == "f1")
		return NedKey::F1;
	if (key == "f2")
		return NedKey::F2;
	if (key == "f3")
		return NedKey::F3;
	if (key == "f4")
		return NedKey::F4;
	if (key == "f5")
		return NedKey::F5;
	if (key == "f6")
		return NedKey::F6;
	if (key == "f7")
		return NedKey::F7;
	if (key == "f8")
		return NedKey::F8;
	if (key == "f9")
		return NedKey::F9;
	if (key == "f10")
		return NedKey::F10;
	if (key == "f11")
		return NedKey::F11;
	if (key == "f12")
		return NedKey::F12;
	if (key == "apostrophe" || key == "'")
		return NedKey::Apostrophe;
	if (key == "comma" || key == ",")
		return NedKey::Comma;
	if (key == "minus" || key == "-")
		return NedKey::Minus;
	if (key == "period" || key == ".")
		return NedKey::Period;
	if (key == "slash" || key == "/")
		return NedKey::Slash;
	if (key == "semicolon" || key == ";")
		return NedKey::Semicolon;
	if (key == "equal" || key == "=")
		return NedKey::Equal;
	if (key == "leftbracket" || key == "[")
		return NedKey::LeftBracket;
	if (key == "backslash" || key == "\\")
		return NedKey::Backslash;
	if (key == "rightbracket" || key == "]")
		return NedKey::RightBracket;
	if (key == "graveaccent" || key == "`")
		return NedKey::GraveAccent;

	std::cerr << "[Keybinds] Unrecognized key '" << keyString << "'" << std::endl;
	return NedKey::None;
}
