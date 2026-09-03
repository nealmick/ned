#include "lsp_view.h"

#include "../../../editor/util/doc_path.h"
#include "../../../editor/views/qt/editor_frame.h"
#include "../../../editor/views/qt/ned_key.h"
#include "../../../lsp/lsp_client.h"
#include "../../../lsp/lsp_goto.h"
#include "../../../util/keybinds.h"
#include "lsp_dashboard.h"
#include "lsp_symbol_info.h"
#include "lsp_uri_options.h"

#if NED_ENABLE_LSP

#include <QApplication>
#include <QEvent>
#include <QKeyEvent>
#include <QTimer>

LspView::LspView(LSPClient &clientIn,
				 Settings &settingsIn,
				 QWidget *hostWindowIn,
				 OpenFileFn opener)
	: QObject(hostWindowIn),
	  client(clientIn),
	  settings(settingsIn),
	  hostWindow(hostWindowIn),
	  openFile(std::move(opener))
{
	picker = new LSPUriOptions(hostWindow);
	// Enter/double-click in the picker: jump in place, or open the target
	// document and jump there.
	connect(
		picker, &LSPUriOptions::locationSelected, this, [this](const LSPLocation &loc) {
			jumpTo(loc);
		});
	dashboard_ = new LSPDashboard(
		client,
		[this](const std::string &path) {
			if (openFile)
				openFile(path);
		},
		hostWindow);
	symbolInfo = new LSPSymbolInfo(client, settings, this);

	// Shared picker for both goto requests (ImGui parity). A picker that is
	// already showing another request's results hands ownership over: the
	// previous request's show flag is cleared so only one keeps rendering.
	auto renderThroughPicker = [this](const std::string &title,
									  const std::vector<LSPLocation> &locations,
									  bool &show) {
		if (renderedShowFlag && renderedShowFlag != &show)
			*renderedShowFlag = false;
		renderedShowFlag = &show;
		picker->present(title,
						locations,
						&show,
						client.gotoDef.isPending() || client.gotoRef.isPending());
	};
	client.gotoDef.resultRenderer = renderThroughPicker;
	client.gotoRef.resultRenderer = renderThroughPicker;

	// No frame loop in Qt: a short timer folds async results in (the same
	// role ImGui's per-frame render() call plays).
	pollTimer = new QTimer(this);
	pollTimer->setInterval(30);
	connect(pollTimer, &QTimer::timeout, this, [this] { poll(); });
	pollTimer->start();

	qApp->installEventFilter(this);
}

LspView::~LspView() { qApp->removeEventFilter(this); }

void LspView::editorOpened(EditorFrame &view)
{
	// The editor owns its hover trigger; the LSP layer observes it and
	// targets the hovered document (splits differ from the focused tab).
	// Diagnostic tooltips stay inside the view (it owns the store pointer).
	view.setHoverObserver([this, &view](const HoverTrigger::Info &info) {
		if (info.active)
			symbolInfo->hoverTarget(&view, info);
		else
			symbolInfo->dismiss();
	});
	rebind(&view);
}

void LspView::rebind(EditorFrame *view)
{
	active = view;
	client.bindEditorApi(view);
	symbolInfo->setEditor(view);
}

bool LspView::eventFilter(QObject *watched, QEvent *event)
{
	const QEvent::Type type = event->type();

	// Focus follows the clicked editor (splits, tab switches): rebind before
	// any key can reach it.
	if (type == QEvent::FocusIn)
	{
		if (auto *view = qobject_cast<EditorFrame *>(watched))
			rebind(view);
		return false;
	}

	// Deleted editor (tab closed) — the host also rebinds on editorClosed;
	// this is the safety net so a keybind can never hit a dangling target.
	if (type == QEvent::Destroy)
	{
		if (auto *view = qobject_cast<EditorFrame *>(watched); view && view == active)
			rebind(nullptr);
		return false;
	}

	// Any click or wheel dismisses a visible tooltip. Editor presses used
	// to be exempt (their hover observer dismisses) — but the caret-anchored
	// (keybind) tip bypasses the hover trigger, so a click with a parked
	// mouse never reached it; dismiss is idempotent for the hover path.
	if ((type == QEvent::MouseButtonPress || type == QEvent::Wheel) &&
		symbolInfo->isVisible() && watched != symbolInfo)
		symbolInfo->dismiss();

	// Mouse movement over an editor closes a visible tooltip at once. The
	// caret-anchored (keybind) tip needs this path: it bypasses the hover
	// trigger, so without it only key/click/scroll could retire it and a
	// mouse move would leave it standing ("slow to close").
	if (type == QEvent::MouseMove && symbolInfo->isVisible() &&
		qobject_cast<EditorFrame *>(watched))
		symbolInfo->dismiss();

	if (type != QEvent::KeyPress)
		return false;
	auto *key = static_cast<QKeyEvent *>(event);
	auto *view = qobject_cast<EditorFrame *>(watched);
	if (!view)
		return false;

	// A keybind consumed here never reaches the editor — retire any anchored
	// (caret) hover first, exactly like ImGui's hoverDismissed.
	symbolInfo->dismiss();
	return handleKeybinds(*view, key);
}

bool LspView::handleKeybinds(EditorFrame &view, QKeyEvent *event)
{
	if (!client.isInitialized())
		return false;

	const bool ctrl = event->modifiers() & Qt::ControlModifier;
	const bool meta = event->modifiers() & Qt::MetaModifier;
	if (!ctrl && !meta)
		return false;

	const KeybindsManager &keybinds = client.settingsKeybinds();
	if (event->key() == qtKeyFromNed(keybinds.getActionKey("lsp_symbol_info")))
	{
		symbolInfo->triggerAtCaret();
		return true;
	}
	if (event->key() == qtKeyFromNed(keybinds.getActionKey("lsp_find_def")))
	{
		rebind(&view);
		client.gotoDef.get();
		return true;
	}
	if (event->key() == qtKeyFromNed(keybinds.getActionKey("lsp_find_ref")))
	{
		rebind(&view);
		client.gotoRef.get();
		return true;
	}
	return false;
}

void LspView::jumpTo(const LSPLocation &location)
{
	EditorFrame *target = active;
	if (!target)
		return;
	// clangd canonicalizes paths; the open editor may hold a different
	// spelling of the same file — normalize before deciding "same document"
	// (a mismatch would open a duplicate tab instead of jumping).
	if (DocPath::normalize(location.file) != DocPath::normalize(target->path()) &&
		openFile)
		target = openFile(location.file);
	if (target)
		lspJumpToLocation(*target, location);
}

void LspView::poll()
{
	client.gotoDef.render();
	client.gotoRef.render();
	symbolInfo->poll();
}

#endif // NED_ENABLE_LSP
