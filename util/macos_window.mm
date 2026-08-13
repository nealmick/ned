// macos_window.mm
#import <Cocoa/Cocoa.h>
#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_COCOA
#include <GLFW/glfw3native.h>
#include "macos_window.h"

static NSView *appContainerView = nil;
static NSVisualEffectView *blurView = nil;
static NSWindow *configuredWindow = nil;
static MacTitlebarFn gOnSidebar = nullptr;
static MacTitlebarFn gOnTerminal = nullptr;
static MacTitlebarFn gOnSettings = nullptr;
static BOOL gTitlebarControlsInstalled = NO;

@interface NEDTitlebarActions : NSObject
- (void)toggleSidebar:(id)sender;
- (void)toggleTerminal:(id)sender;
- (void)toggleSettings:(id)sender;
@end

@implementation NEDTitlebarActions
- (void)toggleSidebar:(id)sender
{
	(void)sender;
	if (gOnSidebar)
		gOnSidebar();
}
- (void)toggleTerminal:(id)sender
{
	(void)sender;
	if (gOnTerminal)
		gOnTerminal();
}
- (void)toggleSettings:(id)sender
{
	(void)sender;
	if (gOnSettings)
		gOnSettings();
}
@end

static NEDTitlebarActions *gTitlebarActions = nil;

static NSImage *titlebarSymbol(NSArray<NSString *> *names)
{
	if (@available(macOS 11.0, *))
	{
		NSImageSymbolConfiguration *cfg =
			[NSImageSymbolConfiguration configurationWithPointSize:13.0
															weight:NSFontWeightRegular];
		for (NSString *name in names)
		{
			NSImage *img = [NSImage imageWithSystemSymbolName:name
									 accessibilityDescription:nil];
			if (!img)
				continue;
			[img setTemplate:YES];
			NSImage *sized = [img imageWithSymbolConfiguration:cfg];
			return sized ? sized : img;
		}
	}
	return nil;
}

static NSButton *titlebarButton(NSArray<NSString *> *symbols,
								NSString *tooltip,
								SEL action)
{
	NSButton *b = [[NSButton alloc] initWithFrame:NSMakeRect(0, 0, 26, 22)];
	[b setButtonType:NSButtonTypeMomentaryChange];
	b.bordered = NO;
	b.image = titlebarSymbol(symbols);
	b.imagePosition = NSImageOnly;
	b.target = gTitlebarActions;
	b.action = action;
	b.toolTip = tooltip;
	b.translatesAutoresizingMaskIntoConstraints = NO;
	[b.widthAnchor constraintEqualToConstant:26.0].active = YES;
	[b.heightAnchor constraintEqualToConstant:22.0].active = YES;
	return b;
}

static void installTitlebarControls(NSWindow *nswindow)
{
	if (gTitlebarControlsInstalled || !nswindow)
		return;
	gTitlebarControlsInstalled = YES;
	if (!gTitlebarActions)
		gTitlebarActions = [[NEDTitlebarActions alloc] init];

	NSButton *sidebar = titlebarButton(@[ @"sidebar.left", @"rectangle.split.1x2" ],
									   @"Toggle Explorer",
									   @selector(toggleSidebar:));
	NSButton *panel = titlebarButton(
		@[ @"rectangle.bottomhalf.inset.filled", @"rectangle.bottomhalf.filled",
		   @"dock.rectangle" ],
		@"Toggle Terminal",
		@selector(toggleTerminal:));
	NSButton *settings = titlebarButton(@[ @"gearshape", @"gear" ],
										@"Settings",
										@selector(toggleSettings:));

	NSView *gap = [[NSView alloc] initWithFrame:NSZeroRect];
	gap.translatesAutoresizingMaskIntoConstraints = NO;
	[gap.widthAnchor constraintEqualToConstant:8.0].active = YES;

	NSStackView *stack =
		[NSStackView stackViewWithViews:@[ sidebar, panel, gap, settings ]];
	stack.orientation = NSUserInterfaceLayoutOrientationHorizontal;
	stack.alignment = NSLayoutAttributeCenterY;
	stack.spacing = 2.0;
	stack.edgeInsets = NSEdgeInsetsMake(0, 4, 0, 10);
	stack.frame = NSMakeRect(0, 0, 112, 28);

	NSTitlebarAccessoryViewController *acc =
		[[NSTitlebarAccessoryViewController alloc] init];
	acc.view = stack;
	acc.layoutAttribute = NSLayoutAttributeTrailing;
	[nswindow addTitlebarAccessoryViewController:acc];
}

// Clicks fall through so the title bar still drags the window.
@interface NEDTitleLabel : NSTextField
@end
@implementation NEDTitleLabel
- (NSView *)hitTest:(NSPoint)point
{
	(void)point;
	return nil;
}
@end

static void installTitlebarTitle(NSWindow *nswindow)
{
	NSView *bar = [[nswindow standardWindowButton:NSWindowCloseButton] superview];
	if (!bar)
		return;
	for (NSView *v in bar.subviews)
	{
		if ([v.identifier isEqualToString:@"ned.title"])
			return;
	}

	NEDTitleLabel *label = [[NEDTitleLabel alloc] initWithFrame:NSZeroRect];
	label.identifier = @"ned.title";
	label.stringValue = @"Ned Text Editor";
	label.font = [NSFont systemFontOfSize:13.0 weight:NSFontWeightSemibold];
	label.textColor = [NSColor secondaryLabelColor];
	label.alignment = NSTextAlignmentCenter;
	label.editable = NO;
	label.selectable = NO;
	label.drawsBackground = NO;
	label.bordered = NO;
	label.refusesFirstResponder = YES;
	label.translatesAutoresizingMaskIntoConstraints = NO;
	[bar addSubview:label];
	[NSLayoutConstraint activateConstraints:@[
		[label.centerXAnchor constraintEqualToAnchor:bar.centerXAnchor],
		[label.centerYAnchor constraintEqualToAnchor:bar.centerYAnchor],
	]];
}

@interface NEDAppDelegate : NSObject <NSApplicationDelegate>
@property(nonatomic, assign) BOOL shouldTerminate;
@end

@implementation NEDAppDelegate

- (instancetype)init
{
	self = [super init];
	if (self)
		_shouldTerminate = NO;
	return self;
}

- (BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication *)sender
{
	return YES;
}

- (NSApplicationTerminateReply)applicationShouldTerminate:(NSApplication *)sender
{
	self.shouldTerminate = YES;
	NSEvent *event = [NSEvent otherEventWithType:NSEventTypeApplicationDefined
										location:NSZeroPoint
								   modifierFlags:0
									   timestamp:0
									windowNumber:0
										 context:nil
										 subtype:0
										   data1:0
										   data2:0];
	[NSApp postEvent:event atStart:YES];
	return NSTerminateNow;
}

@end

static NEDAppDelegate *gAppDelegate = nil;

void configureMacOSWindow(void *window, float opacity, bool blurEnabled)
{
	GLFWwindow *glfwWindow = (GLFWwindow *)window;
	NSWindow *nswindow = glfwGetCocoaWindow(glfwWindow);

	if (configuredWindow == nswindow)
	{
		updateMacOSWindowProperties(opacity, blurEnabled);
		return;
	}
	configuredWindow = nswindow;

	nswindow.styleMask = NSWindowStyleMaskTitled | NSWindowStyleMaskClosable |
						 NSWindowStyleMaskMiniaturizable | NSWindowStyleMaskResizable |
						 NSWindowStyleMaskFullSizeContentView;
	nswindow.titlebarAppearsTransparent = YES;
	nswindow.titleVisibility = NSWindowTitleHidden;
	if (@available(macOS 11.0, *))
		nswindow.titlebarSeparatorStyle = NSTitlebarSeparatorStyleNone;
	nswindow.title = @"Ned Text Editor";
	nswindow.hasShadow = YES;
	nswindow.movableByWindowBackground = NO;

	[nswindow setOpaque:NO];
	[nswindow setBackgroundColor:[NSColor clearColor]];
	[nswindow setAlphaValue:1.0];

	NSWindowButton kinds[] = {NSWindowCloseButton,
							  NSWindowMiniaturizeButton,
							  NSWindowZoomButton};
	for (int i = 0; i < 3; ++i)
	{
		NSButton *b = [nswindow standardWindowButton:kinds[i]];
		if (b)
			b.hidden = NO;
	}

	NSRect contentRect = [nswindow.contentView bounds];
	NSView *containerView = [[NSView alloc] initWithFrame:contentRect];
	[containerView setWantsLayer:YES];
	containerView.autoresizingMask = NSViewWidthSizable | NSViewHeightSizable;

	NSVisualEffectView *effectView =
		[[NSVisualEffectView alloc] initWithFrame:containerView.bounds];
	effectView.blendingMode = NSVisualEffectBlendingModeBehindWindow;
	effectView.state = NSVisualEffectStateActive;
	effectView.autoresizingMask = NSViewWidthSizable | NSViewHeightSizable;
	if (@available(macOS 10.14, *))
		effectView.material = NSVisualEffectMaterialHUDWindow;
	else
		effectView.material = NSVisualEffectMaterialDark;

	NSView *appContainer = [[NSView alloc] initWithFrame:containerView.bounds];
	[appContainer setWantsLayer:YES];
	appContainer.layer.backgroundColor = [[NSColor clearColor] CGColor];
	appContainer.layer.opaque = NO;
	appContainer.alphaValue = opacity;
	appContainer.autoresizingMask = NSViewWidthSizable | NSViewHeightSizable;

	NSView *originalGlfwContentView = nswindow.contentView;
	originalGlfwContentView.autoresizingMask = NSViewWidthSizable | NSViewHeightSizable;
	originalGlfwContentView.frame = appContainer.bounds;

	nswindow.contentView = containerView;
	[containerView addSubview:effectView];
	[containerView addSubview:appContainer];
	[appContainer addSubview:originalGlfwContentView];

	[effectView setHidden:!blurEnabled];

	[nswindow setInitialFirstResponder:originalGlfwContentView];
	[nswindow makeFirstResponder:originalGlfwContentView];
	[NSApp activateIgnoringOtherApps:YES];
	[nswindow makeKeyAndOrderFront:nil];

	appContainerView = appContainer;
	blurView = effectView;
	installTitlebarControls(nswindow);
	[nswindow invalidateShadow];
	[nswindow display];
	installTitlebarTitle(nswindow);
	if (![[nswindow standardWindowButton:NSWindowCloseButton] superview])
	{
		dispatch_async(dispatch_get_main_queue(), ^{
		  installTitlebarTitle(nswindow);
		});
	}
}

void setMacOSTitlebarActions(MacTitlebarFn sidebar,
							 MacTitlebarFn terminal,
							 MacTitlebarFn settings)
{
	gOnSidebar = sidebar;
	gOnTerminal = terminal;
	gOnSettings = settings;
	if (configuredWindow)
		installTitlebarControls(configuredWindow);
}

void updateMacOSWindowProperties(float opacity, bool blurEnabled)
{
	dispatch_async(dispatch_get_main_queue(), ^{
	  if (appContainerView)
	  {
		  [appContainerView setAlphaValue:opacity];
		  [appContainerView setNeedsDisplay:YES];
	  }
	  if (blurView)
	  {
		  [blurView setHidden:!blurEnabled];
		  [blurView setNeedsDisplay:YES];
	  }
	  if (configuredWindow)
	  {
		  [configuredWindow invalidateShadow];
		  [configuredWindow displayIfNeeded];
		  [configuredWindow setHasShadow:NO];
		  [configuredWindow setHasShadow:YES];
	  }
	});
}

float macOSTitlebarInset(void)
{
	if (!configuredWindow)
		return 0.0f;
	const NSRect bounds = configuredWindow.contentView.bounds;
	const NSRect layout = [configuredWindow contentLayoutRect];
	const CGFloat h = bounds.size.height - layout.size.height;
	return h > 1.0 ? (float)h : 28.0f;
}

void setupMacOSApplicationDelegate(void)
{
	if (!gAppDelegate)
	{
		gAppDelegate = [[NEDAppDelegate alloc] init];
		[NSApp setDelegate:gAppDelegate];
	}
}

void cleanupMacOSApplicationDelegate(void)
{
	if (gAppDelegate)
	{
		[NSApp setDelegate:nil];
		gAppDelegate = nil;
	}
}

bool shouldTerminateApplication(void)
{
	return gAppDelegate ? gAppDelegate.shouldTerminate : false;
}
