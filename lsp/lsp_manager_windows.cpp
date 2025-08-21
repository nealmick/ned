#include "lsp_manager_windows.h"
#include "lsp_adapter_pyright_windows.h"
#include <iostream>

// Global instance
LSPManagerWindows gLSPManagerWindows;

LSPManagerWindows::LSPManagerWindows() : activeAdapter(NONE)
{
	pyrightAdapter = std::make_unique<LSPAdapterPyrightWindows>();
}

LSPManagerWindows::~LSPManagerWindows() = default;

bool LSPManagerWindows::initialize(const std::string &path)
{
	// Defensive check - don't initialize if path is empty or invalid
	if (path.empty())
	{
		std::cerr << "\033[33mLSP Manager Windows:\033[0m Cannot initialize with empty path" << std::endl;
		return false;
	}

	workspacePath = path;

	// Only initialize if we have an active adapter
	bool success = false;

	switch (activeAdapter)
	{
	case PYRIGHT:
		if (!pyrightAdapter->isInitialized())
		{
			success = pyrightAdapter->initialize(workspacePath);
			if (success)
			{
				std::cout << "\033[32mLSP Manager Windows:\033[0m Initialized pyright adapter for " << workspacePath << std::endl;
			}
			else
			{
				std::cout << "\033[33mLSP Manager Windows:\033[0m Pyright adapter initialization failed - LSP support will be disabled for Python files" << std::endl;
				success = false;
			}
		}
		else
		{
			std::cout << "\033[32mLSP Manager Windows:\033[0m Pyright adapter already initialized." << std::endl;
			success = true;
		}
		break;

	case NONE:
	default:
		std::cerr << "\033[31mLSP Manager Windows:\033[0m Cannot initialize, no active adapter selected" << std::endl;
		return false;
	}

	return success;
}

bool LSPManagerWindows::isInitialized() const
{
	switch (activeAdapter)
	{
	case PYRIGHT:
		return pyrightAdapter && pyrightAdapter->isInitialized();
	case NONE:
	default:
		return false;
	}
}

bool LSPManagerWindows::selectAdapterForFile(const std::string &filePath)
{
	size_t dot_pos = filePath.find_last_of(".");
	if (dot_pos == std::string::npos)
	{
		return false; // No extension
	}

	std::string ext = filePath.substr(dot_pos + 1);
	AdapterType newAdapter = NONE;

	if (ext == "py")
	{
		newAdapter = PYRIGHT;
	}

	if (newAdapter != NONE)
	{
		if (activeAdapter != newAdapter)
		{
			std::cout << "\033[35mLSP Manager Windows:\033[0m Switching active adapter to: " << newAdapter << " for file: " << filePath << std::endl;
			activeAdapter = newAdapter;
		}
		return true;
	}

	// No specific adapter found
	std::cout << "\033[33mLSP Manager Windows:\033[0m No specific adapter found for extension '" << ext << "'." << std::endl;
	return false;
}

bool LSPManagerWindows::sendRequest(const std::string &request)
{
	switch (activeAdapter)
	{
	case PYRIGHT:
		return pyrightAdapter && pyrightAdapter->isInitialized()
				   ? pyrightAdapter->sendRequest(request)
				   : false;
	case NONE:
	default:
		std::cerr << "\033[31mLSP Manager Windows:\033[0m Cannot send request, no active adapter" << std::endl;
		return false;
	}
}

std::string LSPManagerWindows::readResponse(int *contentLength)
{
	if (contentLength)
		*contentLength = -1; // Default for safety

	switch (activeAdapter)
	{
	case PYRIGHT:
		return pyrightAdapter && pyrightAdapter->isInitialized()
				   ? pyrightAdapter->readResponse(contentLength)
				   : "";
	case NONE:
	default:
		std::cerr << "\033[31mLSP Manager Windows:\033[0m Cannot read response, no active adapter" << std::endl;
		return "";
	}
}

std::string LSPManagerWindows::getLanguageId(const std::string &filePath) const
{
	switch (activeAdapter)
	{
	case PYRIGHT:
		return pyrightAdapter && pyrightAdapter->isInitialized()
				   ? pyrightAdapter->getLanguageId(filePath)
				   : "plaintext";
	case NONE:
	default:
		return "plaintext";
	}
}

bool LSPManagerWindows::hasWorkingAdapter() const
{
	switch (activeAdapter)
	{
	case PYRIGHT:
		return pyrightAdapter && pyrightAdapter->isInitialized();
	case NONE:
	default:
		return false;
	}
}