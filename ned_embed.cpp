/*
	File: ned_embed.cpp
	Description: Embeddable NED with multi-tab editors (ImGui docking).
	Standalone Ned is unchanged (single buffer).
*/

#include <GL/glew.h>

#include "ned_embed.h"

#include "editor/editor_events.h"
#include "files/file_explorer_events.h"
#include "util/keybinds.h"

#include "imgui_internal.h"

#include <filesystem>
#include <iostream>

namespace fs = std::filesystem;

extern "C" void updateMacOSWindowProperties(float opacity, bool blurEnabled)
{
	(void)opacity;
	(void)blurEnabled;
}

// ---------------------------------------------------------------------------
// Path helpers
// ---------------------------------------------------------------------------

std::string NedEmbed::canonicalizePath(const std::string &path)
{
	std::error_code ec;
	fs::path p = fs::weakly_canonical(path, ec);
	if (ec)
		p = fs::absolute(path, ec);
	if (ec)
		return path;
	return p.string();
}

std::string NedEmbed::tabWindowTitle(const Tab &tab, int index) const
{
	const std::string name = tab.path.empty() ? std::string("Untitled")
											  : fs::path(tab.path).filename().string();
	// Stable ### id by slot so renaming the label (file name) keeps the dock tab.
	return name + "###ned_tab_" + std::to_string(index);
}

// ---------------------------------------------------------------------------
// Lifetime
// ---------------------------------------------------------------------------

NedEmbed::NedEmbed() : projectUndo(projectRoot), splitter(settings)
{
	// Bootstrap one editor so FileExplorer / LSP can bind an EditorApi.
	tabs.push_back(
		Tab{"", std::make_unique<Editor>(settings, projectRoot, icons, projectUndo)});
	activeIndex = 0;

	fileExplorer =
		std::make_unique<FileExplorer>(tabs[0].editor->api, settings, projectRoot, icons);
	lspClient = std::make_unique<LSPClient>(tabs[0].editor->api, *fileExplorer, settings);
	welcome = std::make_unique<Welcome>(settings, *fileExplorer);

	// Multi-tab open path (standalone FileExplorer keeps default single-buffer).
	fileExplorer->openOverride = [this](const std::string &path,
										std::function<void()> after) {
		openOrFocus(path, std::move(after));
	};
	// Mute every open editor (not only the active tab's EditorApi).
	fileExplorer->blockInputOverride = [this](bool blocked) {
		for (auto &tab : tabs)
			tab.editor->api.setBlockInput(blocked);
	};

	initialize();
}

NedEmbed::~NedEmbed() { cleanup(); }

Editor *NedEmbed::activeEditor()
{
	if (activeIndex < 0 || activeIndex >= static_cast<int>(tabs.size()))
		return nullptr;
	return tabs[static_cast<size_t>(activeIndex)].editor.get();
}

EditorApi *NedEmbed::activeApi()
{
	Editor *ed = activeEditor();
	return ed ? &ed->api : nullptr;
}

void NedEmbed::setActiveIndex(int index)
{
	if (index < 0 || index >= static_cast<int>(tabs.size()))
		return;
	if (activeIndex == index)
		return;
	activeIndex = index;
	syncActiveBindings();
}

void NedEmbed::syncActiveBindings()
{
	EditorApi *api = activeApi();
	if (!api || !fileExplorer || !lspClient)
		return;
	fileExplorer->api = api;
	lspClient->bindEditorApi(*api);
}

bool NedEmbed::initialize()
{
	if (initialized)
		return true;

	wireTabEditor(*tabs[0].editor);

	fileExplorer->events.subscribeDidOpenProject(
		[this](const FileExplorerEvents::DidOpenProject &e) {
			// One shared undo store for the project (not per-tab load).
			projectUndo.loadProject(e.root);
			if (lspClient)
				lspClient->setWorkspace(e.root);
			// Per-editor git only.
			for (auto &tab : tabs)
				tab.editor->api.onProjectOpened(e.root);
		});

	fileExplorer->events.subscribeDidOpenDocument(
		[this](const FileExplorerEvents::DidOpenDocument &e) {
			if (!lspClient)
				return;
			// Prefer the tab that actually holds this path (not only activeIndex).
			EditorApi *api = nullptr;
			for (auto &tab : tabs)
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

	settings.isEmbedded = true;
	ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_DockingEnable;

	if (!settings.keybinds.loadKeybinds())
		std::cerr << "Failed to load keybinds\n";

	settings.apply(true, tabs[0].editor->api);
	icons.load();
	ImGui::GetIO().ConfigWindowsMoveFromTitleBarOnly = true;

	initialized = true;
	return true;
}

// ---------------------------------------------------------------------------
// Open / focus / close tabs
// ---------------------------------------------------------------------------

void NedEmbed::wireTabEditor(Editor &ed)
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
		for (auto &t : tabs)
		{
			if (t.editor->api.path() == ev.path)
			{
				lspClient->didEdit(ev.path, t.editor->api.text(), ev.version);
				break;
			}
		}
	});
}

void NedEmbed::ensureBootstrapTab()
{
	if (!tabs.empty())
		return;
	Tab tab;
	tab.editor = std::make_unique<Editor>(settings, projectRoot, icons, projectUndo);
	wireTabEditor(*tab.editor);
	if (!projectRoot.empty())
		tab.editor->api.onProjectOpened(projectRoot);
	tabs.push_back(std::move(tab));
	activeIndex = 0;
	syncActiveBindings();
}

void NedEmbed::closeTab(int index)
{
	if (index < 0 || index >= static_cast<int>(tabs.size()))
		return;

	Tab &tab = tabs[static_cast<size_t>(index)];
	// Persist before discard (no-op if clean / no path).
	// Capture path before save/erase so LSP can didClose after any final didChange.
	std::string pathToClose;
	if (tab.editor)
	{
		pathToClose = !tab.path.empty() ? tab.path : tab.editor->api.path();
		tab.editor->api.save();
	}

	if (!pathToClose.empty() && fileExplorer)
		fileExplorer->events.emitDidCloseDocument({pathToClose});

	tabs.erase(tabs.begin() + index);

	if (tabs.empty())
	{
		ensureBootstrapTab();
		return;
	}

	if (activeIndex == index)
		activeIndex = std::min(index, static_cast<int>(tabs.size()) - 1);
	else if (activeIndex > index)
		--activeIndex;

	if (activeIndex < 0 || activeIndex >= static_cast<int>(tabs.size()))
		activeIndex = static_cast<int>(tabs.size()) - 1;

	syncActiveBindings();
	tabs[static_cast<size_t>(activeIndex)].wantFocus = true;
}

void NedEmbed::openOrFocus(const std::string &path, std::function<void()> after)
{
	if (!fileExplorer || path.empty())
		return;

	const std::string abs = canonicalizePath(path);

	// Already open → focus that tab.
	for (int i = 0; i < static_cast<int>(tabs.size()); ++i)
	{
		if (!tabs[static_cast<size_t>(i)].path.empty() &&
			tabs[static_cast<size_t>(i)].path == abs)
		{
			setActiveIndex(i);
			tabs[static_cast<size_t>(i)].wantFocus = true;
			// Dock window focus + document child keyboard (explorer click steals both).
			tabs[static_cast<size_t>(i)].editor->api.requestFocus();
			if (after)
				after();
			return;
		}
	}

	std::string raw;
	const bool ok = fileExplorer->readFileRaw(path, raw);
	if (!ok && raw.empty())
		raw = "Error: Unable to open file.";

	// Reuse bootstrap untitled tab if still empty.
	int index = -1;
	if (tabs.size() == 1 && tabs[0].path.empty())
	{
		index = 0;
	} else
	{
		Tab tab;
		tab.editor = std::make_unique<Editor>(settings, projectRoot, icons, projectUndo);
		wireTabEditor(*tab.editor);
		if (!projectRoot.empty())
			tab.editor->api.onProjectOpened(projectRoot);
		tabs.push_back(std::move(tab));
		index = static_cast<int>(tabs.size()) - 1;
	}

	Tab &tab = tabs[static_cast<size_t>(index)];
	tab.path = abs;
	tab.wantFocus = true;

	if (ok)
	{
		// Absolute path so editor/LSP/tab keys agree (documentKey-friendly).
		tab.editor->api.openDocument(abs, raw);
		// File-tree click leaves focus on the explorer; pull it into this document.
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

void NedEmbed::ensureDockLayout(ImGuiID dockspaceId)
{
	const bool wantSidebar = Splitter::showSidebar;
	if (dockLayoutBuilt && dockLayoutHadSidebar == wantSidebar)
		return;

	dockLayoutBuilt = true;
	dockLayoutHadSidebar = wantSidebar;

	ImGui::DockBuilderRemoveNode(dockspaceId);
	ImGui::DockBuilderAddNode(dockspaceId, ImGuiDockNodeFlags_DockSpace);
	ImGui::DockBuilderSetNodeSize(dockspaceId, ImGui::GetContentRegionAvail());

	if (wantSidebar)
	{
		ImGuiID dockMain = dockspaceId;
		ImGuiID dockLeft = 0;
		ImGui::DockBuilderSplitNode(dockMain, ImGuiDir_Left, 0.25f, &dockLeft, &dockMain);
		editorDockNodeId = dockMain;
		ImGui::DockBuilderDockWindow("File Explorer", dockLeft);
	} else
	{
		editorDockNodeId = dockspaceId;
	}

	// All editor tabs share the right (or full) node → ImGui tab bar.
	for (int i = 0; i < static_cast<int>(tabs.size()); ++i)
	{
		const Tab &tab = tabs[static_cast<size_t>(i)];
		if (tab.path.empty() && tabs.size() > 1)
			continue; // skip leftover empty bootstrap if we still have others
		ImGui::DockBuilderDockWindow(tabWindowTitle(tab, i).c_str(), editorDockNodeId);
	}

	ImGui::DockBuilderFinish(dockspaceId);
}

void NedEmbed::renderDockedWorkspace(ImFont *font)
{
	const ImGuiID dockspaceId = ImGui::GetID("NedEmbedDock");
	ImGui::DockSpace(dockspaceId, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_None);
	ensureDockLayout(dockspaceId);

	if (Splitter::showSidebar)
	{
		if (ImGui::Begin("File Explorer"))
			fileExplorer->renderFileExplorer(/*fill*/ -1.0f);
		ImGui::End();
	}

	// Close after the loop — erasing while iterating invalidates indices.
	std::vector<int> toClose;

	for (int i = 0; i < static_cast<int>(tabs.size()); ++i)
	{
		Tab &tab = tabs[static_cast<size_t>(i)];
		// Hide empty bootstrap once we have at least one real file tab.
		if (tab.path.empty() && tabs.size() > 1)
			continue;

		const std::string title = tabWindowTitle(tab, i);
		if (editorDockNodeId != 0)
			ImGui::SetNextWindowDockID(editorDockNodeId, ImGuiCond_Once);

		if (tab.wantFocus)
		{
			ImGui::SetNextWindowFocus();
			tab.wantFocus = false;
		}

		// Non-null p_open → ImGui shows the dock tab close (X) button.
		bool open = true;
		if (ImGui::Begin(title.c_str(), &open))
		{
			// Prefer the window that actually received focus this frame so we
			// don't thrash activeIndex while several editors are visible.
			if (ImGui::IsWindowFocused(ImGuiFocusedFlags_ChildWindows) ||
				ImGui::IsWindowFocused(0))
			{
				if (activeIndex != i)
					setActiveIndex(i);
			}
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
// Frame
// ---------------------------------------------------------------------------

void NedEmbed::render()
{
	if (!initialized)
		return;

	EditorApi *api = activeApi();
	if (!api)
		return;

	// Always handle app shortcuts while the host is drawing us.
	// Skip editor-affecting keys while file finder owns the keyboard.
	if (!fileExplorer->fileFinder.showFFWindow)
	{
		settings.keybinds.handleKeyboardShortcuts(
			*api, *fileExplorer, *lspClient, terminal);
	}

	if (fileExplorer->handleFileDialog())
		showWelcome = false;

	terminal.setProjectRoot(projectRoot);

	// Block all editor tabs *before* they process input this frame.
	if (fileExplorer->fileFinder.showFFWindow)
		fileExplorer->setEditorsBlockInput(true);

	ImGui::PushFont(settings.font.getMainFont());

	if (terminal.visible())
	{
		terminal.renderFullscreen();
		settings.renderSettingsWindow(*api, *fileExplorer, *lspClient);
		lspClient->dashboard.render();
		settings.renderNotification("");
		ImGui::PopFont();
		return;
	}

	if (showWelcome || fileExplorer->showWelcomeScreen)
	{
		welcome->render();
		ImGui::Dummy(ImVec2(0, 0));
		settings.renderSettingsWindow(*api, *fileExplorer, *lspClient);
		lspClient->dashboard.render();
		settings.renderNotification("");
		ImGui::PopFont();
		return;
	}

	renderDockedWorkspace(settings.font.getMainFont());

	lspClient->render();
	fileExplorer->renderFileFinder();

	settings.renderSettingsWindow(*api, *fileExplorer, *lspClient);
	lspClient->dashboard.render();
	settings.renderNotification("");
	settings.keybinds.checkKeybindsFile();

	ImGui::PopFont();
}

void NedEmbed::applySettingsChanges()
{
	if (!initialized)
		return;

	EditorApi *api = activeApi();
	if (!api)
		return;

	const bool settingsApplied = settings.apply(false, *api);
	if (settingsApplied)
	{
		for (auto &tab : tabs)
			tab.editor->api.forceColorUpdate();
	}
	// Font rebuilds apply to the shared atlas; re-sync terminal if needed.
	if (terminal.isStarted() && (settingsApplied || terminal.consumeNeedsFontResync()))
	{
		terminal.reloadTerminalFonts(settings.font.getFontSize());
		settings.font.load(/*clearAtlas=*/false);
	}
}

void NedEmbed::cleanup()
{
	projectUndo.flush();
	terminal.shutdown();
	if (lspClient)
		lspClient->shutdown();
	initialized = false;
}
