// macos_window.mm
#import <Cocoa/Cocoa.h>
#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_COCOA
#include <GLFW/glfw3native.h>
#include "macos_window.h"
#import <QuartzCore/QuartzCore.h>

// Static variables to track views and settings
static NSView* appContainerView = nil;
static NSVisualEffectView* blurView = nil;
static NSWindow* configuredWindow = nil;

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
    NSLog(@"Setting frame: x=%f, width=%f", frame.origin.x, frame.size.width);
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

// Make the view ignore mouse events except for dragging
- (NSView *)hitTest:(NSPoint)point {
    // Convert point to window coordinates
    NSPoint windowPoint = [self convertPoint:point toView:nil];
    
    // Check if the point is in the left margin area (where the ImGui buttons are)
    if (windowPoint.x < _leftMargin) {
        return nil; // Let the event pass through to ImGui
    }
    
    // For the rest of the view, allow dragging
    return [super hitTest:point];
}
@end

// Add window state change notification handling
@interface WindowStateObserver : NSObject
{
    NSWindow* _window;
    NSView* _titleBarView;
    NSView* _appContainer;
}

- (instancetype)initWithWindow:(NSWindow*)window titleBarView:(NSView*)titleBarView appContainer:(NSView*)appContainer;
- (void)windowDidMiniaturize:(NSNotification*)notification;
- (void)windowDidDeminiaturize:(NSNotification*)notification;
@end

@implementation WindowStateObserver

- (instancetype)initWithWindow:(NSWindow*)window titleBarView:(NSView*)titleBarView appContainer:(NSView*)appContainer {
    self = [super init];
    if (self) {
        _window = [window retain];
        _titleBarView = [titleBarView retain];
        _appContainer = [appContainer retain];
        
        [[NSNotificationCenter defaultCenter] addObserver:self
                                               selector:@selector(windowDidMiniaturize:)
                                                   name:NSWindowDidMiniaturizeNotification
                                                 object:window];
        
        [[NSNotificationCenter defaultCenter] addObserver:self
                                               selector:@selector(windowDidDeminiaturize:)
                                                   name:NSWindowDidDeminiaturizeNotification
                                                 object:window];
    }
    return self;
}

- (void)dealloc {
    [[NSNotificationCenter defaultCenter] removeObserver:self];
    [_window release];
    [_titleBarView release];
    [_appContainer release];
    [super dealloc];
}

- (void)windowDidMiniaturize:(NSNotification*)notification {
    NSLog(@"Window did miniaturize");
    
    // Force window to update its view hierarchy
    [_window.contentView setNeedsDisplay:YES];
    [_window.contentView setNeedsLayout:YES];
    
    // Ensure the window is properly focused
    [_window makeKeyAndOrderFront:nil];
    [NSApp activateIgnoringOtherApps:YES];
    
    // Force a complete redraw
    [_window display];
    
    // Ensure the app container is visible and properly positioned
    _appContainer.hidden = NO;
    _appContainer.alphaValue = 1.0;
    
    // Force the window to update its first responder
    [_window makeFirstResponder:_window.contentView];
    
    NSLog(@"Window restoration complete");
}

- (void)windowDidDeminiaturize:(NSNotification*)notification {
    NSLog(@"Window did deminiaturize");
    
    // Force window to update its view hierarchy
    [_window.contentView setNeedsDisplay:YES];
    [_window.contentView setNeedsLayout:YES];
    
    // Ensure the window is properly focused
    [_window makeKeyAndOrderFront:nil];
    [NSApp activateIgnoringOtherApps:YES];
    
    // Force a complete redraw
    [_window display];
    
    // Ensure the app container is visible and properly positioned
    _appContainer.hidden = NO;
    _appContainer.alphaValue = 1.0;
    
    // Force the window to update its first responder
    [_window makeFirstResponder:_window.contentView];
    
    NSLog(@"Window restoration complete");
}

@end

static WindowStateObserver* windowStateObserver = nil;

void configureMacOSWindow(void* window, float opacity, bool blurEnabled) {
    GLFWwindow* glfwWindow = (GLFWwindow*)window;
    NSWindow* nswindow = glfwGetCocoaWindow(glfwWindow);

    // If already configured, just update settings
    if (configuredWindow == nswindow) {
        updateMacOSWindowProperties(opacity, blurEnabled);
        return;
    }
    
    configuredWindow = nswindow;
    NSLog(@"Configuring window: %@", nswindow);

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
    containerView.layer.cornerRadius = 12.0f;
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
    
    // Create draggable title bar area
    CGFloat titleBarHeight = 44.0;
    CGFloat windowWidth = contentRect.size.width;
    CGFloat leftMargin = 100.0;  // Fixed 100px left margin
    CGFloat rightMargin = 100.0;  // Fixed 100px right margin
    CGFloat titleBarWidth = windowWidth - (leftMargin + rightMargin);
    NSRect titleBarRect = NSMakeRect(leftMargin, contentRect.size.height - titleBarHeight, titleBarWidth, titleBarHeight);
    DraggableView* titleBarView = [[DraggableView alloc] initWithFrame:titleBarRect leftMargin:leftMargin rightMargin:rightMargin];
    titleBarView.autoresizingMask = NSViewMinYMargin | NSViewWidthSizable;
    
    // Ensure the view is properly set up
    [titleBarView setWantsLayer:YES];
    titleBarView.layer.backgroundColor = [[NSColor clearColor] CGColor];
    
    // Rebuild view hierarchy with proper z-ordering:
    nswindow.contentView = containerView;
    [containerView addSubview:effectView];
    [containerView addSubview:appContainer];
    [appContainer addSubview:originalGlfwContentView];
    [containerView addSubview:titleBarView];
    
    // Apply initial blur setting
    [effectView setHidden:!blurEnabled];
    
    // Create and store window state observer
    WindowStateObserver* observer = [[WindowStateObserver alloc] initWithWindow:nswindow 
                                                                  titleBarView:titleBarView 
                                                                  appContainer:appContainer];
    [observer autorelease];
    
    // Store references for future updates
    appContainerView = appContainer;
    blurView = effectView;

    // Force window refresh
    [nswindow invalidateShadow];
    [nswindow display];
    
    NSLog(@"Window configuration complete");
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
            
            // Ensure the window is properly focused
            [configuredWindow makeKeyAndOrderFront:nil];
            [NSApp activateIgnoringOtherApps:YES];
        }
    });
}

