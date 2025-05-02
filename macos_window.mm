// macos_window.mm
#import <Cocoa/Cocoa.h>
#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_COCOA
#include <GLFW/glfw3native.h>
#include "macos_window.h"

void configureMacOSWindow(void* window) {
  NSWindow* nswindow = glfwGetCocoaWindow((GLFWwindow*)window);
  nswindow.styleMask = NSWindowStyleMaskBorderless | NSWindowStyleMaskResizable | NSWindowStyleMaskMiniaturizable;
  nswindow.titlebarAppearsTransparent = YES;
  nswindow.titleVisibility = NSWindowTitleHidden;
  nswindow.collectionBehavior = NSWindowCollectionBehaviorFullScreenPrimary | 
                               NSWindowCollectionBehaviorManaged;
}