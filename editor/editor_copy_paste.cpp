#include "editor_copy_paste.h"
#include "../ai/ai_tab.h"
#include "../files/files.h"

#include "editor.h"
#include "editor_highlight.h"
#include "editor_tree_sitter.h"
#include <algorithm>

// Global instance
EditorCopyPaste gEditorCopyPaste;

void EditorCopyPaste::processClipboardShortcuts()
{
	// If input is blocked (e.g., by agent history text selection), don't
	// process editor shortcuts
	if (editor_state.block_input)
	{
		return;
	}

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
		// Save the state before making any changes
		std::string beforeContent = editor_state.fileContent;
		int beforeCursor = editor_state.cursor_index;
		int cutStart = getSelectionStart();
		int cutEnd = getSelectionEnd();

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

		// Update line starts immediately to prevent visual glitch
		gEditor.updateLineStarts();
	}
}

void EditorCopyPaste::cutWholeLine()
{
	gAITab.cancel_request();
	gAITab.dismiss_completion();
	gFileExplorer.addUndoState();
	// Save the state before making any changes
	std::string beforeContent = editor_state.fileContent;
	int beforeCursor = editor_state.cursor_index;

	int line = EditorUtils::GetLineFromPosition(editor_state.editor_content_lines,
												editor_state.cursor_index);
	int line_start = editor_state.editor_content_lines[line];
	int line_end = (line + 1 < editor_state.editor_content_lines.size())
					   ? editor_state.editor_content_lines[line + 1]
					   : editor_state.fileContent.size();

	std::string line_text =
		editor_state.fileContent.substr(line_start, line_end - line_start);
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
	gAITab.cancel_request();
	gAITab.dismiss_completion();

	// Save the state before making any changes
	std::string beforeContent = editor_state.fileContent;
	int beforeCursor = editor_state.cursor_index;

	const char *clipboard_text = ImGui::GetClipboardText();
	if (clipboard_text != nullptr)
	{
		std::string paste_content = clipboard_text;
		if (!paste_content.empty())
		{
			// Handle indentation conversion
			bool fileUsesTabs = checkIndentationType();
			if (fileUsesTabs)
			{
				// File uses tabs, convert spaces to tabs in paste content
				paste_content = convertSpacesToTabs(paste_content);
			} else
			{
				// File uses spaces, convert tabs to spaces in paste content
				paste_content = convertTabsToSpaces(paste_content);
			}

			// Get the proper default text color from the theme
			TreeSitter::updateThemeColors();
			ImVec4 defaultColor = TreeSitter::cachedColors.text;

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
											   defaultColor);
				paste_start = start;
				paste_end = start + paste_content.size();
			} else
			{
				editor_state.fileContent.insert(editor_state.cursor_index, paste_content);
				editor_state.fileColors.insert(editor_state.fileColors.begin() +
												   editor_state.cursor_index,
											   paste_content.size(),
											   defaultColor);
			}
			editor_state.cursor_index = paste_end;
			editor_state.selection_start = editor_state.selection_end =
				editor_state.cursor_index;
			editor_state.text_changed = true;

			// Update line starts immediately to prevent visual glitch
			gEditor.updateLineStarts();

			// Trigger syntax highlighting for the pasted content
			gEditorHighlight.highlightContent();
		}
	}
}

bool EditorCopyPaste::checkIndentationType() const
{
	// Check if the file contains any tabs
	// Returns true for tabs, false for spaces
	return editor_state.fileContent.find('\t') != std::string::npos;
}

std::string EditorCopyPaste::convertSpacesToTabs(const std::string &text) const
{
	std::string result;
	result.reserve(text.size()); // Pre-allocate space for efficiency

	for (size_t i = 0; i < text.size(); ++i)
	{
		if (text[i] == ' ')
		{
			// Check if we have 4 consecutive spaces
			int spaceCount = 0;
			while (i < text.size() && text[i] == ' ' && spaceCount < 4)
			{
				spaceCount++;
				i++;
			}

			if (spaceCount == 4)
			{
				result += '\t';
			} else
			{
				// Add back the spaces we consumed
				for (int j = 0; j < spaceCount; ++j)
				{
					result += ' ';
				}
			}

			// Adjust i since we already incremented it in the while loop
			i--;
		} else
		{
			result += text[i];
		}
	}

	return result;
}

std::string EditorCopyPaste::convertTabsToSpaces(const std::string &text) const
{
	std::string result;
	result.reserve(text.size() * 4); // Pre-allocate space for efficiency

	for (char c : text)
	{
		if (c == '\t')
		{
			result += "    "; // 4 spaces
		} else
		{
			result += c;
		}
	}

	return result;
}
