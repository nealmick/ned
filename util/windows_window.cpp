#ifdef _WIN32

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif

#include "windows_window.h"

#include "imgui.h"

#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>

#include <dwmapi.h>
#include <windows.h>
#include <windowsx.h>

#include <algorithm>
#include <vector>

#ifndef DWMWA_WINDOW_CORNER_PREFERENCE
#define DWMWA_WINDOW_CORNER_PREFERENCE 33
#endif
#ifndef DWMWCP_ROUND
#define DWMWCP_ROUND 2
#endif
#ifndef DWMWCP_DONOTROUND
#define DWMWCP_DONOTROUND 1
#endif
#ifndef DWMWA_USE_IMMERSIVE_DARK_MODE
#define DWMWA_USE_IMMERSIVE_DARK_MODE 20
#endif

namespace {

HWND gHwnd = nullptr;
WNDPROC gPrevProc = nullptr;
float gCaptionH = 32.0f;
float gResizeBorder = 8.0f;

struct Exclude
{
	float x0, y0, x1, y1;
	int ht;
};
std::vector<Exclude> gExcludes;
int gHoverHt = HTCLIENT;

int htFromPart(WindowsCaptionHit hit)
{
	switch (hit)
	{
	case WindowsCaptionHit::Min:
		return HTMINBUTTON;
	case WindowsCaptionHit::Max:
		return HTMAXBUTTON;
	case WindowsCaptionHit::Close:
		return HTCLOSE;
	default:
		return HTCLIENT;
	}
}

WindowsCaptionHit partFromHt(int ht)
{
	switch (ht)
	{
	case HTMINBUTTON:
		return WindowsCaptionHit::Min;
	case HTMAXBUTTON:
		return WindowsCaptionHit::Max;
	case HTCLOSE:
		return WindowsCaptionHit::Close;
	default:
		return WindowsCaptionHit::Client;
	}
}

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

LRESULT CALLBACK nedWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
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

	case WM_NCHITTEST:
	{
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
			gHoverHt = ht;
			return ht;
		}
		gHoverHt = HTCLIENT;

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

	case WM_NCLBUTTONUP:
	{
		POINT pt{GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
		ScreenToClient(hwnd, &pt);
		const int ht = hitTestExcludes((float)pt.x, (float)pt.y);
		if (ht == HTMINBUTTON)
			windowsMinimize();
		else if (ht == HTMAXBUTTON)
			windowsToggleMaximize();
		else if (ht == HTCLOSE)
			windowsClose();
		if (ht == HTMINBUTTON || ht == HTMAXBUTTON || ht == HTCLOSE ||
			wParam == HTMINBUTTON || wParam == HTMAXBUTTON || wParam == HTCLOSE)
			return 0;
		break;
	}

	case WM_NCLBUTTONDBLCLK:
		if (wParam == HTCAPTION)
		{
			windowsToggleMaximize();
			return 0;
		}
		break;

	case WM_NCMOUSELEAVE:
	case WM_MOUSELEAVE:
		gHoverHt = HTCLIENT;
		break;

	case WM_SIZE:
		applyCornerPref(hwnd, wParam == SIZE_MAXIMIZED);
		break;
	}

	return CallWindowProc(gPrevProc, hwnd, msg, wParam, lParam);
}

} // namespace

void configureWindowsWindow(GLFWwindow *window)
{
	if (!window)
		return;
	HWND hwnd = glfwGetWin32Window(window);
	if (!hwnd)
		return;
	gHwnd = hwnd;

	// GLFW_DECORATED=false creates WS_POPUP. Popup+caption+thickframe is
	// flaky for Aero snap, restore animation, and Alt-Tab.
	LONG_PTR style = GetWindowLongPtr(hwnd, GWL_STYLE);
	style &= ~(WS_POPUP | WS_CHILD);
	style |= WS_OVERLAPPEDWINDOW | WS_CLIPSIBLINGS | WS_CLIPCHILDREN;
	SetWindowLongPtr(hwnd, GWL_STYLE, style);

	MARGINS margins{0, 0, 1, 0};
	DwmExtendFrameIntoClientArea(hwnd, &margins);

	const BOOL dark = TRUE;
	DwmSetWindowAttribute(hwnd, DWMWA_USE_IMMERSIVE_DARK_MODE, &dark, sizeof(dark));
	applyCornerPref(hwnd, false);

	if (!gPrevProc)
		gPrevProc = (WNDPROC)SetWindowLongPtr(hwnd, GWLP_WNDPROC, (LONG_PTR)nedWndProc);

	SetWindowPos(hwnd,
				 nullptr,
				 0,
				 0,
				 0,
				 0,
				 SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER);
}

void windowsSetTitlebarHeight(float height)
{
	gCaptionH = std::max(24.0f, height);
	gResizeBorder = std::max(6.0f, height * 0.22f);
}

float windowsTitlebarInset() { return gCaptionH; }

void windowsClearCaptionExcludes() { gExcludes.clear(); }

void windowsExcludeCaptionRect(const ImVec2 &min,
							   const ImVec2 &max,
							   WindowsCaptionHit hit)
{
	gExcludes.push_back({min.x, min.y, max.x, max.y, htFromPart(hit)});
}

WindowsCaptionHit windowsCaptionHover() { return partFromHt(gHoverHt); }

void windowsMinimize()
{
	if (gHwnd)
		ShowWindow(gHwnd, SW_MINIMIZE);
}

void windowsToggleMaximize()
{
	if (!gHwnd)
		return;
	ShowWindow(gHwnd, IsZoomed(gHwnd) ? SW_RESTORE : SW_MAXIMIZE);
}

void windowsClose()
{
	if (gHwnd)
		PostMessage(gHwnd, WM_CLOSE, 0, 0);
}

bool windowsIsMaximized() { return gHwnd && IsZoomed(gHwnd); }

#endif
