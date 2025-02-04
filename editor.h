/*  
    editor.h
    Main editor logic for displaying file content, handling keybinds and more...
*/

#pragma once
#include "imgui.h"
#include "lexers/python.h"
#include "lexers/cpp.h"
#include "lexers/html.h"


#include <string>
#include <vector>
#include <regex>
#include <unordered_map>
#include <future>
#include <atomic>
#include <mutex>
#include <cmath>



struct SyntaxRule {
    std::regex pattern;
    ImVec4 color;
};
struct EditorState {
    int cursor_pos;
    int current_line;
    int selection_start;
    int selection_end;
    bool is_selecting;
    std::vector<int> line_starts;
    ImVec2 scroll_pos;
    float scroll_x;
    bool rainbow_cursor;
    float cursor_blink_time;
    bool activateFindBox;
    bool blockInput;
    bool full_text_selected;
    int ensure_cursor_visible_frames;
    bool pendingBookmarkScroll = false;
    float pendingScrollX = 0;
    float pendingScrollY = 0;
    EditorState()
        :  ensure_cursor_visible_frames(0),cursor_pos(0), selection_start(0), selection_end(0), is_selecting(false),
        line_starts({0}), scroll_pos(0, 0), scroll_x(0.0f), rainbow_cursor(true),
        cursor_blink_time(0.0f), activateFindBox(false), blockInput(false), current_line(0) {}
};

struct ScrollChange {
    bool vertical;
    bool horizontal;
};
struct CursorVisibility {
    bool vertical;
    bool horizontal;
};

class Editor {
    std::mutex highlight_mutex; 
public:
    void setLanguage(const std::string& extension);
    void highlightContent(const std::string& content, std::vector<ImVec4>& colors, int start_pos, int end_pos);
    void setTheme(const std::string& themeName);
    void moveWordForward(const std::string& text, EditorState& state);
    void moveWordBackward(const std::string& text, EditorState& state);
    void removeIndentation(std::string& text, EditorState& state);
    void cancelHighlighting();
    void requestScroll(float x, float y) {
        requestedScrollX = x;
        requestedScrollY = y;
        hasScrollRequest = true;
    }
    
    // Add this new method to handle scroll requests
    bool handleScrollRequest(float& outScrollX, float& outScrollY) {
        if (hasScrollRequest) {
            outScrollX = requestedScrollX;
            outScrollY = requestedScrollY;
            hasScrollRequest = false;
            return true;
        }
        return false;
    }
    void forceColorUpdate() {
        pythonLexer.forceColorUpdate();
        cppLexer.forceColorUpdate();
        htmlLexer.forceColorUpdate();
    }
    
private:
    std::vector<SyntaxRule> rules;
    std::vector<SyntaxRule> htmlRules;
    std::vector<SyntaxRule> javascriptRules;
    std::vector<SyntaxRule> cssRules;
    std::vector<SyntaxRule> cppRules;
    std::vector<SyntaxRule> markdownRules;
    std::vector<SyntaxRule> jsonRules;
    std::vector<SyntaxRule> goRules;
    std::vector<SyntaxRule> javaRules;
    std::vector<SyntaxRule> csharpRules;
    PythonLexer::Lexer pythonLexer;
    CppLexer::Lexer cppLexer;
    HtmlLexer::Lexer htmlLexer;
    void setupCppRules();
    void setupJavaScriptRules();
    void setupMarkdownRules();
    void setupHtmlRules();
    void setupCssRules();
    void setupJsonRules();
    void setupGoRules();
    void setupJavaRules();
    void setupCSharpRules();

    void loadTheme(const std::string& themeName);
    std::unordered_map<std::string, ImVec4> themeColors;
    void applyRules(const std::string& view, std::vector<ImVec4>& colors, int start_pos, const std::vector<SyntaxRule>& rules);
    void applyPythonHighlighting(const std::string& view, std::vector<ImVec4>& colors, int start_pos);


    std::future<void> highlightFuture;
    std::atomic<bool> highlightingInProgress{false};
    std::mutex colorsMutex;
    std::atomic<bool> cancelHighlightFlag{false};

    //bookmark scroll jumps
    float requestedScrollX = 0;
    float requestedScrollY = 0;
    bool hasScrollRequest = false;
};
extern EditorState editor_state;
extern Editor gEditor;


inline ImVec4 GetRainbowColor(float t) {
    float r = sin(t) * 0.5f + 0.5f;
    float g = sin(t + 2.0944f) * 0.5f + 0.5f; // 2.0944 is 2π/3
    float b = sin(t + 4.1888f) * 0.5f + 0.5f; // 4.1888 is 4π/3
    return ImVec4(r, g, b, 1.0f);
}


void HandleEditorInput(const std::string& text, EditorState& state, const ImVec2& text_start_pos, float line_height, bool& text_changed, std::vector<ImVec4>& colors);
bool CustomTextEditor(const char* label, std::string& text, std::vector<ImVec4>& colors, EditorState& editor_state);
ScrollChange EnsureCursorVisible(const ImVec2& text_pos, const std::string& text, EditorState& state, float line_height, float window_height, float window_width);

