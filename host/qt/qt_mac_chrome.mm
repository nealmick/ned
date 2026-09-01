/*
	File: host/qt/qt_mac_chrome.mm
	Description: macOS chrome bridge for the Qt host — the reparent-free
	variant of the GLFW look: transparent title bar, SF-symbol accessory
	buttons, centered native title. Keeps QNSView responders intact.
*/

#import <Cocoa/Cocoa.h>
#include "../../util/macos_window.h"

// Theme color for the window/title bar (RGB 0..1). Called at startup and
// whenever the settings profile changes.
void applyNedQtWindowColor(void *qtWinId, float r, float g, float b)
{
	NSView *view = reinterpret_cast<NSView *>(qtWinId);
	if (NSWindow *nswindow = view.window)
	{
		// Opaque window background: the transparent title bar shows this
		// color, matching the editor theme like the GLFW build.
		nswindow.backgroundColor = [NSColor colorWithCalibratedRed:r
															  green:g
															   blue:b
															  alpha:1.0];
		[nswindow setOpaque:YES];
		[nswindow invalidateShadow];
	}
}

void configureNedQtChrome(void *qtWinId, float opacity, bool blurEnabled)
{
	// Qt hands out the platform view (QNSView*); the window owns the chrome.
	NSView *view = reinterpret_cast<NSView *>(qtWinId);
	if (NSWindow *nswindow = view.window)
		configureMacOSNSWindowChrome(nswindow, opacity, blurEnabled);
}
