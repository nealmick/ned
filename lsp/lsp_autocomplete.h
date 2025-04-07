// lsp_autocomplete.h
#pragma once

#include "../editor/editor.h"
#include "../editor/editor_cursor.h"
#include "../lib/json.hpp"
#include "imgui.h"
#include <string>
#include <vector>

using json = nlohmann::json;

struct CompletionDisplayItem
{
    std::string label;
    std::string detail;
    std::string insertText;
    int kind;
};

class LSPAutocomplete
{
  public:
    LSPAutocomplete();
    ~LSPAutocomplete();

    void requestCompletion(const std::string &filePath, int line, int character);
    void renderCompletions();

    bool showCompletions = false;
    int selectedCompletionIndex = 0;
    ImVec2 completionPopupPos = ImVec2(0, 0);

    bool blockTab = false;
    bool blockEnter = false;

  private:
    std::vector<CompletionDisplayItem> currentCompletionItems;

    // --- Private Helper Functions for Rendering ---
    bool shouldRender();
    bool handleInputAndCheckClose(); // Returns true if the window should close
    void calculateWindowGeometry(ImVec2 &outWindowSize, ImVec2 &outSafePos);
    void applyStyling();
    void renderCompletionListItems();
    bool handleClickOutside();
    void finalizeRenderState();

    // State tracking for focus/frame logic
    static bool wasShowingLastFrame;

    // requesting logic
    std::string formCompletionRequest(int requestId, const std::string &filePath, int line, int character);
    bool processResponse(const std::string &response, int requestId);
    void parseCompletionResult(const json &result);
    void updatePopupPosition();
};

// Global instance
extern LSPAutocomplete gLSPAutocomplete;
