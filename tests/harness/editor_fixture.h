/*
	Shared headless wiring for layer unit tests.
	Same core construction order as Editor (no FileExplorer / highlight / LSP).
	ProjectUndo is project-scoped — one store shared like production multi-tab.
*/

#pragma once

#include "editor/editor_commands.h"
#include "editor/editor_events.h"
#include "editor/editor_operations.h"
#include "editor/editor_state.h"
#include "editor/editor_view_state.h"
#include "editor/services/save_service.h"
#include "util/project_undo.h"

#include "imgui.h"

#include <initializer_list>
#include <string>
#include <utility>
#include <vector>

namespace test {

// In-process clipboard so paste/copy work without a platform backend.
inline std::string &clipboardStore()
{
	static std::string s;
	return s;
}

inline void platformSetClipboard(ImGuiContext *, const char *text)
{
	clipboardStore() = text ? text : "";
}

inline const char *platformGetClipboard(ImGuiContext *)
{
	return clipboardStore().c_str();
}

inline void ensureImGui()
{
	if (ImGui::GetCurrentContext() != nullptr)
		return;
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiPlatformIO &pio = ImGui::GetPlatformIO();
	pio.Platform_SetClipboardTextFn = platformSetClipboard;
	pio.Platform_GetClipboardTextFn = platformGetClipboard;
}

struct EditorFixture
{
	// Same order as Editor core members + shared project undo.
	std::string projectRoot;
	ProjectUndo undo;
	EditorState state;
	EditorEvents events;
	EditorOperations ops;
	EditorViewState view;
	EditorSave save;
	EditorCommands commands;

	EditorFixture()
		: undo(projectRoot),
		  ops(state),
		  view(state),
		  save(state, events),
		  commands(state, view, ops, undo, events, save)
	{
		ensureImGui();
		clipboardStore().clear();
	}

	void setDocument(const std::string &text, const std::string &undoKey = "test://doc")
	{
		state.setFromString(text);
		state.path = undoKey;
		if (text.find('\r') == std::string::npos)
			state.lineEnding = "\n";
		ops.clearPending();
		ops.bumpGeneration();
		view.setBoth(0, 0);
		view.cursorColumnPreferred = 0;
		undo.ensureFile(undoKey);
	}

	void setCaret(int row, int column)
	{
		Selection s;
		s.setBoth(row, column);
		view.setSelections({s}, 0);
	}

	void setSelection(int ar, int ac, int br, int bc)
	{
		Selection s;
		s.anchorRow = ar;
		s.anchorColumn = ac;
		s.headRow = br;
		s.headColumn = bc;
		view.setSelections({s}, 0);
	}

	// Multiple collapsed carets (heads only). primaryIndex defaults to last.
	void setCarets(std::initializer_list<std::pair<int, int>> heads, int primary = -1)
	{
		std::vector<Selection> sels;
		sels.reserve(heads.size());
		for (const auto &h : heads)
		{
			Selection s;
			s.setBoth(h.first, h.second);
			sels.push_back(s);
		}
		const int pi =
			primary >= 0 ? primary : std::max(0, static_cast<int>(sels.size()) - 1);
		view.setSelections(std::move(sels), pi);
	}

	std::string content() const { return state.join(); }

	void setClip(const std::string &text) { clipboardStore() = text; }

	std::string getClip() const { return clipboardStore(); }
};

} // namespace test
