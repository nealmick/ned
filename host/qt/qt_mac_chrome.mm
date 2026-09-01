/*
	File: host/qt/qt_mac_chrome.mm
	Description: macOS chrome for the Qt host. Deliberately gentler than
	the GLFW path (configureMacOSNSWindow): no content-view reparenting
	(that breaks QNSView responders and window dragging) and no ImGui
	titlebar accessory buttons. Just a transparent title bar so the window
	looks at home next to the ImGui build while keeping fully native drag,
	traffic lights and title.
*/

#import <Cocoa/Cocoa.h>

void configureNedQtChrome(void *qtWinId, float opacity, bool blurEnabled)
{
	(void)opacity;
	(void)blurEnabled; // vibrancy needs content reparenting; deferred for Qt
	NSView *view = reinterpret_cast<NSView *>(qtWinId);
	if (NSWindow *nswindow = view.window)
	{
		if (@available(macOS 11.0, *))
			nswindow.titlebarSeparatorStyle = NSTitlebarSeparatorStyleNone;
		nswindow.titlebarAppearsTransparent = YES;
	}
}
