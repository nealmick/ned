#include "editor_copy_paste.h"
#include "editor.h"
#include <algorithm>

// Global instance
EditorCopyPaste gEditorCopyPaste;

void EditorCopyPaste::processClipboardShortcuts(std::string &text, std::vector<ImVec4> &colors, EditorState &state, bool &text_changed, CursorVisibility &ensure_cursor_visible)
{
    if (ImGui::IsKeyPressed(ImGuiKey_C, false)) {
        gEditorCopyPaste.copySelectedText(text, state);
    }
    if (ImGui::IsKeyPressed(ImGuiKey_X, false)) {
        if (state.selection_start != state.selection_end)
            gEditorCopyPaste.cutSelectedText(text, colors, state, text_changed);
        else
            gEditorCopyPaste.cutWholeLine(text, colors, state, text_changed);
        ensure_cursor_visible.vertical = true;
        ensure_cursor_visible.horizontal = true;
    }
    if (ImGui::IsKeyPressed(ImGuiKey_V, false)) {
        gEditorCopyPaste.pasteText(text, colors, state, text_changed);
        ensure_cursor_visible.vertical = true;
        ensure_cursor_visible.horizontal = true;
    }
}

int EditorCopyPaste::getSelectionStart(const EditorState &state) const { return std::min(state.selection_start, state.selection_end); }

int EditorCopyPaste::getSelectionEnd(const EditorState &state) const { return std::max(state.selection_start, state.selection_end); }

void EditorCopyPaste::copySelectedText(const std::string &text, const EditorState &state)
{
    if (state.selection_start != state.selection_end) {
        int start = getSelectionStart(state);
        int end = getSelectionEnd(state);
        std::string selected_text = text.substr(start, end - start);
        ImGui::SetClipboardText(selected_text.c_str());
    }
}

void EditorCopyPaste::cutSelectedText(std::string &text, std::vector<ImVec4> &colors, EditorState &state, bool &text_changed)
{
    if (state.selection_start != state.selection_end) {
        int start = getSelectionStart(state);
        int end = getSelectionEnd(state);
        std::string selected_text = text.substr(start, end - start);
        ImGui::SetClipboardText(selected_text.c_str());
        text.erase(start, end - start);
        colors.erase(colors.begin() + start, colors.begin() + end);
        state.cursor_pos = start;
        state.selection_start = state.selection_end = start;
        text_changed = true;
    }
}

void EditorCopyPaste::cutWholeLine(std::string &text, std::vector<ImVec4> &colors, EditorState &state, bool &text_changed)
{
    int line = EditorUtils::GetLineFromPosition(state.line_starts, state.cursor_pos);
    int line_start = state.line_starts[line];
    int line_end = (line + 1 < state.line_starts.size()) ? state.line_starts[line + 1] : text.size();

    std::string line_text = text.substr(line_start, line_end - line_start);
    ImGui::SetClipboardText(line_text.c_str());

    text.erase(line_start, line_end - line_start);
    colors.erase(colors.begin() + line_start, colors.begin() + line_end);

    state.cursor_pos = line > 0 ? state.line_starts[line] : 0;
    text_changed = true;
    gEditor.updateLineStarts(text, state.line_starts);
}

void EditorCopyPaste::pasteText(std::string &text, std::vector<ImVec4> &colors, EditorState &state, bool &text_changed)
{
    const char *clipboard_text = ImGui::GetClipboardText();
    if (clipboard_text != nullptr) {
        std::string paste_content = clipboard_text;
        if (!paste_content.empty()) {
            int paste_start = state.cursor_pos;
            int paste_end = paste_start + paste_content.size();
            if (state.selection_start != state.selection_end) {
                int start = getSelectionStart(state);
                int end = getSelectionEnd(state);
                text.replace(start, end - start, paste_content);
                colors.erase(colors.begin() + start, colors.begin() + end);
                colors.insert(colors.begin() + start, paste_content.size(), ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
                paste_start = start;
                paste_end = start + paste_content.size();
            } else {
                text.insert(state.cursor_pos, paste_content);
                colors.insert(colors.begin() + state.cursor_pos, paste_content.size(), ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
            }
            state.cursor_pos = paste_end;
            state.selection_start = state.selection_end = state.cursor_pos;
            text_changed = true;

            // Trigger syntax highlighting for the pasted content
            gEditor.highlightContent(text, colors, paste_start, paste_end);
        }
    }
}