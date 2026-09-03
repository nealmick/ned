#include "qt_lsp_symbol_info.h"

#include "../../../editor/platform/lsp_editor.h"
#include "../../../editor/services/highlight/tree_sitter.h"
#include "../../../editor/util/hover_markdown.h"
#include "../../../lsp/lsp_client.h"
#include "editor/views/qt/ned_color_qt.h"
#include "editor/views/qt/qt_editor_view.h"
#include "editor/views/qt/qt_theme.h"

#include <QCursor>
#include <QGuiApplication>
#include <QLabel>
#include <QScreen>

#include <algorithm>
#include <iostream>

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

// One code line as colored runs — the same span walk the ImGui tooltip
// (drawCodeLine) and the editor's own text painter use. Span offsets are
// byte offsets into the line.
QString
codeLineToHtml(const std::string &line, const LineColorSpans &spans, LspEditor &doc)
{
	const QColor fallback = toQColor(doc.defaultTextColor());
	QString html;
	size_t spanIdx = 0;
	int i = 0;
	const int n = static_cast<int>(line.size());
	while (i < n)
	{
		while (spanIdx < spans.size() && spans[spanIdx].end <= i)
			++spanIdx;

		QColor color = fallback;
		int runEnd = n;
		if (spanIdx < spans.size() && spans[spanIdx].start <= i)
		{
			color = toQColor(doc.syntaxColor(spans[spanIdx].slot));
			runEnd = std::min(n, spans[spanIdx].end);
		} else if (spanIdx < spans.size())
		{
			runEnd = std::min(n, spans[spanIdx].start);
		}

		html += colorSpan(
			QString::fromStdString(line.substr(i, runEnd - i)).toHtmlEscaped(), color);
		i = runEnd;
	}
	return html;
}

// Fenced block: tree-sitter snippet highlight with the editor's theme
// colors (ImGui drawCodeBlock parity — no background tint, colored runs).
QString codeBlockToHtml(const HoverMdBlock &block, LspEditor &doc)
{
	std::string lang = block.language.empty() ? doc.languageId() : block.language;
	ColorRangeMap colors;
	if (!lang.empty())
		colors = TreeSitter::highlightSnippet(lang, block.text);

	std::vector<std::string> lines = SplitHoverLines(block.text);
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

// Prose inline subset — ImGui drawProseLine parity: `code` colored as the
// theme's String slot, **bold** brightened, links as plain text.
QString proseLineToHtml(const std::string &line, LspEditor &doc)
{
	const QColor text = toQColor(doc.defaultTextColor());
	const QColor code = toQColor(doc.syntaxColor(ThemeSlot::String));
	const QString src = QString::fromStdString(line);

	QString html;
	for (int i = 0; i < src.size();)
	{
		if (src[i] == '`')
		{
			const int end = src.indexOf('`', i + 1);
			if (end > i)
			{
				html += colorSpan(src.mid(i + 1, end - i - 1).toHtmlEscaped(), code);
				i = end + 1;
				continue;
			}
		}
		if (src[i] == '*' && i + 1 < src.size() && src[i + 1] == '*')
		{
			const int end = src.indexOf("**", i + 2);
			if (end > i)
			{
				html += "<b>" +
						colorSpan(src.mid(i + 2, end - i - 2).toHtmlEscaped(),
								  boosted(text)) +
						"</b>";
				i = end + 2;
				continue;
			}
		}
		if (src[i] == '[')
		{
			const int textEnd = src.indexOf(']', i + 1);
			if (textEnd > i && textEnd + 1 < src.size() && src[textEnd + 1] == '(')
			{
				const int urlEnd = src.indexOf(')', textEnd + 2);
				if (urlEnd > textEnd)
				{
					html +=
						colorSpan(src.mid(i + 1, textEnd - i - 1).toHtmlEscaped(), text);
					i = urlEnd + 1;
					continue;
				}
			}
		}

		// Plain run up to the next inline marker.
		int next = src.size();
		for (int j = i + 1; j < src.size(); ++j)
		{
			if (src[j] == '`' || src[j] == '[' ||
				(j + 1 < src.size() && src[j] == '*' && src[j + 1] == '*'))
			{
				next = j;
				break;
			}
		}
		html += colorSpan(src.mid(i, next - i).toHtmlEscaped(), text);
		i = next;
	}
	return html;
}

// The same block pipeline the ImGui renderer runs (ParseHoverMarkdown →
// code/rules/prose), emitted as rich text instead of draw calls. Blocks
// join with <br> and spans only — no <p>: Qt's default paragraph margins
// double the line spacing.
QString hoverMarkdownToHtml(const std::string &markdown, LspEditor &doc)
{
	QString html;
	bool firstBlock = true;
	for (const HoverMdBlock &block : ParseHoverMarkdown(markdown))
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
			for (const std::string &line : SplitHoverLines(block.text))
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

QtLspSymbolInfo::QtLspSymbolInfo(LSPClient &client, Settings &settings, QObject *parent)
	: QObject(parent), client(client), settings(settings)
{
	// App switch (Cmd+Tab): the tip window hides itself (QtHoverTip); drop
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

QtLspSymbolInfo::~QtLspSymbolInfo()
{
	hideTooltip();
	delete tip; // created parentless (tooltip window) — owned here
}

void QtLspSymbolInfo::setEditor(QtEditorView *editor)
{
	if (view == editor)
		return;
	view = editor;
	dismiss();
}

void QtLspSymbolInfo::triggerAtCaret()
{
	if (!view)
		return;
	anchored = true;
	hoverRow = -1;
	hoverCol = -1;
	hoverView = nullptr;
	client.hover.get();
}

void QtLspSymbolInfo::hoverTarget(QtEditorView *hovered, const HoverTrigger::Info &info)
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

void QtLspSymbolInfo::dismiss()
{
	anchored = false;
	hoverRow = -1;
	hoverCol = -1;
	requestedFor = nullptr;
	hoverView = nullptr;
	hideTooltip();
	client.hover.cancel();
}

void QtLspSymbolInfo::poll()
{
	// Visibility IS the request state: no delivered text, no tooltip (an
	// empty answer is an answer — deliver clears pending either way).
	const auto snap = client.hover.snapshot();
	if (!snap || snap->empty() || (!view && !hoverView))
	{
		hideTooltip();
		return;
	}

	QtEditorView *const target = hoverView ? hoverView : view;
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
		tip = new QtHoverTip();
	tip->present(hoverMarkdownToHtml(markdown, *target),
				 target->font(),
				 NedQtTheme::raised(NedQtTheme::background(settings)),
				 toQColor(target->defaultTextColor()),
				 anchor);
}

bool QtLspSymbolInfo::isVisible() const { return tip && tip->isVisible(); }

void QtLspSymbolInfo::hideTooltip()
{
	if (tip)
	{
		tip->hide();
		shownMarkdown.clear();
	}
}
