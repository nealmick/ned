/*
	File: views/imgui/lsp_view.h
	Description: ImGui-facing LSP UI. Owns the dashboard, symbol-info tooltip
	and URI-options picker, polls LSP keybinds and renders goto results.
	The LSPClient itself stays a pure session/transport class.
*/

#pragma once

#ifndef NED_ENABLE_LSP
#define NED_ENABLE_LSP 1
#endif

#include "../../../util/settings.h"

#if NED_ENABLE_LSP
#include "lsp_dashboard.h"
#include "lsp_symbol_info.h"
#include "lsp_uri_options.h"

class EditorApi;
class FileExplorer;
class LSPClient;

class LspImGuiView
{
  public:
	LspImGuiView(LSPClient &client,
				 EditorApi &api,
				 FileExplorer &fileExplorer,
				 Settings &settings);

	// Handle all LSP keybinds (Ctrl-modified goto/symbol-info shortcuts).
	bool keybinds();

	// Point goto/hover/uri UI at a different editor (multi-tab embed).
	void bindEditorApi(EditorApi &api);
	// Mouse-hover tooltip targets the editor under the mouse (splits differ
	// from the focused editor).
	void setHoverApi(EditorApi &api);

	// Render all LSP UI elements (symbol info, goto results, picker).
	void render();

	LSPDashboard dashboard;

  private:
	LSPClient &client;
	LSPSymbolInfo symbolInfo;
	// Shared URI-options picker
	LSPUriOptions uriOptions;
};

#else // !NED_ENABLE_LSP

// Minimal stand-in so keybinds/settings/workbench compile without lsp-framework.
class LspImGuiView
{
  public:
	struct DashboardStub
	{
		void render() {}
		void setShow(bool) {}
	};

	LspImGuiView(class LSPClient &client,
				 class EditorApi &api,
				 class FileExplorer &fileExplorer,
				 Settings &settings)
	{
		(void)client;
		(void)api;
		(void)fileExplorer;
	}

	DashboardStub dashboard;

	bool keybinds() { return false; }
	void bindEditorApi(class EditorApi &api) { (void)api; }
	void setHoverApi(class EditorApi &api) { (void)api; }
	void render() {}
};

#endif // NED_ENABLE_LSP
