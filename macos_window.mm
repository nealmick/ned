// macos_window.mm
#import <Cocoa/Cocoa.h>
#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_COCOA
#include <GLFW/glfw3native.h>
#include "macos_window.h"

#import <QuartzCore/QuartzCore.h> // Add this at the top
void configureMacOSWindow(void* window) {
    NSWindow* nswindow = glfwGetCocoaWindow((GLFWwindow*)window);
    
    // Existing configuration
    nswindow.styleMask = NSWindowStyleMaskBorderless | NSWindowStyleMaskResizable | NSWindowStyleMaskMiniaturizable;
    nswindow.titlebarAppearsTransparent = YES;
    nswindow.titleVisibility = NSWindowTitleHidden;
    nswindow.collectionBehavior = NSWindowCollectionBehaviorFullScreenPrimary | NSWindowCollectionBehaviorManaged;

    // Window appearance setup
    nswindow.backgroundColor = [NSColor clearColor];
    nswindow.opaque = NO;
    nswindow.hasShadow = YES;

    // Get the content view and configure layer
    NSView* contentView = nswindow.contentView;
    contentView.wantsLayer = YES;
    
    // Rounded corners
    contentView.layer.cornerRadius = 8.0f;
    contentView.layer.masksToBounds = YES;
    
    // Add border (NEW)
    contentView.layer.borderWidth = 1.0f;
    contentView.layer.borderColor = [[NSColor colorWithWhite:0.7 alpha:0.3] CGColor]; // Subtle grey border
}