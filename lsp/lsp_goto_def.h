#pragma once
#include "../lib/json.hpp"
#include "editor_types.h" // Assuming this includes necessary editor state/types
#include "imgui.h"
#include "lsp_manager.h" // Include your LSP Manager
#include <memory>
#include <string>
#include <vector>

// Use the nlohmann::json namespace
using json = nlohmann::json;

// Structure to hold location information (remains the same)
struct DefinitionLocation
{
	std::string uri;
	int startLine;
	int startChar;
	int endLine; // Keep these, might be useful later
	int endChar;
};

class LSPGotoDef
{
  public:
	LSPGotoDef();
	~LSPGotoDef();

	// Core goto definition functionality
	bool gotoDefinition(const std::string &filePath, int line, int character);

	// Definition options window (no changes needed in declaration)
	void renderDefinitionOptions();
	bool hasDefinitionOptions() const;

	// Embedded mode methods
	void setEmbedded(bool embedded) { isEmbedded = embedded; }
	bool getEmbedded() const { return isEmbedded; }

  private:
	// Helper methods
	void parseDefinitionResponse(const std::string &response);
	// New helper to parse the array part of the response
	void parseDefinitionArray(const json &results_array); // <<< ADDED DECLARATION
	int getNextRequestId() { return ++currentRequestId; }

	// Request tracking
	int currentRequestId = 2000; // Start at different value than EditorLSP

	// Definition options state (remains the same)
	std::vector<DefinitionLocation> definitionLocations;
	int selectedDefinitionIndex = 0;
	bool showDefinitionOptions = false;
	bool isEmbedded = false;
};

// Global instance (remains the same)
extern LSPGotoDef gLSPGotoDef;