#include "editor_mouse.h"
#include "../files/files.h"
#include "editor.h"
#include "editor_copy_paste.h"
#include <algorithm>
#include <iostream>

// Global instance
EditorMouse gEditorMouse;

EditorMouse::EditorMouse() : is_dragging(false), anchor_pos(-1), show_context_menu(false) {}

void EditorMouse::handleMouseInput(const std::string &text, EditorState &state, const ImVec2 &text_start_pos, float line_height)
{
    ImVec2 mouse_pos = ImGui::GetMousePos();
    int char_index = getCharIndexFromCoords(text, mouse_pos, text_start_pos, state.editor_content_lines, line_height);

    // Handle left click
    if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
        handleMouseClick(state, char_index, state.editor_content_lines);
    }
    // Handle drag
    else if (ImGui::IsMouseDragging(ImGuiMouseButton_Left) && is_dragging) {
        handleMouseDrag(state, char_index);
    }
    // Handle release
    else if (ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
        handleMouseRelease();
    }

    // Handle right click for context menu
    if (ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
        // Store the position where the context menu should appear
        context_menu_pos = ImGui::GetMousePos();
        show_context_menu = true;

        // If no active selection, set cursor at the clicked position
        if (!state.selection_active) {
            state.cursor_column = char_index;
        }
    }
}

void EditorMouse::handleMouseClick(EditorState &state, int char_index, const std::vector<int> &line_starts)
{
    if (ImGui::GetIO().KeyShift) {
        // If shift is held and we're not already selecting, set the anchor to the current
        // cursor position.
        if (!state.selection_active) {
            anchor_pos = state.cursor_column;
            state.selection_start = anchor_pos;
            state.selection_active = true;
        }
        // Update the selection end based on the new click.
        state.selection_end = char_index;
        state.cursor_column = char_index;
    } else {
        // On a regular click (without shift), reset the selection and update the anchor.
        state.cursor_column = char_index;
        anchor_pos = char_index;
        state.selection_start = char_index;
        state.selection_end = char_index;
        state.selection_active = false;
        int current_line = gEditor.getLineFromPos(line_starts, state.cursor_column);
        state.cursor_column_prefered = state.cursor_column - line_starts[current_line];
    }
    is_dragging = true;
}

void EditorMouse::handleMouseDrag(EditorState &state, int char_index)
{
    if (ImGui::GetIO().KeyShift) {
        // If shift is held, use the existing anchor for updating selection.
        if (anchor_pos == -1) {
            anchor_pos = state.cursor_column;
            state.selection_start = anchor_pos;
        }
        state.selection_end = char_index;
        state.cursor_column = char_index;
    } else {
        // Normal drag selection without shift: use the initial click as the anchor.
        state.selection_active = true;
        if (anchor_pos == -1) {
            anchor_pos = state.cursor_column;
        }
        state.selection_start = anchor_pos;
        state.selection_end = char_index;
        state.cursor_column = char_index;
    }
}

void EditorMouse::handleMouseRelease()
{
    is_dragging = false;
    anchor_pos = -1;
}

void EditorMouse::handleContextMenu(std::string &text, std::vector<ImVec4> &colors, EditorState &state, bool &text_changed)
{
    // For debugging
    static bool popupWasOpen = false;
    bool popupIsOpen = ImGui::IsPopupOpen("##EditorContextMenu");

    // Detect state changes for tracking
    if (popupIsOpen && !popupWasOpen) {
        std::cout << "Popup was just opened" << std::endl;
    }
    if (!popupIsOpen && popupWasOpen) {
        std::cout << "Popup was just closed" << std::endl;
        show_context_menu = false; // Sync our state when ImGui closes the popup
    }
    popupWasOpen = popupIsOpen;

    // Check if right click just happened to open menu
    if (ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
        context_menu_pos = ImGui::GetMousePos();
        show_context_menu = true;
        ImGui::OpenPopup("##EditorContextMenu");
    }

    // Set position of popup if we're showing it
    if (show_context_menu) {
        ImGui::SetNextWindowPos(context_menu_pos);
        // Make the menu wider to prevent text cramping and allow room for shortcuts
        ImGui::SetNextWindowSize(ImVec2(220, 0), ImGuiCond_Always); // Width of 220, auto height
    }

    // Apply styling - improved with rounded corners and better padding
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(12, 12));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 10.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 6.0f);       // Round the menu items
    ImGui::PushStyleVar(ImGuiStyleVar_PopupRounding, 10.0f);      // Specific rounding for popups
    ImGui::PushStyleVar(ImGuiStyleVar_PopupBorderSize, 1.0f);     // Thin border for the popup
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8, 6)); // Reduced vertical spacing

    // Colors for the popup
    ImGui::PushStyleColor(ImGuiCol_PopupBg, ImVec4(0.2f, 0.2f, 0.2f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.9f, 0.9f, 0.9f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.3f, 0.3f, 0.3f, 1.0f));

    // Set hover color to match your selection highlight
    ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(1.0f, 0.1f, 0.7f, 0.3f)); // Pink hover
    ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(1.0f, 0.1f, 0.7f, 0.5f));        // Pink selection

    // Begin the popup with flags to prevent it from closing on outside clicks
    if (ImGui::BeginPopup("##EditorContextMenu", ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar)) {
        // We're inside the popup so we must be showing it
        show_context_menu = true;

        // Manual click outside detection
        if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) && !ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByPopup)) {
            show_context_menu = false;
            ImGui::CloseCurrentPopup();
        }

        // Check for escape key to close
        if (ImGui::IsKeyPressed(ImGuiKey_Escape)) {
            show_context_menu = false;
            ImGui::CloseCurrentPopup();
        }

        // Menu title - without extra spacing after
        ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Menu");
        ImGui::Separator();

        // Simple function to draw a menu item with a shortcut in a fixed-width manner
        auto MenuItemWithAlignedShortcut = [](const char *label, const char *shortcut, bool *selected, bool enabled) -> bool {
            float contentWidth = ImGui::GetContentRegionAvail().x;
            float labelWidth = ImGui::CalcTextSize(label).x;
            float shortcutWidth = ImGui::CalcTextSize(shortcut).x;

            // Create the basic menu item
            bool activated = ImGui::MenuItem(label, nullptr, selected, enabled);

            // Draw the shortcut aligned to the right
            if (shortcut && shortcut[0]) {
                ImGui::SameLine(contentWidth - shortcutWidth);
                ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "%s", shortcut);
            }

            return activated;
        };

        // Cut action
        if (MenuItemWithAlignedShortcut("Cut", "Ctrl+X", nullptr, state.selection_active)) {
            gEditorCopyPaste.cutSelectedText(text, colors, state, text_changed);
            show_context_menu = false;
            ImGui::CloseCurrentPopup();
        }

        // Copy action
        if (MenuItemWithAlignedShortcut("Copy", "Ctrl+C", nullptr, state.selection_active)) {
            gEditorCopyPaste.copySelectedText(text, state);
            show_context_menu = false;
            ImGui::CloseCurrentPopup();
        }

        // Paste action
        if (MenuItemWithAlignedShortcut("Paste", "Ctrl+V", nullptr, true)) {
            gEditorCopyPaste.pasteText(text, colors, state, text_changed);
            show_context_menu = false;
            ImGui::CloseCurrentPopup();
        }

        // Add a subtle separator for better organization
        ImGui::Separator();

        // Save file action
        if (MenuItemWithAlignedShortcut("Save", "Ctrl+S", nullptr, true)) {
            gFileExplorer.saveCurrentFile();
            show_context_menu = false;
            ImGui::CloseCurrentPopup();
        }

        // Another subtle separator
        ImGui::Separator();

        // Select All
        if (MenuItemWithAlignedShortcut("Select All", "Ctrl+A", nullptr, true)) {
            state.selection_active = true;
            state.selection_start = 0;
            state.selection_end = text.size();
            state.cursor_column = text.size();
            show_context_menu = false;
            ImGui::CloseCurrentPopup();
        }
        ImGui::Separator();

        // Go to Definition
        if (MenuItemWithAlignedShortcut("Go to Definition", "F12", nullptr, true)) {
            // Get current line number from state
            int current_line = gEditor.getLineFromPos(state.editor_content_lines, state.cursor_column);

            // Get character offset in current line
            int line_start = state.editor_content_lines[current_line];
            int char_offset = state.cursor_column - line_start;

            // Call LSP goto definition
            gEditorLSP.gotoDefinition(gFileExplorer.getCurrentFile(), current_line, char_offset);

            show_context_menu = false;
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }

    // Pop styling
    ImGui::PopStyleColor(5);
    ImGui::PopStyleVar(6);
}

int EditorMouse::getCharIndexFromCoords(const std::string &text, const ImVec2 &click_pos, const ImVec2 &text_start_pos, const std::vector<int> &line_starts, float line_height)
{
    // Determine which line was clicked (clamped to valid indices)
    int clicked_line = std::clamp(static_cast<int>((click_pos.y - text_start_pos.y) / line_height), 0, static_cast<int>(line_starts.size()) - 1);

    // Get start/end indices for that line in the text.
    int line_start = line_starts[clicked_line];
    int line_end = (clicked_line + 1 < line_starts.size()) ? line_starts[clicked_line + 1] : text.size();

    // Adjust line_end to exclude newline character if present
    if (line_end > line_start && line_end <= text.size() && text[line_end - 1] == '\n') {
        line_end--;
    }

    int n = line_end - line_start; // number of characters in the line

    // If the line is empty, return its start.
    if (n <= 0)
        return line_start;

    // Compute the click's x-coordinate relative to the beginning of the text.
    float click_x = click_pos.x - text_start_pos.x;

    // For more accuracy, calculate character widths individually to avoid accumulating errors
    // This is especially important for longer lines where small errors add up
    std::vector<float> charWidths(n + 1, 0.0f);
    std::vector<float> insertionPositions(n + 1, 0.0f);

    insertionPositions[0] = 0.0f;

    // Calculate the width of each character individually
    for (int i = 0; i < n; i++) {
        char buf[2] = {text[line_start + i], '\0'};
        charWidths[i] = ImGui::CalcTextSize(buf).x;
    }

    // Calculate cumulative positions
    for (int i = 1; i <= n; i++) {
        insertionPositions[i] = insertionPositions[i - 1] + charWidths[i - 1];
    }

    // Find the insertion index (0...n) whose position is closest to click_x.
    int bestIndex = 0;
    float bestDist = std::abs(click_x - insertionPositions[0]);

    for (int i = 1; i <= n; i++) {
        float d = std::abs(click_x - insertionPositions[i]);
        if (d < bestDist) {
            bestDist = d;
            bestIndex = i;
        }
    }

    // Return the character index in the full text corresponding to that insertion point.
    return line_start + bestIndex;
}