// macos_window.h
#pragma once
#ifdef __APPLE__
#ifdef __cplusplus
extern "C" {
#endif
void configureMacOSWindow(void *window, float initialOpacity, bool initialBlurEnabled);
void updateMacOSWindowProperties(float opacity, bool blurEnabled); // Add this
void setupMacOSApplicationDelegate(void);	// Add this for proper Cmd+Q handling
void cleanupMacOSApplicationDelegate(void); // Add this for cleanup
bool shouldTerminateApplication(void);		// Add this to check termination status
#ifdef __cplusplus
}
#endif
#endif