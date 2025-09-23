// macos_window.mm
#import <Cocoa/Cocoa.h>
#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_COCOA
#include <GLFW/glfw3native.h>
#include "macos_window.h"
#import <QuartzCore/QuartzCore.h>
#include <cstdlib> // for exit()
#include <iostream> // for std::cout and std::endl

// Static variables to track views and settings
static NSView* appContainerView = nil;
static NSVisualEffectView* blurView = nil;
static NSWindow* configuredWindow = nil;

// Application delegate for proper Cmd+Q handling
@interface NEDAppDelegate : NSObject <NSApplicationDelegate>
@property (nonatomic, assign) BOOL shouldTerminate;
@end

@implementation NEDAppDelegate

- (instancetype)init {
    self = [super init];
    if (self) {
        _shouldTerminate = NO;
    }
    return self;
}

- (BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication *)sender {
    return YES;
}

- (NSApplicationTerminateReply)applicationShouldTerminate:(NSApplication *)sender {
    // For Cmd+Q, immediately kill the process to avoid slow cleanup
    // This prevents the app from hanging during exit, especially after long running sessions
    
    // Get the current event to check if it's Cmd+Q
    NSEvent* currentEvent = [NSApp currentEvent];
    if (currentEvent && (currentEvent.modifierFlags & NSEventModifierFlagCommand) && 
        currentEvent.type == NSEventTypeKeyDown && currentEvent.keyCode == 12) { // 12 is 'Q' key
        
        // Immediate exit for Cmd+Q - kill the process
        std::cout << "Cmd+Q detected - immediately exiting..." << std::endl;
        exit(0);
    }
    
    // For other termination methods, use normal cleanup
    self.shouldTerminate = YES;
    
    // Post a custom event to trigger cleanup in the main loop
    NSEvent* event = [NSEvent otherEventWithType:NSEventTypeApplicationDefined
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

// Global app delegate instance
static NEDAppDelegate* gAppDelegate = nil;

// Custom view for draggable areas
@interface DraggableView : NSView
@end

@implementation DraggableView {
    CGFloat _leftMargin;
    CGFloat _rightMargin;
}

- (instancetype)initWithFrame:(NSRect)frame leftMargin:(CGFloat)leftMargin rightMargin:(CGFloat)rightMargin {
    self = [super initWithFrame:frame];
    if (self) {
        _leftMargin = leftMargin;
        _rightMargin = rightMargin;
    }
    return self;
}

- (BOOL)mouseDownCanMoveWindow {
    return YES;
}

- (void)mouseDown:(NSEvent *)event {
    [self.window performWindowDragWithEvent:event];
}

- (void)setFrame:(NSRect)frame {
    NSRect windowFrame = self.window.frame;
    frame.origin.x = _leftMargin;
    frame.size.width = windowFrame.size.width - (_leftMargin + _rightMargin);
    [super setFrame:frame];
}

- (void)viewDidEndLiveResize {
    [super viewDidEndLiveResize];
    [self setFrame:self.frame];
}

- (void)viewDidMoveToWindow {
    [super viewDidMoveToWindow];
    if (self.window) {
        [self setFrame:self.frame];
    }
}

- (void)viewDidChangeEffectiveAppearance {
    [super viewDidChangeEffectiveAppearance];
    [self setFrame:self.frame];
}

- (void)layout {
    [super layout];
    [self setFrame:self.frame];
}

@end

// Custom view for top-left menu
@interface TopLeftMenuView : NSView
@property (nonatomic, strong) NSButton* closeButton;
@property (nonatomic, strong) NSButton* minimizeButton;
@property (nonatomic, strong) NSButton* maximizeButton;
@property (nonatomic, assign) BOOL isHovered;
@property (nonatomic, assign) int displayFrame;
@property (nonatomic, assign) CGFloat currentOpacity;
@end

@implementation TopLeftMenuView {
    NSWindow* _window;
    NSTrackingArea* _trackingArea;
    NSTrackingArea* _closeButtonTrackingArea;
    NSTrackingArea* _minimizeButtonTrackingArea;
    NSTrackingArea* _maximizeButtonTrackingArea;
    NSTimer* _fadeTimer;
}

- (instancetype)initWithFrame:(NSRect)frame window:(NSWindow*)window {
    self = [super initWithFrame:frame];
    if (self) {
        _window = window;
        _displayFrame = 0;
        _currentOpacity = 0.0;
        self.wantsLayer = YES;
        self.layer.backgroundColor = [NSColor clearColor].CGColor;
        self.alphaValue = 0.0;
        
        // Create traffic light buttons
        CGFloat buttonSize = 16.0;
        CGFloat spacing = 8.0;
        CGFloat startX = 20.0;
        CGFloat y = (frame.size.height - buttonSize) / 2;
        
        // Close button (red)
        self.closeButton = [[NSButton alloc] initWithFrame:NSMakeRect(startX, y, buttonSize, buttonSize)];
        [self.closeButton setButtonType:NSButtonTypeMomentaryLight];
        [self.closeButton setBordered:NO];
        [self.closeButton setBezelStyle:NSBezelStyleRegularSquare];
        [self.closeButton setImageScaling:NSImageScaleProportionallyDown];
        [self.closeButton setImage:[NSImage imageWithSystemSymbolName:@"xmark.circle.fill" accessibilityDescription:@"Close"]];
        [self.closeButton setImagePosition:NSImageOnly];
        [self.closeButton setTarget:self];
        [self.closeButton setAction:@selector(closeButtonClicked:)];
        [self.closeButton setContentTintColor:[NSColor systemRedColor]];
        [self.closeButton setAlphaValue:0.0];
        [self addSubview:self.closeButton];
        
        // Minimize button (yellow)
        self.minimizeButton = [[NSButton alloc] initWithFrame:NSMakeRect(startX + buttonSize + spacing, y, buttonSize, buttonSize)];
        [self.minimizeButton setButtonType:NSButtonTypeMomentaryLight];
        [self.minimizeButton setBordered:NO];
        [self.minimizeButton setBezelStyle:NSBezelStyleRegularSquare];
        [self.minimizeButton setImageScaling:NSImageScaleProportionallyDown];
        [self.minimizeButton setImage:[NSImage imageWithSystemSymbolName:@"minus.circle.fill" accessibilityDescription:@"Minimize"]];
        [self.minimizeButton setImagePosition:NSImageOnly];
        [self.minimizeButton setTarget:self];
        [self.minimizeButton setAction:@selector(minimizeButtonClicked:)];
        [self.minimizeButton setContentTintColor:[NSColor systemYellowColor]];
        [self.minimizeButton setAlphaValue:0.0];
        [self addSubview:self.minimizeButton];
        
        // Maximize button (green)
        self.maximizeButton = [[NSButton alloc] initWithFrame:NSMakeRect(startX + (buttonSize + spacing) * 2, y, buttonSize, buttonSize)];
        [self.maximizeButton setButtonType:NSButtonTypeMomentaryLight];
        [self.maximizeButton setBordered:NO];
        [self.maximizeButton setBezelStyle:NSBezelStyleRegularSquare];
        [self.maximizeButton setImageScaling:NSImageScaleProportionallyDown];
        [self.maximizeButton setImage:[NSImage imageWithSystemSymbolName:@"plus.circle.fill" accessibilityDescription:@"Maximize"]];
        [self.maximizeButton setImagePosition:NSImageOnly];
        [self.maximizeButton setTarget:self];
        [self.maximizeButton setAction:@selector(maximizeButtonClicked:)];
        [self.maximizeButton setContentTintColor:[NSColor systemGreenColor]];
        [self.maximizeButton setAlphaValue:0.0];
        [self addSubview:self.maximizeButton];
        
        // Setup tracking areas
        _trackingArea = [[NSTrackingArea alloc] initWithRect:self.bounds
                                                   options:NSTrackingMouseEnteredAndExited | NSTrackingActiveAlways
                                                     owner:self
                                                  userInfo:nil];
        [self addTrackingArea:_trackingArea];
        
        _closeButtonTrackingArea = [[NSTrackingArea alloc] initWithRect:self.closeButton.bounds
                                                               options:NSTrackingMouseEnteredAndExited | NSTrackingActiveAlways
                                                                 owner:self
                                                              userInfo:@{@"button": @"close"}];
        [self.closeButton addTrackingArea:_closeButtonTrackingArea];
        
        _minimizeButtonTrackingArea = [[NSTrackingArea alloc] initWithRect:self.minimizeButton.bounds
                                                                  options:NSTrackingMouseEnteredAndExited | NSTrackingActiveAlways
                                                                    owner:self
                                                                 userInfo:@{@"button": @"minimize"}];
        [self.minimizeButton addTrackingArea:_minimizeButtonTrackingArea];
        
        _maximizeButtonTrackingArea = [[NSTrackingArea alloc] initWithRect:self.maximizeButton.bounds
                                                                  options:NSTrackingMouseEnteredAndExited | NSTrackingActiveAlways
                                                                    owner:self
                                                                 userInfo:@{@"button": @"maximize"}];
        [self.maximizeButton addTrackingArea:_maximizeButtonTrackingArea];
    }
    return self;
}

- (void)drawRect:(NSRect)dirtyRect {
    if (self.isHovered || _displayFrame < 120) {
        // Draw background with rounded corners (top-left and bottom-right only)
        NSRect bounds = self.bounds;
        CGFloat radius = 8.0;
        
        NSBezierPath* path = [NSBezierPath bezierPath];
        [path moveToPoint:NSMakePoint(bounds.origin.x + radius, bounds.origin.y)];
        [path lineToPoint:NSMakePoint(bounds.origin.x + bounds.size.width - radius, bounds.origin.y)];
        [path curveToPoint:NSMakePoint(bounds.origin.x + bounds.size.width, bounds.origin.y + radius)
             controlPoint1:NSMakePoint(bounds.origin.x + bounds.size.width, bounds.origin.y)
             controlPoint2:NSMakePoint(bounds.origin.x + bounds.size.width, bounds.origin.y)];
        [path lineToPoint:NSMakePoint(bounds.origin.x + bounds.size.width, bounds.origin.y + bounds.size.height - radius)];
        [path curveToPoint:NSMakePoint(bounds.origin.x + bounds.size.width - radius, bounds.origin.y + bounds.size.height)
             controlPoint1:NSMakePoint(bounds.origin.x + bounds.size.width, bounds.origin.y + bounds.size.height)
             controlPoint2:NSMakePoint(bounds.origin.x + bounds.size.width, bounds.origin.y + bounds.size.height)];
        [path lineToPoint:NSMakePoint(bounds.origin.x, bounds.origin.y + bounds.size.height)];
        [path lineToPoint:NSMakePoint(bounds.origin.x, bounds.origin.y + radius)];
        [path curveToPoint:NSMakePoint(bounds.origin.x + radius, bounds.origin.y)
             controlPoint1:NSMakePoint(bounds.origin.x, bounds.origin.y)
             controlPoint2:NSMakePoint(bounds.origin.x, bounds.origin.y)];
        [path closePath];
        
        // Apply the current opacity to the background colors
        [[NSColor colorWithCalibratedWhite:0.25 alpha:self.alphaValue] setFill];
        [path fill];
        
        // Draw border with current opacity
        [[NSColor colorWithCalibratedWhite:0.4 alpha:self.alphaValue] setStroke];
        [path setLineWidth:1.0];
        [path stroke];
    }
}

- (void)fadeIn {
    [NSAnimationContext runAnimationGroup:^(NSAnimationContext *context) {
        context.duration = 0.2;
        self.animator.alphaValue = 1.0;
        self.closeButton.animator.alphaValue = 1.0;
        self.minimizeButton.animator.alphaValue = 1.0;
        self.maximizeButton.animator.alphaValue = 1.0;
        [self setNeedsDisplay:YES];
    } completionHandler:nil];
}

- (void)fadeOut {
    [NSAnimationContext runAnimationGroup:^(NSAnimationContext *context) {
        context.duration = 0.2;
        self.animator.alphaValue = 0.0;
        self.closeButton.animator.alphaValue = 0.0;
        self.minimizeButton.animator.alphaValue = 0.0;
        self.maximizeButton.animator.alphaValue = 0.0;
        [self setNeedsDisplay:YES];
    } completionHandler:nil];
}

- (void)mouseEntered:(NSEvent *)event {
    if (event.trackingArea == _trackingArea) {
        self.isHovered = YES;
        [self fadeIn];
    } else if (event.trackingArea == _closeButtonTrackingArea) {
        [self.closeButton setContentTintColor:[[NSColor systemRedColor] colorWithAlphaComponent:0.5]];
    } else if (event.trackingArea == _minimizeButtonTrackingArea) {
        [self.minimizeButton setContentTintColor:[[NSColor systemYellowColor] colorWithAlphaComponent:0.5]];
    } else if (event.trackingArea == _maximizeButtonTrackingArea) {
        [self.maximizeButton setContentTintColor:[[NSColor systemGreenColor] colorWithAlphaComponent:0.5]];
    }
}

- (void)mouseExited:(NSEvent *)event {
    if (event.trackingArea == _trackingArea) {
        self.isHovered = NO;
        if (_displayFrame >= 120) {
            [self fadeOut];
        }
    } else if (event.trackingArea == _closeButtonTrackingArea) {
        [self.closeButton setContentTintColor:[NSColor systemRedColor]];
    } else if (event.trackingArea == _minimizeButtonTrackingArea) {
        [self.minimizeButton setContentTintColor:[NSColor systemYellowColor]];
    } else if (event.trackingArea == _maximizeButtonTrackingArea) {
        [self.maximizeButton setContentTintColor:[NSColor systemGreenColor]];
    }
}

- (void)updateDisplayFrame {
    _displayFrame++;
    if (_displayFrame >= 120) {
        _displayFrame = 120;
        if (!self.isHovered) {
            [self fadeOut];
        }
    } else {
        [self fadeIn];
    }
    [self setNeedsDisplay:YES];
}

- (BOOL)mouseDownCanMoveWindow {
    return YES;
}

- (void)mouseDown:(NSEvent *)event {
    // Only handle dragging if we're not clicking a button
    NSPoint location = [self convertPoint:event.locationInWindow fromView:nil];
    if (![self.closeButton hitTest:location] && 
        ![self.minimizeButton hitTest:location] && 
        ![self.maximizeButton hitTest:location]) {
        [_window performWindowDragWithEvent:event];
    }
}

- (void)closeButtonClicked:(id)sender {
    [NSApp terminate:nil];
}

- (void)minimizeButtonClicked:(id)sender {
    [_window miniaturize:nil];
}

- (void)maximizeButtonClicked:(id)sender {
    [_window toggleFullScreen:nil];
}

@end

void configureMacOSWindow(void* window, float opacity, bool blurEnabled) {
    GLFWwindow* glfwWindow = (GLFWwindow*)window;
    NSWindow* nswindow = glfwGetCocoaWindow(glfwWindow);

    // If already configured, just update settings
    if (configuredWindow == nswindow) {
        updateMacOSWindowProperties(opacity, blurEnabled);
        return;
    }
    
    configuredWindow = nswindow;

    // Window style configuration
    nswindow.styleMask = NSWindowStyleMaskBorderless | 
                         NSWindowStyleMaskResizable | 
                         NSWindowStyleMaskMiniaturizable | 
                         NSWindowStyleMaskFullSizeContentView;
    nswindow.titlebarAppearsTransparent = YES;
    nswindow.titleVisibility = NSWindowTitleHidden;
    nswindow.hasShadow = YES;
    
    // Disable background dragging
    nswindow.movableByWindowBackground = NO;
    
    // Base transparency setup
    [nswindow setOpaque:NO];
    [nswindow setBackgroundColor:[NSColor clearColor]];
    [nswindow setAlphaValue:1.0];

    // Create clipping view for rounded corners
    NSRect contentRect = [nswindow.contentView bounds];
    NSView* containerView = [[NSView alloc] initWithFrame:contentRect];
    [containerView setWantsLayer:YES];

    // Use appropriate corner radius based on macOS version
    // macOS Tahoe (16.0+) uses 26pt, earlier versions use 12pt
    CGFloat cornerRadius = 12.0f; // Default for older versions
    if (@available(macOS 16.0, *)) {
        cornerRadius = 26.0f; // Tahoe liquid glass design
    }
    containerView.layer.cornerRadius = cornerRadius;
    containerView.layer.masksToBounds = YES;
    containerView.autoresizingMask = NSViewWidthSizable | NSViewHeightSizable;

    // Create blurred background view
    NSVisualEffectView* effectView = [[NSVisualEffectView alloc] initWithFrame:containerView.bounds];
    effectView.blendingMode = NSVisualEffectBlendingModeBehindWindow;
    effectView.state = NSVisualEffectStateActive;
    effectView.autoresizingMask = NSViewWidthSizable | NSViewHeightSizable;
    
    // Strong blur material
    if (@available(macOS 10.14, *)) {
        effectView.material = NSVisualEffectMaterialHUDWindow;
    } else {
        effectView.material = NSVisualEffectMaterialDark;
    }
    
    // Create app content container
    NSView* appContainer = [[NSView alloc] initWithFrame:containerView.bounds];
    [appContainer setWantsLayer:YES];
    appContainer.layer.backgroundColor = [[NSColor clearColor] CGColor];
    appContainer.layer.opaque = NO;
    appContainer.alphaValue = opacity;
    appContainer.autoresizingMask = NSViewWidthSizable | NSViewHeightSizable;

    // Get original GLFW content
    NSView* originalGlfwContentView = nswindow.contentView;
    originalGlfwContentView.autoresizingMask = NSViewWidthSizable | NSViewHeightSizable;
    originalGlfwContentView.frame = appContainer.bounds;
    

    // Create top-left menu view
    NSRect menuRect = NSMakeRect(0, contentRect.size.height - 40.0, 104.0, 40.0);  // Increased width from 96 to 104 for more padding
    TopLeftMenuView* menuView = [[TopLeftMenuView alloc] initWithFrame:menuRect window:nswindow];
    menuView.autoresizingMask = NSViewMinYMargin | NSViewMaxXMargin;
    
    // Rebuild view hierarchy with proper z-ordering:
    nswindow.contentView = containerView;
    [containerView addSubview:effectView];
    [containerView addSubview:appContainer];
    [appContainer addSubview:originalGlfwContentView];
    [containerView addSubview:menuView];
    
    // Set the title bar view to be behind other views
    
    // Apply initial blur setting
    [effectView setHidden:!blurEnabled];
    
    // --- FOCUS FIX STARTS HERE ---
    [nswindow setInitialFirstResponder:originalGlfwContentView];
    [nswindow makeFirstResponder:originalGlfwContentView];
    [NSApp activateIgnoringOtherApps:YES];
    [nswindow makeKeyAndOrderFront:nil];
    // --- FOCUS FIX ENDS HERE ---
    
    // Store references for future updates
    appContainerView = appContainer;
    blurView = effectView;
    
    // Store menu view reference for updates
    static TopLeftMenuView* menuViewRef = nil;
    menuViewRef = menuView;
    
    // Start display frame update timer
    [NSTimer scheduledTimerWithTimeInterval:1.0/60.0
                                   repeats:YES
                                     block:^(NSTimer * _Nonnull timer) {
        [menuViewRef updateDisplayFrame];
    }];

    // Force window refresh
    [nswindow invalidateShadow];
    [nswindow display];
}


void updateMacOSWindowProperties(float opacity, bool blurEnabled) {
    // Ensure UI updates happen on main thread
    dispatch_async(dispatch_get_main_queue(), ^{
        // Update existing views if available
        if (appContainerView) {
            [appContainerView setAlphaValue:opacity];
            // Force immediate redraw of this view
            [appContainerView setNeedsDisplay:YES];
        }
        
        if (blurView) {
            [blurView setHidden:!blurEnabled];
            [blurView setNeedsDisplay:YES];
        }
        
        // Refresh window if configured
        if (configuredWindow) {
            // Invalidate window shadow to force refresh
            [configuredWindow invalidateShadow];
            
            // Force immediate redraw of the entire window
            [configuredWindow displayIfNeeded];
            
            // This additional call ensures the transparency is updated
            [configuredWindow setHasShadow:NO];
            [configuredWindow setHasShadow:YES];
        }
    });
}

void setupMacOSApplicationDelegate(void) {
    // Create and set up the application delegate
    if (!gAppDelegate) {
        gAppDelegate = [[NEDAppDelegate alloc] init];
        [NSApp setDelegate:gAppDelegate];
    }
}

void cleanupMacOSApplicationDelegate(void) {
    // Clean up the application delegate
    if (gAppDelegate) {
        [NSApp setDelegate:nil];
        gAppDelegate = nil;
    }
}

// Function to check if termination was requested
bool shouldTerminateApplication(void) {
    return gAppDelegate ? gAppDelegate.shouldTerminate : false;
}

