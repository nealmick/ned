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
class EditorSurface;
class FileExplorer;
class LSPClient;

class LSPView
{
  public:
	LSPView(LSPClient &client,
			EditorApi &api,
			EditorSurface &surface,
			FileExplorer &fileExplorer,
			Settings &settings);

	// Handle all LSP keybinds (Ctrl-modified goto/symbol-info shortcuts).
	bool handleKeybinds();

	// Point goto/hover/uri UI at a different editor (multi-tab embed).
	void rebind(EditorApi &api, EditorSurface &surface);
	// Mouse-hover tooltip targets the editor under the mouse (splits differ
	// from the focused editor).
	void setHoverApi(EditorApi &api, EditorSurface &surface);

	// Poll async LSP results and render all LSP UI elements (symbol info,
	// goto results, picker).
	void poll();

	LSPDashboard &dashboard();

  private:
	LSPClient &client;
	LSPDashboard dashboard_;
	LSPSymbolInfo symbolInfo;
	// Shared URI-options picker
	LSPUriOptions uriOptions;
};

#else // !NED_ENABLE_LSP

// Minimal stand-in so keybinds/settings/workbench compile without lsp-framework.
class LSPView
{
  public:
	struct DashboardStub
	{
		void render() {}
		void setShow(bool) {}
	};

	LSPView(class LSPClient &client,
			class EditorApi &api,
			class EditorSurface &surface,
			class FileExplorer &fileExplorer,
			Settings &settings)
	{
		(void)client;
		(void)api;
		(void)surface;
		(void)fileExplorer;
	}

	DashboardStub dashboard_;
	DashboardStub &dashboard() { return dashboard_; }

	bool handleKeybinds() { return false; }
	void rebind(class EditorApi &api, class EditorSurface &surface)
	{
		(void)api;
		(void)surface;
	}
	void setHoverApi(class EditorApi &api, class EditorSurface &surface)
	{
		(void)api;
		(void)surface;
	}
	void poll() {}
};

#endif // NED_ENABLE_LSP
