#pragma once
#include "../lib/json.hpp"
#include "editor_types.h"
#include "imgui.h"
#include "lsp_manager.h"
#include <memory>
#include <string>
#include <vector>

struct ReferenceLocation
{
	std::string uri;
	int startLine;
	int startChar;
	int endLine;
	int endChar;
};

using json = nlohmann::json;

class LSPGotoRef
{
  public:
	LSPGotoRef();
	~LSPGotoRef();

	// Core find references functionality
	bool findReferences(const std::string &filePath, int line, int character);

	// Reference options window rendering
	void renderReferenceOptions();
	bool hasReferenceOptions() const;

	// Embedded mode methods
	void setEmbedded(bool embedded) { isEmbedded = embedded; }
	bool getEmbedded() const { return isEmbedded; }

  private:
	// Helper methods
	void parseReferenceResponse(const std::string &response);
	int getNextRequestId() { return ++currentRequestId; }
	void handleReferenceSelection();
	// Request tracking (using a different range than LSPGotoDef)
	int currentRequestId = 3000;

	// Reference options state
	std::vector<ReferenceLocation> referenceLocations;
	int selectedReferenceIndex = 0;
	bool showReferenceOptions = false;
	bool isEmbedded = false;
	
	// Request state tracking
	bool inProgress = false;
};

// Global instance
extern LSPGotoRef gLSPGotoRef;