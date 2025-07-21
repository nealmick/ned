#pragma once
#include "../lexers/cpp.h"
#include "../lexers/csharp.h"
#include "../lexers/css.h"
#include "../lexers/html.h"
#include "../lexers/java.h"
#include "../lexers/jsx.h"
#include "../lexers/python.h"
#include "../lexers/tsx.h"

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

	void highlightContent(bool fullRehighlight = false, bool sync = false);

	void cancelHighlighting();

	void forceColorUpdate();

	bool validateHighlightContentParams();

	void loadTheme(const std::string &themeName);

	void setTheme(const std::string &themeName);

  private:
	// Lexer instances
	PythonLexer::Lexer pythonLexer;
	CppLexer::Lexer cppLexer;
	HtmlLexer::Lexer htmlLexer;
	JsxLexer::Lexer jsxLexer;
	TsxLexer::Lexer tsxLexer;
	JavaLexer::Lexer javaLexer;
	CSharpLexer::Lexer csharpLexer;
	CssLexer::Lexer cssLexer;

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