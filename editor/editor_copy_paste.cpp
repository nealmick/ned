#include "editor_copy_paste.h"
#include "editor.h"
#include "editor_highlight.h"
#include <algorithm>

// Global instance
EditorCopyPaste gEditorCopyPaste;

void EditorCopyPaste::processClipboardShortcuts()
{
	if (ImGui::IsKeyPressed(ImGuiKey_C, false))
	{
		gEditorCopyPaste.copySelectedText(editor_state.fileContent);
	}
	if (ImGui::IsKeyPressed(ImGuiKey_X, false))
	{
		if (editor_state.selection_start != editor_state.selection_end)
			gEditorCopyPaste.cutSelectedText();
		else
			gEditorCopyPaste.cutWholeLine();
		editor_state.ensure_cursor_visible.vertical = true;
		editor_state.ensure_cursor_visible.horizontal = true;
		editor_state.selection_active = false;
		editor_state.selection_start = editor_state.cursor_index;
		editor_state.selection_end = editor_state.cursor_index;
	}
	if (ImGui::IsKeyPressed(ImGuiKey_V, false))
	{
		gEditorCopyPaste.pasteText();
		editor_state.ensure_cursor_visible.vertical = true;
		editor_state.ensure_cursor_visible.horizontal = true;
	}
}

int EditorCopyPaste::getSelectionStart() const
{
	return std::min(editor_state.selection_start, editor_state.selection_end);
}

int EditorCopyPaste::getSelectionEnd() const
{
	return std::max(editor_state.selection_start, editor_state.selection_end);
}

void EditorCopyPaste::copySelectedText(const std::string &text)
{
	if (editor_state.selection_start != editor_state.selection_end)
	{
		int start = getSelectionStart();
		int end = getSelectionEnd();
		std::string selected_text = text.substr(start, end - start);
		ImGui::SetClipboardText(selected_text.c_str());
	}
}

void EditorCopyPaste::cutSelectedText()
{
	if (editor_state.selection_start != editor_state.selection_end)
	{
		int start = getSelectionStart();
		int end = getSelectionEnd();
		std::string selected_text = editor_state.fileContent.substr(start, end - start);
		ImGui::SetClipboardText(selected_text.c_str());
		editor_state.fileContent.erase(start, end - start);
		editor_state.fileColors.erase(editor_state.fileColors.begin() + start,
									  editor_state.fileColors.begin() + end);
		editor_state.cursor_index = start;
		editor_state.selection_start = editor_state.selection_end = start;
		editor_state.text_changed = true;
	}
}

void EditorCopyPaste::cutWholeLine()
{
	int line = EditorUtils::GetLineFromPosition(editor_state.editor_content_lines,
												editor_state.cursor_index);
	int line_start = editor_state.editor_content_lines[line];
	int line_end = (line + 1 < editor_state.editor_content_lines.size())
					   ? editor_state.editor_content_lines[line + 1]
					   : editor_state.fileContent.size();

	std::string line_text = editor_state.fileContent.substr(line_start, line_end - line_start);
	ImGui::SetClipboardText(line_text.c_str());

	editor_state.fileContent.erase(line_start, line_end - line_start);
	editor_state.fileColors.erase(editor_state.fileColors.begin() + line_start,
								  editor_state.fileColors.begin() + line_end);

	editor_state.cursor_index = line > 0 ? editor_state.editor_content_lines[line] : 0;
	editor_state.text_changed = true;
	gEditor.updateLineStarts();
}

void EditorCopyPaste::pasteText()
{
	const char *clipboard_text = ImGui::GetClipboardText();
	if (clipboard_text != nullptr)
	{
		std::string paste_content = clipboard_text;
		if (!paste_content.empty())
		{
			int paste_start = editor_state.cursor_index;
			int paste_end = paste_start + paste_content.size();
			if (editor_state.selection_start != editor_state.selection_end)
			{
				int start = getSelectionStart();
				int end = getSelectionEnd();
				editor_state.fileContent.replace(start, end - start, paste_content);
				editor_state.fileColors.erase(editor_state.fileColors.begin() + start,
											  editor_state.fileColors.begin() + end);
				editor_state.fileColors.insert(editor_state.fileColors.begin() + start,
											   paste_content.size(),
											   ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
				paste_start = start;
				paste_end = start + paste_content.size();
			} else
			{
				editor_state.fileContent.insert(editor_state.cursor_index, paste_content);
				editor_state.fileColors.insert(editor_state.fileColors.begin() +
												   editor_state.cursor_index,
											   paste_content.size(),
											   ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
			}
			editor_state.cursor_index = paste_end;
			editor_state.selection_start = editor_state.selection_end = editor_state.cursor_index;
			editor_state.text_changed = true;

			// Trigger syntax highlighting for the pasted content
			gEditorHighlight.highlightContent();
		}
	}
}