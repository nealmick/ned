#pragma once
#include <memory>
#include <string>
#include <vector>

// Forward declarations
class EditorLSP;

class LSPAdapterClangd
{
  public:
	LSPAdapterClangd();
	~LSPAdapterClangd();

	// Setup and initialization
	bool initialize(const std::string &workspacePath);
	bool isInitialized() const { return initialized; }

	// Protocol communication
	bool sendRequest(const std::string &request);
	std::string readResponse(int *contentLength = nullptr);

	// Clangd-specific functionality
	std::string getLanguageId(const std::string &filePath) const;

  private:
	class ClangdImpl;
	std::unique_ptr<ClangdImpl> impl;
	bool initialized;
	std::string lspPath;
};