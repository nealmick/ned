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
        float scrollX;
        float scrollY;
        bool isSet;

        // Default constructor
        Bookmark() : filePath(""), cursorPosition(0), lineNumber(0), scrollX(0), scrollY(0), isSet(false) {}
        
        // Main constructor - updated to include scroll positions
        Bookmark(const std::string& path, int cursor, int line, float sx, float sy, bool set)
            : filePath(path), 
              cursorPosition(cursor), 
              lineNumber(line), 
              scrollX(sx), 
              scrollY(sy), 
              isSet(set) {}
    };

    static constexpr size_t NUM_BOOKMARKS = 9;
    std::array<Bookmark, NUM_BOOKMARKS> bookmarks;
    bool showBookmarksWindow = false;

public:
    inline void setBookmark(size_t slot, const std::string& filePath, int cursorPosition, int lineNumber) {
        if (slot < NUM_BOOKMARKS) {
            float scrollX = ImGui::GetScrollX();
            float scrollY = ImGui::GetScrollY();
            bookmarks[slot] = Bookmark(filePath, cursorPosition, lineNumber, scrollX, scrollY, true);
            
            // Debug print
            std::cout << "Set bookmark " << slot << " with scroll positions - X: " 
                      << scrollX << ", Y: " << scrollY << std::endl;
        }
    }
    inline bool jumpToBookmark(size_t slot, FileExplorer& fileExplorer, EditorState& editorState) {
        if (slot < NUM_BOOKMARKS && bookmarks[slot].isSet) {
            if (bookmarks[slot].filePath != fileExplorer.getCurrentFile()) {
                // Create callback to set scroll after file loads
                auto setScroll = [this, slot]() {
                    gEditor.requestScroll(bookmarks[slot].scrollX, bookmarks[slot].scrollY);
                };
                
                fileExplorer.loadFileContent(bookmarks[slot].filePath, setScroll);
            } else {
                // Same file, request scroll immediately
                gEditor.requestScroll(bookmarks[slot].scrollX, bookmarks[slot].scrollY);
            }
            
            editorState.cursor_pos = bookmarks[slot].cursorPosition;
            editorState.current_line = bookmarks[slot].lineNumber;
            editorState.ensure_cursor_visible_frames = -1;
            return true;
        }
        return false;
    }
    
    inline void handleBookmarkInput(FileExplorer& fileExplorer, EditorState& editorState) {
        bool main_key = ImGui::GetIO().KeyCtrl || ImGui::GetIO().KeySuper;
        bool shift_pressed = ImGui::GetIO().KeyShift;
        bool alt_pressed = ImGui::GetIO().KeyAlt;

        if (main_key && ImGui::IsKeyPressed(ImGuiKey_B, false)) {  // false to not repeat
            showBookmarksWindow = !showBookmarksWindow;
            //editorState.blockInput = showBookmarksWindow;  // Block input when window is open
            return;
        }

        const ImGuiKey numberKeys[9] = {
            ImGuiKey_1, ImGuiKey_2, ImGuiKey_3, ImGuiKey_4, ImGuiKey_5,
            ImGuiKey_6, ImGuiKey_7, ImGuiKey_8, ImGuiKey_9
        };

        for (size_t i = 0; i < NUM_BOOKMARKS; ++i) {
            if (main_key && ImGui::IsKeyPressed(numberKeys[i])) {
                if (shift_pressed || alt_pressed) {
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
    inline void renderBookmarksWindow() {
        if (showBookmarksWindow) {
            ImGui::SetNextWindowSize(ImVec2(600, 400));
            ImGui::SetNextWindowPos(ImVec2(ImGui::GetIO().DisplaySize.x * 0.5f, ImGui::GetIO().DisplaySize.y * 0.5f), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
            
            ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | 
                                           ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings | 
                                           ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoTitleBar;

            ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.0f);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 5.0f);
            ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.2f, 0.2f, 0.2f, 1.0f)); // Solid, opaque dark gray background
            ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.7f, 0.7f, 0.7f, 1.0f));
            
            ImGui::Begin("Bookmarks", nullptr, windowFlags);
            
            ImGui::SetWindowFontScale(1.2f);
            ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), "Bookmarks");
            ImGui::Separator();

            ImGui::BeginChild("ScrollingRegion", ImVec2(0, -ImGui::GetFrameHeightWithSpacing()), false, ImGuiWindowFlags_HorizontalScrollbar);
            for (size_t i = 0; i < NUM_BOOKMARKS; ++i) {
                if (bookmarks[i].isSet) {
                    ImGui::TextColored(ImVec4(0.7f, 0.7f, 1.0f, 1.0f), "%zu:", i + 1);
                    ImGui::SameLine();
                    ImGui::TextWrapped("%s (Line %d, Pos %d)", 
                        bookmarks[i].filePath.c_str(), 
                        bookmarks[i].lineNumber, bookmarks[i].cursorPosition);
                } else {
                    ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), " %zu: Empty slot", i + 1);
                }
            }
            ImGui::EndChild();

            ImGui::Separator();
            ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Press Cmd+B to close this window");

            ImGui::End();

            ImGui::PopStyleColor(2);
            ImGui::PopStyleVar(2);
        }
    }

    inline bool isWindowOpen() const {
        return showBookmarksWindow;
    }
};

extern Bookmarks gBookmarks;