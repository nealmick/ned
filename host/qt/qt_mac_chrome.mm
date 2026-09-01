/*
	File: host/qt/qt_mac_chrome.mm
	Description: macOS chrome for the Qt host — applies the same NSWindow
	treatment as the GLFW host (transparent title bar, vibrancy blur).
*/

#import <Cocoa/Cocoa.h>
#include "../../util/macos_window.h"

void configureNedQtChrome(void *qtWinId, float opacity, bool blurEnabled)
{
	// Qt hands out the platform view (QNSView*), not the window.
	NSView *view = reinterpret_cast<NSView *>(qtWinId);
	if (NSWindow *nswindow = view.window)
		configureMacOSNSWindow(nswindow, opacity, blurEnabled);
}
