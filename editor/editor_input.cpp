/*
	File: editor_input.cpp
	Description: Keyboard + mouse → EditorCommands.
*/

#include "editor_input.h"
#include "editor_commands.h"
#include "editor_state.h"
#include "editor_view_state.h"
#include "util/editor_utils.h"
#include "util/project_undo.h"
#include "views/view_layout.h"

#include <algorithm>
#include <string>

void EditorInput::process()
{
	// AllowWhenBlockedByActiveItem: other docked editor may hold ActiveId.
	if (ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem))
	{
		processMouse();
		if (viewState && layout)
			viewState->processMouseWheelScrolling(*layout);
	}

	// Do NOT request ensureCursorVisible when blockInput — that used to run
	// every frame on unfocused multi-tab editors and yanked scroll to the caret.
	if (viewState && !viewState->blockInput)
		processKeyboard();
}

// ---------------------------------------------------------------------------
// Keyboard
// ---------------------------------------------------------------------------

void EditorInput::processKeyboard()
{
	if (!commands || !viewState)
		return;

	const ImGuiIO &io = ImGui::GetIO();
	const bool primary_mod = io.KeyCtrl || io.KeySuper;
	const bool shift = io.KeyShift;

	// Option/Alt: word left/right; add caret above/below (not with Ctrl/Cmd).
	if (io.KeyAlt && !primary_mod)
	{
		if (ImGui::IsKeyPressed(ImGuiKey_LeftArrow))
			commands->moveWordLeft(shift);
		if (ImGui::IsKeyPressed(ImGuiKey_RightArrow))
			commands->moveWordRight(shift);
		if (ImGui::IsKeyPressed(ImGuiKey_UpArrow))
			commands->addCursorAbove();
		if (ImGui::IsKeyPressed(ImGuiKey_DownArrow))
			commands->addCursorBelow();
	}

	if (ImGui::IsKeyPressed(ImGuiKey_Escape))
		commands->collapseSelection();

	if (ImGui::IsWindowFocused() && viewState && !viewState->blockInput)
	{
		if (ImGui::IsKeyPressed(ImGuiKey_Tab))
		{
			if (shift)
				commands->outdent();
			else
				commands->indent();
			ImGui::SetKeyboardFocusHere(-1);
		}

		if (primary_mod)
		{
			if (ImGui::IsKeyPressed(ImGuiKey_A))
				commands->selectAll();

			// Home/end / multi-line jump. Skip when Alt is held (word move wins).
			if (!io.KeyAlt)
			{
				if (ImGui::IsKeyPressed(ImGuiKey_LeftArrow))
					commands->moveLineStart(shift);
				if (ImGui::IsKeyPressed(ImGuiKey_RightArrow))
					commands->moveLineEnd(shift);
				if (ImGui::IsKeyPressed(ImGuiKey_UpArrow))
					commands->moveLines(-5, shift);
				if (ImGui::IsKeyPressed(ImGuiKey_DownArrow))
					commands->moveLines(5, shift);
			}
		}
	}

	processTextInput();

	// Plain arrows (no Alt/Ctrl/Cmd).
	if (!io.KeyAlt && !io.KeyCtrl && !io.KeySuper)
	{
		if (ImGui::IsKeyPressed(ImGuiKey_UpArrow))
			commands->moveUp(shift);
		if (ImGui::IsKeyPressed(ImGuiKey_DownArrow))
			commands->moveDown(shift);
		if (ImGui::IsKeyPressed(ImGuiKey_LeftArrow))
			commands->moveLeft(shift);
		if (ImGui::IsKeyPressed(ImGuiKey_RightArrow))
			commands->moveRight(shift);
	}

	if (io.KeyCtrl || io.KeySuper)
	{
		if (viewState)
			viewState->blockInput = false;
		processPrimaryShortcuts();
	}
}

void EditorInput::processPrimaryShortcuts()
{
	if (!commands)
		return;
	const bool shift = ImGui::GetIO().KeyShift;

	if (ImGui::IsKeyPressed(ImGuiKey_C, false))
		commands->copy();
	if (ImGui::IsKeyPressed(ImGuiKey_X, false))
		commands->cut();
	if (ImGui::IsKeyPressed(ImGuiKey_V, false))
		commands->paste();
	if (ImGui::IsKeyPressed(ImGuiKey_Z))
	{
		if (shift)
			commands->redo();
		else
			commands->undo();
	}
	if (ImGui::IsKeyPressed(ImGuiKey_S, false))
		commands->save();
}

void EditorInput::processTextInput()
{
	if (!commands || (viewState && viewState->blockInput))
		return;

	processCharacterInput();

	if (ImGui::IsKeyPressed(ImGuiKey_Enter))
	{
		if (suppressNextEnter)
			suppressNextEnter = false;
		else
			commands->insertNewline();
	}

	if (ImGui::IsKeyPressed(ImGuiKey_Backspace))
		commands->deleteLeft(ImGui::GetIO().KeyAlt);
	if (ImGui::IsKeyPressed(ImGuiKey_Delete))
		commands->deleteRight(ImGui::GetIO().KeyAlt);
}

void EditorInput::processCharacterInput()
{
	// ImGui queues Unicode codepoints (ImWchar), not bytes. Encode each to UTF-8
	// and insert through typeText — never cast codepoints to char.
	ImGuiIO &io = ImGui::GetIO();
	std::string inputText;
	inputText.reserve(static_cast<size_t>(io.InputQueueCharacters.Size) * 4);
	for (int n = 0; n < io.InputQueueCharacters.Size; n++)
	{
		const unsigned int cp = static_cast<unsigned int>(io.InputQueueCharacters[n]);
		// Skip C0 controls and DEL; printable ASCII and all non-ASCII codepoints pass.
		if (cp < 32 || cp == 127)
			continue;
		EditorUtils::AppendUtf8Codepoint(inputText, cp);
	}
	io.InputQueueCharacters.clear();
	if (inputText.empty())
		return;

	for (char c : inputText)
	{
		if (c == ' ' || c == '(' || c == ')' || c == '[' || c == ']' || c == '{' ||
			c == '}' || c == ',' || c == ';' || c == ':' || c == '+' || c == '-' ||
			c == '*' || c == '/' || c == '=' || c == '!' || c == '&' || c == '|' ||
			c == '^' || c == '%' || c == '<' || c == '>')
		{
			if (viewState)
				viewState->blockInput = false;
			break;
		}
	}
	commands->typeText(inputText);
}

// ---------------------------------------------------------------------------
// Mouse
// ---------------------------------------------------------------------------

void EditorInput::processMouse()
{
	if (!commands || !state || !viewState || !layout)
		return;

	// Scrollbars sit inside IsWindowHovered but outside the content region.
	// Ignore presses/drags there so grabbing a scrollbar does not select text.
	const ImVec2 winPos = ImGui::GetWindowPos();
	const ImVec2 relMin = ImGui::GetWindowContentRegionMin();
	const ImVec2 relMax = ImGui::GetWindowContentRegionMax();
	const ImVec2 contentMin(winPos.x + relMin.x, winPos.y + relMin.y);
	const ImVec2 contentMax(winPos.x + relMax.x, winPos.y + relMax.y);
	const bool overContent =
		ImGui::IsMouseHoveringRect(contentMin, contentMax, /*clip=*/false);

	if (!overContent)
	{
		// Release may land on the bar after a text-drag; still clear isDragging.
		if (isDragging && ImGui::IsMouseReleased(ImGuiMouseButton_Left))
			handleMouseRelease();
		return;
	}

	// Scrollbar grab keeps an ActiveId while the cursor may still be over content
	// after a fast drag — do not start a new selection in that case.
	if (!isDragging && ImGui::IsAnyItemActive())
		return;

	auto [row, column] = rowColFromMouse();

	if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
	{
		commands->selectWordAt(row, column);
		return;
	}

	if (ImGui::IsMouseClicked(ImGuiMouseButton_Left))
		handleMouseClick(row, column);
	else if (ImGui::IsMouseDragging(ImGuiMouseButton_Left) && isDragging)
		handleMouseDrag(row, column);
	else if (ImGui::IsMouseReleased(ImGuiMouseButton_Left))
		handleMouseRelease();
}

void EditorInput::handleMouseClick(int row, int column)
{
	column = EditorUtils::SnapToUtf8CharBoundary(state->line(row), column);

	if (ImGui::GetIO().KeyShift)
	{
		if (viewState->selectionEmpty())
		{
			anchorRow = viewState->row;
			anchorColumn = viewState->column;
		} else
		{
			const Selection &p = viewState->primary();
			anchorRow = p.anchorRow;
			anchorColumn = p.anchorColumn;
		}
		commands->setSelection(anchorRow, anchorColumn, row, column);
	} else
	{
		commands->setCursor(row, column, false);
		anchorRow = row;
		anchorColumn = column;
	}

	isDragging = true;
	if (projectUndo && state && !state->path.empty())
		projectUndo->updatePendingCursor(state->path, viewState->row, viewState->column);
}

void EditorInput::handleMouseDrag(int row, int column)
{
	column = EditorUtils::SnapToUtf8CharBoundary(state->line(row), column);

	if (anchorRow < 0)
	{
		anchorRow = viewState->row;
		anchorColumn = viewState->column;
	}
	commands->setSelection(anchorRow, anchorColumn, row, column);
}

void EditorInput::handleMouseRelease()
{
	isDragging = false;
	anchorRow = -1;
	anchorColumn = -1;
}

std::pair<int, int> EditorInput::rowColFromMouse() const
{
	if (state->lineCount() <= 0)
		return {0, 0};

	const ImVec2 mouse_pos = ImGui::GetMousePos();
	const int clicked_row = std::clamp(
		static_cast<int>((mouse_pos.y - layout->textPos.y) / layout->lineHeight),
		0,
		state->lineCount() - 1);

	const std::string line = state->line(clicked_row);
	if (line.empty())
		return {clicked_row, 0};

	const float click_x = mouse_pos.x - layout->textPos.x;
	return {clicked_row, EditorUtils::ColumnAtX(line, click_x)};
}
