/*
    editor.h
    Main editor logic for displaying file content, handling keybinds and more...
*/

#pragma once
#include "imgui.h"
#include "lexers/cpp.h"
#include "lexers/html.h"
#include "lexers/jsx.h"
#include "lexers/python.h"

#include <atomic>
#include <cmath>
#include <future>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

// Utility structures
struct ScrollChange {
    bool vertical;
    bool horizontal;
};

struct CursorVisibility {
    bool vertical;
    bool horizontal;
};

// Editor state management
struct EditorState {
    // Cursor and selection state
    int cursor_pos;
    int current_line;
    int selection_start;
    int selection_end;
    bool is_selecting;

    // Display and scrolling
    std::vector<int> line_starts;
    ImVec2 scroll_pos;
    float scroll_x;
    float cursor_blink_time;
    int ensure_cursor_visible_frames;

    // Visual settings
    bool rainbow_cursor;

    // Input state
    bool activateFindBox;
    bool blockInput;
    bool full_text_selected;

    // Bookmark related
    bool pendingBookmarkScroll;
    float pendingScrollX;
    float pendingScrollY;

    EditorState() : ensure_cursor_visible_frames(0), cursor_pos(0), selection_start(0), selection_end(0), is_selecting(false), line_starts({0}), scroll_pos(0, 0), scroll_x(0.0f), rainbow_cursor(true), cursor_blink_time(0.0f), activateFindBox(false), blockInput(false), current_line(0) {}
};

// Forward declarations
class Editor;
extern Editor gEditor;
extern EditorState editor_state;

class Editor {
  public:
    // main editor function
    bool textEditor(const char *label, std::string &text, std::vector<ImVec4> &colors, EditorState &editor_state);

    // Core text editing operations
    void moveWordForward(const std::string &text, EditorState &state);
    void moveWordBackward(const std::string &text, EditorState &state);
    void removeIndentation(std::string &text, EditorState &state);

    // Syntax highlighting and themes
    void setTheme(const std::string &themeName);
    void highlightContent(const std::string &content, std::vector<ImVec4> &colors, int start_pos, int end_pos);
    void cancelHighlighting();
    void forceColorUpdate();

    // Scroll management
    void requestScroll(float x, float y) {
        requestedScrollX = x;
        requestedScrollY = y;
        hasScrollRequest = true;
    }

    bool handleScrollRequest(float &outScrollX, float &outScrollY) {
        if (hasScrollRequest) {
            outScrollX = requestedScrollX;
            outScrollY = requestedScrollY;
            hasScrollRequest = false;
            return true;
        }
        return false;
    }

    int getCharIndexFromCoords(const std::string &text, const ImVec2 &click_pos, const ImVec2 &text_start_pos, const std::vector<int> &line_starts, float line_height);

    void handleCursorMovement(const std::string &text, EditorState &state, const ImVec2 &text_pos, float line_height, float window_height, float window_width);
    void handleEditorInput(std::string &text, EditorState &state, const ImVec2 &text_start_pos, float line_height, bool &text_changed, std::vector<ImVec4> &colors, CursorVisibility &ensure_cursor_visible);

    void updateLineStarts(const std::string &text, std::vector<int> &line_starts);
    int getLineFromPos(const std::vector<int> &line_starts, int pos);

    // Selection management
    void startSelection(EditorState &state);
    void updateSelection(EditorState &state);
    void endSelection(EditorState &state);
    int getSelectionStart(const EditorState &state);
    int getSelectionEnd(const EditorState &state);
    float getCursorYPosition(const EditorState &state, float line_height);

    // Clipboard operations
    void copySelectedText(const std::string &text, const EditorState &state);
    void cutSelectedText(std::string &text, std::vector<ImVec4> &colors, EditorState &state, bool &text_changed);
    void cutWholeLine(std::string &text, std::vector<ImVec4> &colors, EditorState &state, bool &text_changed);
    void pasteText(std::string &text, std::vector<ImVec4> &colors, EditorState &state, bool &text_changed);

    // Cursor and viewport management
    float calculateCursorXPosition(const ImVec2 &text_pos, const std::string &text, int cursor_pos);
    ScrollChange ensureCursorVisible(const ImVec2 &text_pos, const std::string &text, EditorState &state, float line_height, float window_height, float window_width);

    // Text selection
    void selectAllText(EditorState &state, const std::string &text);

    // Input handling
    void handleMouseInput(const std::string &text, EditorState &state, const ImVec2 &text_start_pos, float line_height);
    void handleTextInput(std::string &text, std::vector<ImVec4> &colors, EditorState &state, bool &text_changed);

    // Cursor movement
    void cursorLeft(EditorState &state);
    void cursorRight(const std::string &text, EditorState &state);
    void cursorUp(const std::string &text, EditorState &state, float line_height, float window_height);
    void cursorDown(const std::string &text, EditorState &state, float line_height, float window_height);
    void moveCursorVertically(std::string &text, EditorState &state, int line_delta);

    // Rendering
    void renderTextWithSelection(ImDrawList *drawList, const ImVec2 &pos, const std::string &text, const std::vector<ImVec4> &colors, const EditorState &state, float line_height);
    void renderCursor(ImDrawList *draw_list, const ImVec2 &cursor_screen_pos, float line_height, float blink_time);
    void renderLineNumbers(const ImVec2 &pos, float line_number_width, float line_height, int num_lines, float scroll_y, float window_height, const EditorState &editor_state, float blink_time);

    // Utility functions
    float calculateTextWidth(const std::string &text, const std::vector<int> &line_starts);

  private:
    // Lexer instances
    PythonLexer::Lexer pythonLexer;
    CppLexer::Lexer cppLexer;
    HtmlLexer::Lexer htmlLexer;
    JsxLexer::Lexer jsxLexer;

    // Theme management
    std::unordered_map<std::string, ImVec4> themeColors;
    void loadTheme(const std::string &themeName);

    // Highlighting state management
    std::mutex highlight_mutex;
    std::future<void> highlightFuture;
    std::atomic<bool> highlightingInProgress{false};
    std::mutex colorsMutex;
    std::atomic<bool> cancelHighlightFlag{false};

    // Scroll state
    float requestedScrollX = 0;
    float requestedScrollY = 0;
    bool hasScrollRequest = false;

    void handleCharacterInput(std::string &text, std::vector<ImVec4> &colors, EditorState &state, bool &text_changed, int &input_start, int &input_end);
    void handleEnterKey(std::string &text, std::vector<ImVec4> &colors, EditorState &state, bool &text_changed, int &input_end);
    void handleDeleteKey(std::string &text, std::vector<ImVec4> &colors, EditorState &state, bool &text_changed, int &input_end);
    void handleBackspaceKey(std::string &text, std::vector<ImVec4> &colors, EditorState &state, bool &text_changed, int &input_start);
    void handleTabKey(std::string &text, std::vector<ImVec4> &colors, EditorState &state, bool &text_changed, int &input_end);

    // handling editor input
    bool processIndentRemoval(std::string &text, EditorState &state, bool &text_changed, CursorVisibility &ensure_cursor_visible);
    void processFontSizeAdjustment(CursorVisibility &ensure_cursor_visible);
    void processSelectAll(std::string &text, EditorState &state, CursorVisibility &ensure_cursor_visible);
    void processUndoRedo(std::string &text, std::vector<ImVec4> &colors, EditorState &state, bool &text_changed, CursorVisibility &ensure_cursor_visible, bool shift_pressed);
    void processWordMovement(std::string &text, EditorState &state, CursorVisibility &ensure_cursor_visible, bool shift_pressed);
    void processCursorJump(std::string &text, EditorState &state, CursorVisibility &ensure_cursor_visible);
    void processMouseWheelScrolling(float line_height, EditorState &state);
    void processClipboardShortcuts(std::string &text, std::vector<ImVec4> &colors, EditorState &state, bool &text_changed, CursorVisibility &ensure_cursor_visible);

    bool validateAndResizeColors(std::string &text, std::vector<ImVec4> &colors);
    void setupEditorWindow(const char *label, ImVec2 &size, float &line_number_width, float &line_height, float &editor_top_margin, float &text_left_margin);
    ImVec2 renderLineNumbersPanel(float line_number_width, float editor_top_margin);
    void beginTextEditorChild(const char *label, float remaining_width, float content_width, float content_height, float &current_scroll_y, float &current_scroll_x, ImVec2 &text_pos, float editor_top_margin, float text_left_margin, EditorState &editor_state);
    void processTextEditorInput(std::string &text, EditorState &editor_state, const ImVec2 &text_start_pos, float line_height, bool &text_changed, std::vector<ImVec4> &colors, CursorVisibility &ensure_cursor_visible, int initial_cursor_pos);
    void processMouseWheelForEditor(float line_height, float &current_scroll_y, float &current_scroll_x, EditorState &editor_state);
    void adjustScrollForCursorVisibility(const ImVec2 &text_pos, const std::string &text, EditorState &editor_state, float line_height, float window_height, float window_width, float &current_scroll_y, float &current_scroll_x, CursorVisibility &ensure_cursor_visible);
    void renderEditorContent(const std::string &text, const std::vector<ImVec4> &colors, EditorState &editor_state, float line_height, const ImVec2 &text_pos);
    void updateFinalScrollAndRenderLineNumbers(const ImVec2 &line_numbers_pos, float line_number_width, float editor_top_margin, const ImVec2 &size, EditorState &editor_state, float line_height, float total_height);
    bool validateHighlightContentParams(const std::string &content, const std::vector<ImVec4> &colors, int start_pos, int end_pos);
};

// Utility functions
inline ImVec4 GetRainbowColor(float t) {
    float r = sin(t) * 0.5f + 0.5f;
    float g = sin(t + 2.0944f) * 0.5f + 0.5f; // 2.0944 is 2π/3
    float b = sin(t + 4.1888f) * 0.5f + 0.5f; // 4.1888 is 4π/3
    return ImVec4(r, g, b, 1.0f);
}
