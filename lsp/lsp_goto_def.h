#pragma once
#include "editor_types.h"
#include "imgui.h"
#include "lsp_manager.h"
#include <memory>
#include <string>
#include <vector>

struct DefinitionLocation
{
    std::string uri;
    int startLine;
    int startChar;
    int endLine;
    int endChar;
};

class LSPGotoDef
{
  public:
    LSPGotoDef();
    ~LSPGotoDef();

    // Core goto definition functionality
    bool gotoDefinition(const std::string &filePath, int line, int character);

    // Definition options window
    void renderDefinitionOptions(EditorState &state);
    bool hasDefinitionOptions() const;

  private:
    // Helper methods
    void parseDefinitionResponse(const std::string &response);
    int getNextRequestId() { return ++currentRequestId; }

    // Request tracking
    int currentRequestId = 2000; // Start at different value than EditorLSP

    // Definition options state
    std::vector<DefinitionLocation> definitionLocations;
    int selectedDefinitionIndex = 0;
    bool showDefinitionOptions = false;
};

// Global instance
extern LSPGotoDef gLSPGotoDef;