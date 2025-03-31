#include "editor_mouse.h"
#include "../files/files.h"
#include "../lsp/lsp_goto_def.h"
#include "../lsp/lsp_goto_ref.h"
#include "editor.h"
#include "editor_copy_paste.h"
#include <algorithm>
#include <iostream>

// Global instance
EditorMouse gEditorMouse;

EditorMouse::EditorMouse() : is_dragging(false), anchor_pos(-1), show_context_menu(false) {}

void EditorMouse::handleMouseInput()
{
    int char_index = getCharIndexFromCoords();

    // Handle left click
    if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
        handleMouseClick(char_index);
    }
    // Handle drag
    else if (ImGui::IsMouseDragging(ImGuiMouseButton_Left) && is_dragging) {
        handleMouseDrag(char_index);
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
        if (!editor_state.selection_active) {
            editor_state.cursor_index = char_index;
        }
    }
}

void EditorMouse::handleMouseClick(int char_index)
{
    if (ImGui::GetIO().KeyShift) {
        // If shift is held and we're not already selecting, set the anchor to the current
        // cursor position.
        if (!editor_state.selection_active) {
            anchor_pos = editor_state.cursor_index;
            editor_state.selection_start = anchor_pos;
            editor_state.selection_active = true;
        }
        // Update the selection end based on the new click.
        editor_state.selection_end = char_index;
        editor_state.cursor_index = char_index;
    } else {
        // On a regular click (without shift), reset the selection and update the anchor.
        editor_state.cursor_index = char_index;
        anchor_pos = char_index;
        editor_state.selection_start = char_index;
        editor_state.selection_end = char_index;
        editor_state.selection_active = false;
        int current_line = gEditor.getLineFromPos(editor_state.cursor_index);
        editor_state.cursor_column_prefered = editor_state.cursor_index - editor_state.editor_content_lines[current_line];
    }
    is_dragging = true;
}

void EditorMouse::handleMouseDrag(int char_index)
{
    if (ImGui::GetIO().KeyShift) {
        // If shift is held, use the existing anchor for updating selection.
        if (anchor_pos == -1) {
            anchor_pos = editor_state.cursor_index;
            editor_state.selection_start = anchor_pos;
        }
        editor_state.selection_end = char_index;
        editor_state.cursor_index = char_index;
    } else {
        // Normal drag selection without shift: use the initial click as the anchor.
        editor_state.selection_active = true;
        if (anchor_pos == -1) {
            anchor_pos = editor_state.cursor_index;
        }
        editor_state.selection_start = anchor_pos;
        editor_state.selection_end = char_index;
        editor_state.cursor_index = char_index;
    }
}

void EditorMouse::handleMouseRelease()
{
    is_dragging = false;
    anchor_pos = -1;
}

void EditorMouse::handleContextMenu()
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
        if (MenuItemWithAlignedShortcut("Cut", "Ctrl+X", nullptr, editor_state.selection_active)) {
            gEditorCopyPaste.cutSelectedText();
            show_context_menu = false;
            ImGui::CloseCurrentPopup();
        }

        // Copy action
        if (MenuItemWithAlignedShortcut("Copy", "Ctrl+C", nullptr, editor_state.selection_active)) {
            gEditorCopyPaste.copySelectedText(editor_state.fileContent);
            show_context_menu = false;
            ImGui::CloseCurrentPopup();
        }

        // Paste action
        if (MenuItemWithAlignedShortcut("Paste", "Ctrl+V", nullptr, true)) {
            gEditorCopyPaste.pasteText();
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
            editor_state.selection_active = true;
            editor_state.selection_start = 0;
            editor_state.selection_end = editor_state.fileContent.size();
            editor_state.cursor_index = editor_state.fileContent.size();
            show_context_menu = false;
            ImGui::CloseCurrentPopup();
        }
        ImGui::Separator();

        // Go to Definition
        if (MenuItemWithAlignedShortcut("Go to Definition", "F12", nullptr, true)) {
            // Get current line number from editor_state
            int current_line = gEditor.getLineFromPos(editor_state.cursor_index);

            // Get character offset in current line
            int line_start = editor_state.editor_content_lines[current_line];
            int char_offset = editor_state.cursor_index - line_start;

            // Call LSP goto definition
            gLSPGotoDef.gotoDefinition(gFileExplorer.currentFile, current_line, char_offset);

            show_context_menu = false;
            ImGui::CloseCurrentPopup();
        }

        // --- ADD FIND REFERENCES OPTION HERE ---
        if (MenuItemWithAlignedShortcut("Find References", "Shift+F12", nullptr, true)) { // Using Shift+F12 as a common shortcut
                                                                                          // Get current line number from editor_state (same as above)
            int current_line = gEditor.getLineFromPos(editor_state.cursor_index);

            // Get character offset in current line (same as above)
            int line_start = 0; // Default to 0
            if (current_line >= 0 && current_line < editor_state.editor_content_lines.size()) {
                line_start = editor_state.editor_content_lines[current_line];
            }
            int char_offset = editor_state.cursor_index - line_start;
            char_offset = std::max(0, char_offset); // Ensure non-negative

            // Call LSP find references using the new global instance
            gLSPGotoRef.findReferences(gFileExplorer.currentFile, current_line, char_offset);

            show_context_menu = false;
            ImGui::CloseCurrentPopup();
        }
        // --- END OF ADDED OPTION ---

        ImGui::EndPopup();
    } else {
        // If popup is not open (closed by ImGui itself)
        if (show_context_menu) {
            show_context_menu = false;
            std::cout << "Popup closed by ImGui" << std::endl;
        }
    }

    // Pop styling
    ImGui::PopStyleColor(5);
    ImGui::PopStyleVar(6);
}

int EditorMouse::getCharIndexFromCoords()
{
    ImVec2 mouse_pos = ImGui::GetMousePos();

    // Determine which line was clicked (clamped to valid indices)
    int clicked_line = std::clamp(static_cast<int>((mouse_pos.y - editor_state.text_pos.y) / editor_state.line_height), 0, static_cast<int>(editor_state.editor_content_lines.size()) - 1);

    // Get start/end indices for that line in the text.
    int line_start = editor_state.editor_content_lines[clicked_line];
    int line_end = (clicked_line + 1 < editor_state.editor_content_lines.size()) ? editor_state.editor_content_lines[clicked_line + 1] : editor_state.fileContent.size();

    // Adjust line_end to exclude newline character if present
    if (line_end > line_start && line_end <= editor_state.fileContent.size() && editor_state.fileContent[line_end - 1] == '\n') {
        line_end--;
    }

    int n = line_end - line_start; // number of characters in the line

    // If the line is empty, return its start.
    if (n <= 0)
        return line_start;

    // Compute the click's x-coordinate relative to the beginning of the text.
    float click_x = mouse_pos.x - editor_state.text_pos.x;

    // For more accuracy, calculate character widths individually to avoid accumulating errors
    // This is especially important for longer lines where small errors add up
    std::vector<float> charWidths(n + 1, 0.0f);
    std::vector<float> insertionPositions(n + 1, 0.0f);

    insertionPositions[0] = 0.0f;

    // Calculate the width of each character individually
    for (int i = 0; i < n; i++) {
        char buf[2] = {editor_state.fileContent[line_start + i], '\0'};
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