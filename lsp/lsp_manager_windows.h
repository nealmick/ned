#pragma once

#include <memory>
#include <string>

// Forward declaration
class LSPAdapterPyrightWindows;

class LSPManagerWindows
{
  public:
	LSPManagerWindows();
	~LSPManagerWindows();

	// Adapter selection and initialization
	bool initialize(const std::string &workspacePath);
	bool isInitialized() const;

	// Determine appropriate adapter for a file
	bool selectAdapterForFile(const std::string &filePath);

	// Communication methods
	bool sendRequest(const std::string &request);
	std::string readResponse(int *contentLength = nullptr);

	// Language-specific helpers
	std::string getLanguageId(const std::string &filePath) const;

	// Check if the current adapter is working
	bool hasWorkingAdapter() const;

  private:
	// LSP adapter instances - only pyright for now
	std::unique_ptr<LSPAdapterPyrightWindows> pyrightAdapter;

	// Simple adapter type
	enum AdapterType { NONE, PYRIGHT };

	AdapterType activeAdapter;
	std::string workspacePath;
};

// Global instance
extern LSPManagerWindows gLSPManagerWindows;