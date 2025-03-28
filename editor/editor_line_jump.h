/*
    util/line_jump.h
    This utility handles line changes, such as jumping to line 300...
    Use keybind cmd : to activate the number input box, type a number then press enter.
    The cursor jumps to the line number and scrolls the view to move show the cursor.
*/

#pragma once
#include "../files/files.h"
#include "../imgui/imgui.h"
#include "../util/close_popper.h"
#include "editor.h"
#include "editor_scroll.h"
#include "editor_types.h"

#include <string>

class LineJump
{
  private:
    char lineNumberBuffer[32] = "";
    bool justJumped = false; // Track if we just performed a jump

  public:
    bool showLineJumpWindow = false;
    inline void handleLineJumpInput()
    {
        bool main_key = ImGui::GetIO().KeyCtrl || ImGui::GetIO().KeySuper;
        bool shift_pressed = ImGui::GetIO().KeyShift;

        // Reset justJumped if Enter isn't being pressed
        if (!ImGui::IsKeyPressed(ImGuiKey_Enter)) {
            justJumped = false;
        }

        if (main_key && (ImGui::IsKeyPressed(ImGuiKey_Semicolon) || (shift_pressed && ImGui::IsKeyPressed(ImGuiKey_Semicolon)))) {
            showLineJumpWindow = !showLineJumpWindow;
            if (showLineJumpWindow) {                                     // Only close others if we're opening
                ClosePopper::closeAllExcept(ClosePopper::Type::LineJump); // RIGHT
            }
            if (showLineJumpWindow) {
                memset(lineNumberBuffer, 0, sizeof(lineNumberBuffer));
                ImGui::SetKeyboardFocusHere();
            }
            editor_state.block_input = showLineJumpWindow;
        }
    }

    inline void renderLineJumpWindow()
    {
        if (!showLineJumpWindow)
            return;

        ImGui::GetIO().WantTextInput = true;
        editor_state.block_input = true;

        // Push custom styles for the window
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 10.0f);  // Add rounded corners
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.0f); // Add border

        // Balanced window padding - not too much, not too little
        float fontHeight = ImGui::GetFontSize();
        float paddingHorizontal = fontHeight * 0.75f; // Horizontal padding scales with font
        float paddingVertical = fontHeight * 0.5f;    // Vertical padding scales with font

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(paddingHorizontal, paddingVertical));
        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.2f, 0.2f, 0.2f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.3f, 0.3f, 0.3f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.15f, 0.15f, 0.15f, 1.0f));

        // Calculate dimensions based on font size
        float itemSpacing = fontHeight * 0.3f;  // Space between elements
        float inputHeight = fontHeight * 1.5f;  // Input field height
        float windowWidth = fontHeight * 20.0f; // Width scales with font size

        // Calculate window height with proper spacing
        float windowHeight = paddingVertical * 2 + // Top and bottom padding
                             fontHeight +          // Title text
                             itemSpacing +         // Space after title
                             inputHeight +         // Input field
                             itemSpacing +         // Space after input
                             fontHeight;           // Help text

        // Position window
        ImGui::SetNextWindowSize(ImVec2(windowWidth, windowHeight), ImGuiCond_Always);
        ImGui::SetNextWindowPos(ImVec2(ImGui::GetIO().DisplaySize.x * 0.5f, ImGui::GetIO().DisplaySize.y * 0.2f), ImGuiCond_Always, ImVec2(0.5f, 0.5f));

        ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;

        // Set custom item spacing
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(paddingHorizontal, itemSpacing));

        if (ImGui::Begin("Line Jump", nullptr, windowFlags)) {
            // Make input field use full width
            ImGui::PushItemWidth(-1);

            // Title
            ImGui::TextUnformatted("Jump to line:");

            // Custom frame padding for the input field
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(paddingHorizontal * 0.5f, fontHeight * 0.25f));

            // Set keyboard focus to input field
            ImGui::SetKeyboardFocusHere();

            if (ImGui::InputText("##linejump", lineNumberBuffer, sizeof(lineNumberBuffer), ImGuiInputTextFlags_CharsDecimal | ImGuiInputTextFlags_EnterReturnsTrue)) {
                int lineNumber = std::atoi(lineNumberBuffer);
                jumpToLine(lineNumber - 1);
                showLineJumpWindow = false;
                editor_state.block_input = false;
                memset(lineNumberBuffer, 0, sizeof(lineNumberBuffer));
                justJumped = true;
                ImGui::GetIO().ClearInputCharacters();
            }

            ImGui::PopStyleVar(); // Pop FramePadding style

            // Instruction text
            ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Type line number then Enter");

            if (ImGui::IsKeyPressed(ImGuiKey_Escape)) {
                showLineJumpWindow = false;
                editor_state.block_input = false;
                memset(lineNumberBuffer, 0, sizeof(lineNumberBuffer));
            }

            ImGui::PopItemWidth();
        }
        ImGui::End();

        // Pop the styles we pushed earlier
        ImGui::PopStyleVar(); // Pop ItemSpacing style
        ImGui::PopStyleColor(3);
        ImGui::PopStyleVar(3); // Pop WindowRounding, WindowBorderSize, WindowPadding
    }

    inline void jumpToLine(int lineNumber)
    {
        if (lineNumber < 0)
            lineNumber = 0;
        if (lineNumber >= static_cast<int>(editor_state.editor_content_lines.size())) {
            lineNumber = static_cast<int>(editor_state.editor_content_lines.size()) - 1;
        }

        // Set cursor to the beginning of the requested line
        editor_state.cursor_index = editor_state.editor_content_lines[lineNumber];
        editor_state.selection_start = editor_state.cursor_index;
        editor_state.selection_end = editor_state.cursor_index;
        std::cout << "seting line number in linejump " << lineNumber << std::endl;
        editor_state.selection_active = false;

        // Calculate the target scroll position
        float line_height = ImGui::GetTextLineHeight();
        float window_height = ImGui::GetWindowHeight();

        // Calculate the exact middle of the window in pixels
        float middle_pixel = window_height / 2.0f;

        // For a line to appear in the middle, we need:
        // scroll_position = (line_number * line_height) - middle_pixel
        float target_scroll = (lineNumber * line_height) - middle_pixel;

        // Apply a fixed offset to adjust how many lines from top the target appears
        // Negative values move the line higher (fewer lines from top)
        // Positive values move the line lower (more lines from top)
        // Try different values to find the perfect position
        float vertical_offset = -250.0f; // Adjust this to change vertical position
        target_scroll += vertical_offset;

        // Ensure we don't scroll past the boundaries
        target_scroll = std::max(0.0f, target_scroll);

        // REQUEST scroll through EditorScroll
        gEditorScroll.requestScroll(0, target_scroll);

        // Set these flags to ensure the cursor will be made visible
        gEditorScroll.setEnsureCursorVisibleFrames(5); // Ensure visibility for several frames

        // Force a direct scroll application in ImGui before the next frame
        ImGui::SetScrollY(target_scroll);

        justJumped = true;
    }

    inline bool isWindowOpen() const { return showLineJumpWindow; }

    inline bool hasJustJumped() const { return justJumped; }
};

extern LineJump gLineJump;