#include "lsp_hover.h"
#include "../editor/platform/lsp_editor.h"
#include "../editor/util/utf8.h"
#include "lsp_client.h"
#include "lsp_includes.h"
#include "lsp_trace.h"

#include <optional>
#include <stdexcept>
#include <variant>

namespace {

std::string fenceWrap(const std::string &lang, const std::string &body)
{
	if (body.empty())
		return {};
	// The renderer highlights fenced blocks; plaintext gets the document's
	// language so it renders as code, which hover plaintext almost always is.
	return "```" + lang + "\n" + body + "\n```";
}

std::string markedStringText(const lsp::MarkedString &ms)
{
	if (std::holds_alternative<lsp::String>(ms))
		return std::get<lsp::String>(ms);
	if (std::holds_alternative<lsp::MarkedStringWithLanguage>(ms))
	{
		const auto &block = std::get<lsp::MarkedStringWithLanguage>(ms);
		return fenceWrap(block.language, block.value);
	}
	return {};
}

std::string formatHoverContents(
	const lsp::OneOf<lsp::MarkupContent, lsp::MarkedString, lsp::Array<lsp::MarkedString>>
		&contents,
	const std::string &fallbackLang)
{
	if (std::holds_alternative<lsp::MarkupContent>(contents))
	{
		const auto &md = std::get<lsp::MarkupContent>(contents);
		if (md.kind == lsp::MarkupKind::PlainText)
			return fenceWrap(fallbackLang, md.value);
		return md.value;
	}
	if (std::holds_alternative<lsp::MarkedString>(contents))
		return markedStringText(std::get<lsp::MarkedString>(contents));
	if (std::holds_alternative<lsp::Array<lsp::MarkedString>>(contents))
	{
		std::string out;
		for (const auto &part : std::get<lsp::Array<lsp::MarkedString>>(contents))
		{
			const std::string piece = markedStringText(part);
			if (piece.empty())
				continue;
			if (!out.empty())
				// MarkedString arrays already describe distinct hover sections.
				// One boundary is enough; two becomes a visible blank paragraph.
				out += '\n';
			out += piece;
		}
		return out;
	}
	return {};
}

} // namespace

LSPHover::LSPHover(LSPClient &client, LSPEditor &api) : client(&client), api(&api) {}

LSPHover::~LSPHover() = default;

void LSPHover::get()
{
	if (!client || !api || !client->isInitialized())
		return;

	int row = 0, column = 0;
	api->getCaret(row, column);
	requestAt(row, column);
}

bool LSPHover::requestAt(int row, int utf8Column, LSPEditor *target)
{
	LSPEditor *const doc = target ? target : api;
	if (!doc || !client || !client->getMessageHandler())
		return false;

	const int utf16 = EditorUtils::Utf8ByteOffsetToUtf16(doc->line(row), utf8Column);
	const auto ticket = state.begin();
	NED_LSP_TRACE("hover req " << doc->path() << " " << row << ":" << utf8Column);

	lsp::HoverParams params;
	params.textDocument.uri = lsp::Uri::fileUriFromPath(doc->path());
	params.position.line = static_cast<lsp::uint>(row);
	params.position.character = static_cast<lsp::uint>(utf16);
	const std::string lang = doc->languageId();

	try
	{
		client->getMessageHandler()->sendRequest<lsp::requests::TextDocument_Hover>(
			std::move(params),
			[this, ticket, lang](auto &&result) {
				std::optional<std::string> text;
				if (!result.isNull())
				{
					text = formatHoverContents(result.value().contents, lang);
					if (text->empty())
						text = std::nullopt;
				}
				NED_LSP_TRACE("hover result "
							  << (text ? std::to_string(text->size()) : "none")
							  << " bytes");
				state.deliver(ticket, std::move(text));
			},
			[this, ticket](const lsp::ResponseError &err) {
				NED_LSP_TRACE("hover error: " << err.message());
				state.deliver(ticket, std::nullopt);
			});
	} catch (const std::exception &e)
	{
		std::cerr << "[LSP] hover request failed: " << e.what() << std::endl;
		state.deliver(ticket, std::nullopt);
	}
	return true;
}
