/*
File: app.h
Description: Combined application and graphics management class for NED text editor.
This class combines the functionality of ApplicationManager and GraphicsManager.
*/

#pragma once

#ifndef NED_ENABLE_SHADERS
#define NED_ENABLE_SHADERS 1
#endif

#include <GL/glew.h>
#include <GLFW/glfw3.h>

struct TimingState
{
	double lastSettingsCheck = 0.0;
	double lastFileTreeRefresh = 0.0;
};

class Editor;
class FileExplorer;
class LSPClient;
class NedTerminal;
class Settings;
class Splitter;
class Welcome;
#if NED_ENABLE_SHADERS
class ShaderManager;
struct FramebufferState;
#endif

class App
{
  public:
	App(Settings &settings,
		Editor &editor,
		FileExplorer &fileExplorer,
		LSPClient &lspClient,
#if NED_ENABLE_SHADERS
		ShaderManager &shaderManager,
		FramebufferState &fb,
#endif
		Splitter &splitter,
		Welcome &welcome,
		NedTerminal &terminal);
	~App();

	bool initialize();
	void runMainLoop();

	void renderFrame();
	void renderMainWindow();
	void handleBackgroundUpdates();

	static constexpr double SETTINGS_CHECK_INTERVAL = 2.0;
	static constexpr double FILE_TREE_REFRESH_INTERVAL = 2.0;

	GLFWwindow *window = nullptr;
	double scrollXAccumulator = 0.0;
	double scrollYAccumulator = 0.0;

  private:
	Settings &settings;
	Editor &editor;
	FileExplorer &fileExplorer;
	LSPClient &lspClient;
#if NED_ENABLE_SHADERS
	ShaderManager &shaderManager;
	FramebufferState &fb;
#endif
	Splitter &splitter;
	Welcome &welcome;
	NedTerminal &terminal;

	bool windowFocused;
	TimingState timing;

	bool initializeImGui();
	bool createWindow();
	void setWindowIcon();
	bool initializeGLEW();
	void handleScrollAccumulators();
};
