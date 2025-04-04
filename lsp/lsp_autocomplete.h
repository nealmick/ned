// lsp_autocomplete.h
#pragma once
#include "lsp.h"

#include "../lib/json.hpp" // Include json header consistently
#include <string>
#include <vector>

using json = nlohmann::json; // Use the alias consistently

class LSPAutocomplete
{
  public:
    LSPAutocomplete();
    ~LSPAutocomplete();
    std::string getPrefix(const std::string &lineContent, int character);
    void requestCompletion(const std::string &filePath, int line, int character);

  private:
};

// Global instance
extern LSPAutocomplete gLSPAutocomplete;