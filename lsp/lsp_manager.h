#pragma once
#include "lsp_adapter_clangd.h"
#include "lsp_adapter_go.h" // Your new Go adapter
#include "lsp_adapter_pyright.h"
#include "lsp_adapter_typescript.h"

#include <memory>
#include <string>

// Forward declarations
// class EditorLSP; // Not strictly needed in lsp_manager.h if not used directly

class LSPManager
{
  public:
	LSPManager();
	~LSPManager();

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
	// LSP adapter instances
	std::unique_ptr<LSPAdapterClangd> clangdAdapter;
	std::unique_ptr<LSPAdapterPyright> pyrightAdapter;
	std::unique_ptr<LSPAdapterTypescript> typescriptAdapter;
	std::unique_ptr<LSPAdapterGo> goAdapter; // For Go

	// Make sure the enum matches all your adapters
	enum AdapterType {
		NONE,
		CLANGD,
		PYRIGHT,
		TYPESCRIPT,
		OMNISHARP, // For C#
		GOADAPTER  // For Go
	};

	AdapterType activeAdapter;
	std::string workspacePath;
};

// Global instance
extern LSPManager gLSPManager;