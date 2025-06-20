/*
	File: editor.cpp
	Description: Main editor coordinator that manages the text editing
   interface.

	The editor follows a clear execution flow:
	1. Initialization & Setup - Prepare the editor window, initialize data
   structures
	2. Content Analysis - Parse text to identify lines and compute layout
   metrics
	3. Input Processing - Handle keyboard and mouse input to modify text
	4. Scroll Management - Handle scrolling and cursor visibility
	5. Rendering - Draw the text, cursor, selection, and UI elements
	6. Cleanup - Finalize the frame and return controlq
*/

#include "editor.h"
#include "editor_bookmarks.h"
#include "editor_copy_paste.h"
#include "editor_cursor.h"
#include "editor_highlight.h"
#include "editor_keyboard.h"
#include "editor_line_jump.h"
#include "editor_line_numbers.h"
#include "editor_mouse.h"
#include "editor_render.h"
#include "editor_selection.h"
#include "editor_utils.h"

#include "../ai/ai_tab.h"
#include "../files/file_finder.h"
#include "../files/files.h"
#include "../lsp/lsp_autocomplete.h"
#include "../util/settings.h"

#include <algorithm>
#include <cmath>
#include <iostream>

Editor gEditor;
EditorState editor_state;

void Editor::textEditor()
{
	// process auto complete before edtior input....
	gLSPAutocomplete.renderCompletions();
	gAITab.update();

	setupEditorDisplay();

	processEditorInput();

	gEditorRender.renderEditorFrame();
	/*
	std::cout << "---- Multi-Selection Ranges (After Movement) ----" << std::endl;
	for (size_t i = 0; i < editor_state.multi_selections.size(); ++i)
	{
		if (i < editor_state.multi_cursor_indices.size())
		{
			const auto &selection = editor_state.multi_selections[i];
			std::cout << "  Cursor " << i << " (idx "
					  << (i < editor_state.multi_cursor_indices.size()
							  ? editor_state.multi_cursor_indices[i]
							  : -1)
					  << "): "
					  << "Sel Start = " << selection.start_index
					  << ", Sel End = " << selection.end_index << std::endl;
		}
	}
	std::cout << "--------------------------------------------------" << std::endl;
	*/
}

void Editor::setupEditorDisplay()
{
	gEditorRender.validateAndResizeColors();

	gEditorCursor.updateBlinkTime();

	gEditorRender.setupEditorWindow("##editor");

	editor_state.line_numbers_pos = gEditorLineNumbers.createLineNumbersPanel();

	updateLineStarts();
	editor_state.total_height = editor_state.line_height * editor_state.editor_content_lines.size();

	float remaining_width = editor_state.size.x - editor_state.line_number_width;
	float content_width = calculateTextWidth() + ImGui::GetFontSize() * 10.0f;
	float content_height = editor_state.editor_content_lines.size() * editor_state.line_height;

	gEditorRender.beginTextEditorChild("##editor", remaining_width, content_width, content_height);
}

void Editor::processEditorInput()
{
	gEditorKeyboard.processTextEditorInput();

	gEditorMouse.handleContextMenu();

	gEditorScroll.processMouseWheelForEditor();

	gEditorScroll.adjustScrollForCursorVisibility();

	gEditorScroll.updateScrollAnimation();

	ImGui::SetScrollY(editor_state.current_scroll_y);
	ImGui::SetScrollX(editor_state.current_scroll_x);
}

void Editor::updateLineStarts()
{
	if (editor_state.fileContent == editor_state.cached_text)
	{
		return;
	}

	editor_state.cached_text = editor_state.fileContent;
	editor_state.editor_content_lines.clear();
	editor_state.line_widths.clear();

	editor_state.editor_content_lines.reserve(
		editor_state.fileContent.size() / 40); // Heuristic: assume average line length of 40 chars
	editor_state.editor_content_lines.push_back(0);

	size_t pos = 0;
	while ((pos = editor_state.fileContent.find('\n', pos)) != std::string::npos)
	{
		editor_state.editor_content_lines.push_back(pos +
													1); // Position after the newline character
		++pos;
	}

	for (size_t i = 0; i < editor_state.editor_content_lines.size(); ++i)
	{
		int start = editor_state.editor_content_lines[i];
		int end = (i + 1 < editor_state.editor_content_lines.size())
					  ? editor_state.editor_content_lines[i + 1] - 1
					  : editor_state.fileContent.size();
		float width = ImGui::CalcTextSize(editor_state.fileContent.c_str() + start,
										  editor_state.fileContent.c_str() + end)
						  .x;
		editor_state.line_widths.push_back(width);
	}
}

int Editor::getLineFromPos(int pos)
{
	auto it = std::upper_bound(editor_state.editor_content_lines.begin(),
							   editor_state.editor_content_lines.end(),
							   pos);
	return std::distance(editor_state.editor_content_lines.begin(), it) - 1;
}


float Editor::calculateTextWidth()
{
    float max_width = 0.0f;
    ImFont* font = ImGui::GetFont();
    
    for (size_t i = 0; i < editor_state.editor_content_lines.size(); ++i)
    {
        size_t start = editor_state.editor_content_lines[i];
        size_t end = (i + 1 < editor_state.editor_content_lines.size()) 
                     ? editor_state.editor_content_lines[i + 1] 
                     : editor_state.fileContent.size();
        
        if (end > start) {
            std::string line = editor_state.fileContent.substr(start, end - start);
            float width = font->CalcTextSizeA(font->FontSize, FLT_MAX, 0.0f, line.c_str()).x;
            
            // Apply compensation based on line length and font size
            float compensation = (line.length() * 0.1f) * (24.0f / font->FontSize) *10;
            width += compensation;
            
            // Add extra safety margin
            width *= 1.01f; // 1% extra
            
            if (width > max_width) max_width = width;
        }
    }
    
    // Add generous padding (15% or 150px, whichever is larger)
    float padding = std::max(150.0f, max_width * 0.15f);
    return max_width + padding;
}
