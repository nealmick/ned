#include "editor_finder.h"
#include "../../util/settings.h"
#include "../editor_api.h"
#include "../editor_commands.h"
#include "../editor_events.h"
#include "../editor_input.h"
#include "../editor_state.h"
#include "../editor_view_state.h"
#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstring>
#include <limits>
#include <vector>

namespace {

std::string toLower(const std::string &s)
{
	std::string out = s;
	std::transform(out.begin(), out.end(), out.begin(), [](unsigned char c) {
		return static_cast<char>(std::tolower(c));
	});
	return out;
}

struct ScopedFindStyle
{
	ScopedFindStyle(Settings *settings)
	{
		const float fs = ImGui::GetFontSize();
		ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, fs * 0.3f);
		ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);
		ImGui::PushStyleColor(
			ImGuiCol_FrameBg,
			ImVec4(settings->settings["backgroundColor"][0].get<float>() * 0.8f,
				   settings->settings["backgroundColor"][1].get<float>() * 0.8f,
				   settings->settings["backgroundColor"][2].get<float>() * 0.8f,
				   1.0f));
		ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.3f, 0.3f, 0.3f, 1.0f));
	}
	~ScopedFindStyle()
	{
		ImGui::PopStyleColor(2);
		ImGui::PopStyleVar(2);
	}
};

} // namespace

void EditorFinder::update()
{
	if (!viewState)
		return;

	syncInputBlock();
	pollOpenCloseKeys();

	if (!active)
	{
		boxRectValid = false;
		return;
	}

	viewState->blockInput = true;

	if (boxRectValid && ImGui::IsMouseClicked(ImGuiMouseButton_Left) &&
		!ImGui::IsMouseHoveringRect(boxMin, boxMax, true))
	{
		close();
		return;
	}

	draw();
	handleEnterShortcuts();
}

void EditorFinder::dismiss()
{
	if (!active)
		return;
	close();
}

void EditorFinder::syncInputBlock()
{
	if (active)
	{
		viewState->blockInput = true;
		releaseBlockNextFrame = false;
	} else if (releaseBlockNextFrame)
	{
		viewState->blockInput = false;
		releaseBlockNextFrame = false;
	}
}

void EditorFinder::pollOpenCloseKeys()
{
	// Multi-tab: only the focused editor host may open find (same pattern as line jump).
	const bool hostFocused = ImGui::IsWindowFocused(ImGuiFocusedFlags_ChildWindows) ||
							 ImGui::IsWindowFocused(0) ||
							 ImGui::IsWindowFocused(ImGuiFocusedFlags_RootWindow);

	ImGuiIO &io = ImGui::GetIO();
	if (hostFocused && (io.KeyCtrl || io.KeySuper) && ImGui::IsKeyPressed(ImGuiKey_F))
		open();
	if (active && ImGui::IsKeyPressed(ImGuiKey_Escape))
		close();
}

void EditorFinder::open()
{
	if (api)
		api->requestExclusiveOverlay(EditorEvents::DidRequestExclusiveOverlay::Keep::Find);
	active = true;
	releaseBlockNextFrame = false;
	viewState->blockInput = true;
	shouldFocus = true;

	if (viewState && viewState->hasSelection() && state)
	{
		int sr, sc, er, ec;
		viewState->getOrdered(sr, sc, er, ec);
		if (sr == er)
		{
			setQuery(state->line(sr).substr(static_cast<size_t>(sc),
											static_cast<size_t>(ec - sc)));
		}
	}
}

void EditorFinder::close()
{
	active = false;
	boxRectValid = false;
	// Hold block through this frame's document input, release next update().
	viewState->blockInput = true;
	releaseBlockNextFrame = true;
	// InputText had keyboard focus — return it to the document so typing works
	// (Escape, click-outside, Cmd/Ctrl+Enter multi-match).
	if (api)
		api->requestFocus();
	// Same Enter that closed find must not insert a newline in the document.
	if (input)
		input->suppressNextEnter = true;
}

void EditorFinder::setQuery(const std::string &query)
{
	if (findText == query)
		return;
	findText = query;
	matchIndex = -1;
	matchesDirty = true;
}

void EditorFinder::rebuildMatches()
{
	if (!matchesDirty)
		return;
	matchesDirty = false;
	matchIndex = -1;
	matches.clear();

	if (findText.empty() || !state)
		return;

	const std::string needle = ignoreCase ? toLower(findText) : findText;

	for (int r = 0; r < state->lineCount(); ++r)
	{
		state->lineInto(r, lineScratch);
		const std::string *hay = &lineScratch;
		if (ignoreCase)
		{
			hayScratch = toLower(lineScratch);
			hay = &hayScratch;
		}
		for (size_t pos = 0;;)
		{
			pos = hay->find(needle, pos);
			if (pos == std::string::npos)
				break;
			matches.push_back({r, static_cast<int>(pos)});
			++pos;
		}
	}
}

void EditorFinder::selectMatch(int index, bool highlight)
{
	if (!commands || index < 0 || index >= static_cast<int>(matches.size()))
		return;

	matchIndex = index;
	const Match &m = matches[static_cast<size_t>(index)];
	using Reveal = EditorCommands::CursorReveal;
	if (highlight)
	{
		const int endCol = m.column + static_cast<int>(findText.size());
		commands->setSelection(m.row, m.column, m.row, endCol, Reveal::center);
	} else
	{
		commands->setCursor(m.row, m.column, false, Reveal::center);
	}
}

void EditorFinder::stepMatch(int direction)
{
	rebuildMatches();
	if (matches.empty() || !viewState)
		return;

	const int n = static_cast<int>(matches.size());
	if (matchIndex < 0)
	{
		int best = 0;
		for (int i = 0; i < n; ++i)
		{
			const Match &m = matches[i];
			if (m.row > viewState->row ||
				(m.row == viewState->row && m.column >= viewState->column))
			{
				best = i;
				break;
			}
			best = i;
		}
		if (direction < 0)
			best = (best - 1 + n) % n;
		matchIndex = best;
	} else
	{
		matchIndex = (matchIndex + direction + n) % n;
	}
	selectMatch(matchIndex, true);
}

void EditorFinder::selectAllMatches()
{
	rebuildMatches();
	if (!commands || matches.empty() || findText.empty())
		return;

	const int needleLen = static_cast<int>(findText.size());
	const int prefRow = viewState ? viewState->row : 0;
	const int prefCol = viewState ? viewState->column : 0;

	std::vector<Selection> sels;
	sels.reserve(matches.size());
	int primary = 0;
	int bestScore = std::numeric_limits<int>::max();

	for (size_t i = 0; i < matches.size(); ++i)
	{
		const Match &m = matches[i];
		Selection s;
		s.anchorRow = m.row;
		s.anchorColumn = m.column;
		s.headRow = m.row;
		s.headColumn = m.column + needleLen;
		sels.push_back(s);

		// Prefer match start closest to previous primary (row weighted).
		const int score =
			std::abs(m.row - prefRow) * 100000 + std::abs(m.column - prefCol);
		if (score < bestScore)
		{
			bestScore = score;
			primary = static_cast<int>(i);
		}
	}

	using Reveal = EditorCommands::CursorReveal;
	commands->setSelections(std::move(sels), primary, Reveal::center);
	close();
}

void EditorFinder::handleEnterShortcuts()
{
	ImGuiIO &io = ImGui::GetIO();
	if (!ImGui::IsKeyPressed(ImGuiKey_Enter, false))
		return;

	if (io.KeyCtrl || io.KeySuper)
	{
		selectAllMatches();
	} else if (io.KeyShift)
	{
		stepMatch(-1);
	} else
	{
		stepMatch(+1);
	}
}

void EditorFinder::draw()
{
	if (!settings)
		return;

	if (shouldFocus)
	{
		std::strncpy(inputBuffer, findText.c_str(), INPUT_CAP - 1);
		inputBuffer[INPUT_CAP - 1] = '\0';
		shouldFocus = false;
	}

	// Editor host uses WindowPadding 0 (title flush under dock tabs). Give the
	// find row air so the top border/frame is not clipped by the tab strip.
	const float fs = ImGui::GetFontSize();
	const float kPadX = fs * 0.5f;
	const float kPadTop = fs * 0.5f;
	const float kPadBottom = fs * 0.3f;
	ImGui::SetCursorPos(
		ImVec2(ImGui::GetCursorPosX() + kPadX, ImGui::GetCursorPosY() + kPadTop));

	ImGui::BeginGroup();
	{
		// Leave room for status/checkbox on the right of this padded row.
		const float rowW = std::max(fs * 6.0f, ImGui::GetContentRegionAvail().x - kPadX);
		ImGui::SetNextItemWidth(rowW * 0.5f);
		{
			ScopedFindStyle style(settings);
			// Unique id per EditorFinder instance (side-by-side tabs).
			char inputId[64];
			std::snprintf(
				inputId, sizeof(inputId), "##findbox_%p", static_cast<const void *>(this));
			// Keep the find box focused every frame while open. Enter deactivates
			// InputText otherwise and focus falls into the void (blockInput stays on).
			ImGui::SetKeyboardFocusHere();
			ImGui::InputText(
				inputId, inputBuffer, INPUT_CAP, ImGuiInputTextFlags_AutoSelectAll);
		}
		setQuery(inputBuffer);

		if (!findText.empty())
		{
			rebuildMatches();
			ImGui::SameLine();
			ImGui::Dummy(ImVec2(fs * 0.5f, 0));
			ImGui::SameLine();
			if (matchIndex < 0 || matches.empty())
				ImGui::Text("Not Found");
			else
				ImGui::Text("%d/%d", matchIndex + 1, static_cast<int>(matches.size()));
		}

		ImGui::SameLine();
		ImGui::Dummy(ImVec2(fs * 0.5f, 0));
		ImGui::SameLine();
		const bool prev = ignoreCase;
		{
			ScopedFindStyle style(settings);
			ImGui::Checkbox("Case Insensitive", &ignoreCase);
		}
		if (ignoreCase != prev)
		{
			matchIndex = -1;
			matchesDirty = true;
		}
	}
	ImGui::EndGroup();

	boxMin = ImGui::GetItemRectMin();
	boxMax = ImGui::GetItemRectMax();
	boxRectValid = true;

	ImGui::Dummy(ImVec2(0.0f, kPadBottom));
}
