#pragma once
#include <memory>
#include <string>
#include <vector>

class LSPAdapterTypescript
{
  public:
	LSPAdapterTypescript();
	~LSPAdapterTypescript();

	// Setup and initialization
	bool initialize(const std::string &workspacePath);
	bool isInitialized() const { return initialized; }

	// Protocol communication
	bool sendRequest(const std::string &request);
	// outContentLength will be:
	// -1 on a read error (e.g., pipe closed, fgets/fread error)
	//  0 if Content-Length was 0 (empty body)
	// >0 for actual content length read.
	// The returned string will be empty on error or if body was empty.
	std::string readResponse(int *outContentLength = nullptr);

	// Typescript-specific functionality
	std::string getLanguageId(const std::string &filePath) const;

  private:
	class TypescriptImpl; // PImpl idiom
	std::unique_ptr<TypescriptImpl> impl;
	bool initialized;
	std::string lspPath;
};