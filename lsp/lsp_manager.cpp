#include "lsp_manager.h"
#include <iostream>
#include <sys/select.h> // Not strictly used in the provided snippet, but was included

// Global instance
LSPManager gLSPManager;

LSPManager::LSPManager() : activeAdapter(NONE)
{
	clangdAdapter = std::make_unique<LSPAdapterClangd>();
	pyrightAdapter = std::make_unique<LSPAdapterPyright>();
	typescriptAdapter = std::make_unique<LSPAdapterTypescript>();
}

LSPManager::~LSPManager() = default;

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
				std::cout << "\033[32mLSP Manager:\033[0m Initialized clangd adapter for "
						  << workspacePath << std::endl;
			} else
			{
				std::cerr << "\033[31mLSP Manager:\033[0m Failed to initialize clangd adapter for "
						  << workspacePath << std::endl;
			}
		} else
		{
			std::cout << "\033[32mLSP Manager:\033[0m Clangd adapter already initialized."
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
				std::cout << "\033[32mLSP Manager:\033[0m Initialized pyright adapter for "
						  << workspacePath << std::endl;
			} else
			{
				std::cerr << "\033[31mLSP Manager:\033[0m Failed to initialize pyright adapter for "
						  << workspacePath << std::endl;
			}
		} else
		{
			std::cout << "\033[32mLSP Manager:\033[0m Pyright adapter already initialized."
					  << std::endl;
			success = true;
		}
		break;

	case TYPESCRIPT: // <<< ADD THIS CASE
		if (!typescriptAdapter->isInitialized())
		{
			success = typescriptAdapter->initialize(workspacePath);
			if (success)
			{
				std::cout << "\033[32mLSP Manager:\033[0m Initialized typescript adapter for "
						  << workspacePath << std::endl;
			} else
			{
				std::cerr
					<< "\033[31mLSP Manager:\033[0m Failed to initialize typescript adapter for "
					<< workspacePath << std::endl;
			}
		} else
		{
			std::cout << "\033[32mLSP Manager:\033[0m Typescript adapter already initialized."
					  << std::endl;
			success = true;
		}
		break;

	case NONE:
	default:
		std::cerr << "\033[31mLSP Manager:\033[0m Cannot initialize, no active adapter selected or "
					 "unknown type."
				  << std::endl;
		return false;
	}

	return success;
}

bool LSPManager::isInitialized() const
{
	switch (activeAdapter)
	{
	case CLANGD:
		return clangdAdapter && clangdAdapter->isInitialized(); // Add null check for adapter
	case PYRIGHT:
		return pyrightAdapter && pyrightAdapter->isInitialized();
	case TYPESCRIPT: // <<< ADD THIS CASE
		return typescriptAdapter && typescriptAdapter->isInitialized();
	case NONE:
	default:
		return false;
	}
}

bool LSPManager::selectAdapterForFile(const std::string &filePath)
{
	// Get file extension
	size_t dot_pos = filePath.find_last_of(".");
	if (dot_pos == std::string::npos)
	{
		activeAdapter = NONE; // No extension, no specific adapter can be selected
		return false;
	}

	std::string ext = filePath.substr(dot_pos + 1);
	AdapterType newAdapter = NONE;

	// Select appropriate adapter based on file extension
	if (ext == "c" || ext == "cpp" || ext == "cc" || ext == "cxx" || ext == "h" || ext == "hpp")
	{
		newAdapter = CLANGD;
	} else if (ext == "py")
	{
		newAdapter = PYRIGHT;
	} else if (ext == "ts" || ext == "tsx" || ext == "js" || ext == "jsx") // <<< ADD THIS
	{
		newAdapter = TYPESCRIPT;
	}

	if (newAdapter != NONE)
	{
		// If the selected adapter is different from the current one,
		// and the current one was initialized, you might want to shut it down.
		// For simplicity now, we just switch. The old adapter remains in memory.
		// If activeAdapter != NONE && activeAdapter != newAdapter && isInitialized() {
		//    std::cout << "Switching adapter. Previous adapter might need shutdown." << std::endl;
		// }
		activeAdapter = newAdapter;
		return true;
	}

	// If no specific adapter is found for the extension
	activeAdapter = NONE;
	return false;
}

bool LSPManager::sendRequest(const std::string &request)
{
	switch (activeAdapter)
	{
	case CLANGD:
		return clangdAdapter->sendRequest(request);
	case PYRIGHT:
		return pyrightAdapter->sendRequest(request);
	case TYPESCRIPT: // <<< ADD THIS CASE
		return typescriptAdapter->sendRequest(request);
	case NONE:
	default:
		std::cerr << "\033[31mLSP Manager:\033[0m Cannot send request, no active adapter."
				  << std::endl;
		return false;
	}
}

std::string LSPManager::readResponse(int *contentLength)
{
	switch (activeAdapter)
	{
	case CLANGD:
		return clangdAdapter->readResponse(contentLength);
	case PYRIGHT:
		return pyrightAdapter->readResponse(contentLength);
	case TYPESCRIPT: // <<< ADD THIS CASE
		return typescriptAdapter->readResponse(contentLength);
	case NONE:
	default:
		std::cerr << "\033[31mLSP Manager:\033[0m Cannot read response, no active adapter."
				  << std::endl;
		if (contentLength)
			*contentLength = -1; // Indicate error
		return "";
	}
}

std::string LSPManager::getLanguageId(const std::string &filePath) const
{
	// This method is typically called after selectAdapterForFile has set the activeAdapter
	// based on the filePath. So, we can trust activeAdapter here.
	switch (activeAdapter)
	{
	case CLANGD:
		// The adapter itself can look at filePath if needed for C vs C++ etc.
		return clangdAdapter->getLanguageId(filePath);
	case PYRIGHT:
		return pyrightAdapter->getLanguageId(filePath);
	case TYPESCRIPT:
		return typescriptAdapter->getLanguageId(filePath);
	case NONE:
	default:
		// If selectAdapterForFile failed (e.g. unknown extension), activeAdapter is NONE.
		// EditorLSP::didOpen should bail out before calling this if selectAdapterForFile returned
		// false. If called in another context, "plaintext" is a safe default.
		return "plaintext";
	}
}