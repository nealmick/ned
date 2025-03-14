/*
    util/bookmarks.h
    This utility allows for saving a bookmark and restore back to the bookmark position.
    Each bookmark stores the current file, the cursor location, and scroll position.
    Up to 10 bookmarks can be set at once, to set a bookmark press cmd option 0-9.
    To restore a bookmark postion press cmd 0-9 with the bookmark you want to restore.
    This system allows for quick file and cursor position swapping without the need to
    keep multiple files open.
*/

#pragma once
#include "editor.h"
#include "editor_scroll.h"
#include "editor_types.h"

#include "imgui.h"
#include <array>
#include <string>

#include "../files/files.h"

class Bookmarks
{
  private:
    struct Bookmark
    {
        std::string filePath;
        int cursorPosition;
        int lineNumber;
        float scrollX;
        float scrollY;
        bool isSet;

        // Default constructor
        Bookmark() : filePath(""), cursorPosition(0), lineNumber(0), scrollX(0), scrollY(0), isSet(false) {}

        // Main constructor - updated to include scroll positions
        Bookmark(const std::string &path, int cursor, int line, float sx, float sy, bool set) : filePath(path), cursorPosition(cursor), lineNumber(line), scrollX(sx), scrollY(sy), isSet(set) {}
    };

    static constexpr size_t NUM_BOOKMARKS = 9;
    std::array<Bookmark, NUM_BOOKMARKS> bookmarks;

  public:
    bool showBookmarksWindow = false;

    inline void setBookmark(size_t slot, const std::string &filePath, int cursorPosition, int lineNumber)
    {
        if (slot < NUM_BOOKMARKS) {
            float scrollX = ImGui::GetScrollX();
            float scrollY = ImGui::GetScrollY();
            bookmarks[slot] = Bookmark(filePath, cursorPosition, lineNumber, scrollX, scrollY, true);

            // Debug print
            std::cout << "Set bookmark " << slot << " with scroll positions - X: " << scrollX << ", Y: " << scrollY << std::endl;
        }
    }
    inline bool jumpToBookmark(size_t slot, FileExplorer &fileExplorer, EditorState &editorState)
    {
        if (slot < NUM_BOOKMARKS && bookmarks[slot].isSet) {
            if (bookmarks[slot].filePath != fileExplorer.getCurrentFile()) {
                // Create callback to set scroll after file loads
                auto setScroll = [this, slot]() { gEditorScroll.requestScroll(bookmarks[slot].scrollX, bookmarks[slot].scrollY); };

                fileExplorer.loadFileContent(bookmarks[slot].filePath, setScroll);
            } else {
                // Same file, request scroll immediately
                gEditorScroll.requestScroll(bookmarks[slot].scrollX, bookmarks[slot].scrollY);
            }

            editorState.cursor_index = bookmarks[slot].cursorPosition; // use content string index...
            // editorState.cursor_row = bookmarks[slot].lineNumber;
            gEditorScroll.setEnsureCursorVisibleFrames(-1); // Use EditorScroll method instead
            return true;
        }
        return false;
    }

    inline void handleBookmarkInput(FileExplorer &fileExplorer, EditorState &editorState)
    {
        bool main_key = ImGui::GetIO().KeyCtrl || ImGui::GetIO().KeySuper;
        bool shift_pressed = ImGui::GetIO().KeyShift;
        bool alt_pressed = ImGui::GetIO().KeyAlt;

        if (main_key && ImGui::IsKeyPressed(ImGuiKey_B, false)) {
            showBookmarksWindow = !showBookmarksWindow;
            if (showBookmarksWindow) {                                     // Only close others if we're opening
                ClosePopper::closeAllExcept(ClosePopper::Type::Bookmarks); // RIGHT
            }
            return;
        }
        const ImGuiKey numberKeys[9] = {ImGuiKey_1, ImGuiKey_2, ImGuiKey_3, ImGuiKey_4, ImGuiKey_5, ImGuiKey_6, ImGuiKey_7, ImGuiKey_8, ImGuiKey_9};

        for (size_t i = 0; i < NUM_BOOKMARKS; ++i) {
            if (main_key && ImGui::IsKeyPressed(numberKeys[i])) {
                if (shift_pressed || alt_pressed) {
                    int lineNumber = EditorUtils::GetLineFromPosition(editor_state.editor_content_lines, editor_state.cursor_index);
                    setBookmark(i, fileExplorer.getCurrentFile(), editorState.cursor_index, lineNumber);
                    std::cout << "Bookmark " << (i + 1) << " set at line " << lineNumber << std::endl;
                } else {
                    if (jumpToBookmark(i, fileExplorer, editorState)) {
                        std::cout << "Jumped to bookmark " << (i + 1) << std::endl;
                    } else {
                        std::cout << "Bookmark " << (i + 1) << " not set" << std::endl;
                    }
                }
            }
        }
    }

    inline void renderBookmarksWindow()
    {
        if (showBookmarksWindow) {
            ImGui::SetNextWindowSize(ImVec2(500, 300), ImGuiCond_Always);
            ImGui::SetNextWindowPos(ImVec2(ImGui::GetIO().DisplaySize.x * 0.5f, ImGui::GetIO().DisplaySize.y * 0.35f), ImGuiCond_Always, ImVec2(0.5f, 0.5f));

            ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;

            // Push custom styles for the window
            ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 10.0f);  // Add rounded corners
            ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.0f); // Add border
            ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.2f, 0.2f, 0.2f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.3f, 0.3f, 0.3f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.15f, 0.15f, 0.15f, 1.0f));

            ImGui::Begin("Bookmarks", nullptr, windowFlags);

            ImGui::TextUnformatted("Bookmarks");
            ImGui::Separator();

            ImGui::BeginChild("ScrollingRegion", ImVec2(0, -ImGui::GetFrameHeightWithSpacing()), false, ImGuiWindowFlags_HorizontalScrollbar);
            for (size_t i = 0; i < NUM_BOOKMARKS; ++i) {
                if (bookmarks[i].isSet) {
                    ImGui::TextColored(ImVec4(0.7f, 0.7f, 1.0f, 1.0f), "%zu:", i + 1);
                    ImGui::SameLine();
                    ImGui::TextWrapped("%s Line %d", bookmarks[i].filePath.c_str(), bookmarks[i].lineNumber - 1);
                } else {
                    ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), " %zu: Empty slot", i + 1);
                }
            }
            ImGui::EndChild();

            ImGui::Separator();
            ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Press Cmd+B to close this window");

            ImGui::End();

            // Pop the styles we pushed earlier
            ImGui::PopStyleColor(3);
            ImGui::PopStyleVar(2);
        }
    }
    inline bool isWindowOpen() const { return showBookmarksWindow; }
};

extern Bookmarks gBookmarks;