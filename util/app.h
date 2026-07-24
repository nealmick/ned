/*
File: app.h
Description: Combined application and graphics management class for NED text editor.
This class combines the functionality of ApplicationManager and GraphicsManager.
*/

#pragma once

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
class ShaderManager;
class Splitter;
class Welcome;
struct FramebufferState;

class App
{
  public:
	App(Settings &settings,
		Editor &editor,
		FileExplorer &fileExplorer,
		LSPClient &lspClient,
		ShaderManager &shaderManager,
		Splitter &splitter,
		Welcome &welcome,
		FramebufferState &fb,
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
	ShaderManager &shaderManager;
	Splitter &splitter;
	Welcome &welcome;
	FramebufferState &fb;
	NedTerminal &terminal;

	bool windowFocused;
	TimingState timing;

	bool initializeImGui();
	bool createWindow();
	bool initializeGLEW();
	void handleScrollAccumulators();
};
