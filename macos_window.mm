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

@implementation DraggableView
- (BOOL)mouseDownCanMoveWindow {
    return YES;
}

- (void)mouseDown:(NSEvent *)event {
    [self.window performWindowDragWithEvent:event];
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
    
    // Enable background dragging
    nswindow.movableByWindowBackground = YES;
    
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
    CGFloat titleBarHeight = 24.0;
    NSRect titleBarRect = NSMakeRect(0, contentRect.size.height - titleBarHeight, contentRect.size.width, titleBarHeight);
    DraggableView* titleBarView = [[DraggableView alloc] initWithFrame:titleBarRect];
    titleBarView.autoresizingMask = NSViewWidthSizable | NSViewMinYMargin;
    
    // Rebuild view hierarchy:
    nswindow.contentView = containerView;
    [containerView addSubview:effectView];
    [containerView addSubview:appContainer];
    [appContainer addSubview:originalGlfwContentView];
    [containerView addSubview:titleBarView];
    
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

