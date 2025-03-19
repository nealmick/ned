/*
    File: editor_render.cpp
    Description: Handles rendering for the editor, separating rendering logic from the main editor
   class.
*/

#include "editor_render.h"
#include "../files/file_finder.h"
#include "../lsp/lsp.h"
#include "editor.h"
#include "editor_bookmarks.h"
#include "editor_cursor.h"
#include "editor_highlight.h"
#include "editor_line_jump.h"
#include "editor_line_numbers.h"
#include "editor_scroll.h"
#include "editor_selection.h"
#include "editor_utils.h"

#include <iostream>

EditorRender gEditorRender;

//==============================================================================
// Main Render Pipeline
//==============================================================================

void EditorRender::renderEditorFrame(std::string &text, std::vector<ImVec4> &colors, EditorState &editor_state, ImVec2 &text_pos, float line_height, ImVec2 &line_numbers_pos, float line_number_width, ImVec2 &size, float total_height, float editor_top_margin)
{
    // Render main editor content (text, selection, cursor)
    gEditorRender.renderEditorContent(text, colors, editor_state, line_height, text_pos);

    // Render File Finder window
    gFileFinder.renderWindow();

    // Set cursor position for remaining content
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + total_height + editor_top_margin);

    // Get final scroll positions from ImGui
    float scrollY = ImGui::GetScrollY();
    float scrollX = ImGui::GetScrollX();

    // Update scroll manager with final positions
    gEditorScroll.setScrollPosition(ImVec2(scrollX, scrollY));
    gEditorScroll.setScrollX(scrollX);

    if (gEditorLSP.hasDefinitionOptions()) {
        gEditorLSP.renderDefinitionOptions(editor_state);
    }

    // End the editor child window
    ImGui::EndChild();
    ImGui::PopStyleColor(4);
    ImGui::PopStyleVar(4);

    // Render line numbers with proper clipping
    ImGui::PushClipRect(line_numbers_pos, ImVec2(line_numbers_pos.x + line_number_width, line_numbers_pos.y + size.y - editor_top_margin), true);

    gEditorLineNumbers.renderLineNumbers(line_numbers_pos, line_number_width, line_height, editor_state.editor_content_lines.size(), gEditorScroll.getScrollPosition().y, size.y - editor_top_margin, editor_state, editor_state.cursor_blink_time);

    // Cleanup
    ImGui::PopClipRect();
    ImGui::EndGroup();
    ImGui::PopID();
}

bool EditorRender::validateAndResizeColors(std::string &text, std::vector<ImVec4> &colors)
{
    if (colors.size() != text.size()) {
        std::cout << "Warning: colors vector size (" << colors.size() << ") does not match text size (" << text.size() << "). Resizing." << std::endl;
        colors.resize(text.size(), ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
        return true;
    }
    return false;
}

void EditorRender::setupEditorWindow(const char *label, ImVec2 &size, float &line_number_width, float &line_height, float &editor_top_margin, float &text_left_margin)
{
    size = ImGui::GetContentRegionAvail();
    line_number_width = ImGui::CalcTextSize("0").x * 4 + 8.0f;
    line_height = ImGui::GetTextLineHeight();
    editor_top_margin = 2.0f;
    text_left_margin = 7.0f;
    ImGui::PushID(label);
}

ImVec2 EditorRender::renderLineNumbersPanel(float line_number_width, float editor_top_margin) { return gEditorLineNumbers.createLineNumbersPanel(line_number_width, editor_top_margin); }

void EditorRender::beginTextEditorChild(const char *label, float remaining_width, float content_width, float content_height, float &current_scroll_y, float &current_scroll_x, ImVec2 &text_pos, float editor_top_margin, float text_left_margin, EditorState &editor_state)
{
    ImGui::PushStyleVar(ImGuiStyleVar_ChildBorderSize, 1.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.5f, 0.5f, 0.5f, 0.5f));
    ImGui::PushStyleColor(ImGuiCol_ScrollbarBg, ImVec4(0.05f, 0.05f, 0.05f, 0.0f));
    ImGui::PushStyleColor(ImGuiCol_ScrollbarGrabHovered, ImVec4(0.6f, 0.6f, 0.6f, 0.7f));
    ImGui::PushStyleColor(ImGuiCol_ScrollbarGrabActive, ImVec4(0.8f, 0.8f, 0.8f, 0.9f));
    ImGui::PushStyleVar(ImGuiStyleVar_ScrollbarSize, 12.0f);
    ImGui::SetNextWindowContentSize(ImVec2(content_width, content_height));
    ImGui::BeginChild(label, ImVec2(remaining_width, ImGui::GetContentRegionAvail().y), false, ImGuiWindowFlags_HorizontalScrollbar);

    // Set keyboard focus to this child window if appropriate.
    if (!gBookmarks.isWindowOpen() && !editor_state.block_input) {
        if (ImGui::IsWindowAppearing() || (ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows) && !ImGui::IsAnyItemActive())) {
            ImGui::SetKeyboardFocusHere();
        }
    }

    // Get current scroll positions from ImGui
    current_scroll_y = ImGui::GetScrollY();
    current_scroll_x = ImGui::GetScrollX();

    // Update EditorScroll with the latest scroll values
    gEditorScroll.setScrollPosition(ImVec2(current_scroll_x, current_scroll_y));
    gEditorScroll.setScrollX(current_scroll_x);

    text_pos = ImGui::GetCursorScreenPos();
    text_pos.y += editor_top_margin;
    text_pos.x += text_left_margin;
}

void EditorRender::renderEditorContent(const std::string &text, const std::vector<ImVec4> &colors, EditorState &editor_state, float line_height, const ImVec2 &text_pos)
{
    gLineJump.handleLineJumpInput(editor_state);
    gLineJump.renderLineJumpWindow(editor_state);

    // Render the text (with selection) using our character-by-character function
    renderText(ImGui::GetWindowDrawList(), text_pos, text, colors, editor_state, line_height);

    // Compute the cursor's line by finding which line the cursor is on
    int cursor_line = gEditor.getLineFromPos(editor_state.editor_content_lines, editor_state.cursor_index);

    // Calculate cursor x position character-by-character using EditorCursor
    float cursor_x = gEditorCursor.getCursorXPosition(text_pos, text, editor_state.cursor_index);

    ImVec2 cursor_screen_pos = text_pos;
    cursor_screen_pos.x = cursor_x;
    cursor_screen_pos.y = text_pos.y + cursor_line * line_height;

    // Draw the cursor using EditorCursor
    gEditorCursor.renderCursor(ImGui::GetWindowDrawList(), cursor_screen_pos, line_height, editor_state.cursor_blink_time);
}

void EditorRender::renderText(ImDrawList *drawList, const ImVec2 &pos, const std::string &text, const std::vector<ImVec4> &colors, const EditorState &state, float line_height)
{
    ImVec2 text_pos = pos;
    int sel_start = gEditorSelection.getSelectionStart(state);
    int sel_end = gEditorSelection.getSelectionEnd(state);

    // Calculate visible range - use gEditorScroll instead of direct access
    float scroll_y = gEditorScroll.getScrollPosition().y;
    float window_height = ImGui::GetWindowHeight();
    int start_line = static_cast<int>(scroll_y / line_height);
    int end_line = start_line + static_cast<int>(window_height / line_height) + 1;

    int current_line = 0;
    for (size_t i = 0; i < text.size(); i++) {
        // Safety check
        if (i >= colors.size()) {
            std::cerr << "Error: Color index out of bounds in renderTextWithSelection" << std::endl;
            break;
        }

        // Handle newline: update line count and reset x
        if (text[i] == '\n') {
            current_line++;
            if (current_line > end_line)
                break;
            text_pos.x = pos.x;
            text_pos.y += line_height;
            continue;
        }

        // Skip lines above visible area
        if (current_line < start_line) {
            while (i < text.size() && text[i] != '\n')
                i++;
            i--; // Adjust for the outer loop's increment
            continue;
        }
        if (current_line > end_line)
            break;

        // Check if this character is selected
        bool is_selected = (i >= sel_start && i < sel_end);
        if (is_selected) {
            // Draw selection highlight
            float char_width = ImGui::CalcTextSize(&text[i], &text[i + 1]).x;
            ImVec2 sel_start_pos = text_pos;
            ImVec2 sel_end_pos = ImVec2(sel_start_pos.x + char_width, sel_start_pos.y + line_height);
            drawList->AddRectFilled(sel_start_pos, sel_end_pos, ImGui::ColorConvertFloat4ToU32(ImVec4(1.0f, 0.1f, 0.7f, 0.3f)));
        }

        // Draw individual character
        char buf[2] = {text[i], '\0'};
        drawList->AddText(text_pos, ImGui::ColorConvertFloat4ToU32(colors[i]), buf);
        text_pos.x += ImGui::CalcTextSize(buf).x;
    }
}