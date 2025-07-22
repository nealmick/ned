#pragma once
#include <memory>
#include <string>
#include <vector> // Include for std::vector if used in Impl or elsewhere

class LSPAdapterGo
{
  public:
	LSPAdapterGo();
	~LSPAdapterGo();

	// Setup and initialization
	bool initialize(const std::string &workspacePath);
	bool isInitialized() const { return initialized; }

	// Protocol communication
	bool sendRequest(const std::string &request);
	std::string readResponse(
		int *outContentLength = nullptr); // Added outContentLength like Omnisharp

	// Go-specific functionality (or common LSP functionality)
	std::string getLanguageId(const std::string &filePath) const;

  private:
	class GoImpl; // PImpl pattern
	std::unique_ptr<GoImpl> impl;
	bool initialized;
	std::string lspPath; // Path to the gopls executable
};