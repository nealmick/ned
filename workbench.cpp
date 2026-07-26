/*
	File: workbench.cpp
	Description: Shared workbench — docked multi-tab IDE shell for standalone + embed.
*/

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "workbench.h"

#include "editor/editor_events.h"
#include "files/file_explorer_events.h"
#include "util/keybinds.h"

#include "imgui_internal.h"

#include <filesystem>
#include <iostream>

namespace fs = std::filesystem;

// ---------------------------------------------------------------------------
// Path helpers
// ---------------------------------------------------------------------------

std::string Workbench::canonicalizePath(const std::string &path)
{
	std::error_code ec;
	fs::path p = fs::weakly_canonical(path, ec);
	if (ec)
		p = fs::absolute(path, ec);
	if (ec)
		return path;
	return p.string();
}

std::string Workbench::tabWindowTitle(const Tab &tab) const
{
	const std::string name = tab.path.empty() ? std::string("Untitled")
											  : fs::path(tab.path).filename().string();
	// Unique ### id per tab instance (not vector index) so closed splits don't
	// leave dock memory that steals the next open into the wrong pane.
	return name + "###ned_tab_" + std::to_string(tab.windowId);
}

Workbench::Tab Workbench::makeTab(const std::string &path)
{
	Tab tab;
	tab.path = path;
	tab.windowId = nextTabWindowId_++;
	tab.editor = std::make_unique<Editor>(settings, projectRoot, icons, projectUndo);
	// Open into the group that currently has focus (VS Code–style).
	tab.preferredDockNodeId =
		activeEditorDockNodeId_ != 0 ? activeEditorDockNodeId_ : editorDockNodeId_;
	tab.forceDock = true;
	return tab;
}

// ---------------------------------------------------------------------------
// Lifetime
// ---------------------------------------------------------------------------

Workbench::Workbench() : projectUndo(projectRoot)
{
	// Bootstrap one editor so FileExplorer / LSP can bind an EditorApi.
	// wireTabEditor needs the editor already constructed; makeTab does that.
	// initialize() also wires event subscriptions — bootstrap must exist first.
	Tab bootstrap;
	bootstrap.windowId = nextTabWindowId_++;
	bootstrap.editor =
		std::make_unique<Editor>(settings, projectRoot, icons, projectUndo);
	// forceDock once layout exists (editorDockNodeId_ set in ensureDockLayout).
	bootstrap.forceDock = true;
	tabs_.push_back(std::move(bootstrap));
	activeIndex_ = 0;

	fileExplorer = std::make_unique<FileExplorer>(
		tabs_[0].editor->api, settings, projectRoot, icons);
	lspClient =
		std::make_unique<LSPClient>(tabs_[0].editor->api, *fileExplorer, settings);
	welcome = std::make_unique<Welcome>(settings, *fileExplorer);

	fileExplorer->openOverride = [this](const std::string &path,
										std::function<void()> after) {
		openOrFocus(path, std::move(after));
	};
	// Mute every open editor (not only the active tab's EditorApi).
	fileExplorer->blockInputOverride = [this](bool blocked) {
		for (auto &tab : tabs_)
			tab.editor->api.setBlockInput(blocked);
	};
}

Workbench::~Workbench() { cleanup(); }

Editor *Workbench::activeEditor()
{
	if (activeIndex_ < 0 || activeIndex_ >= static_cast<int>(tabs_.size()))
		return nullptr;
	return tabs_[static_cast<size_t>(activeIndex_)].editor.get();
}

EditorApi *Workbench::activeApi()
{
	Editor *ed = activeEditor();
	return ed ? &ed->api : nullptr;
}

void Workbench::setActiveIndex(int index)
{
	if (index < 0 || index >= static_cast<int>(tabs_.size()))
		return;
	if (activeIndex_ == index)
		return;
	activeIndex_ = index;
	syncActiveBindings();
}

void Workbench::syncActiveBindings()
{
	EditorApi *api = activeApi();
	if (!api || !fileExplorer || !lspClient)
		return;
	fileExplorer->api = api;
	lspClient->bindEditorApi(*api);
}

bool Workbench::initialize(WorkbenchHostMode mode)
{
	if (initialized_)
	{
		mode_ = mode;
		settings.isEmbedded = (mode == WorkbenchHostMode::Floating);
		return true;
	}

	mode_ = mode;
	settings.isEmbedded = (mode == WorkbenchHostMode::Floating);

	wireTabEditor(*tabs_[0].editor);

	fileExplorer->events.subscribeDidOpenProject(
		[this](const FileExplorerEvents::DidOpenProject &e) {
			projectUndo.loadProject(e.root);
			if (lspClient)
				lspClient->setWorkspace(e.root);
			for (auto &tab : tabs_)
				tab.editor->api.onProjectOpened(e.root);
		});

	fileExplorer->events.subscribeDidOpenDocument(
		[this](const FileExplorerEvents::DidOpenDocument &e) {
			if (!lspClient)
				return;
			EditorApi *api = nullptr;
			for (auto &tab : tabs_)
			{
				if (!tab.editor)
					continue;
				if (tab.path == e.path || tab.editor->api.path() == e.path)
				{
					api = &tab.editor->api;
					break;
				}
			}
			if (!api)
				api = activeApi();
			if (!api)
				return;
			lspClient->init(e.path);
			if (lspClient->isInitialized())
				lspClient->didOpen(e.path, api->text(), api->version(), api->languageId());
		});

	fileExplorer->events.subscribeDidCloseDocument(
		[this](const FileExplorerEvents::DidCloseDocument &e) {
			if (lspClient && lspClient->isInitialized())
				lspClient->didClose(e.path);
		});

	ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_DockingEnable;
	ImGui::GetIO().ConfigWindowsMoveFromTitleBarOnly = true;

	if (!settings.keybinds.loadKeybinds())
		std::cerr << "Failed to load keybinds\n";

	settings.apply(true, tabs_[0].editor->api);
	icons.load();

	initialized_ = true;
	return true;
}

// ---------------------------------------------------------------------------
// Open / focus / close tabs
// ---------------------------------------------------------------------------

void Workbench::wireTabEditor(Editor &ed)
{
	ed.api.events().subscribeDidRequestExclusiveOverlay(
		[this](const EditorEvents::DidRequestExclusiveOverlay &e) {
			using Keep = EditorEvents::DidRequestExclusiveOverlay::Keep;
			if (e.keep != Keep::Settings && !settings.isEmbedded)
				settings.showSettingsWindow = false;
			if (e.keep != Keep::FileFinder && fileExplorer)
				fileExplorer->fileFinder.showFFWindow = false;
		});
	ed.api.events().subscribeDidSave([this](const EditorEvents::DidSave &ev) {
		if (!lspClient || !lspClient->isInitialized())
			return;
		for (auto &t : tabs_)
		{
			if (t.editor->api.path() == ev.path)
			{
				lspClient->didEdit(ev.path, t.editor->api.text(), ev.version);
				break;
			}
		}
	});
}

void Workbench::ensureBootstrapTab()
{
	if (!tabs_.empty())
		return;
	Tab tab = makeTab();
	wireTabEditor(*tab.editor);
	if (!projectRoot.empty())
		tab.editor->api.onProjectOpened(projectRoot);
	tabs_.push_back(std::move(tab));
	activeIndex_ = 0;
	syncActiveBindings();
}

void Workbench::closeTab(int index)
{
	if (index < 0 || index >= static_cast<int>(tabs_.size()))
		return;

	Tab &tab = tabs_[static_cast<size_t>(index)];
	std::string pathToClose;
	if (tab.editor)
	{
		pathToClose = !tab.path.empty() ? tab.path : tab.editor->api.path();
		tab.editor->api.save();
	}

	if (!pathToClose.empty() && fileExplorer)
		fileExplorer->events.emitDidCloseDocument({pathToClose});

	tabs_.erase(tabs_.begin() + index);

	if (tabs_.empty())
	{
		ensureBootstrapTab();
		return;
	}

	if (activeIndex_ == index)
		activeIndex_ = std::min(index, static_cast<int>(tabs_.size()) - 1);
	else if (activeIndex_ > index)
		--activeIndex_;

	if (activeIndex_ < 0 || activeIndex_ >= static_cast<int>(tabs_.size()))
		activeIndex_ = static_cast<int>(tabs_.size()) - 1;

	syncActiveBindings();
	tabs_[static_cast<size_t>(activeIndex_)].wantFocus = true;
}

void Workbench::openOrFocus(const std::string &path, std::function<void()> after)
{
	if (!fileExplorer || path.empty())
		return;

	const std::string abs = canonicalizePath(path);

	for (int i = 0; i < static_cast<int>(tabs_.size()); ++i)
	{
		if (!tabs_[static_cast<size_t>(i)].path.empty() &&
			tabs_[static_cast<size_t>(i)].path == abs)
		{
			setActiveIndex(i);
			tabs_[static_cast<size_t>(i)].wantFocus = true;
			tabs_[static_cast<size_t>(i)].editor->api.requestFocus();
			if (after)
				after();
			return;
		}
	}

	std::string raw;
	const bool ok = fileExplorer->readFileRaw(path, raw);
	if (!ok && raw.empty())
		raw = "Error: Unable to open file.";

	int index = -1;
	if (tabs_.size() == 1 && tabs_[0].path.empty())
	{
		index = 0;
		// Reuse bootstrap window — still force into the active editor group.
		tabs_[0].preferredDockNodeId =
			activeEditorDockNodeId_ != 0 ? activeEditorDockNodeId_ : editorDockNodeId_;
		tabs_[0].forceDock = true;
	} else
	{
		Tab tab = makeTab();
		wireTabEditor(*tab.editor);
		if (!projectRoot.empty())
			tab.editor->api.onProjectOpened(projectRoot);
		tabs_.push_back(std::move(tab));
		index = static_cast<int>(tabs_.size()) - 1;
	}

	Tab &tab = tabs_[static_cast<size_t>(index)];
	tab.path = abs;
	tab.wantFocus = true;

	if (ok)
	{
		tab.editor->api.openDocument(abs, raw);
		tab.editor->api.requestFocus();
		setActiveIndex(index);
		fileExplorer->events.emitDidOpenDocument({abs});
	} else
	{
		tab.path.clear();
		tab.editor->api.failOpen(raw);
		tab.editor->api.requestFocus();
		setActiveIndex(index);
	}

	if (after)
		after();
}

// ---------------------------------------------------------------------------
// Dock layout
// ---------------------------------------------------------------------------

void Workbench::ensureDockLayout(ImGuiID dockspaceId, ImVec2 size)
{
	// DockBuilder asserts size > 0 — never build until we have a real region.
	if (size.x <= 1.0f || size.y <= 1.0f)
		return;

	const bool wantSidebar = settings.sidebarVisible;
	if (dockLayoutBuilt_ && dockLayoutHadSidebar_ == wantSidebar)
		return;

	dockLayoutBuilt_ = true;
	dockLayoutHadSidebar_ = wantSidebar;

	ImGui::DockBuilderRemoveNode(dockspaceId);
	ImGui::DockBuilderAddNode(dockspaceId, ImGuiDockNodeFlags_DockSpace);
	ImGui::DockBuilderSetNodeSize(dockspaceId, size);

	if (wantSidebar)
	{
		ImGuiID dockMain = dockspaceId;
		ImGuiID dockLeft = 0;
		ImGui::DockBuilderSplitNode(dockMain, ImGuiDir_Left, 0.25f, &dockLeft, &dockMain);
		editorDockNodeId_ = dockMain;
		// Chrome-less explorer strip in both host modes (no tab/title bar).
		if (ImGuiDockNode *node = ImGui::DockBuilderGetNode(dockLeft))
		{
			node->LocalFlags |=
				ImGuiDockNodeFlags_NoTabBar | ImGuiDockNodeFlags_NoWindowMenuButton |
				ImGuiDockNodeFlags_NoCloseButton | ImGuiDockNodeFlags_NoUndocking;
		}
		ImGui::DockBuilderDockWindow("File Explorer", dockLeft);
	} else
	{
		editorDockNodeId_ = dockspaceId;
	}

	for (int i = 0; i < static_cast<int>(tabs_.size()); ++i)
	{
		const Tab &tab = tabs_[static_cast<size_t>(i)];
		if (tab.path.empty() && tabs_.size() > 1)
			continue;
		ImGui::DockBuilderDockWindow(tabWindowTitle(tab).c_str(), editorDockNodeId_);
	}

	ImGui::DockBuilderFinish(dockspaceId);
}

void Workbench::renderDockedWorkspace(ImFont *font)
{
	const ImGuiID dockspaceId = ImGui::GetID("NedWorkbenchDock");
	// Capture size *before* DockSpace — after it, ContentRegionAvail is often (0,0)
	// and DockBuilderSetNodeSize asserts.
	ImVec2 dockSize = ImGui::GetContentRegionAvail();
	if (dockSize.x <= 1.0f || dockSize.y <= 1.0f)
		dockSize = ImGui::GetWindowSize();
	if (dockSize.x <= 1.0f || dockSize.y <= 1.0f)
		dockSize = ImGui::GetIO().DisplaySize;

	// Build layout first so the dockspace has a valid tree on this frame.
	ensureDockLayout(dockspaceId, dockSize);
	ImGui::DockSpace(dockspaceId, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_None);

	if (settings.sidebarVisible)
	{
		// Both modes: no title/tab bar, no undock chrome on the explorer panel.
		ImGuiWindowClass explorerClass;
		explorerClass.DockNodeFlagsOverrideSet =
			ImGuiDockNodeFlags_NoTabBar | ImGuiDockNodeFlags_NoUndocking |
			ImGuiDockNodeFlags_NoWindowMenuButton | ImGuiDockNodeFlags_NoCloseButton;
		ImGui::SetNextWindowClass(&explorerClass);
		if (ImGui::Begin("File Explorer",
						 nullptr,
						 ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse |
							 ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize))
			fileExplorer->renderFileExplorer(/*fill*/ -1.0f);
		ImGui::End();
	}

	std::vector<int> toClose;

	for (int i = 0; i < static_cast<int>(tabs_.size()); ++i)
	{
		Tab &tab = tabs_[static_cast<size_t>(i)];
		if (tab.path.empty() && tabs_.size() > 1)
			continue;

		const std::string title = tabWindowTitle(tab);

		// New tabs: dock into the group that was active when opened (not ImGui
		// memory of a recycled window id / leftover empty split node).
		if (tab.forceDock)
		{
			const ImGuiID target =
				tab.preferredDockNodeId != 0
					? tab.preferredDockNodeId
					: (activeEditorDockNodeId_ != 0 ? activeEditorDockNodeId_
													: editorDockNodeId_);
			if (target != 0)
				ImGui::SetNextWindowDockID(target, ImGuiCond_Always);
		}

		if (tab.wantFocus)
		{
			ImGui::SetNextWindowFocus();
			tab.wantFocus = false;
		}

		// Hide the dock tab-bar "list" / window-menu button (▶ tabs dropdown).
		// Per https://github.com/ocornut/imgui/issues/4880 — NoWindowMenuButton.
		ImGuiWindowClass editorClass;
		editorClass.DockNodeFlagsOverrideSet = ImGuiDockNodeFlags_NoWindowMenuButton;
		ImGui::SetNextWindowClass(&editorClass);

		bool open = true;
		if (ImGui::Begin(title.c_str(), &open))
		{
			if (ImGui::IsWindowFocused(ImGuiFocusedFlags_ChildWindows) ||
				ImGui::IsWindowFocused(0))
			{
				if (activeIndex_ != i)
					setActiveIndex(i);
				const ImGuiID dockId = ImGui::GetWindowDockID();
				if (dockId != 0)
					activeEditorDockNodeId_ = dockId;
			}
			if (tab.forceDock && ImGui::IsWindowDocked())
				tab.forceDock = false;
			tab.editor->renderEditor(font, /*fill*/ -1.0f);
		}
		ImGui::End();

		if (!open)
			toClose.push_back(i);
	}

	for (int i = static_cast<int>(toClose.size()) - 1; i >= 0; --i)
		closeTab(toClose[static_cast<size_t>(i)]);
}

// ---------------------------------------------------------------------------
// Chrome
// ---------------------------------------------------------------------------

bool Workbench::beginRootChrome()
{
	if (mode_ == WorkbenchHostMode::Floating)
	{
		ImGui::SetNextWindowPos(floatingPos_, ImGuiCond_FirstUseEver);
		ImGui::SetNextWindowSize(floatingSize_, ImGuiCond_FirstUseEver);
		const ImGuiWindowFlags flags =
			ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoDocking;
		// Embed host shell title (not ImGui's implicit "Debug" fallback).
		const bool open = ImGui::Begin("TextEditor", nullptr, flags);
		if (open)
		{
			floatingPos_ = ImGui::GetWindowPos();
			floatingSize_ = ImGui::GetWindowSize();
		}
		return open;
	}

	// Fullscreen: fill the OS window, not draggable.
	ImGui::SetNextWindowPos(ImVec2(0, 0));
	ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);
	return ImGui::Begin(
		"##ned_root",
		nullptr,
		ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
			ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollWithMouse |
			ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoDocking |
			ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus);
}

void Workbench::endRootChrome() { ImGui::End(); }

void Workbench::renderOverlays(EditorApi &api)
{
	settings.renderSettingsWindow(api, *fileExplorer, *lspClient);
	lspClient->dashboard.render();
	settings.renderNotification("");
}

// ---------------------------------------------------------------------------
// Frame
// ---------------------------------------------------------------------------

void Workbench::tick()
{
	if (!initialized_ || !fileExplorer)
		return;

	const double currentTime = glfwGetTime();
	if (currentTime - timing_.lastSettingsCheck >= SETTINGS_CHECK_INTERVAL)
	{
		settings.checkSettingsFile();
		settings.keybinds.checkKeybindsFile();
		timing_.lastSettingsCheck = currentTime;
	}

	if (currentTime - timing_.lastFileTreeRefresh >= FILE_TREE_REFRESH_INTERVAL)
	{
		fileExplorer->fileTree.refreshFileTree();
		timing_.lastFileTreeRefresh = currentTime;
	}
}

void Workbench::applySettings()
{
	if (!initialized_)
		return;

	EditorApi *api = activeApi();
	if (!api)
		return;

	const bool settingsApplied = settings.apply(false, *api);
	if (settingsApplied)
	{
		for (auto &tab : tabs_)
			tab.editor->api.forceColorUpdate();
	}
	if (terminal.isStarted() && (settingsApplied || terminal.consumeNeedsFontResync()))
	{
		terminal.reloadTerminalFonts(settings.font.getFontSize());
		settings.font.load(/*clearAtlas=*/false);
	}
}

void Workbench::render()
{
	if (!initialized_)
		return;

	EditorApi *api = activeApi();
	if (!api)
		return;

	if (!fileExplorer->fileFinder.showFFWindow)
	{
		settings.keybinds.handleKeyboardShortcuts(
			*api, *fileExplorer, *lspClient, terminal);
	}

	if (fileExplorer->handleFileDialog())
		showWelcome = false;

	terminal.setProjectRoot(projectRoot.empty() ? fileExplorer->projectRoot : projectRoot);

	if (fileExplorer->fileFinder.showFFWindow)
		fileExplorer->setEditorsBlockInput(true);

	ImGui::PushFont(settings.font.getMainFont());

	if (terminal.visible())
	{
		if (beginRootChrome())
			terminal.renderFullscreen();
		endRootChrome();
		renderOverlays(*api);
		ImGui::PopFont();
		return;
	}

	if (showWelcome || fileExplorer->showWelcomeScreen)
	{
		// Standalone welcome draws its own fullscreen window; floating needs chrome.
		// Do not call widgets outside Begin/End here — that forces ImGui's implicit
		// "Debug##Default" window to appear.
		if (mode_ == WorkbenchHostMode::Floating)
		{
			if (beginRootChrome())
				welcome->render();
			endRootChrome();
		} else
		{
			welcome->render();
		}
		renderOverlays(*api);
		ImGui::PopFont();
		return;
	}

	if (beginRootChrome())
		renderDockedWorkspace(settings.font.getMainFont());
	endRootChrome();

	lspClient->render();
	fileExplorer->renderFileFinder();
	renderOverlays(*api);

	ImGui::PopFont();
}

void Workbench::cleanup()
{
	if (!initialized_)
		return;
	// Persist every open tab (no-op when clean / no path).
	for (auto &tab : tabs_)
	{
		if (tab.editor)
			tab.editor->api.save();
	}
	projectUndo.flush();
	terminal.shutdown();
	if (lspClient)
		lspClient->shutdown();
	settings.saveSettings();
	initialized_ = false;
}
