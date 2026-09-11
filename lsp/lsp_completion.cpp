#include "lsp_completion.h"
#include "../editor/platform/lsp_editor.h"
#include "../editor/util/utf8.h"
#include "lsp_client.h"
#include "lsp_includes.h"
#include "lsp_trace.h"

#include <iostream>
#include <variant>

namespace {

// Detail strings are server-provided display text, but they can carry
// newlines/runs of whitespace (clangd signatures) — flatten to one clean
// line so the popup's right column renders tidily.
std::string sanitizeDetail(const std::string &raw)
{
	if (raw.empty())
		return {};
	std::string out;
	out.reserve(raw.size());
	bool pendingSpace = false;
	for (char c : raw)
	{
		if (c == '\n' || c == '\r' || c == '\t' || c == ' ')
		{
			pendingSpace = !out.empty();
			continue;
		}
		if (pendingSpace)
		{
			out.push_back(' ');
			pendingSpace = false;
		}
		out.push_back(c);
	}
	return out;
}

// One generated CompletionItem -> the plain view/test-facing shape. v1 is
// plain text only: snippet-format inserts fall back to the label (we
// advertise snippetSupport=false; servers that send snippets anyway must
// not get "$0" garbage typed into the document).
LSPCompletionEntry normalizeItem(const lsp::CompletionItem &item)
{
	LSPCompletionEntry entry;
	entry.label = item.label;
	// Some servers pad labels with whitespace — trim so every row in the
	// popup starts at the same x.
	while (!entry.label.empty() &&
		   (entry.label.front() == ' ' || entry.label.front() == '\t'))
		entry.label.erase(entry.label.begin());
	while (!entry.label.empty() &&
		   (entry.label.back() == ' ' || entry.label.back() == '\t'))
		entry.label.pop_back();
	entry.detail = sanitizeDetail(item.detail.value_or(""));
	entry.kind = item.kind ? static_cast<int>(item.kind->value()) : 0;
	entry.preselect = item.preselect.value_or(false);
	entry.deprecated = item.deprecated.value_or(false);
	if (item.tags)
		for (const auto &tag : *item.tags)
			if (tag == lsp::CompletionItemTag::Deprecated)
				entry.deprecated = true;

	entry.insertText = item.insertText.value_or("");
	const bool snippet =
		item.insertTextFormat && *item.insertTextFormat == lsp::InsertTextFormat::Snippet;
	if (entry.insertText.empty() || snippet)
		entry.insertText = entry.label; // label is plain by definition
	entry.filterText = item.filterText.value_or("");
	entry.sortText = item.sortText.value_or("");
	return entry;
}

} // namespace

LSPCompletion::LSPCompletion(LSPClient &clientIn, LSPEditor &editorApi)
	: client(&clientIn), api(&editorApi)
{
}

LSPCompletion::~LSPCompletion() = default;

bool LSPCompletion::hasTriggerCharacter(char c) const
{
	for (const std::string &t : triggerChars)
		if (t.size() == 1 && t[0] == c)
			return true;
	return false;
}

bool LSPCompletion::requestAtCaret(Trigger trigger, char triggerCharacter)
{
	if (!client || !api || !client->isInitialized() || !client->getMessageHandler())
		return false;

	int row = 0, column = 0;
	api->getCaret(row, column);
	const int utf16 = EditorUtils::Utf8ByteOffsetToUtf16(api->line(row), column);
	const auto ticket = state.begin();
	NED_LSP_TRACE("completion req " << api->path() << " " << row << ":" << column);

	lsp::CompletionParams params;
	params.textDocument.uri = lsp::Uri::fileUriFromPath(api->path());
	params.position.line = static_cast<lsp::uint>(row);
	params.position.character = static_cast<lsp::uint>(utf16);
	params.context = lsp::CompletionContext{};
	switch (trigger)
	{
	case Trigger::TriggerCharacter:
		params.context->triggerKind = lsp::CompletionTriggerKind::TriggerCharacter;
		params.context->triggerCharacter = std::string(1, triggerCharacter);
		break;
	case Trigger::Incomplete:
		params.context->triggerKind =
			lsp::CompletionTriggerKind::TriggerForIncompleteCompletions;
		break;
	case Trigger::Invoked:
		params.context->triggerKind = lsp::CompletionTriggerKind::Invoked;
		break;
	}

	try
	{
		client->getMessageHandler()->sendRequest<lsp::requests::TextDocument_Completion>(
			std::move(params),
			[this, ticket](auto &&result) {
				LSPCompletionResult out;
				if (!result.isNull())
				{
					auto &value = result.value();
					const lsp::Array<lsp::CompletionItem> *items = nullptr;
					if (std::holds_alternative<lsp::CompletionList>(value))
					{
						const auto &list = std::get<lsp::CompletionList>(value);
						out.isIncomplete = list.isIncomplete;
						items = &list.items;
					} else if (std::holds_alternative<lsp::Array<lsp::CompletionItem>>(
								   value))
						items = &std::get<lsp::Array<lsp::CompletionItem>>(value);
					if (items)
					{
						out.items.reserve(items->size());
						for (const auto &item : *items)
							out.items.push_back(normalizeItem(item));
					}
				}
				NED_LSP_TRACE("completion result " << out.items.size() << " items");
				// Raw server labels (first few): run with NED_LSP_TRACE=1 to
				// see exactly what the server sends when a label renders oddly.
				for (size_t i = 0; i < out.items.size() && i < 8; ++i)
					NED_LSP_TRACE("  item " << i << " label='" << out.items[i].label
											<< "' detail='" << out.items[i].detail
											<< "'");
				state.deliver(ticket, std::move(out));
			},
			[this, ticket](const lsp::ResponseError &err) {
				NED_LSP_TRACE("completion error: " << err.message());
				// Errors deliver an EMPTY result, not nullopt: nullopt must
				// always mean "not delivered (or consumed)" so the poller
				// never mistakes a consumed answer for a failed request —
				// an empty list dismisses the popup through the normal path.
				state.deliver(ticket, LSPCompletionResult{});
			});
	} catch (const std::exception &e)
	{
		std::cerr << "[LSP] completion request failed: " << e.what() << std::endl;
		state.deliver(ticket, LSPCompletionResult{});
	}
	return true;
}
