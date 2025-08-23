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

class LSPGotoDef
{
  public:
	LSPGotoDef();
	~LSPGotoDef();

	// response data structure
	std::vector<std::map<std::string, std::string>> definitions;

	// Check keybind and trigger goto definition if conditions are met
	void get();

	// Render the goto definition UI
	void render();

	// Send LSP goto definition request
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
extern LSPGotoDef gLSPGotoDef;