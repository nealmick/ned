#pragma once
#include "../lib/json.hpp"
#include "editor_types.h"
#include "imgui.h"
#include "lsp_manager.h"
#include <string>

using json = nlohmann::json;

class LSPSymbolInfo
{
  public:
    LSPSymbolInfo();
    void fetchSymbolInfo(const std::string &filePath);
    void renderSymbolInfo();
    bool hasSymbolInfo() const { return showSymbolInfo && !currentSymbolInfo.empty(); }

  private:
    void parseHoverResponse(const std::string &response);

    std::string currentSymbolInfo;
    bool showSymbolInfo = false;
    ImVec2 displayPosition;
};

extern LSPSymbolInfo gLSPSymbolInfo;