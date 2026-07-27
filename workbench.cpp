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
	// forceDock once layout exists (editorDockNodeId_ set in ensureEditorDockLayout).
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
// VS Code–style layout:
//   [ File Explorer (fixed panel) | Editor DockSpace (only editors) ]
// The explorer is NOT inside the dockspace, so nothing can dock left of it /
// around the full window. Editors share one WindowClass and can only dock into
// the editor dockspace (or split with other editors). Permanent floating is
// prevented by re-docking after a drag ends outside a valid target.

namespace {

ImGuiWindowClass makeEditorDockClass()
{
	ImGuiWindowClass c;
	c.ClassId = ImHashStr("ned_editor");
	// Only dock with same ClassId — never with unclassed overlays / other panels.
	c.DockingAllowUnclassed = false;
	// Hide the dock tab-bar window-menu (▶ tabs dropdown). See imgui#4880.
	c.DockNodeFlagsOverrideSet = ImGuiDockNodeFlags_NoWindowMenuButton;
	return c;
}

// True while the user is mid-drag with a window dock payload (temporary undock).
bool isWindowDockDragActive()
{
	const ImGuiPayload *payload = ImGui::GetDragDropPayload();
	return payload != nullptr && payload->IsDataType(IMGUI_PAYLOAD_TYPE_WINDOW);
}

// Depth-first: first leaf that currently hosts windows.
ImGuiDockNode *findLeafWithWindows(ImGuiDockNode *node)
{
	if (!node)
		return nullptr;
	if (node->IsLeafNode())
		return node->Windows.Size > 0 ? node : nullptr;
	if (ImGuiDockNode *n = findLeafWithWindows(node->ChildNodes[0]))
		return n;
	return findLeafWithWindows(node->ChildNodes[1]);
}

// ImGui keeps an empty "central" leaf visible so outer-edge docks can leave a
// hole (left | empty | right). VS Code never does that — editors always fill.
//
// Do NOT DockBuilderRemoveNode here: mid-frame that can hit
// DockNodeMoveWindows(src == dst) and abort. Instead move the CentralNode flag
// onto a filled leaf so the empty hole becomes invisible and loses remaining
// space (see DockNodeUpdateVisibleFlag / DockNodeTreeUpdatePosSize).
void compactEmptyCentralDockLeaves(ImGuiID dockspaceId)
{
	ImGuiDockNode *root = ImGui::DockBuilderGetNode(dockspaceId);
	if (!root)
		return;

	ImGuiDockNode *central = root->CentralNode;
	if (!central || !central->IsLeafNode())
		return;
	// Already filled, or empty root workspace (still the drop target) — leave it.
	if (central->Windows.Size > 0 || central == root || central->ParentNode == nullptr)
		return;

	ImGuiDockNode *fill = root->OnlyNodeWithWindows;
	if (!fill || fill->Windows.Size == 0 || fill == central)
		fill = findLeafWithWindows(root);
	if (!fill || fill == central || fill->Windows.Size == 0)
		return;

	central->SetLocalFlags(central->LocalFlags & ~ImGuiDockNodeFlags_CentralNode);
	fill->SetLocalFlags(fill->LocalFlags | ImGuiDockNodeFlags_CentralNode);
	root->CentralNode = fill;
}

} // namespace

void Workbench::ensureEditorDockLayout(ImGuiID dockspaceId, ImVec2 size)
{
	// DockBuilder asserts size > 0 — never build until we have a real region.
	if (size.x <= 1.0f || size.y <= 1.0f)
		return;
	if (editorDockLayoutBuilt_)
		return;

	editorDockLayoutBuilt_ = true;

	ImGui::DockBuilderRemoveNode(dockspaceId);
	ImGui::DockBuilderAddNode(dockspaceId, ImGuiDockNodeFlags_DockSpace);
	ImGui::DockBuilderSetNodeSize(dockspaceId, size);
	// Entire dockspace is the editor workspace (no sidebar split inside it).
	editorDockNodeId_ = dockspaceId;

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
	const ImVec2 avail = ImGui::GetContentRegionAvail();
	const float minExplorer = 140.0f;
	const float maxExplorer =
		avail.x > minExplorer + 200.0f ? avail.x - 200.0f : minExplorer;

	// ---- Fixed explorer strip (outside dockspace) ----
	// Not a dock node: nothing can dock left of it or wrap around the full window.
	if (settings.sidebarVisible)
	{
		explorerWidth_ = ImClamp(explorerWidth_, minExplorer, maxExplorer);

		// WindowPadding 0: flush to root. ChildRounding 0: ResizeX edge is drawn
		// with perp_padding = WindowRounding — non-zero rounding shortens the
		// separator top/bottom (~style.ChildRounding px), unlike full dock splits.
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
		ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 0.0f);
		ImGui::BeginChild("##ned_explorer_host",
						  ImVec2(explorerWidth_, 0.0f),
						  ImGuiChildFlags_ResizeX,
						  ImGuiWindowFlags_NoScrollbar);
		// Persist width after user drag-resize of the child border.
		explorerWidth_ = ImClamp(ImGui::GetWindowWidth(), minExplorer, maxExplorer);
		fileExplorer->renderFileExplorer(/*fill*/ -1.0f);
		ImGui::EndChild();
		ImGui::PopStyleVar(2);

		ImGui::SameLine(0.0f, 0.0f);
	}

	// ---- Editor-only dockspace in the remaining region ----
	// Capture size *before* DockSpace — after it, ContentRegionAvail is often (0,0)
	// and DockBuilderSetNodeSize asserts.
	ImVec2 dockSize = ImGui::GetContentRegionAvail();
	if (dockSize.x <= 1.0f || dockSize.y <= 1.0f)
		dockSize = ImVec2(
			ImMax(1.0f, avail.x - (settings.sidebarVisible ? explorerWidth_ : 0.0f)),
			ImMax(1.0f, avail.y));

	const ImGuiID dockspaceId = ImGui::GetID("NedEditorDock");
	const ImGuiWindowClass editorClass = makeEditorDockClass();

	ensureEditorDockLayout(dockspaceId, dockSize);
	// Only windows with the editor class can drop into this dockspace. Drop
	// targets are limited to this region (not the full OS/root window).
	ImGui::DockSpace(
		dockspaceId, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_None, &editorClass);

	// ---- Editor tab windows (submitted after DockSpace; peer of host) ----
	std::vector<int> toClose;

	for (int i = 0; i < static_cast<int>(tabs_.size()); ++i)
	{
		Tab &tab = tabs_[static_cast<size_t>(i)];
		if (tab.path.empty() && tabs_.size() > 1)
			continue;

		const std::string title = tabWindowTitle(tab);

		const ImGuiID defaultDockTarget =
			activeEditorDockNodeId_ != 0 ? activeEditorDockNodeId_ : editorDockNodeId_;

		// New tabs: immediately dock into the group that was active on open.
		if (tab.forceDock)
		{
			const ImGuiID target = tab.preferredDockNodeId != 0 ? tab.preferredDockNodeId
																: defaultDockTarget;
			if (target != 0)
				ImGui::SetNextWindowDockID(target, ImGuiCond_Always);
		}
		// Orphan float snap-back: only after several stable undocked frames so we
		// do not override a successful split (drop is applied after Begin on the
		// release frame; Cond_Always the next frame would yank the tab back).
		else if (tab.undockedFrames >= 2 && editorDockNodeId_ != 0)
		{
			ImGui::SetNextWindowDockID(editorDockNodeId_, ImGuiCond_Always);
		}

		if (tab.wantFocus)
		{
			ImGui::SetNextWindowFocus();
			tab.wantFocus = false;
		}

		ImGui::SetNextWindowClass(&editorClass);
		// No content padding under the dock tab bar — custom title bar sits flush.
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

		bool open = true;
		if (ImGui::Begin(title.c_str(), &open))
		{
			const bool docked = ImGui::IsWindowDocked();
			if (docked)
			{
				tab.forceDock = false;
				tab.undockedFrames = 0;
				if (ImGui::IsWindowFocused(ImGuiFocusedFlags_ChildWindows) ||
					ImGui::IsWindowFocused(0))
				{
					if (activeIndex_ != i)
						setActiveIndex(i);
					const ImGuiID dockId = ImGui::GetWindowDockID();
					if (dockId != 0)
						activeEditorDockNodeId_ = dockId;
				}
			} else if (isWindowDockDragActive())
			{
				// Temporary undock while dragging a tab — leave it alone.
				tab.undockedFrames = 0;
			} else
			{
				++tab.undockedFrames;
			}

			tab.editor->renderEditor(font, /*fill*/ -1.0f);
		}
		ImGui::End();
		ImGui::PopStyleVar();

		if (!open)
			toClose.push_back(i);
	}

	for (int i = static_cast<int>(toClose.size()) - 1; i >= 0; --i)
		closeTab(toClose[static_cast<size_t>(i)]);

	// After tabs are submitted (window counts are current), collapse empty
	// central holes so split editors always fill the editor workspace.
	// Skip while a dock drag is active so we don't fight live undocking.
	if (!isWindowDockDragActive())
		compactEmptyCentralDockLeaves(dockspaceId);
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
