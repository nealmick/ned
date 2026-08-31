#include "lsp_symbol_info.h"
#include "../editor/editor_api.h"
#include "../editor/util/utf8.h"
#include "../editor/views/hover_tooltip.h"
#include "lsp_includes.h"
#include "lsp_trace.h"

#include <algorithm>
#include <variant>

namespace {

// Mouse slack around the popup that still counts as "over the tooltip".
constexpr float kPopupStickyPadding = 12.0f;

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

LSPSymbolInfo::LSPSymbolInfo(LSPClient &client, EditorApi &api)
	: client(&client), api(&api)
{
}

LSPSymbolInfo::~LSPSymbolInfo() = default;

void LSPSymbolInfo::get()
{
	if (!client || !api || !client->isInitialized())
		return;

	int row = 0, column = 0;
	api->getCaret(row, column);
	atCaret = true;
	requestAt(row, column);
}

void LSPSymbolInfo::hideMouseHover()
{
	if (atCaret)
		return;
	requestedForCell = false;
	hoverRow = -1;
	hoverCol = -1;
	popupRectValid = false;
	hoverState.cancel();
}

void LSPSymbolInfo::requestAt(int row, int utf8Column)
{
	if (!api || !client || !client->getMessageHandler())
		return;
	requestedForCell = true;

	const int utf16 = EditorUtils::Utf8ByteOffsetToUtf16(api->line(row), utf8Column);
	const auto ticket = hoverState.begin();
	NED_LSP_TRACE("hover req " << api->path() << " " << row << ":" << utf8Column);

	lsp::HoverParams params;
	params.textDocument.uri = lsp::Uri::fileUriFromPath(api->path());
	params.position.line = static_cast<lsp::uint>(row);
	params.position.character = static_cast<lsp::uint>(utf16);
	const std::string lang = api->languageId();

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
				hoverState.deliver(ticket, std::move(text));
			},
			[this, ticket](const lsp::ResponseError &err) {
				NED_LSP_TRACE("hover error: " << err.message());
				hoverState.deliver(ticket, std::nullopt);
			});
	} catch (const std::exception &e)
	{
		std::cerr << "LSP: hover request failed: " << e.what() << std::endl;
		hoverState.deliver(ticket, std::nullopt);
	}
}

void LSPSymbolInfo::updateMouseHover()
{
	if (!client || !api || !client->isInitialized())
		return;

	// Keybind hover: anchored at the caret, immune to mouse logic; any
	// dismissal signal (key/click/scroll) retires it.
	if (atCaret)
	{
		if (api->hoverDismissed())
		{
			atCaret = false;
			hoverState.cancel();
		}
		return;
	}

	EditorApi *const hover = hoverApi ? hoverApi : api;

	// Sticky popup: moving onto/inside the rendered tooltip keeps it (VSCode
	// sticky-hover) — but a dismissal signal (key/click/scroll) still wins.
	// This is purely local: the frame's trigger is never mutated, so it can
	// not get wedged into a stale "active" state.
	const ImVec2 mouse = ImGui::GetMousePos();
	const bool overPopup = popupRectValid && ImGui::IsMousePosValid(&mouse) &&
						   mouse.x >= popupMin.x - kPopupStickyPadding &&
						   mouse.x <= popupMax.x + kPopupStickyPadding &&
						   mouse.y >= popupMin.y - kPopupStickyPadding &&
						   mouse.y <= popupMax.y + kPopupStickyPadding;
	if (hover->hoverDismissed())
	{
		hideMouseHover();
		return;
	}
	if (overPopup && (hoverState.isPending() || hoverState.snapshot()))
		return;
	popupRectValid = false;

	// The frame's trigger owns all VSCode-style logic: armed by real mouse
	// moves only, dismissed by keys/clicks/scroll/shifted content, target
	// frozen while showing.
	const HoverTrigger::Info info = hover->hoverInfo();
	if (!info.active || info.zone != HoverTrigger::Zone::Text || hover->path().empty() ||
		!client->isDocumentOpen(hover->path()))
	{
		hideMouseHover();
		return;
	}

	if (info.row != hoverRow || info.column != hoverCol)
	{
		hoverRow = info.row;
		hoverCol = info.column;
		requestedForCell = false;
		hoverState.cancel();
	}

	if (!requestedForCell)
		requestAt(info.row, info.column);
}

void LSPSymbolInfo::render()
{
	updateMouseHover();

	// Visibility IS the request state: no delivered text, no tooltip (an
	// empty answer is an answer — deliver clears pending either way).
	const auto snap = hoverState.snapshot();
	if (!snap || snap->empty())
		return;

	if (atCaret)
	{
		// Anchor just below the caret cell; dismissal retires it.
		const ViewLayout &layout = api->layout();
		int row = 0, col = 0;
		api->getCaret(row, col);
		const float fs = ImGui::GetFontSize();
		const ImVec2 anchor(api->caretScreenX() + fs * 0.25f,
							layout.textPos.y +
								static_cast<float>(row + 1) * layout.lineHeight +
								fs * 0.25f);
		renderMouseTooltip(*snap, &anchor);
		return;
	}

	renderMouseTooltip(*snap);
}

void LSPSymbolInfo::renderMouseTooltip(const std::string &markdown, const ImVec2 *anchor)
{
	if (!api || !api->claimTooltip())
		return;

	const float fs = ImGui::GetFontSize();
	if (anchor)
		ImGui::SetNextWindowPos(*anchor);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(fs * 0.7f, fs * 0.5f));
	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, fs * 0.3f);

	if (ImGui::BeginTooltip())
	{
		EditorApi &hover = anchor ? *api : (hoverApi ? *hoverApi : *api);
		RenderHoverMarkdown(markdown, hover, hover.languageId());
		if (!anchor)
		{
			// Mouse mode: remember the rect for sticky-popup handling.
			popupMin = ImGui::GetWindowPos();
			popupMax = ImVec2(popupMin.x + ImGui::GetWindowSize().x,
							  popupMin.y + ImGui::GetWindowSize().y);
			popupRectValid = true;
		}
		ImGui::EndTooltip();
	}

	ImGui::PopStyleVar(2);
}
