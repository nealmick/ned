/*
	File: host/qt/qt_mac_chrome.mm
	Description: macOS chrome bridge for the Qt host — the reparent-free
	variant of the GLFW look: transparent title bar, SF-symbol accessory
	buttons, centered native title. Keeps QNSView responders intact.
*/

#import <Cocoa/Cocoa.h>

static NSColor *gNedWindowColor = nil;
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
		gNedWindowColor = [NSColor colorWithCalibratedRed:r green:g blue:b alpha:1.0];
		nswindow.backgroundColor = gNedWindowColor;
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

// Qt recomputes the NSWindow styleMask when it re-applies window flags
// (expose, flag changes), silently stripping the transparent-titlebar
// bits while leaving our accessory subviews in place — producing the
// "default gray bar with our buttons" look. This watcher re-asserts the
// chrome periodically, cheaply and idempotently.
void nedQtChromeWatch(void *qtWinId)
{
	static BOOL installed = NO;
	if (installed)
		return;
	installed = YES;

	NSView *view = reinterpret_cast<NSView *>(qtWinId);
	NSWindow *window = view.window;
	if (!window)
		return;

	NSWindow *const watched = window; // app-lifetime watcher keeps it alive
	NSTimer *timer = [NSTimer timerWithTimeInterval:0.25
											  repeats:YES
												block:^(NSTimer *) {
												NSWindow *w = watched;
												if (!w)
													return;
												w.styleMask |=
													NSWindowStyleMaskFullSizeContentView;
												w.titlebarAppearsTransparent = YES;
												w.titleVisibility = NSWindowTitleHidden;
												if (gNedWindowColor)
													w.backgroundColor = gNedWindowColor;
											  }];
	[[NSRunLoop mainRunLoop] addTimer:timer forMode:NSRunLoopCommonModes];
}
