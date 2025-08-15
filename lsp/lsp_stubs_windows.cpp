// Windows stub implementations for LSP functionality
// LSP features are disabled on Windows due to Unix-specific process management requirements

#include "lsp.h"
#include "lsp_autocomplete.h"
#include "lsp_goto_def.h"
#include "lsp_goto_ref.h"
#include "lsp_symbol_info.h"
#include <iostream>

// Global instances - these need to exist for linking
EditorLSP gEditorLSP;
LSPAutocomplete gLSPAutocomplete;
LSPGotoDef gLSPGotoDef;
LSPGotoRef gLSPGotoRef;
LSPSymbolInfo gLSPSymbolInfo;

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

// LSPGotoDef stub implementation
LSPGotoDef::LSPGotoDef() {}
LSPGotoDef::~LSPGotoDef() {}

bool LSPGotoDef::gotoDefinition(const std::string &filePath, int line, int character) {
    return false;
}

void LSPGotoDef::renderDefinitionOptions() {
    // Stub - no action on Windows
}

bool LSPGotoDef::hasDefinitionOptions() const {
    return false;
}

void LSPGotoDef::parseDefinitionResponse(const std::string &response) {
    // Stub - no action on Windows
}

void LSPGotoDef::parseDefinitionArray(const json &results_array) {
    // Stub - no action on Windows
}

// LSPGotoRef stub implementation
LSPGotoRef::LSPGotoRef() {}
LSPGotoRef::~LSPGotoRef() {}

bool LSPGotoRef::findReferences(const std::string &filePath, int line, int character) {
    return false;
}

void LSPGotoRef::renderReferenceOptions() {
    // Stub - no action on Windows
}

bool LSPGotoRef::hasReferenceOptions() const {
    return false;
}

void LSPGotoRef::parseReferenceResponse(const std::string &response) {
    // Stub - no action on Windows
}

void LSPGotoRef::handleReferenceSelection() {
    // Stub - no action on Windows
}

// LSPSymbolInfo stub implementation
LSPSymbolInfo::LSPSymbolInfo() {}

void LSPSymbolInfo::fetchSymbolInfo(const std::string &filePath) {
    // Stub - no action on Windows
}

void LSPSymbolInfo::renderSymbolInfo() {
    // Stub - no action on Windows
}

void LSPSymbolInfo::parseHoverResponse(const std::string &response) {
    // Stub - no action on Windows
}