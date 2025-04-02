#pragma once
#include "editor_types.h"
#include "lsp_manager.h"
#include <memory>
#include <string>
#include <vector>

class EditorLSP
{
  public:
    EditorLSP();
    ~EditorLSP();

    // Core LSP functionality
    bool initialize(const std::string &workspacePath);
    void didOpen(const std::string &filePath, const std::string &content);
    void didChange(const std::string &filePath, int version);

  private:
    // Helper methods
    std::string escapeJSON(const std::string &s) const;
    int getNextRequestId() { return ++currentRequestId; }

    // Request tracking
    int currentRequestId = 1000;
};

// Global instance
extern EditorLSP gEditorLSP;