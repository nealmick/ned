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

class EditorLSP
{
  public:
    EditorLSP();
    ~EditorLSP();

    // Core LSP functionality
    bool initialize(const std::string &workspacePath);
    void didOpen(const std::string &filePath, const std::string &content);
    void didChange(const std::string &filePath, const std::string &newContent, int version);
    bool gotoDefinition(const std::string &filePath, int line, int character);

    // Definition options window
    void renderDefinitionOptions(EditorState &state);
    bool hasDefinitionOptions() const;

  private:
    // Helper methods
    std::string escapeJSON(const std::string &s) const;
    int getNextRequestId() { return ++currentRequestId; }
    void parseDefinitionResponse(const std::string &response);

    // Request tracking
    int currentRequestId = 0;

    // Definition options state
    std::vector<DefinitionLocation> definitionLocations;
    int selectedDefinitionIndex = 0;
    bool showDefinitionOptions = false;
};

// Global instance
extern EditorLSP gEditorLSP;