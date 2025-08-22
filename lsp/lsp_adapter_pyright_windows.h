#pragma once

#include <memory>
#include <string>
#include <windows.h>

class LSPAdapterPyrightWindows
{
  public:
	LSPAdapterPyrightWindows();
	~LSPAdapterPyrightWindows();

	// Core LSP adapter interface
	bool initialize(const std::string &workspacePath);
	bool isInitialized() const { return initialized; }

	// Communication methods
	bool sendRequest(const std::string &request);
	std::string readResponse(int *contentLength = nullptr);

	// Language identification
	std::string getLanguageId(const std::string &filePath) const;

  private:
	// Windows-specific implementation details
	class PyrightImplWindows;
	std::unique_ptr<PyrightImplWindows> impl;

	bool initialized;
	std::string lspPath;

	// Helper methods
	std::string findPyrightPath();
	bool startPyrightProcess();
	bool sendInitializeRequest(const std::string &workspacePath);
};