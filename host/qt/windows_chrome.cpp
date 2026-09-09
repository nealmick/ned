/*
	File: host/qt/windows_chrome.cpp
	Description: Windows counterpart of mac_chrome.mm — theme the NATIVE
	window frame to match the editor profile instead of drawing our own
	caption (the ImGui host goes borderless + hand-drawn; the Qt host keeps
	the native frame everywhere and restyles it).

	Applied via the same DWM attributes the GLFW host sets in
	util/windows_window.cpp:
	  - immersive dark mode (dark caption text/icons on Win10 20H1+)
	  - caption + border color from the profile background (Win11 only;
	    older builds keep the system color silently)
	  - rounded corners preference (Win11)
	All entry points are HWND-based and no-op when passed a null handle.
*/

#ifdef _WIN32

#define WIN32_LEAN_AND_MEAN
#include <dwmapi.h>
#include <windows.h>

// Same attribute constants the GLFW host uses (avoid SDK-version drift).
#ifndef DWMWA_USE_IMMERSIVE_DARK_MODE
#define DWMWA_USE_IMMERSIVE_DARK_MODE 20
#endif
#ifndef DWMWA_WINDOW_CORNER_PREFERENCE
#define DWMWA_WINDOW_CORNER_PREFERENCE 33
#endif
#ifndef DWMWA_CAPTION_COLOR
#define DWMWA_CAPTION_COLOR 35
#endif
#ifndef DWMWA_BORDER_COLOR
#define DWMWA_BORDER_COLOR 34
#endif

static void dwmSet(HWND hwnd, DWORD attr, const void *value, DWORD size)
{
	HRESULT hr = DwmSetWindowAttribute(hwnd, attr, value, size);
	(void)hr; // unsupported attribute/OS: keep the system default silently
}

void configureNedQtChromeWindows(void *hwnd)
{
	if (!hwnd)
		return;
	HWND w = static_cast<HWND>(hwnd);
	// Dark caption + rounded corners: the frame chrome baseline. Colors
	// are applied separately (applyNedQtWindowColorWindows) so theme
	// changes restyle without re-running the one-time setup.
	BOOL dark = TRUE;
	dwmSet(w, DWMWA_USE_IMMERSIVE_DARK_MODE, &dark, sizeof(dark));
	DWORD round = 2; // DWMWCP_ROUND
	dwmSet(w, DWMWA_WINDOW_CORNER_PREFERENCE, &round, sizeof(round));
}

void applyNedQtWindowColorWindows(void *hwnd, float r, float g, float b)
{
	if (!hwnd)
		return;
	HWND w = static_cast<HWND>(hwnd);
	// Caption slightly lighter than the window tint so the frame reads
	// against the dark body; border a bit dimmer (same lift the imgui
	// caption strip uses visually).
	const COLORREF caption = RGB(static_cast<BYTE>((r * 255.0f) + 12.0f < 255.0f
											  ? r * 255.0f + 12.0f
											  : 255.0f),
								 static_cast<BYTE>((g * 255.0f) + 12.0f < 255.0f
													   ? g * 255.0f + 12.0f
													   : 255.0f),
								 static_cast<BYTE>((b * 255.0f) + 12.0f < 255.0f
													   ? b * 255.0f + 12.0f
													   : 255.0f));
	const COLORREF border = RGB(static_cast<BYTE>(r * 255.0f * 0.55f),
								static_cast<BYTE>(g * 255.0f * 0.55f),
								static_cast<BYTE>(b * 255.0f * 0.55f));
	dwmSet(w, DWMWA_CAPTION_COLOR, &caption, sizeof(caption));
	dwmSet(w, DWMWA_BORDER_COLOR, &border, sizeof(border));
}

#endif // _WIN32
