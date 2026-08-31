/*
	File: workbench.h
	Description: Shared IDE workbench — multi-tab dock layout, explorer, terminal,
	welcome, and overlays. Used by standalone Ned (Fullscreen) and NedEmbed (Floating).
*/

#pragma once

#include "editor/editor.h"
#include "files/files.h"
#include "lsp/lsp_client.h"
#include "util/icons.h"
#include "util/ned_terminal.h"
#include "util/project_undo.h"
#include "util/settings.h"
#include "util/welcome.h"

#include <imgui.h>

#include <functional>
#include <memory>
#include <string>
#include <vector>

enum class WorkbenchHostMode {
	Fullscreen, // standalone: fill OS window, no drag chrome
	Floating	// embed: moveable ImGui window
};

class Workbench
{
  public:
	Workbench();
	~Workbench();

	// Requires an ImGui context (and for Fullscreen, host already created backends).
	bool initialize(WorkbenchHostMode mode);
	void tick();		  // disk watches + file tree refresh
	void applySettings(); // settings.apply + terminal font resync
	void render();
	void cleanup();

	Editor *activeEditor();
	EditorApi *activeApi();

	Settings settings;
	std::string projectRoot;
	Icons icons;
	// Shared across all tabs — single JSON writer for the project.
	ProjectUndo projectUndo;

	std::unique_ptr<FileExplorer> fileExplorer;
	std::unique_ptr<LSPClient> lspClient;
	std::unique_ptr<Welcome> welcome;
	NedTerminal terminal;

	bool showWelcome = true;

	WorkbenchHostMode hostMode() const { return mode_; }

  private:
	struct Tab
	{
		std::string path; // absolute; empty = unused/untitled bootstrap
		std::unique_ptr<Editor> editor;
		bool wantFocus = false;
		// Unique window id (###ned_tab_N) — never reuse, so ImGui won't restore a
		// closed split pane's dock slot onto a brand-new tab.
		int windowId = 0;
		// First frame(s): force dock into the editor group that was active on open.
		bool forceDock = false;
		ImGuiID preferredDockNodeId = 0;
		// Consecutive frames undocked while not mid dock-drag. Used to snap orphan
		// floats back without fighting a successful split (ImGui applies the drop
		// after our Begin on the release frame).
		int undockedFrames = 0;
	};

	struct TimingState
	{
		double lastSettingsCheck = 0.0;
		double lastFileTreeRefresh = 0.0;
	};

	static constexpr double SETTINGS_CHECK_INTERVAL = 2.0;
	static constexpr double FILE_TREE_REFRESH_INTERVAL = 2.0;

	WorkbenchHostMode mode_ = WorkbenchHostMode::Floating;
	bool initialized_ = false;
	bool editorDockLayoutBuilt_ = false;
	ImGuiID editorDockNodeId_ = 0;		 // root node of the editor-only dockspace
	ImGuiID activeEditorDockNodeId_ = 0; // dock node of last-focused editor
	int nextTabWindowId_ = 1;
	// 0 until first layout — then seeded from GetFontSize() (260/220 at 20px).
	float explorerWidth_ = 0.0f;
	float terminalHeight_ = 0.0f;
	TimingState timing_;

	std::vector<Tab> tabs_;
	int activeIndex_ = -1;
	int hoveredIndex_ = -1; // tab under the mouse this frame (hover ui)

	// Floating window chrome (embed).
	ImVec2 floatingPos_{200.0f, 200.0f};
	ImVec2 floatingSize_{1100.0f, 700.0f};

	void ensureEditorDockLayout(ImGuiID dockspaceId, ImVec2 size);
	void renderDockedWorkspace(ImFont *font);
	void openOrFocus(const std::string &path, std::function<void()> after = nullptr);
	void closeTab(int index);
	void ensureBootstrapTab();
	void wireTabEditor(Editor &ed);
	void setActiveIndex(int index);
	void syncActiveBindings();
	bool beginRootChrome();
	void endRootChrome();
	void renderOverlays(EditorApi &api);
#ifdef _WIN32
	void drawWindowsTitlebar();
#endif
	std::string tabWindowTitle(const Tab &tab) const;
	Tab makeTab(const std::string &path = {});
	static std::string canonicalizePath(const std::string &path);
};
