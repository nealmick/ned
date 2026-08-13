// macos_window.h
#pragma once
#ifdef __APPLE__
#ifdef __cplusplus
extern "C" {
#endif
void configureMacOSWindow(void *window, float initialOpacity, bool initialBlurEnabled);
void updateMacOSWindowProperties(float opacity, bool blurEnabled);
void setupMacOSApplicationDelegate(void);
void cleanupMacOSApplicationDelegate(void);
bool shouldTerminateApplication(void);
// Content-view points below the native title bar (0 on other platforms).
float macOSTitlebarInset(void);
typedef void (*MacTitlebarFn)(void);
void setMacOSTitlebarActions(MacTitlebarFn sidebar,
							 MacTitlebarFn terminal,
							 MacTitlebarFn settings);
#ifdef __cplusplus
}
#endif
#else
#ifdef _WIN32
#include "windows_window.h"
inline float macOSTitlebarInset(void) { return windowsTitlebarInset(); }
#else
inline float macOSTitlebarInset(void) { return 0.0f; }
#endif
inline void setMacOSTitlebarActions(void (*)(void), void (*)(void), void (*)(void)) {}
#endif
