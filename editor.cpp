#include "editor.h"
#include "settings.h"
#include <iostream>
#include <algorithm>
#include "files.h"

int GetCharIndexFromCoords(const std::string& text, const ImVec2& click_pos, const ImVec2& text_start_pos, const std::vector<int>& line_starts, float line_height);

void HandleCursorMovement(const std::string& text, EditorState& state, const ImVec2& text_pos, float line_height, float window_height, float window_width);

Editor gEditor;




void UpdateLineStarts(const std::string& text, std::vector<int>& line_starts) {
    line_starts.clear();
    line_starts.reserve(text.size() / 40);  // Estimate average line length
    line_starts.push_back(0);
    size_t pos = 0;
    while ((pos = text.find('\n', pos)) != std::string::npos) {
        line_starts.push_back(pos + 1);
        ++pos;
    }
}

int GetLineFromPos(const std::vector<int>& line_starts, int pos) {
    auto it = std::upper_bound(line_starts.begin(), line_starts.end(), pos);
    return std::distance(line_starts.begin(), it) - 1;
}

// Selection and cursor movement functions
void StartSelection(EditorState& state) {
    state.is_selecting = true;
    state.selection_start = state.cursor_pos;
    state.selection_end = state.cursor_pos;
}

void UpdateSelection(EditorState& state) {
    if (state.is_selecting) {
        state.selection_end = state.cursor_pos;
    }
}

void EndSelection(EditorState& state) {
    state.is_selecting = false;
}

int GetSelectionStart(const EditorState& state) {
    return std::min(state.selection_start, state.selection_end);
}

int GetSelectionEnd(const EditorState& state) {
    return std::max(state.selection_start, state.selection_end);
}


float GetCursorYPosition(const EditorState& state, float line_height) {
    int cursor_line = GetLineFromPos(state.line_starts, state.cursor_pos);
    return cursor_line * line_height;
}


// Copy, cut, and paste functions
void CopySelectedText(const std::string& text, const EditorState& state) {
    if (state.selection_start != state.selection_end) {
        int start = GetSelectionStart(state);
        int end = GetSelectionEnd(state);
        std::string selected_text = text.substr(start, end - start);
        ImGui::SetClipboardText(selected_text.c_str());
    }
}

float CalculateCursorXPosition(const ImVec2& text_pos, const std::string& text, int cursor_pos) {
    float x = text_pos.x;
    for (int i = 0; i < cursor_pos; i++) {
        if (text[i] == '\n') {
            x = text_pos.x;
        } else {
            x += ImGui::CalcTextSize(&text[i], &text[i + 1]).x;
        }
    }
    return x;
}
ScrollChange EnsureCursorVisible(const ImVec2& text_pos, const std::string& text, EditorState& state, float line_height, float window_height, float window_width) {
    float cursor_y = (GetLineFromPos(state.line_starts, state.cursor_pos) * line_height);
    float cursor_x = CalculateCursorXPosition(text_pos, text, state.cursor_pos);
    float scroll_y = ImGui::GetScrollY();
    float scroll_x = ImGui::GetScrollX();
    float visible_start_x = text_pos.x + scroll_x;  // Use text_pos.x as the reference point
    float visible_end_x = visible_start_x + window_width;
    ScrollChange changed = {false, false};

    // Vertical scrolling
    if (cursor_y < scroll_y + line_height) {  // Start scrolling up one line earlier
        state.scroll_pos.y = std::max(0.0f, cursor_y - line_height);
        changed.vertical = true;
    } else if (cursor_y > scroll_y + window_height - line_height * 2) {  // Start scrolling down one line earlier
        state.scroll_pos.y = cursor_y - window_height + line_height * 2;
        changed.vertical = true;
    }

    // Horizontal scrolling
    float buffer = ImGui::GetFontSize() * 3.0f;
    float target_scroll_x = scroll_x;

    if (cursor_x < visible_start_x + buffer) {
        // Scrolling left
        target_scroll_x = cursor_x - text_pos.x - buffer;
        changed.horizontal = true;
    } else if (cursor_x > visible_end_x - buffer) {
        // Scrolling right
        target_scroll_x = cursor_x - window_width + buffer - text_pos.x;
        changed.horizontal = true;
    }

    // Ensure we don't scroll past the start of the text
    target_scroll_x = std::max(0.0f, target_scroll_x);

    // Apply scrolling with increased speed
    state.scroll_x = state.scroll_x + (target_scroll_x - state.scroll_x);

    return changed;
}


void SelectAllText(EditorState& state, const std::string& text) {
    state.is_selecting = true;
    state.selection_start = 0;
    state.cursor_pos = text.size(); // Move cursor to the end of the file
    state.selection_end = text.size();
}
void CutSelectedText(std::string& text, std::vector<ImVec4>& colors, EditorState& state, bool& text_changed) {
    if (state.selection_start != state.selection_end) {
        int start = GetSelectionStart(state);
        int end = GetSelectionEnd(state);
        std::string selected_text = text.substr(start, end - start);
        ImGui::SetClipboardText(selected_text.c_str());
        text.erase(start, end - start);
        colors.erase(colors.begin() + start, colors.begin() + end);
        state.cursor_pos = start;
        state.selection_start = state.selection_end = start;
        text_changed = true;
    }
}

void CutWholeLine(std::string& text, std::vector<ImVec4>& colors, EditorState& state, bool& text_changed) {
    int line = GetLineFromPos(state.line_starts, state.cursor_pos);
    int line_start = state.line_starts[line];
    int line_end = (line + 1 < state.line_starts.size()) ? state.line_starts[line + 1] : text.size();
    
    std::string line_text = text.substr(line_start, line_end - line_start);
    ImGui::SetClipboardText(line_text.c_str());
    
    text.erase(line_start, line_end - line_start);
    colors.erase(colors.begin() + line_start, colors.begin() + line_end);
    
    state.cursor_pos = line > 0 ? state.line_starts[line - 1] : 0;
    text_changed = true;
    UpdateLineStarts(text, state.line_starts);
}

void PasteText(std::string& text, std::vector<ImVec4>& colors, EditorState& state, bool& text_changed) {
    const char* clipboard_text = ImGui::GetClipboardText();
    if (clipboard_text != nullptr) {
        std::string paste_content = clipboard_text;
        if (!paste_content.empty()) {
            int paste_start = state.cursor_pos;
            int paste_end = paste_start + paste_content.size();
            if (state.selection_start != state.selection_end) {
                int start = GetSelectionStart(state);
                int end = GetSelectionEnd(state);
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


void HandleMouseInput(const std::string& text, EditorState& state, const ImVec2& text_start_pos, float line_height) {
    static bool is_dragging = false;
    static int drag_start_pos = -1;

    ImVec2 mouse_pos = ImGui::GetMousePos();
    int char_index = GetCharIndexFromCoords(text, mouse_pos, text_start_pos, state.line_starts, line_height);

    if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
        if (ImGui::GetIO().KeyShift) {
            // Shift-click selection
            if (!state.is_selecting) {
                state.selection_start = state.cursor_pos;
                state.is_selecting = true;
            }
            state.selection_end = char_index;
        } else {
            // Regular click
            state.cursor_pos = char_index;
            state.selection_start = state.selection_end = char_index;
            state.is_selecting = false;
        }
        is_dragging = true;
        drag_start_pos = char_index;
    } else if (ImGui::IsMouseDragging(ImGuiMouseButton_Left) && is_dragging) {
        // Click-and-drag selection
        state.is_selecting = true;
        state.selection_start = drag_start_pos;
        state.selection_end = char_index;
        state.cursor_pos = char_index;
    } else if (ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
        is_dragging = false;
        drag_start_pos = -1;
    }
}

void MoveCursorLeft(EditorState& state) {
    if (state.cursor_pos > 0) {
        state.cursor_pos--;
    }
}

void MoveCursorRight(const std::string& text, EditorState& state) {
    if (state.cursor_pos < text.size()) {
        state.cursor_pos++;
    }
}

void MoveCursorUp(const std::string& text, EditorState& state, float line_height, float window_height) {
    int current_line = GetLineFromPos(state.line_starts, state.cursor_pos);
    if (current_line > 0) {
        int current_column = state.cursor_pos - state.line_starts[current_line];
        state.cursor_pos = std::min(state.line_starts[current_line - 1] + current_column, 
                                    state.line_starts[current_line] - 1);
        state.scroll_pos.y = std::max(0.0f, state.scroll_pos.y - line_height);
    }
}

void MoveCursorDown(const std::string& text, EditorState& state, float line_height, float window_height) {
    int current_line = GetLineFromPos(state.line_starts, state.cursor_pos);
    if (current_line < state.line_starts.size() - 1) {
        int current_column = state.cursor_pos - state.line_starts[current_line];
        state.cursor_pos = std::min(state.line_starts[current_line + 1] + current_column, 
                                    static_cast<int>(text.size()));
        state.scroll_pos.y = state.scroll_pos.y + line_height;
    }
}

void MoveCursorVertically(std::string& text, EditorState& state, int line_delta) {
    int current_line = GetLineFromPos(state.line_starts, state.cursor_pos);
    int target_line = std::max(0, std::min(static_cast<int>(state.line_starts.size()) - 1, current_line + line_delta));
    
    // Calculate the current column (horizontal position)
    int current_column = state.cursor_pos - state.line_starts[current_line];
    
    // Set the new cursor position
    int new_line_start = state.line_starts[target_line];
    int new_line_end = (target_line + 1 < state.line_starts.size()) ? state.line_starts[target_line + 1] - 1 : text.size();
    
    // Try to maintain the same column, but don't go past the end of the new line
    state.cursor_pos = std::min(new_line_start + current_column, new_line_end);
}


void HandleCursorMovement(const std::string& text, EditorState& state, const ImVec2& text_pos, float line_height, float window_height, float window_width) {
    float visible_start_y = ImGui::GetScrollY();
    float visible_end_y = visible_start_y + window_height;
    float visible_start_x = ImGui::GetScrollX();
    float visible_end_x = visible_start_x + window_width;

    bool shift_pressed = ImGui::GetIO().KeyShift;
    state.current_line = GetLineFromPos(state.line_starts, state.cursor_pos);
    // Start a new selection only if Shift is pressed and we're not already selecting
    if (shift_pressed && !state.is_selecting) {
        state.is_selecting = true;
        state.selection_start = state.cursor_pos;
    }

    // Clear selection only if a movement key is pressed without Shift
    if (!shift_pressed && (ImGui::IsKeyPressed(ImGuiKey_UpArrow) || ImGui::IsKeyPressed(ImGuiKey_DownArrow) ||
        ImGui::IsKeyPressed(ImGuiKey_LeftArrow) || ImGui::IsKeyPressed(ImGuiKey_RightArrow))) {
        state.is_selecting = false;
        state.selection_start = state.selection_end = state.cursor_pos;
    }

    if (ImGui::IsKeyPressed(ImGuiKey_UpArrow)) {
        MoveCursorUp(text, state, line_height, window_height);
    }
    if (ImGui::IsKeyPressed(ImGuiKey_DownArrow)) {
        MoveCursorDown(text, state, line_height, window_height);
    }
    if (ImGui::IsKeyPressed(ImGuiKey_LeftArrow)) {
        MoveCursorLeft(state);
    }
    if (ImGui::IsKeyPressed(ImGuiKey_RightArrow)) {
        MoveCursorRight(text, state);
    }

    if (state.is_selecting) {
        state.selection_end = state.cursor_pos;
    }

    // Ensure cursor stays in view after movement
    float cursor_y = text_pos.y + (GetLineFromPos(state.line_starts, state.cursor_pos) * line_height);
    float cursor_x = CalculateCursorXPosition(text_pos, text, state.cursor_pos);

    if (cursor_y < visible_start_y) {
        state.scroll_pos.y = cursor_y - text_pos.y;
    } else if (cursor_y + line_height > visible_end_y) {
        state.scroll_pos.y = cursor_y + line_height - window_height;
    }   

    if (cursor_x < visible_start_x) {
        state.scroll_x = cursor_x - text_pos.x;
    } else if (cursor_x > visible_end_x) {
        state.scroll_x = cursor_x - window_width + ImGui::GetFontSize();
    }
}

// Text input handling
void HandleCharacterInput(std::string& text, std::vector<ImVec4>& colors, EditorState& state, bool& text_changed, int& input_start, int& input_end) {
    ImGuiIO& io = ImGui::GetIO();
    std::string input;
    input.reserve(io.InputQueueCharacters.Size);
    for (int n = 0; n < io.InputQueueCharacters.Size; n++) {
        char c = static_cast<char>(io.InputQueueCharacters[n]);
        if (c != 0 && c >= 32) {
            input += c;
        }
    }
    if (!input.empty()) {
        // Clear any existing selection
        if (state.selection_start != state.selection_end) {
            int start = GetSelectionStart(state);
            int end = GetSelectionEnd(state);
            text.erase(start, end - start);
            colors.erase(colors.begin() + start, colors.begin() + end);
            state.cursor_pos = start;
        }
        
        // Insert new text
        text.insert(state.cursor_pos, input);
        colors.insert(colors.begin() + state.cursor_pos, input.size(), ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
        state.cursor_pos += input.size();
        
        // Reset selection state
        state.selection_start = state.selection_end = state.cursor_pos;
        state.is_selecting = false;  // Add this line to ensure selection mode is turned off
        
        text_changed = true;
        input_end = state.cursor_pos;
    }
}

void HandleEnterKey(std::string& text, std::vector<ImVec4>& colors, EditorState& state, bool& text_changed, int& input_end) {
    if (ImGui::IsKeyPressed(ImGuiKey_Enter)) {
        text.insert(state.cursor_pos, 1, '\n');
        colors.insert(colors.begin() + state.cursor_pos, 1, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
        state.cursor_pos++;
        
        // Reset selection state
        state.selection_start = state.selection_end = state.cursor_pos;
        state.is_selecting = false;
        
        text_changed = true;
        input_end = state.cursor_pos;
    }
}
void HandleDeleteKey(std::string& text, std::vector<ImVec4>& colors, EditorState& state, bool& text_changed, int& input_end) {
    if (ImGui::IsKeyPressed(ImGuiKey_Delete)) {
        if (state.selection_start != state.selection_end) {
            // There's a selection, delete it
            int start = GetSelectionStart(state);
            int end = GetSelectionEnd(state);
            text.erase(start, end - start);
            colors.erase(colors.begin() + start, colors.begin() + end);
            state.cursor_pos = start;
            text_changed = true;
            input_end = start;
        } else if (state.cursor_pos < text.size()) {
            // No selection, delete the character at cursor position
            text.erase(state.cursor_pos, 1);
            colors.erase(colors.begin() + state.cursor_pos);
            text_changed = true;
            input_end = state.cursor_pos + 1;
        }
        
        // Clear selection after deletion
        state.selection_start = state.selection_end = state.cursor_pos;
        state.is_selecting = false;
    }
}

void HandleBackspaceKey(std::string& text, std::vector<ImVec4>& colors, EditorState& state, bool& text_changed, int& input_start) {
    if (ImGui::IsKeyPressed(ImGuiKey_Backspace)) {
        if (state.selection_start != state.selection_end) {
            // There's a selection, delete it
            int start = GetSelectionStart(state);
            int end = GetSelectionEnd(state);
            text.erase(start, end - start);
            colors.erase(colors.begin() + start, colors.begin() + end);
            state.cursor_pos = start;
            text_changed = true;
            input_start = start;
        } else if (state.cursor_pos > 0) {
            // No selection, delete the character before cursor position
            text.erase(state.cursor_pos - 1, 1);
            colors.erase(colors.begin() + state.cursor_pos - 1);
            state.cursor_pos--;
            text_changed = true;
            input_start = state.cursor_pos;
        }
        
        // Clear selection after deletion
        state.selection_start = state.selection_end = state.cursor_pos;
        state.is_selecting = false;
    }
}

void HandleTabKey(std::string& text, std::vector<ImVec4>& colors, EditorState& state, bool& text_changed, int& input_end) {
    if (ImGui::IsKeyPressed(ImGuiKey_Tab)) {
        // Insert a single tab character
        text.insert(state.cursor_pos, 1, '\t');
        colors.insert(colors.begin() + state.cursor_pos, 1, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
        state.cursor_pos += 1;
        
        // Reset selection state
        state.selection_start = state.selection_end = state.cursor_pos;
        state.is_selecting = false;
        
        text_changed = true;
        input_end = state.cursor_pos;
    }
}

void HandleTextInput(std::string& text, std::vector<ImVec4>& colors, EditorState& state, bool& text_changed){
    int input_start = state.cursor_pos;
    int input_end = state.cursor_pos;

    // Handle selection deletion only for Enter key
    if (state.selection_start != state.selection_end && ImGui::IsKeyPressed(ImGuiKey_Enter)) {
        int start = GetSelectionStart(state);
        int end = GetSelectionEnd(state);
        text.erase(start, end - start);
        colors.erase(colors.begin() + start, colors.begin() + end);
        state.cursor_pos = start;
        state.selection_start = state.selection_end = start;
        text_changed = true;
        input_start = input_end = start;
    }

    HandleCharacterInput(text, colors, state, text_changed, input_start, input_end);
    HandleEnterKey(text, colors, state, text_changed, input_end);
    HandleBackspaceKey(text, colors, state, text_changed, input_start);
    HandleDeleteKey(text, colors, state, text_changed, input_end);
    HandleTabKey(text, colors, state, text_changed, input_end);

    if (text_changed) {
        // Get the start of the line where the change began
        int line_start = state.line_starts[GetLineFromPos(state.line_starts, input_start)];
        
        // Get the end of the line where the change ended (or the end of the text if it's the last line)
        int line_end = input_end < text.size() ? state.line_starts[GetLineFromPos(state.line_starts, input_end) + 1] : text.size();
        
        // Update syntax highlighting only for the affected lines
        gEditor.highlightContent(text, colors, line_start, line_end);
        
        // Update line starts
        UpdateLineStarts(text, state.line_starts);
        
        // Add undo state with change range
        gFileExplorer.addUndoState(line_start, line_end);
    }
}
// Rendering functions
void RenderTextWithSelection(ImDrawList* draw_list, const ImVec2& pos, const std::string& text, const std::vector<ImVec4>& colors, const EditorState& state, float line_height) {
    ImVec2 text_pos = pos;
    int selection_start = GetSelectionStart(state);
    int selection_end = GetSelectionEnd(state);

    for (int i = 0; i < text.size(); i++) {
        if (text[i] == '\n') {
            text_pos.x = pos.x; // Reset x to the original position (including left margin)
            text_pos.y += line_height;
        } else {
            if (i >= selection_start && i < selection_end) {
                ImVec2 sel_start = text_pos;
                ImVec2 sel_end = ImVec2(text_pos.x + ImGui::CalcTextSize(&text[i], &text[i+1]).x, text_pos.y + line_height);
                draw_list->AddRectFilled(sel_start, sel_end, IM_COL32(100, 100, 200, 100));
            }

            char buf[2] = { text[i], '\0' };
            ImU32 color = ImGui::ColorConvertFloat4ToU32(colors[i]);
            draw_list->AddText(text_pos, color, buf);
            text_pos.x += ImGui::CalcTextSize(buf).x;
        }
    }
}

void RenderCursor(ImDrawList* draw_list, const ImVec2& cursor_screen_pos, float line_height, bool rainbow_cursor, float blink_time) {
    float blink_alpha = (sinf(blink_time * 4.0f) + 1.0f) * 0.5f; // Blink frequency
    ImU32 cursor_color;
    
    if (rainbow_cursor) {
        ImVec4 rainbow = GetRainbowColor(blink_time * 2.0f); // Rainbow frequency
        cursor_color = ImGui::ColorConvertFloat4ToU32(rainbow);
    } else {
        cursor_color = IM_COL32(255, 255, 255, (int)(blink_alpha * 255));
    }

    draw_list->AddLine(
        cursor_screen_pos,
        ImVec2(cursor_screen_pos.x, cursor_screen_pos.y + line_height),
        cursor_color
    );
}

int GetCharIndexFromCoords(const std::string& text, const ImVec2& click_pos, const ImVec2& text_start_pos, const std::vector<int>& line_starts, float line_height) {
    int clicked_line = static_cast<int>((click_pos.y - text_start_pos.y) / line_height);
    clicked_line = std::max(0, std::min(clicked_line, static_cast<int>(line_starts.size()) - 1));

    int line_start = line_starts[clicked_line];
    int line_end = (clicked_line + 1 < line_starts.size()) ? line_starts[clicked_line + 1] - 1 : text.size();
    
    float x = text_start_pos.x;
    for (int i = line_start; i < line_end; ++i) {
        char buf[2] = { text[i], '\0' };
        float char_width = ImGui::CalcTextSize(buf).x;
        if (x + char_width / 2 > click_pos.x) {
            return i;
        }
        x += char_width;
    }
    
    return line_end;
}
void RenderLineNumbers(const ImVec2& pos, float line_height, int num_lines, float scroll_y, float window_height, const EditorState& editor_state, float blink_time, bool rainbow_mode) {
    static char line_number_buffer[16];
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    const ImU32 default_line_number_color = IM_COL32(128, 128, 128, 255);
    const ImU32 current_line_color = IM_COL32(255, 255, 255, 255);
    const ImU32 selected_line_color = IM_COL32(200, 200, 200, 255);
    
    int start_line = static_cast<int>(scroll_y / line_height);
    int end_line = std::min(num_lines, static_cast<int>((scroll_y + window_height) / line_height) + 1);
    
    // Pre-calculate rainbow color
    ImU32 rainbow_color = current_line_color;
    if (rainbow_mode) {
        // Update color less frequently
        static float last_update_time = 0.0f;
        static ImU32 last_rainbow_color = current_line_color;
        if (blink_time - last_update_time > 0.05f) { // Update every 50ms
            ImVec4 rainbow = GetRainbowColor(blink_time * 2.0f);
            last_rainbow_color = ImGui::ColorConvertFloat4ToU32(rainbow);
            last_update_time = blink_time;
        }
        rainbow_color = last_rainbow_color;
    }

    // Determine the selected lines, accounting for selection direction
    int selection_start = std::min(editor_state.selection_start, editor_state.selection_end);
    int selection_end = std::max(editor_state.selection_start, editor_state.selection_end);
    int selection_start_line = std::lower_bound(editor_state.line_starts.begin(), editor_state.line_starts.end(), selection_start) - editor_state.line_starts.begin();
    int selection_end_line = std::lower_bound(editor_state.line_starts.begin(), editor_state.line_starts.end(), selection_end) - editor_state.line_starts.begin();

    for (int i = start_line; i < end_line; i++) {
        float y_pos = pos.y + (i * line_height) - scroll_y;
        snprintf(line_number_buffer, sizeof(line_number_buffer), "%3d", i + 1);
        
        ImU32 line_number_color;
        if (i >= selection_start_line && i <= selection_end_line && editor_state.is_selecting) {
            line_number_color = selected_line_color;
        } else if (i == editor_state.current_line) {
            line_number_color = rainbow_mode ? rainbow_color : current_line_color;
        } else {
            line_number_color = default_line_number_color;
        }
        
        draw_list->AddText(ImVec2(pos.x, y_pos), line_number_color, line_number_buffer);
    }
}
float CalculateTextWidth(const std::string& text, const std::vector<int>& line_starts) {
    float max_width = 0.0f;
    for (size_t i = 0; i < line_starts.size(); ++i) {
        int start = line_starts[i];
        int end = (i + 1 < line_starts.size()) ? line_starts[i + 1] - 1 : text.size();
        float line_width = ImGui::CalcTextSize(text.c_str() + start, text.c_str() + end).x;
        max_width = std::max(max_width, line_width);
    }
    return max_width;
}
void HandleEditorInput(std::string& text, EditorState& state, const ImVec2& text_start_pos, float line_height, bool& text_changed, std::vector<ImVec4>& colors, CursorVisibility& ensure_cursor_visible) {
    bool ctrl_pressed = ImGui::GetIO().KeyCtrl;
    bool shift_pressed = ImGui::GetIO().KeyShift;

    if (ImGui::IsWindowFocused() && !state.blockInput) {
        if (ctrl_pressed) {
            
            if (ImGui::IsKeyPressed(ImGuiKey_A)) {
                SelectAllText(state, text);
                ensure_cursor_visible.vertical = true;
                ensure_cursor_visible.horizontal = true;
                std::cout << "Ctrl+A: Selected all text" << std::endl;
            }
            if (ImGui::IsKeyPressed(ImGuiKey_Z)) {
                std::cout << "Z key pressed. Ctrl: " << ctrl_pressed << ", Shift: " << shift_pressed << std::endl;
                
                // Store the current cursor position and line
                int oldCursorPos = state.cursor_pos;
                int oldLine = GetLineFromPos(state.line_starts, oldCursorPos);
                int oldColumn = oldCursorPos - state.line_starts[oldLine];

                if (shift_pressed) {
                    std::cout << "Attempting Redo" << std::endl;
                    gFileExplorer.handleRedo();
                } else {
                    std::cout << "Attempting Undo" << std::endl;
                    gFileExplorer.handleUndo();
                }

                // Update text and colors
                text = gFileExplorer.getFileContent();
                colors = gFileExplorer.getFileColors();
                
                // Update line starts
                UpdateLineStarts(text, state.line_starts);
                
                // Recalculate cursor position
                int newLine = std::min(oldLine, static_cast<int>(state.line_starts.size()) - 1);
                int lineStart = state.line_starts[newLine];
                int lineEnd = (newLine + 1 < state.line_starts.size()) ? state.line_starts[newLine + 1] - 1 : text.size();
                int lineLength = lineEnd - lineStart;
                
                // Try to maintain the same column position
                state.cursor_pos = lineStart + std::min(oldColumn, lineLength);
                
                state.selection_start = state.selection_end = state.cursor_pos;
                text_changed = true;
                ensure_cursor_visible.vertical = true;
                ensure_cursor_visible.horizontal = true;
                
                gFileExplorer.currentUndoManager->printStacks();  // Print stack sizes for debugging
            }

            else if (ImGui::IsKeyPressed(ImGuiKey_LeftArrow)) {
                // Jump to start of line
                int current_line = GetLineFromPos(state.line_starts, state.cursor_pos);
                state.cursor_pos = state.line_starts[current_line]+1;
                ensure_cursor_visible.horizontal = true;
                std::cout << "Ctrl/Cmd+Left: Cursor moved to " << state.cursor_pos << std::endl;
            }
            else if (ImGui::IsKeyPressed(ImGuiKey_RightArrow)) {
                // Jump to end of line
                int current_line = GetLineFromPos(state.line_starts, state.cursor_pos);
                int next_line = current_line + 1;
                if (next_line < state.line_starts.size()) {
                    state.cursor_pos = state.line_starts[next_line] - 2;  // Position before the newline
                } else {
                    state.cursor_pos = text.size();  // End of the text
                }
                ensure_cursor_visible.horizontal = true;
                std::cout << "Ctrl+Right: Cursor moved to " << state.cursor_pos << std::endl;
            }
            else if (ImGui::IsKeyPressed(ImGuiKey_UpArrow)) {
                // Jump 5 lines up
                MoveCursorVertically(text, state, -5);
                ensure_cursor_visible.vertical = true;
                ensure_cursor_visible.horizontal = true;
                std::cout << "Ctrl+Up: Cursor moved 5 lines up to " << state.cursor_pos << std::endl;
            }
            else if (ImGui::IsKeyPressed(ImGuiKey_DownArrow)) {
                // Jump 5 lines down
                MoveCursorVertically(text, state, 5);
                ensure_cursor_visible.vertical = true;
                ensure_cursor_visible.horizontal = true;
                std::cout << "Ctrl+Down: Cursor moved 5 lines down to " << state.cursor_pos << std::endl;
            }
        }
    }

    if (ImGui::IsWindowHovered()) {
        HandleMouseInput(text, state, text_start_pos, line_height);

        float wheel_y = ImGui::GetIO().MouseWheel;
        float wheel_x = ImGui::GetIO().MouseWheelH;

        if (wheel_y != 0 || wheel_x != 0) {
            if (shift_pressed) {
                // Horizontal scrolling with Shift + Scroll
                float scroll_amount = wheel_y * ImGui::GetFontSize() * 3;
                float new_scroll_x = ImGui::GetScrollX() - scroll_amount;
                new_scroll_x = std::max(0.0f, std::min(new_scroll_x, ImGui::GetScrollMaxX()));
                ImGui::SetScrollX(new_scroll_x);
                state.scroll_pos.x = new_scroll_x;
            } else {
                // Vertical scrolling
                float new_scroll_y = ImGui::GetScrollY() - wheel_y * line_height * 3;
                new_scroll_y = std::max(0.0f, std::min(new_scroll_y, ImGui::GetScrollMaxY()));
                ImGui::SetScrollY(new_scroll_y);
                state.scroll_pos.y = new_scroll_y;
            }
        }
    }

    
    if (ImGui::IsKeyPressed(ImGuiKey_UpArrow) || ImGui::IsKeyPressed(ImGuiKey_DownArrow)) {
        ensure_cursor_visible.vertical = true;
    }
    if (ImGui::IsKeyPressed(ImGuiKey_LeftArrow) || ImGui::IsKeyPressed(ImGuiKey_RightArrow)) {
        ensure_cursor_visible.horizontal = true;
    }
    HandleCursorMovement(text, state, text_start_pos, line_height, ImGui::GetWindowHeight(), ImGui::GetWindowWidth());


    HandleTextInput(text, colors, state, text_changed);

    if (ImGui::IsWindowFocused()) {
        if (ctrl_pressed && ImGui::IsKeyPressed(ImGuiKey_C, false)) {
            CopySelectedText(text, state);
        }
        if (ctrl_pressed && ImGui::IsKeyPressed(ImGuiKey_X, false)) {
            if (state.selection_start != state.selection_end) {
                CutSelectedText(text, colors, state, text_changed);
            } else {
                CutWholeLine(text, colors, state, text_changed);
            }
            ensure_cursor_visible.vertical = true;
            ensure_cursor_visible.horizontal = true;
        }
        if (ctrl_pressed && ImGui::IsKeyPressed(ImGuiKey_V, false)) {
            PasteText(text, colors, state, text_changed);
            ensure_cursor_visible.vertical = true;
            ensure_cursor_visible.horizontal = true;
        }
    }

    // If text has changed, we should ensure the cursor is visible
    if (text_changed) {
        ensure_cursor_visible.vertical = true;
        ensure_cursor_visible.horizontal = true;
    }
}
bool CustomTextEditor(const char* label, std::string& text, std::vector<ImVec4>& colors, EditorState& editor_state) {
    if (colors.size() != text.size()) {
        std::cout << "Warning: colors vector size (" << colors.size() 
                << ") does not match text size (" << text.size() << "). Resizing." << std::endl;
        colors.resize(text.size(), ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
    }
    bool text_changed = false;
    CursorVisibility ensure_cursor_visible = {false, false};

    editor_state.cursor_blink_time += ImGui::GetIO().DeltaTime;

    ImGui::PushID(label);
    ImVec2 size = ImGui::GetContentRegionAvail();
    
    int num_lines = std::count(text.begin(), text.end(), '\n') + 1;
    float line_number_width = ImGui::CalcTextSize("999").x + 5.0f;
    float line_height = ImGui::GetTextLineHeight();
    float editor_top_margin = 2.0f;
    float text_left_margin = 7.0f;

    ImGui::BeginGroup();

    // Render line numbers
    ImGui::PushStyleVar(ImGuiStyleVar_ChildBorderSize, 0.0f);
    ImGui::BeginChild("LineNumbers", ImVec2(line_number_width, size.y), false, ImGuiWindowFlags_NoScrollbar);
    ImVec2 line_numbers_pos = ImGui::GetCursorScreenPos();
    line_numbers_pos.y += editor_top_margin;
    ImGui::EndChild();
    ImGui::PopStyleVar();

    ImGui::SameLine();

    // Create the main text editor window
    ImGui::PushStyleVar(ImGuiStyleVar_ChildBorderSize, 1.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.5f, 0.5f, 0.5f, 0.5f));
    ImGui::PushStyleColor(ImGuiCol_ScrollbarBg, ImVec4(0.05f, 0.05f, 0.05f, 0.0f));
    ImGui::PushStyleColor(ImGuiCol_ScrollbarGrabHovered, ImVec4(0.6f, 0.6f, 0.6f, 0.7f));
    ImGui::PushStyleColor(ImGuiCol_ScrollbarGrabActive, ImVec4(0.8f, 0.8f, 0.8f, 0.9f));
    ImGui::PushStyleVar(ImGuiStyleVar_ScrollbarSize, 12.0f);

    float remaining_width = size.x - line_number_width;
    float content_width = CalculateTextWidth(text, editor_state.line_starts);
    ImGui::SetNextWindowContentSize(ImVec2(content_width, 0.0f));
    ImGui::BeginChild(label, ImVec2(remaining_width, size.y), false, ImGuiWindowFlags_HorizontalScrollbar);
    // Set keyboard focus to this child window
    if (!editor_state.blockInput) {
        if (ImGui::IsWindowAppearing() || (ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows) && !ImGui::IsAnyItemActive())) {
            ImGui::SetKeyboardFocusHere();
        }
    }
    // Store the current scroll position before any changes
    float current_scroll_y = ImGui::GetScrollY();
    float current_scroll_x = ImGui::GetScrollX();

    ImVec2 text_pos = ImGui::GetCursorScreenPos();
    text_pos.y += editor_top_margin;
    text_pos.x += text_left_margin;
    
    UpdateLineStarts(text, editor_state.line_starts);

    float total_height = line_height * editor_state.line_starts.size();
    ImVec2 text_start_pos = text_pos;

    int initial_cursor_pos = editor_state.cursor_pos;
    if (!editor_state.blockInput) {
        HandleEditorInput(text, editor_state, text_start_pos, line_height, text_changed, colors, ensure_cursor_visible);
    } else {
        ensure_cursor_visible.vertical = true;
        ensure_cursor_visible.horizontal = true;
    }
    
    // Check if cursor position has changed
    if (editor_state.cursor_pos != initial_cursor_pos) {
        ensure_cursor_visible.vertical = true;
        ensure_cursor_visible.horizontal = true;
    }

    // Handle mouse wheel scrolling
    if (ImGui::IsWindowHovered() && !editor_state.blockInput) {
        float wheel_y = ImGui::GetIO().MouseWheel;
        float wheel_x = ImGui::GetIO().MouseWheelH;

        if (wheel_y != 0) {
            current_scroll_y -= wheel_y * line_height * 3;
            current_scroll_y = std::max(0.0f, std::min(current_scroll_y, ImGui::GetScrollMaxY()));
        }
        if (wheel_x != 0) {
            current_scroll_x -= wheel_x * ImGui::GetFontSize() * 3;
            current_scroll_x = std::max(0.0f, std::min(current_scroll_x, ImGui::GetScrollMaxX()));
        }
    }

    if (ensure_cursor_visible.vertical || ensure_cursor_visible.horizontal) {
        ScrollChange scroll_change = EnsureCursorVisible(text_pos, text, editor_state, line_height, size.y, size.x);
        if (scroll_change.vertical) {
            current_scroll_y = editor_state.scroll_pos.y;
        }
        if (scroll_change.horizontal) {
            current_scroll_x = editor_state.scroll_x;
        }
    }

    // Apply the calculated scroll position
    ImGui::SetScrollY(current_scroll_y);
    ImGui::SetScrollX(current_scroll_x);

    RenderTextWithSelection(ImGui::GetWindowDrawList(), text_pos, text, colors, editor_state, line_height);

    // Calculate cursor screen position
    ImVec2 cursor_screen_pos = text_pos;
    for (int i = 0; i < editor_state.cursor_pos; i++) {
        if (text[i] == '\n') {
            cursor_screen_pos.x = text_pos.x;
            cursor_screen_pos.y += line_height;
        } else {
            cursor_screen_pos.x += ImGui::CalcTextSize(&text[i], &text[i + 1]).x;
        }
    }

    RenderCursor(ImGui::GetWindowDrawList(), cursor_screen_pos, line_height, editor_state.rainbow_cursor, editor_state.cursor_blink_time);

    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + total_height + editor_top_margin);

    // Update the editor state with the final scroll position
    editor_state.scroll_pos.y = ImGui::GetScrollY();
    editor_state.scroll_x = ImGui::GetScrollX();

    ImGui::EndChild();

    // Pop styles and colors
    ImGui::PopStyleColor(4);
    ImGui::PopStyleVar(4);

    // Render line numbers with clipping
    ImGui::PushClipRect(line_numbers_pos, ImVec2(line_numbers_pos.x + line_number_width, line_numbers_pos.y + size.y - editor_top_margin), true);
   RenderLineNumbers(line_numbers_pos, line_height, editor_state.line_starts.size(), editor_state.scroll_pos.y, size.y - editor_top_margin, editor_state, editor_state.cursor_blink_time, true);
    ImGui::PopClipRect();

    ImGui::EndGroup();

    ImGui::PopID();

    return text_changed;
}
void Editor::highlightContent(const std::string& content, std::vector<ImVec4>& colors, int start_pos, int end_pos) {
    if (content.size() != colors.size()) {
        std::cerr << "Error: content and colors size mismatch. content: " << content.size() 
                  << ", colors: " << colors.size() << std::endl;
        return;
    }

    if (start_pos < 0) start_pos = 0;
    if (end_pos > static_cast<int>(content.size()) || end_pos < 0) end_pos = static_cast<int>(content.size());

    if (start_pos >= end_pos || start_pos >= static_cast<int>(content.size())) {
        std::cerr << "Error: Invalid start_pos or end_pos in highlightContent. "
                  << "start_pos: " << start_pos << ", end_pos: " << end_pos 
                  << ", content size: " << content.size() << std::endl;
        return;
    }

    std::string view = content.substr(start_pos, end_pos - start_pos);

    applyRules(view, colors, start_pos, rules);

    std::regex scriptTag(R"(<script\b[^>]*>([\s\S]*?)</script>)");
    std::regex styleTag(R"(<style\b[^>]*>([\s\S]*?)</style>)");

    auto applyTagRules = [&](const std::smatch& match, const std::vector<SyntaxRule>& tagRules) {
        std::string tagContent = match[1].str();
        int tagStart = start_pos + static_cast<int>(match.position(1));
        int tagEnd = tagStart + static_cast<int>(tagContent.length());
        if (tagEnd > start_pos && tagStart < end_pos) {
            int highlightStart = std::max(start_pos, tagStart);
            int highlightEnd = std::min(end_pos, tagEnd);
            if (highlightStart < highlightEnd && highlightStart < static_cast<int>(content.size())) {
                applyRules(content.substr(highlightStart, highlightEnd - highlightStart), colors, highlightStart, tagRules);
            }
        }
    };
    
    int content_start = std::max(0, start_pos - 100);
    int content_end = std::min(static_cast<int>(content.size()), end_pos + 100);
    std::string content_str = content.substr(content_start, content_end - content_start);

    for (std::sregex_iterator it(content_str.begin(), content_str.end(), scriptTag), end_it; it != end_it; ++it) {
        applyTagRules(*it, javascriptRules);
    }

    for (std::sregex_iterator it(content_str.begin(), content_str.end(), styleTag), end_it; it != end_it; ++it) {
        applyTagRules(*it, cssRules);
    }
}

void Editor::applyRules(const std::string& view, std::vector<ImVec4>& colors, int start_pos, const std::vector<SyntaxRule>& rules) {
    if (start_pos < 0 || start_pos >= static_cast<int>(colors.size())) {
        std::cerr << "Error: Invalid start_pos in applyRules. start_pos: " << start_pos 
                  << ", colors size: " << colors.size() << std::endl;
        return;
    }

    if (start_pos + view.length() > colors.size()) {
        std::cerr << "Error: View extends beyond colors vector in applyRules. "
                  << "start_pos: " << start_pos << ", view length: " << view.length() 
                  << ", colors size: " << colors.size() << std::endl;
        return;
    }

    std::fill(colors.begin() + start_pos, colors.begin() + start_pos + view.length(), themeColors["text"]);

    for (const auto& rule : rules) {
        std::sregex_iterator it(view.begin(), view.end(), rule.pattern);
        std::sregex_iterator end;

        while (it != end) {
            size_t match_start = start_pos + it->position();
            size_t match_end = match_start + it->length();

            if (match_start >= colors.size() || match_end > colors.size()) {
                std::cerr << "Error: Match extends beyond colors vector in applyRules. "
                          << "match_start: " << match_start << ", match_end: " << match_end 
                          << ", colors size: " << colors.size() << std::endl;
                break;
            }

            for (size_t i = match_start; i < match_end; ++i) {
                colors[i] = (rule.color.w > 0.0f) ? rule.color : themeColors["text"];
            }

            ++it;
        }
    }
    // Ensure all necessary colors are defined
    std::vector<std::string> requiredColors = {"text", "comment", "keyword", "builtin", "function", "string", "number"};
    for (const auto& colorName : requiredColors) {
        if (themeColors.find(colorName) == themeColors.end()) {
            std::cout << "Warning: Color '" << colorName << "' not defined in theme. Using default text color." << std::endl;
            themeColors[colorName] = themeColors["text"];
        }
    }

    // Print out all colors for debugging
    for (const auto& [key, color] : themeColors) {
        //std::cout << "Color '" << key << "': (" << color.x << ", " << color.y << ", " << color.z << ", " << color.w << ")" << std::endl;
    }
}
// Syntax highlighting functions
void Editor::setLanguage(const std::string& extension) {
    std::cout << "Setting language for extension: " << extension << std::endl;
    setupHtmlRules();
    setupJavaScriptRules();
    setupCssRules();
    
    if (extension == ".cpp" || extension == ".h") {
        setupCppRules();
        rules = cppRules;
    } else if (extension == ".py") {
        setupPythonRules();
        rules = pythonRules;
    } else if (extension == ".js") {
        rules = javascriptRules;
    } else if (extension == ".md") {
        setupMarkdownRules();
        rules = markdownRules;
    } else if (extension == ".html") {
        rules = htmlRules;
    } else if (extension == ".css") {
        rules = cssRules;
    } else if (extension == ".json") {
        setupJsonRules();
        rules = jsonRules;
    } else {
        // Default to no syntax highlighting
        rules.clear();
    }
}
void Editor::setTheme(const std::string& themeName) {
    loadTheme(themeName);
}
void Editor::setupCppRules() {
    cppRules = {
        {std::regex(R"(\b(int|float|double|char|void|bool|auto|const|static|struct|class|namespace|using|return|if|else|for|while|do|switch|case|break|continue|true|false|nullptr)\b)"), themeColors["keyword"]},
        {std::regex(R"("(?:\\.|[^\\"])*")"), themeColors["string"]},
        {std::regex(R"(\b[0-9]+\b)"), themeColors["number"]},
        {std::regex(R"(//.*|/\*[\s\S]*?\*/)"), themeColors["comment"]},
    };
}

void Editor::setupPythonRules() {
    pythonRules = {
        
        // Keywords
        {std::regex(R"(\b(def|class|if|else|elif|for|while|try|except|finally|with|as|import|from|return|and|or|not|in|is|lambda|None|True|False|async|await)\b)"), themeColors["keyword"]},
        
        // Self and cls
        {std::regex(R"(\b(self|cls)\b)"), themeColors["self"]},
        
        // Multiline strings (triple-quoted)
        {std::regex(R"('''[\s\S]*?'''|"""[\s\S]*?""")"), themeColors["multiline_string"]},
        
        // Single-line strings
        {std::regex(R"("(?:[^"\\]|\\.)*"|'(?:[^'\\]|\\.)*')"), themeColors["string"]},
        
        // Numbers
        {std::regex(R"(\b\d+(\.\d*)?([eE][+-]?\d+)?\b)"), themeColors["number"]},
        
        // Built-in functions
        {std::regex(R"(\b(print|len|range|list|dict|set|tuple|int|float|str|bool|abs|all|any|chr|dir|divmod|enumerate|filter|hex|id|input|isinstance|map|max|min|oct|ord|pow|repr|round|sorted|sum|zip)\b)"), themeColors["builtin"]},
        
        // Decorators
        {std::regex(R"(^@\w+)"), themeColors["decorator"]},
        // Comments (moved to the top to ensure they're not overwritten)
        {std::regex(R"(#[^\n]*)"), themeColors["comment"]},
        // Function calls
        {std::regex(R"(\b\w+(?=\s*\())"), themeColors["function"]},
    };

    // Ensure we have all necessary colors
    std::vector<std::string> requiredColors = {"text", "comment", "keyword", "self", "multiline_string", "string", "number", "builtin", "decorator", "function"};
    for (const auto& color : requiredColors) {
        if (themeColors.find(color) == themeColors.end()) {
            themeColors[color] = ImVec4(0.9f, 0.9f, 0.9f, 1.0f);  // Default to light grey if not found
        }
    }

    std::cout << "Python rules set up with " << pythonRules.size() << " rules." << std::endl;
    for (const auto& [key, color] : themeColors) {
        std::cout << "Theme color '" << key << "': (" 
                << color.x << ", " << color.y << ", " 
                << color.z << ", " << color.w << ")" << std::endl;
    }
}



// Add these new setup functions
void Editor::setupMarkdownRules() {
    rules = {
        {std::regex(R"(^#+\s.+$)"), themeColors["heading"]},
        {std::regex(R"(\*\*.*?\*\*|__.*?__)"), themeColors["bold"]},
        {std::regex(R"(\*.*?\*|_.*?_)"), themeColors["italic"]},
        {std::regex(R"(\[.*?\]\(.*?\))"), themeColors["link"]},
        {std::regex(R"(```[\s\S]*?```)"), themeColors["code_block"]},
        {std::regex(R"(`.*?`)"), themeColors["inline_code"]},
    };
}
void Editor::setupHtmlRules() {
    htmlRules = {
        {std::regex(R"(<[^>]*>)"), themeColors["tag"]},
        {std::regex(R"((\w+)=)"), themeColors["attribute"]},
        {std::regex(R"(".*?"|'.*?')"), themeColors["string"]},
        {std::regex(R"(<!--[\s\S]*?-->)"), themeColors["comment"]},
    };
}

void Editor::setupJavaScriptRules() {
    javascriptRules = {
        {std::regex(R"(\b(var|let|const|function|class|if|else|for|while|do|switch|case|break|continue|return|true|false|null|undefined)\b)"), themeColors["keyword"]},
        {std::regex(R"("(?:\\.|[^\\"])*"|'(?:\\.|[^\\'])*'|`(?:\\.|[^\\`])*`)"), themeColors["string"]},
        {std::regex(R"(\b[0-9]+\b)"), themeColors["number"]},
        {std::regex(R"(//.*|/\*[\s\S]*?\*/)"), themeColors["comment"]},
    };
}

void Editor::setupCssRules() {
    cssRules = {
        {std::regex(R"([\.\#]?\w+\s*\{)"), themeColors["selector"]},
        {std::regex(R"([\w-]+\s*:)"), themeColors["property"]},
        {std::regex(R"(:.*?;)"), themeColors["value"]},
        {std::regex(R"(/\*[\s\S]*?\*/)"), themeColors["comment"]},
    };
}

void Editor::setupJsonRules() {
    rules = {
        {std::regex(R"(".*?"\s*:)"), themeColors["key"]},
        {std::regex(R"(:\s*".*?")"), themeColors["string"]},
        {std::regex(R"(:\s*-?\d+(\.\d+)?)"), themeColors["number"]},
        {std::regex(R"(:\s*(true|false|null))"), themeColors["keyword"]},
    };
}

void Editor::loadTheme(const std::string& themeName) {
    auto& settings = gSettings.getSettings();
    if (settings.contains("themes") && settings["themes"].contains(themeName)) {
        auto& theme = settings["themes"][themeName];
        for (const auto& [key, value] : theme.items()) {
            themeColors[key] = ImVec4(value[0], value[1], value[2], value[3]);
        }
    } else {
        // Set default colors if theme not found
        themeColors["keyword"] = ImVec4(0.0f, 0.4f, 1.0f, 1.0f);
        themeColors["string"] = ImVec4(0.87f, 0.87f, 0.0f, 1.0f);
        themeColors["number"] = ImVec4(0.0f, 0.8f, 0.8f, 1.0f);
        themeColors["comment"] = ImVec4(0.5f, 0.5f, 0.5f, 1.0f);
    }
}