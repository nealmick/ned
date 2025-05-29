// macos_window.mm
#import <Cocoa/Cocoa.h>
#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_COCOA
#include <GLFW/glfw3native.h>
#include "macos_window.h"
#import <QuartzCore/QuartzCore.h>

// Custom view for draggable areas
@interface DraggableView : NSView
@end

@implementation DraggableView
- (BOOL)mouseDownCanMoveWindow {
    return YES;
}
@end

void configureMacOSWindow(void* window) {
    GLFWwindow* glfwWindow = (GLFWwindow*)window;
    NSWindow* nswindow = glfwGetCocoaWindow(glfwWindow);

    // Window style configuration
    nswindow.styleMask = NSWindowStyleMaskBorderless | 
                         NSWindowStyleMaskResizable | 
                         NSWindowStyleMaskMiniaturizable | 
                         NSWindowStyleMaskFullSizeContentView;
    nswindow.titlebarAppearsTransparent = YES;
    nswindow.titleVisibility = NSWindowTitleHidden;
    nswindow.hasShadow = YES;
    
    // Turn off background dragging - we'll handle dragging ourselves
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
    NSVisualEffectView* blurView = [[NSVisualEffectView alloc] initWithFrame:containerView.bounds];
    blurView.blendingMode = NSVisualEffectBlendingModeBehindWindow;
    blurView.state = NSVisualEffectStateActive;
    blurView.autoresizingMask = NSViewWidthSizable | NSViewHeightSizable;
    
    // Strong blur material
    if (@available(macOS 10.14, *)) {
        blurView.material = NSVisualEffectMaterialHUDWindow;
    } else {
        blurView.material = NSVisualEffectMaterialDark;
    }
    
    // Create app content container
    NSView* appContainer = [[NSView alloc] initWithFrame:containerView.bounds];
    [appContainer setWantsLayer:YES];
    appContainer.layer.backgroundColor = [[NSColor clearColor] CGColor];
    appContainer.layer.opaque = NO;
    appContainer.alphaValue = 0.55; // Semi-transparent app content
    appContainer.autoresizingMask = NSViewWidthSizable | NSViewHeightSizable;

    // Get original GLFW content
    NSView* originalGlfwContentView = nswindow.contentView;
    originalGlfwContentView.autoresizingMask = NSViewWidthSizable | NSViewHeightSizable;
    originalGlfwContentView.frame = appContainer.bounds;
    
    // Create draggable title bar area
    CGFloat titleBarHeight = 24.0;
    NSRect titleBarRect = NSMakeRect(0, contentRect.size.height - titleBarHeight, contentRect.size.width, titleBarHeight);
    DraggableView* titleBarView = [[DraggableView alloc] initWithFrame:titleBarRect];
    titleBarView.autoresizingMask = NSViewWidthSizable | NSViewMinYMargin;
    
    // Rebuild view hierarchy:
    // 1. Window's content view -> Container View (rounds corners)
    // 2. Container View -> Blur View (background blur)
    // 3. Container View -> App Container (semi-transparent layer for app content)
    // 4. App Container -> Original GLFW Content
    // 5. Container View -> Title Bar View (for dragging, on top)
    nswindow.contentView = containerView;
    [containerView addSubview:blurView];
    [containerView addSubview:appContainer];
    [appContainer addSubview:originalGlfwContentView];
    [containerView addSubview:titleBarView]; // Add on top
    
    // Make sure views are positioned correctly
    [blurView setFrameOrigin:NSMakePoint(0, 0)];
    [appContainer setFrameOrigin:NSMakePoint(0, 0)];
    [titleBarView setFrameOrigin:NSMakePoint(0, contentRect.size.height - titleBarHeight)];
    
    // Force window refresh
    [nswindow invalidateShadow];
    [nswindow display];
}