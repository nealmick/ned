/*
	File: host/qt/mac_chrome.mm
	Description: macOS chrome bridge for the Qt host — the GLFW look:
	transparent title bar, SF-symbol accessory buttons, centered native
	title, plus window vibrancy. The QNSView is NEVER re-parented (that
	blanked rendering); instead an NSVisualEffectView is added as a
	sibling BELOW the content view inside the window's frame view, the
	window is non-opaque with a clear background (kept alive by the
	watcher — Qt re-asserts opacity at show time otherwise), and the
	opacity setting travels in the theme color's alpha (see theme.h).
*/

#import <Cocoa/Cocoa.h>

static NSColor *gNedWindowColor = nil;
static float gNedOpacity = 1.0f;
static NSVisualEffectView *gNedEffect = nil; // vibrancy, behind content
static BOOL gNedBlur = YES;
#include "../../util/macos_window.h"

// The titlebar's own container view (behind the traffic lights). We paint
// it directly — this works regardless of style-mask state, so even if Qt
// strips the transparency bits the bar still shows the theme color.
static NSView *nedTitlebarBackdrop(NSWindow *w)
{
	NSView *bar = [w standardWindowButton:NSWindowCloseButton].superview;
	return bar;
}

// Color-only backdrop: transparent to mouse events so title-bar drags
// and traffic lights keep working (plain NSView would swallow them).
@interface NedTitlebarBackdrop : NSView
@end
@implementation NedTitlebarBackdrop
- (NSView *)hitTest:(NSPoint)point
{
	(void)point;
	return nil;
}
@end

void nedQtApplyTitlebarColor(NSWindow *w, NSColor *color)
{
	NSView *bar = nedTitlebarBackdrop(w);
	if (!bar || !color)
		return;
	// No static caching: AppKit can rebuild the title-bar container (style
	// mask changes), which would orphan a cached view. The bar has only a
	// handful of subviews, so a per-call scan (0.25s cadence) is cheap.
	NedTitlebarBackdrop *backdrop = nil;
	for (NSView *v in bar.subviews)
		if ([v isKindOfClass:[NedTitlebarBackdrop class]])
		{
			backdrop = (NedTitlebarBackdrop *)v;
			break;
		}
	if (!backdrop)
	{
		backdrop = [[NedTitlebarBackdrop alloc] initWithFrame:bar.bounds];
		backdrop.wantsLayer = YES;
		backdrop.autoresizingMask = NSViewWidthSizable | NSViewHeightSizable;
		[bar addSubview:backdrop positioned:NSWindowBelow relativeTo:nil];
	}
	// With full-size content the QNSView (and the Qt tint fill in
	// AppHost::paintEvent) already extends under the title bar — a tint
	// here would stack a second alpha layer and read darker than the rest
	// of the window. Paint clear; fall back to the tint only if Qt strips
	// the full-size-content bit (classic title bar needs its own color).
	if (w.styleMask & NSWindowStyleMaskFullSizeContentView)
		backdrop.layer.backgroundColor = [[NSColor clearColor] CGColor];
	else
		backdrop.layer.backgroundColor = color.CGColor;
}

// Vibrancy without touching the content view: the effect view is a
// SIBLING below the QNSView, added to the window's frame view
// (contentView.superview). AppKit can rebuild that hierarchy, so this is
// idempotent and re-run by the watcher.
static void nedEnsureEffectView(NSWindow *w)
{
	NSView *content = w.contentView;
	if (!content)
		return;
	NSView *frame = content.superview; // the window's theme frame
	if (!frame)
		return;
	if (!gNedEffect)
	{
		gNedEffect = [[NSVisualEffectView alloc] initWithFrame:frame.bounds];
		gNedEffect.blendingMode = NSVisualEffectBlendingModeBehindWindow;
		gNedEffect.state = NSVisualEffectStateActive;
		gNedEffect.autoresizingMask = NSViewWidthSizable | NSViewHeightSizable;
		if (@available(macOS 10.14, *))
			gNedEffect.material = NSVisualEffectMaterialHUDWindow;
		else
			gNedEffect.material = NSVisualEffectMaterialDark;
	}
	if (gNedEffect.superview != frame)
		[frame addSubview:gNedEffect
			 positioned:NSWindowBelow
			 relativeTo:content];
	gNedEffect.hidden = !gNedBlur;
}

// Non-opaque + clear background: the theme tint comes from the Qt content
// (alpha in the palette / editor paints), the blur from the effect view.
// Qt re-asserts opacity itself unless WA_TranslucentBackground is set on
// the widget — the host does that — but re-assert here anyway (cheap).
static void nedAssertTransparency(NSWindow *w)
{
	[w setOpaque:NO];
	w.backgroundColor = [NSColor clearColor];
	[w invalidateShadow];
}

void applyNedQtWindowColor(void *qtWinId, float r, float g, float b)
{
	NSView *view = reinterpret_cast<NSView *>(qtWinId);
	if (NSWindow *nswindow = view.window)
	{
		// Title-bar strip tint, carrying the opacity setting. This file
		// builds WITHOUT ARC: the convenience constructor is autoreleased,
		// so the static MUST retain it — the watcher tick (0.25s) messages
		// this color long after the pool drains. Retaining failure here is
		// what crashed the app when changing opacity in settings.
		[gNedWindowColor release];
		gNedWindowColor = [[NSColor colorWithSRGBRed:r
											   green:g
												blue:b
											   alpha:gNedOpacity] retain];
		nedQtApplyTitlebarColor(nswindow, gNedWindowColor);
		nedAssertTransparency(nswindow);
	}
}

void configureNedQtChrome(void *qtWinId, float opacity, bool blurEnabled)
{
	// Qt hands out the platform view (QNSView*); the window owns the chrome.
	NSView *view = reinterpret_cast<NSView *>(qtWinId);
	if (NSWindow *nswindow = view.window)
	{
		configureMacOSNSWindowChrome(nswindow, opacity, blurEnabled);
		gNedOpacity = opacity;
		gNedBlur = blurEnabled;
		nedEnsureEffectView(nswindow);
		nedAssertTransparency(nswindow);
	}
}

// Qt recomputes the NSWindow styleMask when it re-applies window flags
// (expose, flag changes), silently stripping the transparent-titlebar
// bits while leaving our accessory subviews in place — producing the
// "default gray bar with our buttons" look. This watcher re-asserts the
// chrome periodically, cheaply and idempotently.
// The full chrome-reassert sequence (style bits, effect view, transparency,
// title-bar tint) shared by the resize observer and the 0.25s timer.
static void nedReassertChrome(NSWindow *w)
{
	if (!(w.styleMask & NSWindowStyleMaskFullSizeContentView))
		w.styleMask |= NSWindowStyleMaskFullSizeContentView;
	w.titlebarAppearsTransparent = YES;
	w.titleVisibility = NSWindowTitleHidden;
	nedEnsureEffectView(w);
	nedAssertTransparency(w);
	if (gNedWindowColor)
		nedQtApplyTitlebarColor(w, gNedWindowColor);
}

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

	// Immediate mask repair: Qt strips the full-size-content style bit when
	// it recomputes window flags (popup dismissal, key-state changes). The
	// frame then grows by a titlebar height until the timer below re-adds
	// the bit — the visible "window jumps taller" flash on tab open. The
	// strip itself fires a frame-resize notification, so repairing HERE
	// lands before the next paint.
	static id resizeObserver = nil;
	resizeObserver = [[NSNotificationCenter defaultCenter]
		addObserverForName:NSWindowDidResizeNotification
		              object:watched
		               queue:nil
	          usingBlock:^(NSNotification *note) {
	              NSWindow *w = note.object;
	              if (![w isKindOfClass:[NSWindow class]])
	                  return;
	              if (!(w.styleMask & NSWindowStyleMaskFullSizeContentView))
	                  nedReassertChrome(w);
	          }];

	NSTimer *timer = [NSTimer timerWithTimeInterval:0.25
	                                              repeats:YES
	                                              block:^(NSTimer *) {
	                                              NSWindow *w = watched;
	                                              if (!w)
	                                              return;
	                                              // Early-out while the window already carries the
	                                              // full treatment: re-asserting would call
	                                              // invalidateShadow (shadow recomputation) every
	                                              // tick for nothing. The effect-view check keeps
	                                              // catching AppKit frame-view rebuilds, which
	                                              // detach it without touching the flags below.
	                                              if ((w.styleMask &
	                                                   NSWindowStyleMaskFullSizeContentView) &&
	                                                  w.titlebarAppearsTransparent && !w.isOpaque &&
	                                                  gNedEffect &&
	                                                  gNedEffect.superview ==
	                                                      w.contentView.superview &&
	                                                  !gNedEffect.hidden)
	                                                  return;
	                                              // WA_TranslucentBackground makes Qt recompute the
	                                              // style mask at show time WITHOUT our
	                                              // full-size-content bit — re-assert it.
	                                              nedReassertChrome(w);
	                                            }];
	[[NSRunLoop mainRunLoop] addTimer:timer forMode:NSRunLoopCommonModes];
}

