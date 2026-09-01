/*
	File: platform/ned_key.h
	Description: Backend-neutral key codes for keybinds. keybinds.json maps
	action -> NedKey here; each backend converts to its own key type
	(ImGui: editor/views/imgui/ned_key_imgui.h; Qt: key event mapping).
*/

#pragma once

#include <string>

enum class NedKey
{
	None = 0,

	// Letters (A..Z) and digits (0..9) are contiguous: 'a'+n / '0'+n.
	A, B, C, D, E, F, G, H, I, J, K, L, M,
	N, O, P, Q, R, S, T, U, V, W, X, Y, Z,
	D0, D1, D2, D3, D4, D5, D6, D7, D8, D9,

	Space,
	Enter,
	Escape,
	Tab,
	Backspace,
	Delete,
	Insert,
	UpArrow,
	DownArrow,
	LeftArrow,
	RightArrow,
	Home,
	End,
	PageUp,
	PageDown,
	LeftCtrl,
	RightCtrl,
	LeftShift,
	RightShift,
	LeftAlt,
	RightAlt,
	LeftSuper,
	RightSuper,
	F1, F2, F3, F4, F5, F6, F7, F8, F9, F10, F11, F12,

	Apostrophe,
	Comma,
	Minus,
	Period,
	Slash,
	Semicolon,
	Equal,
	LeftBracket,
	Backslash,
	RightBracket,
	GraveAccent,

	Count
};

// Parse a keybinds.json key string ("a", "Enter", "pgup", ";", ...).
// NedKey::None when unrecognized (logs to stderr).
NedKey stringToNedKey(const std::string &keyString);
