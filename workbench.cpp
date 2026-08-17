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
#include "util/macos_window.h"
#ifdef _WIN32
#include "util/windows_window.h"
#endif

#include "imgui_internal.h"

#include <algorithm>
#include <filesystem>
#include <iostream>

namespace fs = std::filesystem;

#ifdef __APPLE__
namespace {
Workbench *gTitlebarWorkbench = nullptr;
}
#endif

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

#ifdef __APPLE__
	if (mode_ == WorkbenchHostMode::Fullscreen)
	{
		gTitlebarWorkbench = this;
		setMacOSTitlebarActions(
			[]() {
				if (gTitlebarWorkbench)
					gTitlebarWorkbench->settings.toggleSidebar();
			},
			[]() {
				if (gTitlebarWorkbench)
					gTitlebarWorkbench->settings.toggleTerminal();
			},
			[]() {
				if (!gTitlebarWorkbench)
					return;
				if (EditorApi *api = gTitlebarWorkbench->activeApi())
					gTitlebarWorkbench->settings.toggleSettingsWindow(*api);
			});
	}
#endif
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
//   [ File Explorer (fixed, full height) | Editor DockSpace          ]
//                                       | Terminal (fixed bottom)   ]
// Explorer + terminal are not dock nodes. Editors share one WindowClass and
// only dock into the editor dockspace. Permanent floating is prevented by
// re-docking after a drag ends outside a valid target.

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
	const float fs = ImGui::GetFontSize();
	// Designed at a 20px font: explorer 260, terminal 220, mins 140/80/100.
	if (explorerWidth_ <= 0.0f)
		explorerWidth_ = fs * 13.0f;
	if (terminalHeight_ <= 0.0f)
		terminalHeight_ = fs * 11.0f;
	const float minExplorer = fs * 7.0f;
	const float minEditorW = fs * 10.0f;
	const float maxExplorer =
		avail.x > minExplorer + minEditorW ? avail.x - minEditorW : minExplorer;
	const float minTerminal = fs * 4.0f;
	const float minEditor = fs * 5.0f;
	// Shared splitter visuals: wide hit target, 1px hairline (ResizeX/Y child
	// borders get covered by the next sibling and lose hover highlight).
	const float kSplitHit = std::max(5.0f, fs * 0.25f);
	constexpr float kSplitLine = 1.0f;

	// ---- Fixed explorer strip (full height, outside dockspace) ----
	if (settings.sidebarVisible)
	{
		explorerWidth_ = ImClamp(explorerWidth_, minExplorer, maxExplorer);

		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
		ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 0.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 0.0f));
		ImGui::BeginChild("##ned_explorer_host",
						  ImVec2(explorerWidth_, 0.0f),
						  ImGuiChildFlags_None,
						  ImGuiWindowFlags_NoScrollbar);
		fileExplorer->renderFileExplorer(/*fill*/ -1.0f);
		ImGui::EndChild();

		// Explicit vertical splitter (##ned_main_column used to sit on top of
		// ImGuiChildFlags_ResizeX and steal hover / SeparatorHovered feedback).
		ImGui::SameLine(0.0f, 0.0f);
		ImGui::InvisibleButton("##ned_explorer_splitter", ImVec2(kSplitHit, -1.0f));
		const bool expHover = ImGui::IsItemHovered();
		const bool expActive = ImGui::IsItemActive();
		if (expActive)
		{
			explorerWidth_ = ImClamp(
				explorerWidth_ + ImGui::GetIO().MouseDelta.x, minExplorer, maxExplorer);
		}
		{
			ImDrawList *dl = ImGui::GetWindowDrawList();
			const ImVec2 a = ImGui::GetItemRectMin();
			const ImVec2 b = ImGui::GetItemRectMax();
			const float x = IM_TRUNC((a.x + b.x) * 0.5f);
			const ImU32 col = ImGui::GetColorU32(expActive	? ImGuiCol_SeparatorActive
												 : expHover ? ImGuiCol_SeparatorHovered
															: ImGuiCol_Border);
			dl->AddRectFilled(ImVec2(x, a.y), ImVec2(x + kSplitLine, b.y), col);
		}
		if (expHover || expActive)
			ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);

		ImGui::SameLine(0.0f, 0.0f);
		ImGui::PopStyleVar(3);
	}

	// ---- Main column: editor dock (top) + optional terminal (bottom) ----
	// Not a dock target — terminal is a fixed split, not a dockable window.
	// ItemSpacing 0: no gap between editor/terminal (otherwise height math drifts).
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
	ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 0.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 0.0f));
	ImGui::BeginChild("##ned_main_column",
					  ImVec2(0.0f, 0.0f),
					  ImGuiChildFlags_None,
					  ImGuiWindowFlags_NoScrollbar);

	// Don't steal focus while Settings is open — blur would dismiss it.
	terminal.setVisible(settings.terminalVisible, !settings.showSettingsWindow);
	const bool termOn = terminal.visible();
	const float colH = ImGui::GetContentRegionAvail().y;
	const float kTermSplitterH = kSplitHit;
	constexpr float kTermBorderThickness = kSplitLine;

	if (termOn && colH > minEditor + minTerminal + kTermSplitterH)
	{
		terminalHeight_ =
			ImClamp(terminalHeight_, minTerminal, colH - minEditor - kTermSplitterH);
	}

	const float editorHostH = termOn ? (colH - terminalHeight_ - kTermSplitterH) : 0.0f;

	ImGui::BeginChild("##ned_editor_host",
					  ImVec2(0.0f, editorHostH),
					  ImGuiChildFlags_None,
					  ImGuiWindowFlags_NoScrollbar);

	// Capture size *before* DockSpace — after it, ContentRegionAvail is often (0,0)
	// and DockBuilderSetNodeSize asserts.
	ImVec2 dockSize = ImGui::GetContentRegionAvail();
	if (dockSize.x <= 1.0f || dockSize.y <= 1.0f)
		dockSize = ImVec2(ImMax(1.0f, ImGui::GetWindowSize().x),
						  ImMax(1.0f, ImGui::GetWindowSize().y));

	const ImGuiID dockspaceId = ImGui::GetID("NedEditorDock");
	const ImGuiWindowClass editorClass = makeEditorDockClass();

	ensureEditorDockLayout(dockspaceId, dockSize);
	// Keep dock node size in sync when the bottom terminal split changes.
	if (ImGuiDockNode *root = ImGui::DockBuilderGetNode(dockspaceId))
	{
		if (root->Size.x != dockSize.x || root->Size.y != dockSize.y)
			ImGui::DockBuilderSetNodeSize(dockspaceId, dockSize);
	}
	// Only windows with the editor class can drop into this dockspace.
	ImGui::DockSpace(
		dockspaceId, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_None, &editorClass);

	ImGui::EndChild(); // ##ned_editor_host

	if (termOn)
	{
		// Drag up → taller terminal. Terminal child uses size.y = 0 so it always
		// fills remaining space to the bottom of the column.
		ImGui::InvisibleButton("##ned_term_splitter", ImVec2(-1.0f, kTermSplitterH));
		const bool splitHover = ImGui::IsItemHovered();
		const bool splitActive = ImGui::IsItemActive();
		if (splitActive && colH > minEditor + minTerminal + kTermSplitterH)
		{
			terminalHeight_ = ImClamp(terminalHeight_ - ImGui::GetIO().MouseDelta.y,
									  minTerminal,
									  colH - minEditor - kTermSplitterH);
		}
		// 1px hairline centered in the hit strip (not a thick filled bar).
		{
			ImDrawList *dl = ImGui::GetWindowDrawList();
			const ImVec2 a = ImGui::GetItemRectMin();
			const ImVec2 b = ImGui::GetItemRectMax();
			const float y = IM_TRUNC((a.y + b.y) * 0.5f);
			const ImU32 col = ImGui::GetColorU32(splitActive  ? ImGuiCol_SeparatorActive
												 : splitHover ? ImGuiCol_SeparatorHovered
															  : ImGuiCol_Border);
			dl->AddRectFilled(ImVec2(a.x, y), ImVec2(b.x, y + kTermBorderThickness), col);
		}
		if (splitHover || splitActive)
			ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeNS);

		ImGui::BeginChild("##ned_terminal_host",
						  ImVec2(0.0f, 0.0f),
						  ImGuiChildFlags_None,
						  ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoDocking);
		terminal.renderPanel();
		ImGui::EndChild();
	}

	ImGui::EndChild(); // ##ned_main_column
	ImGui::PopStyleVar(3);

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

#ifdef _WIN32
void Workbench::drawWindowsTitlebar()
{
	const float fs = ImGui::GetFontSize();
	const float h = std::max(fs * 1.7f, 28.0f);
	windowsSetTitlebarHeight(h);
	windowsClearCaptionExcludes();

	ImGuiViewport *vp = ImGui::GetMainViewport();
	ImGui::SetNextWindowPos(vp->Pos);
	ImGui::SetNextWindowSize(ImVec2(vp->Size.x, h));
	ImGui::SetNextWindowViewport(vp->ID);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 0.0f));
	ImGui::Begin("##ned_win_titlebar",
				 nullptr,
				 ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
					 ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings |
					 ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse |
					 ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoNav |
					 ImGuiWindowFlags_NoBringToFrontOnFocus);

	ImDrawList *dl = ImGui::GetWindowDrawList();
	const ImU32 ink = ImGui::GetColorU32(ImGuiCol_Text);
	const float stroke = std::max(1.0f, fs * 0.07f);
	const float btnW = std::max(fs * 2.15f, 36.0f);
	const float actW = std::max(fs * 1.7f, 28.0f);

	// Title left, then thin split glyphs. Cog stays by the caption buttons.
	const char *title = "Ned Text Editor";
	const ImVec2 ts = ImGui::CalcTextSize(title);
	const float titleX = fs * 0.7f;
	ImGui::SetCursorPos(ImVec2(titleX, (h - ts.y) * 0.5f));
	ImGui::TextUnformatted(title);

	auto actionBtn = [&](const char *id, const char *tip, auto glyph) -> bool {
		const bool hit = ImGui::InvisibleButton(id, ImVec2(actW, h));
		const ImVec2 a = ImGui::GetItemRectMin();
		const ImVec2 b = ImGui::GetItemRectMax();
		windowsExcludeCaptionRect(a, b);
		const bool hov = ImGui::IsItemHovered();
		if (hov)
		{
			ImGui::SetTooltip("%s", tip);
			dl->AddRectFilled(a, b, IM_COL32(255, 255, 255, 28));
		}
		glyph(a, b, ink);
		return hit;
	};

	const float glyphPad = fs * 0.52f;
	auto glyphBox = [&](ImVec2 a, ImVec2 b) {
		const float side = std::min(b.x - a.x, b.y - a.y) - glyphPad * 2.0f;
		const float cx = (a.x + b.x) * 0.5f;
		const float cy = (a.y + b.y) * 0.5f;
		return std::pair<ImVec2, ImVec2>{ImVec2(cx - side * 0.5f, cy - side * 0.5f),
										 ImVec2(cx + side * 0.5f, cy + side * 0.5f)};
	};
	auto drawSidebar = [&](ImVec2 a, ImVec2 b, ImU32 col) {
		const auto [p0, p1] = glyphBox(a, b);
		const float r = std::max(1.0f, fs * 0.06f);
		const float split = p0.x + (p1.x - p0.x) * 0.38f;
		dl->AddRect(p0, p1, col, r, 0, stroke);
		dl->AddLine(ImVec2(split, p0.y + 0.5f), ImVec2(split, p1.y - 0.5f), col, stroke);
	};
	auto drawBottomSplit = [&](ImVec2 a, ImVec2 b, ImU32 col) {
		const auto [p0, p1] = glyphBox(a, b);
		const float r = std::max(1.0f, fs * 0.06f);
		const float split = p0.y + (p1.y - p0.y) * 0.58f;
		dl->AddRect(p0, p1, col, r, 0, stroke);
		dl->AddLine(ImVec2(p0.x + 0.5f, split), ImVec2(p1.x - 0.5f, split), col, stroke);
	};

	ImGui::SetCursorPos(ImVec2(titleX + ts.x + fs * 0.85f, 0.0f));
	if (actionBtn("##tb_sidebar", "Toggle Explorer", drawSidebar))
		settings.toggleSidebar();
	ImGui::SameLine(0.0f, 0.0f);
	if (actionBtn("##tb_term", "Toggle Terminal", drawBottomSplit))
		settings.toggleTerminal();

	float capX = ImGui::GetWindowWidth() - btnW * 3.0f;
	ImGui::SetCursorPos(ImVec2(capX - actW, 0.0f));
	if (actionBtn("##tb_set", "Settings", [&](ImVec2 a, ImVec2 b, ImU32) {
			const float pad = fs * 0.38f;
			ImTextureID gear = icons.get("gear");
			if (ImGui::IsItemHovered())
			{
				ImTextureID hover = icons.get("gear-hover");
				if (hover)
					gear = hover;
			}
			if (gear)
				dl->AddImage(
					gear, ImVec2(a.x + pad, a.y + pad), ImVec2(b.x - pad, b.y - pad));
		}))
	{
		if (EditorApi *api = activeApi())
			settings.toggleSettingsWindow(*api);
	}

	auto capBtn =
		[&](const char *id, bool isClose, WindowsCaptionHit part, auto glyph) -> bool {
		ImGui::SetCursorPos(ImVec2(capX, 0.0f));
		const bool hit = ImGui::InvisibleButton(id, ImVec2(btnW, h));
		const ImVec2 a = ImGui::GetItemRectMin();
		const ImVec2 b = ImGui::GetItemRectMax();
		windowsExcludeCaptionRect(a, b, part);
		const bool hov = ImGui::IsItemHovered() || windowsCaptionHover() == part;
		if (hov)
		{
			dl->AddRectFilled(
				a, b, isClose ? IM_COL32(232, 17, 35, 255) : IM_COL32(255, 255, 255, 28));
		}
		glyph(a, b, hov && isClose ? IM_COL32(255, 255, 255, 255) : ink);
		capX += btnW;
		return hit;
	};

	if (capBtn(
			"##tb_min", false, WindowsCaptionHit::Min, [&](ImVec2 a, ImVec2 b, ImU32 col) {
				const float cx = (a.x + b.x) * 0.5f;
				const float cy = (a.y + b.y) * 0.5f;
				const float w = fs * 0.45f;
				dl->AddLine(ImVec2(cx - w, cy), ImVec2(cx + w, cy), col, stroke);
			}))
		windowsMinimize();

	if (capBtn(
			"##tb_max", false, WindowsCaptionHit::Max, [&](ImVec2 a, ImVec2 b, ImU32 col) {
				const float cx = (a.x + b.x) * 0.5f;
				const float cy = (a.y + b.y) * 0.5f;
				const float s = fs * 0.42f;
				if (windowsIsMaximized())
				{
					dl->AddRect(ImVec2(cx - s + 2.0f, cy - s),
								ImVec2(cx + s, cy + s - 2.0f),
								col,
								0.0f,
								0,
								stroke);
					dl->AddRect(ImVec2(cx - s, cy - s + 3.0f),
								ImVec2(cx + s - 2.0f, cy + s),
								col,
								0.0f,
								0,
								stroke);
				} else
				{
					dl->AddRect(ImVec2(cx - s, cy - s),
								ImVec2(cx + s, cy + s),
								col,
								0.0f,
								0,
								stroke);
				}
			}))
		windowsToggleMaximize();

	if (capBtn(
			"##tb_close",
			true,
			WindowsCaptionHit::Close,
			[&](ImVec2 a, ImVec2 b, ImU32 col) {
				const float cx = (a.x + b.x) * 0.5f;
				const float cy = (a.y + b.y) * 0.5f;
				const float s = fs * 0.38f;
				dl->AddLine(ImVec2(cx - s, cy - s), ImVec2(cx + s, cy + s), col, stroke);
				dl->AddLine(ImVec2(cx + s, cy - s), ImVec2(cx - s, cy + s), col, stroke);
			}))
		windowsClose();

	// Hairline under the bar.
	{
		const ImVec2 wp = ImGui::GetWindowPos();
		const float y1 = wp.y + h - 1.0f;
		dl->AddLine(ImVec2(wp.x, y1),
					ImVec2(wp.x + ImGui::GetWindowWidth(), y1),
					ImGui::GetColorU32(ImGuiCol_Border),
					1.0f);
	}

	ImGui::End();
	ImGui::PopStyleVar(4);
}
#endif

bool Workbench::beginRootChrome()
{
	// Sampled at Begin; 0 so the explorer strip can sit on the window edge.
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);

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

	// Fullscreen: fill the OS window below the native title bar.
	const float top = macOSTitlebarInset();
	ImGui::SetNextWindowPos(ImVec2(0.0f, top));
	ImGui::SetNextWindowSize(
		ImVec2(ImGui::GetIO().DisplaySize.x, ImGui::GetIO().DisplaySize.y - top));
	return ImGui::Begin(
		"##ned_root",
		nullptr,
		ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
			ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollWithMouse |
			ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoDocking |
			ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus);
}

void Workbench::endRootChrome()
{
	ImGui::End();
	ImGui::PopStyleVar(2);
}

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

	// Terminal bakes glyphs at the px we pass (AddText does not apply
	// FontScaleDpi). Atlas rebuilds only happen here, before NewFrame.
	// Windows: 85% of editor size — cells include ascent+descent and read large.
	const float dpi = ImGui::GetStyle().FontScaleDpi;
#ifdef _WIN32
	const float termScale = 0.85f;
#else
	const float termScale = 1.0f;
#endif
	const float termPx =
		settings.font.getFontSize() * (dpi > 0.0f ? dpi : 1.0f) * termScale;
	const bool resync = terminal.consumeNeedsFontResync();
	const bool sizeChanged = ImAbs(terminal.configuredFontPx() - termPx) > 0.05f;
	if (!terminal.isStarted())
	{
		if (sizeChanged)
			terminal.reloadTerminalFonts(termPx);
	} else if (settingsApplied || resync || sizeChanged)
	{
		terminal.reloadTerminalFonts(termPx);
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
		settings.keybinds.handleKeyboardShortcuts(*api, *fileExplorer, *lspClient);
	}

	if (fileExplorer->handleFileDialog())
		showWelcome = false;

	terminal.setProjectRoot(projectRoot.empty() ? fileExplorer->projectRoot : projectRoot);

	if (fileExplorer->fileFinder.showFFWindow)
		fileExplorer->setEditorsBlockInput(true);

	ImGui::PushFont(settings.font.getMainFont());

#ifdef _WIN32
	if (mode_ == WorkbenchHostMode::Fullscreen)
		drawWindowsTitlebar();
#endif

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
#ifdef __APPLE__
	gTitlebarWorkbench = nullptr;
	setMacOSTitlebarActions(nullptr, nullptr, nullptr);
#endif
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
