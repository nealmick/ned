// Windows stub implementations for LSP functionality
// Most LSP features are disabled on Windows due to Unix-specific process management requirements
// However, basic Python support via pyright is implemented using Windows-native LSP manager

#include "lsp.h"
#include "lsp_autocomplete.h"
#include "lsp_goto_def.h"
#include "lsp_goto_ref.h"
#include "lsp_symbol_info.h"
#include "lsp_manager_windows.h"
#include "../editor/editor.h"
#include "../editor/editor_types.h"
#include "../editor/editor_scroll.h"
#include "../files/files.h"
#include "../globals.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <set>
#include <windows.h>

// Global instances - these need to exist for linking
EditorLSP gEditorLSP;
LSPAutocomplete gLSPAutocomplete;
// Note: gLSPGotoDef, gLSPGotoRef, and gLSPSymbolInfo are now defined in their cross-platform files

// Static member definition
bool LSPAutocomplete::wasShowingLastFrame = false;

// EditorLSP stub implementation
EditorLSP::EditorLSP() {}
EditorLSP::~EditorLSP() {}

bool EditorLSP::initialize(const std::string &workspacePath) {
    std::cout << "LSP functionality is disabled on Windows" << std::endl;
    return false;
}

void EditorLSP::didOpen(const std::string &filePath, const std::string &content) {
    // Stub - no action on Windows
}

void EditorLSP::didChange(const std::string &filePath, int version) {
    // Stub - no action on Windows
}

std::string EditorLSP::escapeJSON(const std::string &s) const {
    return s;
}

// LSPAutocomplete stub implementation
LSPAutocomplete::LSPAutocomplete() {}
LSPAutocomplete::~LSPAutocomplete() {}

void LSPAutocomplete::requestCompletion(const std::string &filePath, int line, int character) {
    // Stub - no action on Windows
}

void LSPAutocomplete::renderCompletions() {
    // Stub - no action on Windows
}

void LSPAutocomplete::processPendingResponses() {
    // Stub - no action on Windows
}

bool LSPAutocomplete::shouldRender() {
    return false;
}

bool LSPAutocomplete::handleInputAndCheckClose() {
    return true;
}

void LSPAutocomplete::calculateWindowGeometry(ImVec2 &outWindowSize, ImVec2 &outSafePos) {
    // Stub - no action on Windows
}

void LSPAutocomplete::applyStyling() {
    // Stub - no action on Windows
}

void LSPAutocomplete::renderCompletionListItems() {
    // Stub - no action on Windows
}

bool LSPAutocomplete::handleClickOutside() {
    return false;
}

void LSPAutocomplete::finalizeRenderState() {
    // Stub - no action on Windows
}

void LSPAutocomplete::resetPopupPosition() {
    // Stub - no action on Windows
}

std::string LSPAutocomplete::formCompletionRequest(int requestId, const std::string &filePath, int line, int character) {
    return "";
}

bool LSPAutocomplete::processResponse(const std::string &response, int requestId) {
    return false;
}

void LSPAutocomplete::parseCompletionResult(const json &result, int requestLine, int requestCharacter) {
    // Stub - no action on Windows
}

void LSPAutocomplete::updatePopupPosition() {
    // Stub - no action on Windows
}

void LSPAutocomplete::workerFunction() {
    // Stub - no action on Windows
}

void LSPAutocomplete::insertText(int row_start, int col_start, int row_end, int col_end, std::string text) {
    // Stub - no action on Windows
}

// LSPGotoDef implementation now in main lsp_goto_def.cpp for cross-platform support

// LSPGotoRef implementation now in main lsp_goto_ref.cpp for cross-platform support

// LSPSymbolInfo implementation now in main lsp_symbol_info.cpp for cross-platform support