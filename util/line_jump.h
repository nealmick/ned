/*
    util/line_jump.h
    This utility handles line changes, such as jumping to line 300...
    Use keybind cmd : to activate the number input box, type a number then press enter.
    The cursor jumps to the line number and scrolls the view to move show the cursor.
*/

#pragma once
#include "../editor.h"
#include "../files.h"
#include "../imgui/imgui.h"
#include "close_popper.h"
#include <string>

class LineJump
{
  private:
    char lineNumberBuffer[32] = "";
    bool justJumped = false; // Track if we just performed a jump

  public:
    bool showLineJumpWindow = false;
    inline void handleLineJumpInput(EditorState &state)
    {
        bool main_key = ImGui::GetIO().KeyCtrl || ImGui::GetIO().KeySuper;
        bool shift_pressed = ImGui::GetIO().KeyShift;

        // Reset justJumped if Enter isn't being pressed
        if (!ImGui::IsKeyPressed(ImGuiKey_Enter))
        {
            justJumped = false;
        }

        if (main_key && (ImGui::IsKeyPressed(ImGuiKey_Semicolon) || (shift_pressed && ImGui::IsKeyPressed(ImGuiKey_Semicolon))))
        {
            showLineJumpWindow = !showLineJumpWindow;
            if (showLineJumpWindow)
            {                                                             // Only close others if we're opening
                ClosePopper::closeAllExcept(ClosePopper::Type::LineJump); // RIGHT
            }
            if (showLineJumpWindow)
            {
                memset(lineNumberBuffer, 0, sizeof(lineNumberBuffer));
                ImGui::SetKeyboardFocusHere();
            }
            state.blockInput = showLineJumpWindow;
        }
    }

    inline void renderLineJumpWindow(EditorState &state)
    {
        if (!showLineJumpWindow)
            return;

        ImGui::GetIO().WantTextInput = true;
        state.blockInput = true;

        // Push custom styles for the window
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 10.0f);  // Add rounded corners
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.0f); // Add border
        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.2f, 0.2f, 0.2f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.3f, 0.3f, 0.3f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.15f, 0.15f, 0.15f, 1.0f));

        ImGui::SetNextWindowSize(ImVec2(400, 90), ImGuiCond_Always);
        ImGui::SetNextWindowPos(ImVec2(ImGui::GetIO().DisplaySize.x * 0.5f, ImGui::GetIO().DisplaySize.y * 0.2f), ImGuiCond_Always, ImVec2(0.5f, 0.5f));

        ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;

        if (ImGui::Begin("Line Jump", nullptr, windowFlags))
        {
            ImGui::TextUnformatted("Jump to line:");

            ImGui::SetKeyboardFocusHere();

            if (ImGui::InputText("##linejump", lineNumberBuffer, sizeof(lineNumberBuffer), ImGuiInputTextFlags_CharsDecimal | ImGuiInputTextFlags_EnterReturnsTrue))
            {
                int lineNumber = std::atoi(lineNumberBuffer);
                jumpToLine(lineNumber - 1, state);
                showLineJumpWindow = false;
                state.blockInput = false;
                memset(lineNumberBuffer, 0, sizeof(lineNumberBuffer));
                justJumped = true;
                ImGui::GetIO().ClearInputCharacters();
            }

            ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Type line number then Enter");

            if (ImGui::IsKeyPressed(ImGuiKey_Escape))
            {
                showLineJumpWindow = false;
                state.blockInput = false;
                memset(lineNumberBuffer, 0, sizeof(lineNumberBuffer));
            }
        }
        ImGui::End();

        // Pop the styles we pushed earlier
        ImGui::PopStyleColor(3);
        ImGui::PopStyleVar(2);
    }

    inline void jumpToLine(int lineNumber, EditorState &state)
    {
        if (lineNumber < 0)
            lineNumber = 0;
        if (lineNumber >= static_cast<int>(state.line_starts.size()))
        {
            lineNumber = static_cast<int>(state.line_starts.size()) - 1;
        }

        // Set cursor to the beginning of the requested line
        state.cursor_pos = state.line_starts[lineNumber];
        state.selection_start = state.cursor_pos;
        state.selection_end = state.cursor_pos;
        state.current_line = lineNumber;
        state.is_selecting = false;

        // Calculate the target scroll position
        float line_height = ImGui::GetTextLineHeight();
        float window_height = ImGui::GetWindowHeight();

        // Calculate number of visible lines
        float visible_lines = window_height / line_height;

        // Target the 12th line position (adjust this number to your preference)
        const float TARGET_LINE = 14.0f;

        // Calculate scroll position to put cursor at the target line
        float target_scroll = (lineNumber * line_height) - (TARGET_LINE * line_height);

        // Ensure we don't scroll past the boundaries
        target_scroll = std::max(0.0f, target_scroll);

        // Use the editor's scroll request system
        gEditor.requestScroll(state.scroll_x, target_scroll);

        state.ensure_cursor_visible_frames = 0;
        state.pendingBookmarkScroll = true;
        state.pendingScrollY = target_scroll;

        std::cout << "Jumping to line " << (lineNumber + 1) << " (target scroll pos: " << target_scroll << ", window height: " << window_height << ", visible lines: " << visible_lines << ", line height: " << line_height << ")" << std::endl;
    }

    inline bool isWindowOpen() const { return showLineJumpWindow; }

    inline bool hasJustJumped() const { return justJumped; }
};

extern LineJump gLineJump;