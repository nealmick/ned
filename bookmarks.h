#pragma once
#include <array>
#include <string>
#include "editor.h"
#include "files.h"
#include "imgui.h"

class Bookmarks {
private:
    struct Bookmark {
        std::string filePath;
        int cursorPosition;
        int lineNumber;
        bool isSet;

        Bookmark() : filePath(""), cursorPosition(0), lineNumber(0), isSet(false) {}
    };

    static constexpr size_t NUM_BOOKMARKS = 9;
    std::array<Bookmark, NUM_BOOKMARKS> bookmarks;

public:
    inline void setBookmark(size_t slot, const std::string& filePath, int cursorPosition, int lineNumber) {
        if (slot < NUM_BOOKMARKS) {
            bookmarks[slot].filePath = filePath;
            bookmarks[slot].cursorPosition = cursorPosition;
            bookmarks[slot].lineNumber = lineNumber;
            bookmarks[slot].isSet = true;
        }
    }

    inline bool jumpToBookmark(size_t slot, FileExplorer& fileExplorer, EditorState& editorState) {
        if (slot < NUM_BOOKMARKS && bookmarks[slot].isSet) {
            if (bookmarks[slot].filePath != fileExplorer.getCurrentFile()) {
                fileExplorer.saveCurrentFile();
                fileExplorer.loadFileContent(bookmarks[slot].filePath);
            }
            editorState.cursor_pos = bookmarks[slot].cursorPosition;
            editorState.current_line = bookmarks[slot].lineNumber;
            return true;
        }
        return false;
    }

    inline void handleBookmarkInput(FileExplorer& fileExplorer, EditorState& editorState) {
        bool ctrl_pressed = ImGui::GetIO().KeyCtrl;
        bool shift_pressed = ImGui::GetIO().KeyShift;

        const ImGuiKey numberKeys[9] = {
            ImGuiKey_1, ImGuiKey_2, ImGuiKey_3, ImGuiKey_4, ImGuiKey_5,
            ImGuiKey_6, ImGuiKey_7, ImGuiKey_8, ImGuiKey_9
        };

        for (int i = 0; i < NUM_BOOKMARKS; ++i) {
            if (ctrl_pressed && ImGui::IsKeyPressed(numberKeys[i])) {
                if (shift_pressed) {
                    setBookmark(i, fileExplorer.getCurrentFile(), editorState.cursor_pos, editorState.current_line);
                    std::cout << "Bookmark " << (i + 1) << " set at line " << editorState.current_line << std::endl;
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
};

extern Bookmarks gBookmarks;