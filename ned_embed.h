/*
	File: ned_embed.h
	Description: Embeddable NED — multi-tab editors via ImGui docking.
	Standalone Ned remains single-buffer; only this host opens many tabs.
*/

#pragma once

#include "editor/editor.h"
#include "files/files.h"
#include "lsp/lsp_client.h"
#include "util/icons.h"
#include "util/ned_terminal.h"
#include "util/project_undo.h"
#include "util/settings.h"
#include "util/splitter.h"
#include "util/welcome.h"

#include <imgui.h>

#include <functional>
#include <memory>
#include <string>
#include <vector>

class NedEmbed
{
  public:
	NedEmbed();
	~NedEmbed();

	bool initialize();
	void render();
	void applySettingsChanges();
	void cleanup();

	Settings settings;
	std::string projectRoot;
	Icons icons;
	// Shared across all tabs — single JSON writer for the project.
	ProjectUndo projectUndo;

	// Heap-allocated so tabs can own Editors created after FileExplorer exists.
	std::unique_ptr<FileExplorer> fileExplorer;
	std::unique_ptr<LSPClient> lspClient;
	std::unique_ptr<Welcome> welcome;
	Splitter splitter;
	NedTerminal terminal;

	bool showWelcome = true;

	// Active tab (nullptr if none).
	Editor *activeEditor();
	EditorApi *activeApi();

  private:
	struct Tab
	{
		std::string path; // absolute; empty = unused/untitled bootstrap
		std::unique_ptr<Editor> editor;
		bool wantFocus = false;
	};

	bool initialized = false;
	bool dockLayoutBuilt = false;
	bool dockLayoutHadSidebar = true;
	ImGuiID editorDockNodeId = 0;

	std::vector<Tab> tabs;
	int activeIndex = -1;

	void ensureDockLayout(ImGuiID dockspaceId);
	void renderDockedWorkspace(ImFont *font);
	void openOrFocus(const std::string &path, std::function<void()> after = nullptr);
	void closeTab(int index);
	void ensureBootstrapTab();
	void wireTabEditor(Editor &ed);
	void setActiveIndex(int index);
	void syncActiveBindings();
	std::string tabWindowTitle(const Tab &tab, int index) const;
	static std::string canonicalizePath(const std::string &path);
};
