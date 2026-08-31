#include "lsp_goto.h"
#include "../editor/editor_api.h"
#include "../editor/util/utf8.h"
#include "lsp_includes.h"
#include "lsp_trace.h"
#include "lsp_uri_options.h"

LSPGoto::LSPGoto(LSPClient &client, EditorApi &api, Kind kind)
	: kind(kind), client(&client), api(&api)
{
}

LSPGoto::~LSPGoto() = default;

namespace {

const char *gotoTitle(LSPGoto::Kind kind)
{
	return kind == LSPGoto::Kind::Definition ? "Goto Definition" : "Goto Reference";
}

// Definition and References differ only in params/result shape — one plumbing
// path for both.
template <typename Request, typename Params, typename Convert>
void sendGoto(lsp::MessageHandler &handler,
			  Params params,
			  Convert convert,
			  const char *what,
			  LSPRequestState<std::vector<LSPLocation>>::Ticket ticket,
			  LSPRequestState<std::vector<LSPLocation>> &state)
{
	handler.sendRequest<Request>(
		std::move(params),
		[ticket, &state, convert, what](auto &&result) {
			const auto locations = convert(result);
			NED_LSP_TRACE(what << " results=" << locations.size());
			state.deliver(ticket, locations);
		},
		[ticket, &state, what](const lsp::ResponseError &err) {
			NED_LSP_TRACE(what << " error: " << err.message());
			state.deliver(ticket, {});
		});
}

} // namespace

void LSPGoto::get()
{
	if (!client || !api || !client->isInitialized() || !client->getMessageHandler())
		return;

	int row = 0, column = 0;
	api->getCaret(row, column);
	const int utf16 = EditorUtils::Utf8ByteOffsetToUtf16(api->line(row), column);

	show = true;
	const auto ticket = state.begin();
	lsp::MessageHandler &handler = *client->getMessageHandler();
	NED_LSP_TRACE((kind == Kind::Definition ? "definition req " : "references req ")
				  << api->path());

	try
	{
		if (kind == Kind::Definition)
		{
			lsp::DefinitionParams params;
			params.textDocument.uri = lsp::Uri::fileUriFromPath(api->path());
			params.position.line = static_cast<lsp::uint>(row);
			params.position.character = static_cast<lsp::uint>(utf16);
			sendGoto<lsp::requests::TextDocument_Definition>(
				handler,
				std::move(params),
				[](auto &&result) { return lsp_locations::fromDefinitionResult(result); },
				"definition",
				ticket,
				state);
		} else
		{
			lsp::ReferenceParams params;
			params.textDocument.uri = lsp::Uri::fileUriFromPath(api->path());
			params.position.line = static_cast<lsp::uint>(row);
			params.position.character = static_cast<lsp::uint>(utf16);
			params.context.includeDeclaration = false; // Just references
			sendGoto<lsp::requests::TextDocument_References>(
				handler,
				std::move(params),
				[](auto &&result) { return lsp_locations::fromReferencesResult(result); },
				"references",
				ticket,
				state);
		}
	} catch (const std::exception &e)
	{
		std::cerr << "LSP: goto request failed: " << e.what() << std::endl;
		state.deliver(ticket, {});
	}
}

void LSPGoto::render()
{
	if (!client || !show)
		return;
	client->uriOptions.render(
		gotoTitle(kind), state.snapshot().value_or(std::vector<LSPLocation>{}), show);
}
