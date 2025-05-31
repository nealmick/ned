// macos_window.h
#pragma once
#ifdef __APPLE__
#ifdef __cplusplus
extern "C" {
#endif
void configureMacOSWindow(void *window, float initialOpacity, bool initialBlurEnabled);
void updateMacOSWindowProperties(float opacity, bool blurEnabled); // Add this
#ifdef __cplusplus
}
#endif
#endif