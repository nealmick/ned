/*
	File: host/qt/qt_mac_chrome.mm
	Description: macOS chrome bridge for the Qt host — the reparent-free
	variant of the GLFW look: transparent title bar, SF-symbol accessory
	buttons, centered native title. Keeps QNSView responders intact.
*/

#import <Cocoa/Cocoa.h>
#include "../../util/macos_window.h"

void configureNedQtChrome(void *qtWinId, float opacity, bool blurEnabled)
{
	// Qt hands out the platform view (QNSView*); the window owns the chrome.
	NSView *view = reinterpret_cast<NSView *>(qtWinId);
	if (NSWindow *nswindow = view.window)
		configureMacOSNSWindowChrome(nswindow, opacity, blurEnabled);
}
