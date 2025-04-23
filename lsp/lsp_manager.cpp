#include "lsp_manager.h"
#include <iostream>
#include <sys/select.h>
// Global instance
LSPManager gLSPManager;

LSPManager::LSPManager() : activeAdapter(NONE)
{
	clangdAdapter = std::make_unique<LSPAdapterClangd>();
	pyrightAdapter = std::make_unique<LSPAdapterPyright>();
}

LSPManager::~LSPManager() = default;

bool LSPManager::initialize(const std::string &path)
{
	workspacePath = path;

	// Only initialize the active adapter
	bool success = false;

	switch (activeAdapter)
	{
	case CLANGD:
		success = clangdAdapter->initialize(workspacePath);
		if (success)
		{
			std::cout << "\033[32mLSP Manager:\033[0m Initialized clangd adapter" << std::endl;
		} else
		{
			std::cerr << "\033[31mLSP Manager:\033[0m Failed to initialize "
						 "clangd adapter"
					  << std::endl;
		}
		break;

	case PYRIGHT:
		success = pyrightAdapter->initialize(workspacePath);
		if (success)
		{
			std::cout << "\033[32mLSP Manager:\033[0m Initialized pyright adapter" << std::endl;
		} else
		{
			std::cerr << "\033[31mLSP Manager:\033[0m Failed to initialize "
						 "pyright adapter"
					  << std::endl;
		}
		break;

	case NONE:
	default:
		std::cerr << "\033[31mLSP Manager:\033[0m No adapter selected to initialize" << std::endl;
		return false;
	}

	return success;
}

bool LSPManager::isInitialized() const
{
	switch (activeAdapter)
	{
	case CLANGD:
		return clangdAdapter->isInitialized();
	case PYRIGHT:
		return pyrightAdapter->isInitialized();
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
		std::cout << "\033[33mLSP Manager:\033[0m File has no extension, "
					 "defaulting to no adapter"
				  << std::endl;
		activeAdapter = NONE;
		return false;
	}

	std::string ext = filePath.substr(dot_pos + 1);

	// Select appropriate adapter based on file extension
	if (ext == "c" || ext == "cpp" || ext == "cc" || ext == "cxx" || ext == "h" || ext == "hpp")
	{
		activeAdapter = CLANGD;
		return true;
	} else if (ext == "py")
	{
		activeAdapter = PYRIGHT;
		return true;
	}

	// No appropriate adapter found
	std::cout << "\033[33mLSP Manager:\033[0m No adapter available for extension: " << ext
			  << std::endl;
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
	case NONE:
	default:
		std::cerr << "\033[31mLSP Manager:\033[0m No active adapter to send request" << std::endl;
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
	case NONE:
	default:
		std::cerr << "\033[31mLSP Manager:\033[0m No active adapter to read response" << std::endl;
		return "";
	}
}

std::string LSPManager::getLanguageId(const std::string &filePath) const
{
	switch (activeAdapter)
	{
	case CLANGD:
		return clangdAdapter->getLanguageId(filePath);
	case PYRIGHT:
		return pyrightAdapter->getLanguageId(filePath);
	case NONE:
	default:
		// Default for when no adapter is active
		return "plaintext";
	}
}
