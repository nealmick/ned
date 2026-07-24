/*
File: ned.cpp
Description: Composition root — constructs the object graph in the init list;
ctor body only registers reactions (same pattern as Editor).
*/

#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "imgui_internal.h"
#ifdef IMGUI_ENABLE_FREETYPE
#include "misc/freetype/imgui_freetype.h"
#endif
#ifdef __APPLE__
#include "util/macos_window.h"
#endif

#include "ned.h"

#include "editor/editor_events.h"
#include "files/file_explorer_events.h"
#include "files/files.h"
#include "lsp/lsp_client.h"
#include "util/settings.h"

#include <iostream>

Ned::Ned()
	: projectUndo(projectRoot),
	  editor(settings, projectRoot, icons, projectUndo),
	  fileExplorer(editor.api, settings, projectRoot, icons),
	  lspClient(editor.api, fileExplorer, settings),
	  welcome(settings, fileExplorer),
	  splitter(settings),
	  shaderManager(settings, fb, accum, quad),
	  app(settings,
		  editor,
		  fileExplorer,
		  lspClient,
		  shaderManager,
		  splitter,
		  welcome,
		  fb,
		  terminal),
	  initialized(false)
{
	using Keep = EditorEvents::DidRequestExclusiveOverlay::Keep;

	// Shell overlays (settings / file finder) — editor siblings handled in EditorApi.
	editor.api.events().subscribeDidRequestExclusiveOverlay(
		[this](const EditorEvents::DidRequestExclusiveOverlay &e) {
			if (e.keep != Keep::Settings && !settings.isEmbedded)
				settings.showSettingsWindow = false;
			if (e.keep != Keep::FileFinder)
				fileExplorer.fileFinder.showFFWindow = false;
		});

	editor.api.events().subscribeDidSave([this](const EditorEvents::DidSave &e) {
		if (lspClient.isInitialized())
			lspClient.didEdit(e.path, editor.api.text(), e.version);
	});

	fileExplorer.events.subscribeDidOpenProject(
		[this](const FileExplorerEvents::DidOpenProject &e) {
			projectUndo.loadProject(e.root);
			editor.api.onProjectOpened(e.root);
			lspClient.setWorkspace(e.root);
		});

	fileExplorer.events.subscribeDidOpenDocument(
		[this](const FileExplorerEvents::DidOpenDocument &e) {
			lspClient.init(e.path);
			if (lspClient.isInitialized())
				lspClient.didOpen(e.path,
								  editor.api.text(),
								  editor.api.version(),
								  editor.api.languageId());
		});

	fileExplorer.events.subscribeDidCloseDocument(
		[this](const FileExplorerEvents::DidCloseDocument &e) {
			if (lspClient.isInitialized())
				lspClient.didClose(e.path);
		});
}

Ned::~Ned()
{
	if (initialized)
		cleanup();
}

bool Ned::initialize()
{
	app.initialize();
	quad.initialize();

	glfwSetWindowUserPointer(app.window, this);
	glfwSetScrollCallback(app.window, Ned::scrollCallback);

	initialized = true;
	return true;
}

void Ned::scrollCallback(GLFWwindow *window, double xoffset, double yoffset)
{
	double x = xoffset * 0.2;
	double y = yoffset * 0.2;
	Ned *ned = static_cast<Ned *>(glfwGetWindowUserPointer(window));
	if (!ned)
		return;

	ned->app.scrollXAccumulator += x;
	ned->app.scrollYAccumulator += y;
}

void Ned::run()
{
	if (!initialized)
	{
		std::cerr << "Cannot run: Not initialized" << std::endl;
		return;
	}

	app.runMainLoop();
}

void Ned::cleanup()
{
	lspClient.shutdown();
	terminal.shutdown();

	quad.cleanup();
	shaderManager.cleanupFramebuffers();
	settings.saveSettings();
	editor.api.save();
	projectUndo.flush();
	if (app.window)
		glfwDestroyWindow(app.window);
	glfwTerminate();
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();
}
