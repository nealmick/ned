/*
	File: host/qt/windows_chrome.cpp
	Description: Windows counterpart of mac_chrome.mm — borderless client
	area + DWM shadow/snap with a hand-drawn Qt caption strip, a 1:1 port
	of the ImGui host's util/windows_window.cpp (same WS_OVERLAPPEDWINDOW
	keep + WM_NCCALCSIZE strip + WM_NCHITTEST hit-testing that keeps Aero
	snap, the restore animation, and the Win11 snap-layout flyout working).
	All entry points are HWND-based and no-op when passed a null handle.
*/

#ifdef _WIN32

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <dwmapi.h>
#include <windows.h>
#include <windowsx.h>

#include <algorithm>
#include <vector>

// Same attribute constants the GLFW host uses (avoid SDK-version drift).
#ifndef DWMWA_USE_IMMERSIVE_DARK_MODE
#define DWMWA_USE_IMMERSIVE_DARK_MODE 20
#endif
#ifndef DWMWA_WINDOW_CORNER_PREFERENCE
#define DWMWA_WINDOW_CORNER_PREFERENCE 33
#endif
#ifndef DWMWCP_ROUND
#define DWMWCP_ROUND 2
#endif
#ifndef DWMWCP_DONOTROUND
#define DWMWCP_DONOTROUND 1
#endif

namespace {

HWND gHwnd = nullptr;
WNDPROC gPrevProc = nullptr;
float gCaptionH = 32.0f;
float gResizeBorder = 8.0f;

// Repaint hook: hover over the caption buttons and maximize/restore are
// seen here first (non-client input never reaches Qt) — the title bar
// widget subscribes so its hover fills track the native state.
void (*gNotify)(void *) = nullptr;
void *gNotifyCtx = nullptr;

void notifyChrome()
{
	if (gNotify)
		gNotify(gNotifyCtx);
}

struct Exclude
{
	float x0, y0, x1, y1;
	int ht;
};
std::vector<Exclude> gExcludes;
int gHoverHt = HTCLIENT;

int hitTestExcludes(float x, float y)
{
	for (const Exclude &e : gExcludes)
	{
		if (x >= e.x0 && x < e.x1 && y >= e.y0 && y < e.y1)
			return e.ht;
	}
	return 0;
}

void applyCornerPref(HWND hwnd, bool maximized)
{
	const DWORD pref = maximized ? (DWORD)DWMWCP_DONOTROUND : (DWORD)DWMWCP_ROUND;
	DwmSetWindowAttribute(hwnd, DWMWA_WINDOW_CORNER_PREFERENCE, &pref, sizeof(pref));
}

void nedQtMinimize()
{
	if (gHwnd)
		ShowWindow(gHwnd, SW_MINIMIZE);
}

void nedQtToggleMaximize()
{
	if (!gHwnd)
		return;
	ShowWindow(gHwnd, IsZoomed(gHwnd) ? SW_RESTORE : SW_MAXIMIZE);
}

void nedQtClose()
{
	if (gHwnd)
		PostMessage(gHwnd, WM_CLOSE, 0, 0);
}

LRESULT CALLBACK nedQtWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_NCCALCSIZE:
		if (wParam == TRUE)
		{
			auto *params = reinterpret_cast<NCCALCSIZE_PARAMS *>(lParam);
			if (IsZoomed(hwnd))
			{
				HMONITOR mon = MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST);
				MONITORINFO mi{};
				mi.cbSize = sizeof(mi);
				if (GetMonitorInfo(mon, &mi))
					params->rgrc[0] = mi.rcWork;
			}
			return 0;
		}
		break;

	case WM_NCHITTEST: {
		POINT pt{GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
		RECT wr{};
		GetWindowRect(hwnd, &wr);
		const int x = pt.x - wr.left;
		const int y = pt.y - wr.top;
		const int w = wr.right - wr.left;
		const int h = wr.bottom - wr.top;
		const bool maxed = IsZoomed(hwnd) != 0;
		const int b = maxed ? 0 : (int)std::max(6.0f, gResizeBorder);

		POINT client = pt;
		ScreenToClient(hwnd, &client);
		// Caption buttons win over resize grips so HTMAXBUTTON can trigger
		// the Win11 snap-layout flyout.
		if (const int ht = hitTestExcludes((float)client.x, (float)client.y))
		{
			if (gHoverHt != ht)
			{
				gHoverHt = ht;
				notifyChrome();
			}
			return ht;
		}
		if (gHoverHt != HTCLIENT)
		{
			gHoverHt = HTCLIENT;
			notifyChrome();
		}

		if (!maxed)
		{
			if (x < b && y < b)
				return HTTOPLEFT;
			if (x >= w - b && y < b)
				return HTTOPRIGHT;
			if (x < b && y >= h - b)
				return HTBOTTOMLEFT;
			if (x >= w - b && y >= h - b)
				return HTBOTTOMRIGHT;
			if (x < b)
				return HTLEFT;
			if (x >= w - b)
				return HTRIGHT;
			if (y < b)
				return HTTOP;
			if (y >= h - b)
				return HTBOTTOM;
		}

		if (client.y >= 0 && client.y < (int)gCaptionH)
			return HTCAPTION;
		return HTCLIENT;
	}

	case WM_NCLBUTTONDOWN:
		if (wParam == HTMINBUTTON || wParam == HTMAXBUTTON || wParam == HTCLOSE)
			return 0;
		break;

	case WM_NCLBUTTONUP: {
		POINT pt{GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
		ScreenToClient(hwnd, &pt);
		const int ht = hitTestExcludes((float)pt.x, (float)pt.y);
		if (ht == HTMINBUTTON)
			nedQtMinimize();
		else if (ht == HTMAXBUTTON)
			nedQtToggleMaximize();
		else if (ht == HTCLOSE)
			nedQtClose();
		if (ht == HTMINBUTTON || ht == HTMAXBUTTON || ht == HTCLOSE ||
			wParam == HTMINBUTTON || wParam == HTMAXBUTTON || wParam == HTCLOSE)
			return 0;
		break;
	}

	case WM_NCLBUTTONDBLCLK:
		if (wParam == HTCAPTION)
		{
			nedQtToggleMaximize();
			return 0;
		}
		break;

	case WM_NCMOUSELEAVE:
	case WM_MOUSELEAVE:
		if (gHoverHt != HTCLIENT)
		{
			gHoverHt = HTCLIENT;
			notifyChrome();
		}
		break;

	case WM_SIZE:
		applyCornerPref(hwnd, wParam == SIZE_MAXIMIZED);
		// Maximize/restore flips the maximize glyph — repaint the bar.
		notifyChrome();
		break;
	}

	return CallWindowProc(gPrevProc, hwnd, msg, wParam, lParam);
}

} // namespace

void configureNedQtChromeWindows(void *hwndVoid)
{
	HWND hwnd = static_cast<HWND>(hwndVoid);
	if (!hwnd)
		return;
	if (gHwnd == hwnd && gPrevProc)
		return; // already subclassed (showEvent re-runs applyNativeChrome)
	gHwnd = hwnd;

	// WS_OVERLAPPEDWINDOW with the frame stripped in WM_NCCALCSIZE — same
	// choice as the ImGui host (Qt::FramelessWindowHint / WS_POPUP is flaky
	// for Aero snap, restore animation, and Alt-Tab).
	LONG_PTR style = GetWindowLongPtr(hwnd, GWL_STYLE);
	style &= ~(WS_POPUP | WS_CHILD);
	style |= WS_OVERLAPPEDWINDOW | WS_CLIPSIBLINGS | WS_CLIPCHILDREN;
	SetWindowLongPtr(hwnd, GWL_STYLE, style);

	MARGINS margins{0, 0, 1, 0};
	DwmExtendFrameIntoClientArea(hwnd, &margins);

	const BOOL dark = TRUE;
	DwmSetWindowAttribute(hwnd, DWMWA_USE_IMMERSIVE_DARK_MODE, &dark, sizeof(dark));
	applyCornerPref(hwnd, IsZoomed(hwnd) != 0);

	gPrevProc = (WNDPROC)SetWindowLongPtr(hwnd, GWLP_WNDPROC, (LONG_PTR)nedQtWndProc);

	SetWindowPos(hwnd,
				 nullptr,
				 0,
				 0,
				 0,
				 0,
				 SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER);
}

// Caption metrics in PHYSICAL pixels (WM_NCHITTEST coords are physical;
// the caller converts from logical via devicePixelRatio).
void nedQtSetCaptionHeight(float height)
{
	gCaptionH = std::max(24.0f, height);
	gResizeBorder = std::max(6.0f, height * 0.22f);
}

void nedQtClearCaptionExcludes() { gExcludes.clear(); }

// ht: HTCLIENT for in-widget buttons (Qt gets the click), HTMINBUTTON /
// HTMAXBUTTON / HTCLOSE for the caption cluster (native handling keeps
// the Win11 snap-layout flyout on maximize).
void nedQtExcludeCaptionRect(float x0, float y0, float x1, float y1, int ht)
{
	gExcludes.push_back({x0, y0, x1, y1, ht});
}

int nedQtCaptionHover() { return gHoverHt; }

bool nedQtWindowIsMaximized(void *hwndVoid)
{
	HWND hwnd = static_cast<HWND>(hwndVoid);
	return hwnd && IsZoomed(hwnd) != 0;
}

void nedQtSetChromeNotify(void (*fn)(void *), void *ctx)
{
	gNotify = fn;
	gNotifyCtx = ctx;
}

#endif // _WIN32
