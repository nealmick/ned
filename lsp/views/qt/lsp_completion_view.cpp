#include "lsp_completion_view.h"

#include "../../../editor/services/highlight/capture_map.h"
#include "../../../editor/views/qt/editor_frame.h"
#include "../../../editor/views/qt/ned_color.h"
#include "../../../host/qt/theme.h"
#include "../../../lsp/lsp_client.h"
#include "../../../lsp/lsp_completion.h"
#include "../../../util/settings.h"
#include "lsp_completion_popup.h"

#include <QKeyEvent>
#include <QTimer>

#include <algorithm>

namespace {

// Typing debounce: enough to keep fast typers off the request path, short
// enough that the popup feels immediate.
constexpr int kTriggerDelayMs = 180;
// Minimum typed prefix before an automatic (non-trigger-character) request.
constexpr std::size_t kMinPrefixBytes = 2;

} // namespace

LSPCompletionView::LSPCompletionView(LSPClient &clientIn,
									 Settings &settingsIn,
									 QObject *parent)
	: QObject(parent), client(clientIn), settings(settingsIn)
{
	debounce = new QTimer(this);
	debounce->setSingleShot(true);
	debounce->setInterval(kTriggerDelayMs);
	connect(debounce, &QTimer::timeout, this, [this] { evaluate(); });
}

LSPCompletionView::~LSPCompletionView() = default;

void LSPCompletionView::editorOpened(EditorFrame &opened)
{
	opened.editorEvents().subscribeDidEdit(
		[this, edited = QPointer<EditorFrame>(&opened)](const EditorEvents::DidEdit &) {
			// Only the focused editor's typing drives the session (the
			// client's bound editor is retargeted by LSPView::rebind).
			if (edited && edited == view)
				scheduleTrigger();
		});
}

void LSPCompletionView::setEditor(EditorFrame *next)
{
	view = next;
	dismiss();
}

void LSPCompletionView::triggerAtCaret()
{
	if (!view || !client.isInitialized())
		return;
	int row = 0;
	const int col = caretColumn(row);
	caretRow = row;
	wordStartCol = lspCompletionWordStart(view->line(row), col);
	session = true;
	if (!client.completion.requestAtCaret(LSPCompletion::Trigger::Invoked))
		dismiss();
}

void LSPCompletionView::scheduleTrigger() { debounce->start(); }

void LSPCompletionView::evaluate()
{
	if (!view || !client.isInitialized())
	{
		dismiss();
		return;
	}

	int row = 0;
	const int col = caretColumn(row);
	const std::string line = view->line(row);
	const int start = lspCompletionWordStart(line, col);
	const std::string word =
		line.substr(static_cast<size_t>(start), static_cast<size_t>(col - start));

	// The character just typed (caret sits right after it).
	const char typed = col > 0 ? line[static_cast<size_t>(col - 1)] : '\0';

	const bool sameWord = session && row == caretRow && start == wordStartCol;
	caretRow = row;
	wordStartCol = start;
	prefix = word;

	if (typed && client.completion.hasTriggerCharacter(typed))
	{
		// `.`, `::`-style separators: fresh request anchored at the (empty
		// or partial) word after the separator.
		session = true;
		if (!client.completion.requestAtCaret(LSPCompletion::Trigger::TriggerCharacter,
											  typed))
			dismiss();
		return;
	}

	if (word.size() >= kMinPrefixBytes)
	{
		// Extending/trimming the same word: re-filter locally — the server
		// already answered for this position — unless it flagged the list
		// incomplete (then re-request per spec).
		if (sameWord && !itemsIncomplete && !items.empty())
		{
			refreshPopup();
			return;
		}
		session = true;
		if (!client.completion.requestAtCaret(itemsIncomplete
												  ? LSPCompletion::Trigger::Incomplete
												  : LSPCompletion::Trigger::Invoked))
			dismiss();
		return;
	}

	if (session)
	{
		if (word.empty())
			dismiss();
		else if (!itemsIncomplete && !items.empty())
			refreshPopup(); // shrank to 1 char: still filterable
	}
}

void LSPCompletionView::refreshPopup()
{
	if (!view)
	{
		dismiss();
		return;
	}

	// The caret must still sit in the session's word — a jump or a word
	// change retires the popup (a fresh evaluate() will re-request).
	int row = 0;
	const int col = caretColumn(row);
	if (row != caretRow)
	{
		dismiss();
		return;
	}
	const std::string line = view->line(row);
	if (lspCompletionWordStart(line, col) != wordStartCol)
	{
		dismiss();
		return;
	}
	prefix = line.substr(static_cast<size_t>(wordStartCol),
						 static_cast<size_t>(col - wordStartCol));

	filtered = lspFilterCompletionItems(items, prefix);
	if (filtered.empty())
	{
		dismiss();
		return;
	}

	if (!popup)
	{
		popup = new LSPCompletionPopup(); // no parent: top-level tooltip window
		connect(popup, &LSPCompletionPopup::itemAccepted, this, [this](int index) {
			accept(index);
		});
		// Any hide (outside click, app switch) ends the session.
		connect(popup, &LSPCompletionPopup::dismissed, this, [this] { dismiss(); });
	}

	std::vector<LSPCompletionEntry> shown;
	shown.reserve(filtered.size());
	for (int idx : filtered)
		shown.push_back(items[static_cast<size_t>(idx)]);

	const QPoint anchor = view->mapToGlobal(view->caretWidgetPos() +
											QPoint(0, view->caretLineHeight() + 2));
	// Kind badge palette: the editor's syntax theme colors, so F/V/C/...
	// badges match what those symbols look like in the document.
	LSPCompletionKindColors kindColors;
	kindColors.function = toQColor(view->syntaxColor(ThemeSlot::Function));
	kindColors.type = toQColor(view->syntaxColor(ThemeSlot::Type));
	kindColors.variable = toQColor(view->syntaxColor(ThemeSlot::Variable));
	kindColors.keyword = toQColor(view->syntaxColor(ThemeSlot::Keyword));
	kindColors.number = toQColor(view->syntaxColor(ThemeSlot::Number));
	kindColors.special = toQColor(view->syntaxColor(ThemeSlot::Special));
	kindColors.op = toQColor(view->syntaxColor(ThemeSlot::Operator));

	popup->present(shown,
				   0,
				   view->font(),
				   NedQtTheme::raised(NedQtTheme::background(settings)),
				   toQColor(view->defaultTextColor()),
				   toQColor(view->syntaxColor(ThemeSlot::Keyword)),
				   kindColors,
				   anchor);
}

void LSPCompletionView::accept(int filteredIndex)
{
	EditorFrame *const target = view;
	if (!target || filteredIndex < 0 || filteredIndex >= static_cast<int>(filtered.size()))
	{
		dismiss();
		return;
	}
	const LSPCompletionEntry &entry =
		items[static_cast<size_t>(filtered[static_cast<size_t>(filteredIndex)])];
	int row = 0;
	const int col = caretColumn(row);
	// Replace the typed word with the insert text via the frame's edit API
	// (one batched, undoable edit + afterEdit fan-out).
	target->applyCompletion(row, wordStartCol, col, entry.insertText);
	dismiss();
}

bool LSPCompletionView::handleKey(QKeyEvent *event)
{
	if (!session || !popup || !popup->isVisible())
		return false;

	// Modifier combos (Ctrl/Cmd+…) belong to the keybind layer — Ctrl+L
	// must be able to re-trigger while the popup is open.
	if (event->modifiers() & (Qt::ControlModifier | Qt::MetaModifier | Qt::AltModifier))
		return false;

	switch (event->key())
	{
	case Qt::Key_Escape:
		dismiss();
		return true;
	case Qt::Key_Up:
		popup->moveSelection(-1);
		return true;
	case Qt::Key_Down:
		popup->moveSelection(1);
		return true;
	case Qt::Key_PageUp:
		popup->pageSelection(-1);
		return true;
	case Qt::Key_PageDown:
		popup->pageSelection(1);
		return true;
	case Qt::Key_Return:
	case Qt::Key_Enter:
	case Qt::Key_Tab:
		accept(popup->selection());
		return true;
	default:
		return false; // typing flows to the editor; DidEdit re-evaluates
	}
}

void LSPCompletionView::dismiss()
{
	session = false;
	caretRow = 0;
	wordStartCol = 0;
	prefix.clear();
	items.clear();
	filtered.clear();
	itemsIncomplete = false;
	debounce->stop();
	client.completion.cancel();
	if (popup && popup->isVisible())
		popup->hide();
}

void LSPCompletionView::poll()
{
	if (!session || !view || client.completion.isPending())
		return;

	const auto snap = client.completion.snapshot();
	if (!snap)
		return; // not delivered yet, or already consumed by a previous poll
	// Consume the snapshot: poll runs every 30ms and must not re-present
	// (and reset the popup's selection) on ticks after the delivery.
	items = snap->items;
	itemsIncomplete = snap->isIncomplete;
	client.completion.cancel();
	refreshPopup(); // empty answer -> filtered empty -> dismiss
}

int LSPCompletionView::caretColumn(int &row) const
{
	row = 0;
	int col = 0;
	if (view)
		view->getCaret(row, col);
	return col;
}
