#pragma once
#include <cstring>
#include <memory>
#include <string>
#include <vector>

// Forward declarations
class EditorLSP;

class LSPAdapterPyright
{
  public:
	LSPAdapterPyright();
	~LSPAdapterPyright();

	// Setup and initialization
	bool initialize(const std::string &workspacePath);
	bool isInitialized() const { return initialized; }

	// Protocol communication
	bool sendRequest(const std::string &request);
	std::string readResponse(int *contentLength = nullptr);

	// Pyright-specific functionality
	std::string getLanguageId(const std::string &filePath) const;

  private:
	class PyrightImpl;
	std::unique_ptr<PyrightImpl> impl;
	bool initialized;
	std::string lspPath;
};