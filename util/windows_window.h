#pragma once

#ifdef _WIN32

struct GLFWwindow;
struct ImVec2;

// Borderless client area + DWM shadow/snap. Call once after glfwCreateWindow.
void configureWindowsWindow(GLFWwindow *window);

// Height of the ImGui caption strip (physical px). Updated every frame we draw it.
void windowsSetTitlebarHeight(float height);
float windowsTitlebarInset();

// Client-space interactive caption rects. Cleared each frame.
// Client = ImGui handles the click. Min/Max/Close report native HT* so
// Win11 snap layouts work on the maximize button.
enum class WindowsCaptionHit { Client = 0, Min, Max, Close };

void windowsClearCaptionExcludes();
void windowsExcludeCaptionRect(const ImVec2 &min,
							   const ImVec2 &max,
							   WindowsCaptionHit hit = WindowsCaptionHit::Client);
WindowsCaptionHit windowsCaptionHover();

void windowsMinimize();
void windowsToggleMaximize();
void windowsClose();
bool windowsIsMaximized();

#else

inline float windowsTitlebarInset() { return 0.0f; }

#endif
