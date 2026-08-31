#include "hover_tooltip.h"
#include "../editor_api.h"
#include "../services/diagnostics/diagnostics_store.h"
#include "../services/highlight/tree_sitter.h"
#include "diagnostic_style.h"
#include "hover_markdown.h"

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

float drawCodeLine(const std::string &line, const LineColorSpans &spans, EditorApi &api)
{
	const float h = ImGui::GetTextLineHeight();
	ImVec2 pos = ImGui::GetCursorScreenPos();
	if (line.empty())
	{
		ImGui::Dummy(ImVec2(1.0f, h));
		return 1.0f;
	}

	ImDrawList *dl = ImGui::GetWindowDrawList();
	const ImVec4 fallback = api.defaultTextColor();
	float x = pos.x;
	size_t spanIdx = 0;
	int i = 0;
	const int n = static_cast<int>(line.size());
	while (i < n)
	{
		while (spanIdx < spans.size() && spans[spanIdx].end <= i)
			++spanIdx;

		ImVec4 color = fallback;
		int runEnd = n;
		if (spanIdx < spans.size() && spans[spanIdx].start <= i)
		{
			color = api.syntaxColor(spans[spanIdx].slot);
			runEnd = std::min(n, spans[spanIdx].end);
		} else if (spanIdx < spans.size() && spans[spanIdx].start > i)
		{
			runEnd = std::min(n, spans[spanIdx].start);
		}

		const char *a = line.c_str() + i;
		const char *b = line.c_str() + runEnd;
		dl->AddText(ImVec2(x, pos.y), ImGui::ColorConvertFloat4ToU32(color), a, b);
		x += ImGui::CalcTextSize(a, b).x;
		i = runEnd;
	}
	ImGui::Dummy(ImVec2(std::max(1.0f, x - pos.x), h));
	return x - pos.x;
}

void drawCodeBlock(const HoverMdBlock &block, const ColorRangeMap &colors, EditorApi &api)
{
	std::vector<std::string> lines = SplitHoverLines(block.text);
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
		drawCodeLine(lines[static_cast<size_t>(row)], *spans, api);
	}
	ImGui::PopStyleVar();
	// Block-to-block spacing is owned by RenderHoverMarkdown.
}

void drawProseLine(const std::string &line, EditorApi &api)
{
	const ImVec4 text = api.defaultTextColor();
	const ImVec4 code = api.syntaxColor(ThemeSlot::String);

	std::string_view s = line;

	bool first = true;
	size_t i = 0;
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

	while (i < s.size())
	{
		if (s[i] == '`')
		{
			const size_t end = s.find('`', i + 1);
			if (end != std::string_view::npos)
			{
				emit(s.substr(i + 1, end - i - 1), code, false);
				i = end + 1;
				continue;
			}
		}
		if (i + 1 < s.size() && s[i] == '*' && s[i + 1] == '*')
		{
			const size_t end = s.find("**", i + 2);
			if (end != std::string_view::npos)
			{
				emit(s.substr(i + 2, end - i - 2), text, true);
				i = end + 2;
				continue;
			}
		}
		if (s[i] == '[')
		{
			const size_t close = s.find(']', i + 1);
			if (close != std::string_view::npos && close + 1 < s.size() &&
				s[close + 1] == '(')
			{
				const size_t endParen = s.find(')', close + 2);
				if (endParen != std::string_view::npos)
				{
					emit(s.substr(i + 1, close - i - 1), text, false);
					i = endParen + 1;
					continue;
				}
			}
		}

		size_t next = s.size();
		for (size_t j = i + 1; j < s.size(); ++j)
		{
			if (s[j] == '`' || s[j] == '[' ||
				(j + 1 < s.size() && s[j] == '*' && s[j + 1] == '*'))
			{
				next = j;
				break;
			}
		}
		emit(s.substr(i, next - i), text, false);
		i = next;
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

void RenderHoverMarkdown(const std::string &markdown,
						 EditorApi &api,
						 const std::string &fallbackLanguageId)
{
	const std::vector<HoverMdBlock> blocks = ParseHoverMarkdown(markdown);
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
			drawCodeBlock(block, colors, api);
			continue;
		}

		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 0.0f));
		for (const std::string &line : SplitHoverLines(block.text))
		{
			if (!line.empty())
				drawProseLine(line, api);
			else
				ImGui::Dummy(ImVec2(1.0f, ImGui::GetTextLineHeight() * 0.35f));
		}
		ImGui::PopStyleVar();
	}

	ImGui::PopTextWrapPos();
}

void RenderDiagnosticTooltip(const std::vector<DiagnosticItem> &items,
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
			ImGui::TextColored(DiagnosticSeverityColor(d.severity),
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
