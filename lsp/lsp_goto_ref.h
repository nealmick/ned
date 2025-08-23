#pragma once

#include "imgui.h"
#include <functional>
#include <future>
#include <map>
#include <memory>
#include <string>
#include <vector>

// Need the actual LSP types for the function signature
#ifdef _WIN32
#include "../build/lib/lsp-framework/generated/lsp/types.h"
#else
#include "../.build/lib/lsp-framework/generated/lsp/types.h"
#endif

class LSPGotoRef
{
  public:
	LSPGotoRef();
	~LSPGotoRef();

	// response data structure
	std::vector<std::map<std::string, std::string>> references;

	// Check keybind and trigger goto references if conditions are met
	void get();

	// Render the goto references UI
	void render();

	// Send LSP goto references request
	void
	request(int line,
			int character,
			std::function<void(const std::vector<std::map<std::string, std::string>> &)>
				callback);

  private:
	// Helper function to process the LSP response
	std::vector<std::map<std::string, std::string>>
	processResponse(const lsp::TextDocument_ReferencesResult &result);

	// Helper function to print structured results
	void printResponse(const std::vector<std::map<std::string, std::string>> &results);

	bool show;

	// Async request handling
	bool pending;
	std::shared_ptr<std::future<void>> asyncResponse;
};

// Global instance
extern LSPGotoRef gLSPGotoRef;