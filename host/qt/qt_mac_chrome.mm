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
// The titlebar's own container view (behind the traffic lights). We paint
// it directly — this works regardless of style-mask state, so even if Qt
// strips the transparency bits the bar still shows the theme color.
static NSView *nedTitlebarBackdrop(NSWindow *w)
{
	NSView *bar = [w standardWindowButton:NSWindowCloseButton].superview;
	return bar;
}

void nedQtApplyTitlebarColor(NSWindow *w, NSColor *color)
{
	NSView *bar = nedTitlebarBackdrop(w);
	if (!bar)
		return;
	static NSView *backdrop = nil;
	if (!backdrop)
	{
		backdrop = [[NSView alloc] initWithFrame:bar.bounds];
		backdrop.wantsLayer = YES;
		backdrop.autoresizingMask = NSViewWidthSizable | NSViewHeightSizable;
		[bar addSubview:backdrop positioned:NSWindowBelow relativeTo:nil];
	}
	backdrop.layer.backgroundColor = color.CGColor;
}

void applyNedQtWindowColor(void *qtWinId, float r, float g, float b)
{
	NSView *view = reinterpret_cast<NSView *>(qtWinId);
	if (NSWindow *nswindow = view.window)
	{
		// Opaque window background: the transparent title bar shows this
		// color, matching the editor theme like the GLFW build.
		gNedWindowColor = [NSColor colorWithSRGBRed:r green:g blue:b alpha:1.0];
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
												{
													w.backgroundColor = gNedWindowColor;
													nedQtApplyTitlebarColor(w, gNedWindowColor);
												}
												static int logged = 0;
												if (logged++ < 3)
													NSLog(@"[ned-chrome] tick mask=%lu transparent=%d",
														  (unsigned long)w.styleMask,
														  w.titlebarAppearsTransparent);
											  }];
	[[NSRunLoop mainRunLoop] addTimer:timer forMode:NSRunLoopCommonModes];
}
