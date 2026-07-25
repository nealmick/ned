/*
	File: ned.h
	Description: Main application class for NED text editor.
	Composition root owns shared shell state (projectRoot, icons) then Editor.
*/

#pragma once

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <chrono>
#include <string>

#include "editor/editor.h"
#include "files/file_tree.h"
#include "files/files.h"
#include "lsp/lsp_client.h"
#include "shaders/shader_manager.h"
#include "shaders/shader_types.h"
#include "util/app.h"
#include "util/icons.h"
#include "util/ned_terminal.h"
#include "util/project_undo.h"
#include "util/settings.h"
#include "util/splitter.h"
#include "util/welcome.h"

struct GLFWwindow;
class ImFont;

class Ned
{
  public:
	Ned();
	~Ned();

	bool initialize();
	void run();
	void cleanup();

	Settings settings;
	std::string projectRoot;
	Icons icons;
	ProjectUndo projectUndo;
	Editor editor;
	FileExplorer fileExplorer;
	LSPClient lspClient;
	Welcome welcome;
	Splitter splitter;
	FramebufferState fb;
	ShaderQuad quad;
	AccumulationBuffers accum;
	ShaderManager shaderManager;
	NedTerminal terminal;
	App app;

  private:
	bool initialized;

	static void scrollCallback(GLFWwindow *window, double xoffset, double yoffset);
};
