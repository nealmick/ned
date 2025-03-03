#pragma once
#include "../lexers/cpp.h"
#include "../lexers/html.h"
#include "../lexers/jsx.h"
#include "../lexers/python.h"
#include "imgui.h"
#include <atomic>
#include <filesystem>
#include <future>
#include <mutex>
#include <string>
#include <vector>

namespace fs = std::filesystem;

class EditorHighlight
{
  public:
    EditorHighlight();
    ~EditorHighlight() = default;

    // Highlight content with appropriate lexer based on file extension
    void highlightContent(const std::string &content, std::vector<ImVec4> &colors, int start_pos, int end_pos);

    // Cancel any ongoing highlighting operation
    void cancelHighlighting();

    // Force update of all lexer color caches
    void forceColorUpdate();

    // Validate parameters for the highlighting function
    bool validateHighlightContentParams(const std::string &content, const std::vector<ImVec4> &colors, int start_pos, int end_pos);

    void loadTheme(const std::string &themeName);

    void setTheme(const std::string &themeName);

  private:
    // Lexer instances
    PythonLexer::Lexer pythonLexer;
    CppLexer::Lexer cppLexer;
    HtmlLexer::Lexer htmlLexer;
    JsxLexer::Lexer jsxLexer;

    std::unordered_map<std::string, ImVec4> themeColors;

    // Highlighting state management
    std::mutex highlight_mutex;
    std::future<void> highlightFuture;
    std::atomic<bool> highlightingInProgress{false};
    std::mutex colorsMutex;
    std::atomic<bool> cancelHighlightFlag{false};
};

// Global instance
extern EditorHighlight gEditorHighlight;