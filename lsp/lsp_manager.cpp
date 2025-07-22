#include "lsp_manager.h"
#include <iostream>
// #include <sys/select.h> // Not strictly used here, can be removed if not
// needed elsewhere by LSPManager

// Global instance
LSPManager gLSPManager;

LSPManager::LSPManager() : activeAdapter(NONE)
{
	clangdAdapter = std::make_unique<LSPAdapterClangd>();
	pyrightAdapter = std::make_unique<LSPAdapterPyright>();
	typescriptAdapter = std::make_unique<LSPAdapterTypescript>();
	goAdapter = std::make_unique<LSPAdapterGo>(); // For Go
}

LSPManager::~LSPManager() = default; // Relies on unique_ptr to clean up adapters

bool LSPManager::initialize(const std::string &path)
{
	workspacePath = path; // Store for potential use by any adapter

	// Only initialize the active adapter if it's not already initialized
	bool success = false;

	switch (activeAdapter)
	{
	case CLANGD:
		if (!clangdAdapter->isInitialized())
		{
			success = clangdAdapter->initialize(workspacePath);
			if (success)
			{
				std::cout << "\033[32mLSP Manager:\033[0m Initialized clangd "
							 "adapter for "
						  << workspacePath << std::endl;
			} else
			{
				std::cerr << "\033[31mLSP Manager:\033[0m Failed to initialize "
							 "clangd adapter for "
						  << workspacePath << std::endl;
			}
		} else
		{
			std::cout << "\033[32mLSP Manager:\033[0m Clangd adapter already "
						 "initialized."
					  << std::endl;
			success = true; // Already initialized counts as success for this call
		}
		break;

	case PYRIGHT:
		if (!pyrightAdapter->isInitialized())
		{
			success = pyrightAdapter->initialize(workspacePath);
			if (success)
			{
				std::cout << "\033[32mLSP Manager:\033[0m Initialized pyright "
							 "adapter for "
						  << workspacePath << std::endl;
			} else
			{
				std::cerr << "\033[31mLSP Manager:\033[0m Failed to initialize "
							 "pyright adapter for "
						  << workspacePath << std::endl;
			}
		} else
		{
			std::cout << "\033[32mLSP Manager:\033[0m Pyright adapter already "
						 "initialized."
					  << std::endl;
			success = true;
		}
		break;

	case TYPESCRIPT:
		if (!typescriptAdapter->isInitialized())
		{
			success = typescriptAdapter->initialize(workspacePath);
			if (success)
			{
				std::cout << "\033[32mLSP Manager:\033[0m Initialized "
							 "typescript adapter for "
						  << workspacePath << std::endl;
			} else
			{
				std::cerr << "\033[31mLSP Manager:\033[0m Failed to initialize "
							 "typescript adapter for "
						  << workspacePath << std::endl;
			}
		} else
		{
			std::cout << "\033[32mLSP Manager:\033[0m Typescript adapter "
						 "already initialized."
					  << std::endl;
			success = true;
		}
		break;

	case GOADAPTER: // For Go
		if (!goAdapter->isInitialized())
		{
			success = goAdapter->initialize(workspacePath);
			if (success)
			{
				std::cout << "\033[32mLSP Manager:\033[0m Initialized Go adapter for "
						  << workspacePath << std::endl;
			} else
			{
				std::cerr << "\033[31mLSP Manager:\033[0m Failed to initialize "
							 "Go adapter for "
						  << workspacePath << std::endl;
			}
		} else
		{
			std::cout << "\033[32mLSP Manager:\033[0m Go adapter already initialized."
					  << std::endl;
			success = true;
		}
		break;

	case NONE:
	default:
		std::cerr << "\033[31mLSP Manager:\033[0m Cannot initialize, no active "
					 "adapter selected or "
					 "unknown type. Active adapter type: "
				  << activeAdapter << std::endl;
		return false;
	}

	return success;
}

bool LSPManager::isInitialized() const
{
	switch (activeAdapter)
	{
	case CLANGD:
		return clangdAdapter && clangdAdapter->isInitialized();
	case PYRIGHT:
		return pyrightAdapter && pyrightAdapter->isInitialized();
	case TYPESCRIPT:
		return typescriptAdapter && typescriptAdapter->isInitialized();
	case GOADAPTER: // For Go
		return goAdapter && goAdapter->isInitialized();
	case NONE:
	default:
		return false;
	}
}

bool LSPManager::selectAdapterForFile(const std::string &filePath)
{
	size_t dot_pos = filePath.find_last_of(".");
	if (dot_pos == std::string::npos)
	{
		// No extension, decide if you want to deactivate or keep current.
		// Deactivating might be safer.
		// activeAdapter = NONE;
		// std::cout << "\033[33mLSP Manager:\033[0m No file extension for '" <<
		// filePath << "'. No adapter selected." << std::endl;
		return false; // Or return true if you want to allow no adapter for
					  // extensionless files
	}

	std::string ext = filePath.substr(dot_pos + 1);
	AdapterType newAdapter = NONE;

	if (ext == "c" || ext == "cpp" || ext == "cc" || ext == "cxx" || ext == "h" ||
		ext == "hpp")
	{
		newAdapter = CLANGD;
	} else if (ext == "py")
	{
		newAdapter = PYRIGHT;
	} else if (ext == "ts" || ext == "tsx" || ext == "js" || ext == "jsx")
	{
		newAdapter = TYPESCRIPT;
	} else if (ext == "cs") // For C#
	{
		newAdapter = OMNISHARP;
	} else if (ext == "go") // For Go
	{
		newAdapter = GOADAPTER;
	}
	// Add more else if blocks for other languages/adapters

	if (newAdapter != NONE)
	{
		if (activeAdapter != newAdapter)
		{
			// Optional: If switching from an initialized adapter, you might want
			// to shut it down. For simplicity, this example just switches. The
			// old adapter's process might still run. Consider adding shutdown
			// logic here if resource usage is a concern. e.g., if (isInitialized())
			// { /* get current adapter and call its shutdown methods */ }
			std::cout << "\033[35mLSP Manager:\033[0m Switching active adapter to: "
					  << newAdapter << " for file: " << filePath << std::endl;
			activeAdapter = newAdapter;
		} else
		{
			// It's the same adapter, no need to switch, just ensure it's
			// "selected" This path is fine.
		}
		return true;
	}

	// If no specific adapter is found for the extension
	std::cout << "\033[33mLSP Manager:\033[0m No specific adapter found for "
				 "extension '"
			  << ext << "'. Active adapter remains: " << activeAdapter << std::endl;
	// activeAdapter = NONE; // Or keep the current one if that's desired behavior
	return false; // No *new* adapter was selected for this extension
}

bool LSPManager::sendRequest(const std::string &request)
{
	switch (activeAdapter)
	{
	case CLANGD:
		return clangdAdapter->sendRequest(request);
	case PYRIGHT:
		return pyrightAdapter->sendRequest(request);
	case TYPESCRIPT:
		return typescriptAdapter->sendRequest(request);
	case GOADAPTER: // For Go
		return goAdapter->sendRequest(request);
	case NONE:
	default:
		std::cerr << "\033[31mLSP Manager:\033[0m Cannot send request, no "
					 "active adapter or unknown type."
				  << std::endl;
		return false;
	}
}

std::string LSPManager::readResponse(int *contentLength)
{
	if (contentLength)
		*contentLength = -1; // Default for safety

	switch (activeAdapter)
	{
	case CLANGD:
		return clangdAdapter->readResponse(contentLength);
	case PYRIGHT:
		return pyrightAdapter->readResponse(contentLength);
	case TYPESCRIPT:
		return typescriptAdapter->readResponse(contentLength);
	case GOADAPTER: // For Go
		return goAdapter->readResponse(contentLength);
	case NONE:
	default:
		std::cerr << "\033[31mLSP Manager:\033[0m Cannot read response, no "
					 "active adapter or "
					 "unknown type."
				  << std::endl;
		// if (contentLength) *contentLength = -1; // Already set at the start
		return "";
	}
}

std::string LSPManager::getLanguageId(const std::string &filePath) const
{
	// This method is typically called after selectAdapterForFile has set the
	// activeAdapter based on the filePath. So, we can trust activeAdapter here.
	switch (activeAdapter)
	{
	case CLANGD:
		return clangdAdapter->getLanguageId(filePath);
	case PYRIGHT:
		return pyrightAdapter->getLanguageId(filePath);
	case TYPESCRIPT:
		return typescriptAdapter->getLanguageId(filePath);
	case GOADAPTER: // For Go
		return goAdapter->getLanguageId(filePath);
	case NONE:
	default:
		// If selectAdapterForFile failed (e.g. unknown extension), activeAdapter
		// might be NONE or the previously active one. Returning "plaintext" is
		// a safe default. However, the adapter itself should provide the ID, so
		// this path should ideally not be hit if an adapter is truly active and
		// selected. std::cout << "\033[33mLSP Manager:\033[0m getLanguageId called
		// with no specific active adapter. File: " << filePath << std::endl;
		return "plaintext";
	}
}