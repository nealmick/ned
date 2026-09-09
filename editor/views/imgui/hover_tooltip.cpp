#include "hover_tooltip.h"
#include "../../platform/lsp_editor.h"
#include "../../services/diagnostics/diagnostics_store.h"
#include "../../services/highlight/tree_sitter.h"
#include "../../util/hover_markdown.h"
#include "../../util/hover_runs.h"
#include "diagnostic_style.h"
#include "ned_color.h"

#include "imgui.h"

#include <algorithm>
#include <cctype>
#include <string_view>
#include <vector>

namespace {

// Tooltip geometry, in font-size multiples so it scales with the UI font.
constexpr float kProseWrapWidthFs = 32.0f;		// text wrap position
constexpr float kDiagnosticWrapWidthFs = 28.0f; // diagnostic message wrap
constexpr float kBlockGapFs = 0.15f;			// gap between markdown blocks
constexpr float kBoldBoost = 1.15f;				// **bold** brightness multiplier

float drawCodeLine(const std::string &line, const LineColorSpans &spans, LSPEditor &editor)
{
	const float h = ImGui::GetTextLineHeight();
	ImVec2 pos = ImGui::GetCursorScreenPos();
	if (line.empty())
	{
		ImGui::Dummy(ImVec2(1.0f, h));
		return 1.0f;
	}

	ImDrawList *dl = ImGui::GetWindowDrawList();
	const ImVec4 fallback = toImVec4(editor.defaultTextColor());
	float x = pos.x;
	for (const HoverCodeRun &run : HoverCodeRuns(line, spans))
	{
		const ImVec4 color =
			run.themed ? toImVec4(editor.syntaxColor(run.slot)) : fallback;
		const char *a = run.text.data();
		const char *b = run.text.data() + run.text.size();
		dl->AddText(ImVec2(x, pos.y), ImGui::ColorConvertFloat4ToU32(color), a, b);
		x += ImGui::CalcTextSize(a, b).x;
	}
	ImGui::Dummy(ImVec2(std::max(1.0f, x - pos.x), h));
	return x - pos.x;
}

void drawCodeBlock(const HoverMdBlock &block,
				   const ColorRangeMap &colors,
				   LSPEditor &editor)
{
	std::vector<std::string> lines = splitHoverLines(block.text);
	if (lines.empty())
		lines.emplace_back("");

	const ImVec2 p0 = ImGui::GetCursorScreenPos();

	ImGui::SetCursorScreenPos(ImVec2(p0.x, p0.y));
	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 0.0f));
	static const LineColorSpans kEmpty;
	for (int row = 0; row < static_cast<int>(lines.size()); ++row)
	{
		const LineColorSpans *spans = &kEmpty;
		if (row < static_cast<int>(colors.size()))
			spans = &colors[static_cast<size_t>(row)];
		drawCodeLine(lines[static_cast<size_t>(row)], *spans, editor);
	}
	ImGui::PopStyleVar();
	// Block-to-block spacing is owned by renderHoverMarkdown.
}

void drawProseLine(const std::string &line, LSPEditor &editor)
{
	const ImVec4 text = toImVec4(editor.defaultTextColor());
	const ImVec4 code = toImVec4(editor.syntaxColor(ThemeSlot::String));

	bool first = true;
	auto emit = [&](std::string_view piece, ImVec4 color, bool bold) {
		if (piece.empty())
			return;
		if (!first)
			ImGui::SameLine(0.0f, 0.0f);
		first = false;
		ImGui::PushStyleColor(ImGuiCol_Text, color);
		if (bold)
			ImGui::PushStyleColor(ImGuiCol_Text,
								  ImVec4(std::min(1.0f, color.x * kBoldBoost),
										 std::min(1.0f, color.y * kBoldBoost),
										 std::min(1.0f, color.z * kBoldBoost),
										 color.w));
		ImGui::TextUnformatted(piece.data(), piece.data() + piece.size());
		if (bold)
			ImGui::PopStyleColor();
		ImGui::PopStyleColor();
	};

	for (const HoverRun &run : HoverProseRuns(line))
	{
		switch (run.style)
		{
		case HoverRunStyle::Code:
			emit(run.text, code, false);
			break;
		case HoverRunStyle::Bold:
			emit(run.text, text, true);
			break;
		default: // Text / Link: label text in the default color
			emit(run.text, text, false);
			break;
		}
	}
	if (first)
		ImGui::Dummy(ImVec2(1.0f, ImGui::GetTextLineHeight()));
}

} // namespace

bool TooltipArbiter::claim()
{
	const int frame = ImGui::GetFrameCount();
	if (claimedFrame == frame)
		return false;
	claimedFrame = frame;
	return true;
}

void renderHoverMarkdown(const std::string &markdown,
						 LSPEditor &editor,
						 const std::string &fallbackLanguageId)
{
	const std::vector<HoverMdBlock> blocks = parseHoverMarkdown(markdown);
	const float fs = ImGui::GetFontSize();
	ImGui::PushTextWrapPos(fs * kProseWrapWidthFs);

	bool firstBlock = true;
	for (const auto &block : blocks)
	{
		if (!firstBlock)
			ImGui::Dummy(ImVec2(0.0f, fs * kBlockGapFs));
		firstBlock = false;

		if (block.text == "---")
		{
			ImGui::Separator();
			continue;
		}

		if (block.code)
		{
			std::string lang =
				block.language.empty() ? fallbackLanguageId : block.language;
			ColorRangeMap colors;
			if (!lang.empty())
				colors = TreeSitter::highlightSnippet(lang, block.text);
			drawCodeBlock(block, colors, editor);
			continue;
		}

		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 0.0f));
		for (const std::string &line : splitHoverLines(block.text))
		{
			if (!line.empty())
				drawProseLine(line, editor);
			else
				ImGui::Dummy(ImVec2(1.0f, ImGui::GetTextLineHeight() * 0.35f));
		}
		ImGui::PopStyleVar();
	}

	ImGui::PopTextWrapPos();
}

void renderDiagnosticTooltip(const std::vector<DiagnosticItem> &items,
							 TooltipArbiter &arbiter)
{
	if (items.empty() || !arbiter.claim())
		return;

	const float fs = ImGui::GetFontSize();
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(fs * 0.65f, fs * 0.45f));
	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(fs * 0.4f, fs * 0.25f));
	if (ImGui::BeginTooltip())
	{
		for (size_t i = 0; i < items.size(); ++i)
		{
			if (i)
				ImGui::Separator();
			const DiagnosticItem &d = items[i];
			ImGui::TextColored(DiagnosticSeverityVec4(d.severity),
							   "%s",
							   DiagnosticSeverityLabel(d.severity));
			if (!d.source.empty())
			{
				ImGui::SameLine();
				ImGui::TextDisabled("%s", d.source.c_str());
			}
			ImGui::PushTextWrapPos(fs * kDiagnosticWrapWidthFs);
			ImGui::TextUnformatted(d.message.c_str());
			ImGui::PopTextWrapPos();
		}
		ImGui::EndTooltip();
	}
	ImGui::PopStyleVar(2);
}
