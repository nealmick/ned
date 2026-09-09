#include "lsp_symbol_info.h"

#include "../../../editor/platform/lsp_editor.h"
#include "../../../editor/services/highlight/tree_sitter.h"
#include "../../../editor/util/hover_markdown.h"
#include "../../../editor/util/hover_runs.h"
#include "../../../lsp/lsp_client.h"
#include "editor/views/qt/editor_frame.h"
#include "editor/views/qt/ned_color.h"
#include "host/qt/theme.h"

#include <QCursor>
#include <QGuiApplication>

#include <algorithm>

namespace {

// **bold** brightness boost — ImGui hover_tooltip parity.
constexpr float kBoldBoost = 1.15f;

QColor boosted(const QColor &c)
{
	return QColor(std::min(255, int(c.red() * kBoldBoost)),
				  std::min(255, int(c.green() * kBoldBoost)),
				  std::min(255, int(c.blue() * kBoldBoost)));
}

QString colorSpan(const QString &text, const QColor &c)
{
	return "<span style=\"color:" + c.name() + "\">" + text + "</span>";
}

// One code line as colored runs — the shared span walk (HoverCodeRuns) that
// the ImGui tooltip and the editor's own text painter also use. Span offsets
// are byte offsets into the line.
QString
codeLineToHtml(const std::string &line, const LineColorSpans &spans, LSPEditor &doc)
{
	const QColor fallback = toQColor(doc.defaultTextColor());
	QString html;
	for (const HoverCodeRun &run : HoverCodeRuns(line, spans))
	{
		html += colorSpan(
			QString::fromUtf8(run.text.data(), static_cast<qsizetype>(run.text.size()))
				.toHtmlEscaped(),
			run.themed ? toQColor(doc.syntaxColor(run.slot)) : fallback);
	}
	return html;
}

// Fenced block: tree-sitter snippet highlight with the editor's theme
// colors (ImGui drawCodeBlock parity — no background tint, colored runs).
QString codeBlockToHtml(const HoverMdBlock &block, LSPEditor &doc)
{
	std::string lang = block.language.empty() ? doc.languageId() : block.language;
	ColorRangeMap colors;
	if (!lang.empty())
		colors = TreeSitter::highlightSnippet(lang, block.text);

	std::vector<std::string> lines = splitHoverLines(block.text);
	if (lines.empty())
		lines.emplace_back("");

	static const LineColorSpans kNoSpans;
	QString html;
	for (size_t row = 0; row < lines.size(); ++row)
	{
		if (row)
			html += "<br>";
		html +=
			codeLineToHtml(lines[row], row < colors.size() ? colors[row] : kNoSpans, doc);
	}
	return html;
}

// Prose inline subset — shared run walk (HoverProseRuns): `code` colored as
// the theme's String slot, **bold** brightened, links as plain text.
QString proseLineToHtml(const std::string &line, LSPEditor &doc)
{
	const QColor text = toQColor(doc.defaultTextColor());
	const QColor code = toQColor(doc.syntaxColor(ThemeSlot::String));

	QString html;
	for (const HoverRun &run : HoverProseRuns(line))
	{
		const QString piece =
			QString::fromUtf8(run.text.data(), static_cast<qsizetype>(run.text.size()))
				.toHtmlEscaped();
		switch (run.style)
		{
		case HoverRunStyle::Code:
			html += colorSpan(piece, code);
			break;
		case HoverRunStyle::Bold:
			html += "<b>" + colorSpan(piece, boosted(text)) + "</b>";
			break;
		default: // Text / Link: label text in the default color
			html += colorSpan(piece, text);
			break;
		}
	}
	return html;
}

// The same block pipeline the ImGui renderer runs (parseHoverMarkdown →
// code/rules/prose), emitted as rich text instead of draw calls. Blocks
// join with <br> and spans only — no <p>: Qt's default paragraph margins
// double the line spacing.
QString hoverMarkdownToHtml(const std::string &markdown, LSPEditor &doc)
{
	QString html;
	bool firstBlock = true;
	for (const HoverMdBlock &block : parseHoverMarkdown(markdown))
	{
		if (!firstBlock)
			html += "<br>";
		firstBlock = false;

		if (block.code)
		{
			html += codeBlockToHtml(block, doc);
		} else if (block.text == "---")
		{
			html += "<hr>";
		} else if (!block.text.empty())
		{
			bool firstLine = true;
			for (const std::string &line : splitHoverLines(block.text))
			{
				if (!firstLine)
					html += "<br>";
				firstLine = false;
				html += proseLineToHtml(line, doc);
			}
		}
	}
	return html;
}

} // namespace

LSPSymbolInfo::LSPSymbolInfo(LSPClient &client, Settings &settings, QObject *parent)
	: QObject(parent), client(client), settings(settings)
{
	// App switch (Cmd+Tab): the tip window hides itself (HoverTooltip); drop
	// the request state too, or the still-running poll timer re-presents
	// the last hover over the newly frontmost application. Guarded on
	// visibility: it must only ever retire a tip that is on screen, never
	// touch a pending request otherwise.
	connect(qGuiApp,
			&QGuiApplication::applicationStateChanged,
			this,
			[this](Qt::ApplicationState state) {
				if (state != Qt::ApplicationActive && isVisible())
					dismiss();
			});
}

LSPSymbolInfo::~LSPSymbolInfo()
{
	hideTooltip();
	delete tip; // created parentless (tooltip window) — owned here
}

void LSPSymbolInfo::setEditor(EditorFrame *editor)
{
	if (view == editor)
		return;
	view = editor;
	dismiss();
}

void LSPSymbolInfo::triggerAtCaret()
{
	if (!view)
		return;
	anchored = true;
	hoverRow = -1;
	hoverCol = -1;
	hoverView = nullptr;
	client.hover.get();
}

void LSPSymbolInfo::hoverTarget(EditorFrame *hovered, const HoverTrigger::Info &info)
{
	if (!hovered || !info.active || info.zone != HoverTrigger::Zone::Text)
		return;
	anchored = false;
	// Dedup per document: the same cell of the same editor keeps its
	// delivered text; a different document at the same row/col re-requests.
	if (hovered == requestedFor && info.row == hoverRow && info.column == hoverCol)
		return;
	requestedFor = hovered;
	hoverView = hovered;
	hoverRow = info.row;
	hoverCol = info.column;
	client.hover.requestAt(info.row, info.column, hovered);
}

void LSPSymbolInfo::dismiss()
{
	anchored = false;
	hoverRow = -1;
	hoverCol = -1;
	requestedFor = nullptr;
	hoverView = nullptr;
	hideTooltip();
	client.hover.cancel();
}

void LSPSymbolInfo::poll()
{
	// Visibility IS the request state: no delivered text, no tooltip (an
	// empty answer is an answer — deliver clears pending either way).
	const auto snap = client.hover.snapshot();
	if (!snap || snap->empty() || (!view && !hoverView))
	{
		hideTooltip();
		return;
	}

	EditorFrame *const target = hoverView ? hoverView : view;
	QPoint anchor;
	if (anchored)
	{
		if (!view)
			return;
		// Just below the caret line (ImGui anchors at caretX + fs/4).
		anchor = view->mapToGlobal(view->caretWidgetPos() +
								   QPoint(4, view->caretLineHeight() + 4));
	} else
	{
		anchor = QCursor::pos();
	}

	// Gate on the raw markdown: building the highlighted HTML re-runs the
	// tree-sitter snippet parse, so only do it when the text changed.
	const std::string markdown = *snap;
	if (tip && tip->isVisible() && markdown == shownMarkdown)
	{
		tip->replaceAt(anchor); // follow the caret / mouse between polls
		return;
	}
	shownMarkdown = markdown;

	// The whole tooltip renders in the editor's monospace font (ImGui
	// parity) on the shared themed card.
	if (!tip)
		tip = new HoverTooltip();
	tip->present(hoverMarkdownToHtml(markdown, *target),
				 target->font(),
				 NedQtTheme::raised(NedQtTheme::background(settings)),
				 toQColor(target->defaultTextColor()),
				 anchor);
}

bool LSPSymbolInfo::isVisible() const { return tip && tip->isVisible(); }

void LSPSymbolInfo::hideTooltip()
{
	if (tip)
	{
		tip->hide();
		shownMarkdown.clear();
	}
}
