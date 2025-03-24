#include "editor_copy_paste.h"
#include "editor.h"
#include "editor_highlight.h"
#include <algorithm>

// Global instance
EditorCopyPaste gEditorCopyPaste;

void EditorCopyPaste::processClipboardShortcuts(std::string &text, std::vector<ImVec4> &colors, bool &text_changed, CursorVisibility &ensure_cursor_visible)
{
    if (ImGui::IsKeyPressed(ImGuiKey_C, false)) {
        gEditorCopyPaste.copySelectedText(text);
    }
    if (ImGui::IsKeyPressed(ImGuiKey_X, false)) {
        if (editor_state.selection_start != editor_state.selection_end)
            gEditorCopyPaste.cutSelectedText(text, colors, text_changed);
        else
            gEditorCopyPaste.cutWholeLine(text, colors, text_changed);
        ensure_cursor_visible.vertical = true;
        ensure_cursor_visible.horizontal = true;
    }
    if (ImGui::IsKeyPressed(ImGuiKey_V, false)) {
        gEditorCopyPaste.pasteText(text, colors, text_changed);
        ensure_cursor_visible.vertical = true;
        ensure_cursor_visible.horizontal = true;
    }
}

int EditorCopyPaste::getSelectionStart() const { return std::min(editor_state.selection_start, editor_state.selection_end); }

int EditorCopyPaste::getSelectionEnd() const { return std::max(editor_state.selection_start, editor_state.selection_end); }

void EditorCopyPaste::copySelectedText(const std::string &text)
{
    if (editor_state.selection_start != editor_state.selection_end) {
        int start = getSelectionStart();
        int end = getSelectionEnd();
        std::string selected_text = text.substr(start, end - start);
        ImGui::SetClipboardText(selected_text.c_str());
    }
}

void EditorCopyPaste::cutSelectedText(std::string &text, std::vector<ImVec4> &colors, bool &text_changed)
{
    if (editor_state.selection_start != editor_state.selection_end) {
        int start = getSelectionStart();
        int end = getSelectionEnd();
        std::string selected_text = text.substr(start, end - start);
        ImGui::SetClipboardText(selected_text.c_str());
        text.erase(start, end - start);
        colors.erase(colors.begin() + start, colors.begin() + end);
        editor_state.cursor_index = start;
        editor_state.selection_start = editor_state.selection_end = start;
        text_changed = true;
    }
}

void EditorCopyPaste::cutWholeLine(std::string &text, std::vector<ImVec4> &colors, bool &text_changed)
{
    int line = EditorUtils::GetLineFromPosition(editor_state.editor_content_lines, editor_state.cursor_index);
    int line_start = editor_state.editor_content_lines[line];
    int line_end = (line + 1 < editor_state.editor_content_lines.size()) ? editor_state.editor_content_lines[line + 1] : text.size();

    std::string line_text = text.substr(line_start, line_end - line_start);
    ImGui::SetClipboardText(line_text.c_str());

    text.erase(line_start, line_end - line_start);
    colors.erase(colors.begin() + line_start, colors.begin() + line_end);

    editor_state.cursor_index = line > 0 ? editor_state.editor_content_lines[line] : 0;
    text_changed = true;
    gEditor.updateLineStarts();
}

void EditorCopyPaste::pasteText(std::string &text, std::vector<ImVec4> &colors, bool &text_changed)
{
    const char *clipboard_text = ImGui::GetClipboardText();
    if (clipboard_text != nullptr) {
        std::string paste_content = clipboard_text;
        if (!paste_content.empty()) {
            int paste_start = editor_state.cursor_index;
            int paste_end = paste_start + paste_content.size();
            if (editor_state.selection_start != editor_state.selection_end) {
                int start = getSelectionStart();
                int end = getSelectionEnd();
                text.replace(start, end - start, paste_content);
                colors.erase(colors.begin() + start, colors.begin() + end);
                colors.insert(colors.begin() + start, paste_content.size(), ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
                paste_start = start;
                paste_end = start + paste_content.size();
            } else {
                text.insert(editor_state.cursor_index, paste_content);
                colors.insert(colors.begin() + editor_state.cursor_index, paste_content.size(), ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
            }
            editor_state.cursor_index = paste_end;
            editor_state.selection_start = editor_state.selection_end = editor_state.cursor_index;
            text_changed = true;

            // Trigger syntax highlighting for the pasted content
            gEditorHighlight.highlightContent(text, colors, paste_start, paste_end);
        }
    }
}