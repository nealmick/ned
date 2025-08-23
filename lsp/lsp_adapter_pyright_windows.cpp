#include "lsp_adapter_pyright_windows.h"
#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>

// Windows-specific implementation details
class LSPAdapterPyrightWindows::PyrightImplWindows
{
  public:
	PyrightImplWindows()
		: hProcess(INVALID_HANDLE_VALUE), hInputRead(INVALID_HANDLE_VALUE),
		  hInputWrite(INVALID_HANDLE_VALUE), hOutputRead(INVALID_HANDLE_VALUE),
		  hOutputWrite(INVALID_HANDLE_VALUE)
	{
	}

	~PyrightImplWindows()
	{
		if (hInputRead != INVALID_HANDLE_VALUE)
			CloseHandle(hInputRead);
		if (hInputWrite != INVALID_HANDLE_VALUE)
			CloseHandle(hInputWrite);
		if (hOutputRead != INVALID_HANDLE_VALUE)
			CloseHandle(hOutputRead);
		if (hOutputWrite != INVALID_HANDLE_VALUE)
			CloseHandle(hOutputWrite);
		if (hProcess != INVALID_HANDLE_VALUE)
			CloseHandle(hProcess);
	}

	HANDLE hProcess;
	HANDLE hInputRead, hInputWrite;	  // To LSP server (stdin)
	HANDLE hOutputRead, hOutputWrite; // From LSP server (stdout)
};

LSPAdapterPyrightWindows::LSPAdapterPyrightWindows()
	: impl(new PyrightImplWindows()), initialized(false)
{
	// Will find pyright path during initialization
}

LSPAdapterPyrightWindows::~LSPAdapterPyrightWindows() = default;

std::string LSPAdapterPyrightWindows::findPyrightPath()
{
	// Try common locations for pyright-langserver on Windows
	std::string username = getenv("USERNAME") ? getenv("USERNAME") : "Default";
	std::vector<std::string> possiblePaths = {
		"C:\\Users\\" + username + "\\AppData\\Roaming\\npm\\pyright-langserver.cmd",
		"pyright-langserver.cmd", // In PATH
		"pyright-langserver",	  // In PATH without .cmd
		"C:\\Users\\" + username + "\\AppData\\Roaming\\npm\\pyright-langserver"};

	for (const auto &path : possiblePaths)
	{
		// Test if the file exists
		std::ifstream file(path);
		if (file.good())
		{
			std::cout << "\033[35mPyright Windows:\033[0m Found pyright at: " << path
					  << std::endl;
			return path;
		}
	}

	// Also try just checking if pyright is available
	if (system("pyright --help >nul 2>&1") == 0)
	{
		std::cout << "\033[35mPyright Windows:\033[0m Found pyright, using "
					 "pyright-langserver.cmd"
				  << std::endl;
		return "pyright-langserver.cmd";
	}

	return "";
}

bool LSPAdapterPyrightWindows::startPyrightProcess()
{
	// Find pyright-langserver path
	lspPath = findPyrightPath();
	if (lspPath.empty())
	{
		std::cerr << "\033[31mPyright Windows:\033[0m pyright-langserver not found. "
					 "Install with: npm install -g pyright"
				  << std::endl;
		return false;
	}

	// Create pipes for communication
	SECURITY_ATTRIBUTES sa;
	sa.nLength = sizeof(SECURITY_ATTRIBUTES);
	sa.bInheritHandle = TRUE;
	sa.lpSecurityDescriptor = NULL;

	if (!CreatePipe(&impl->hInputRead, &impl->hInputWrite, &sa, 0) ||
		!CreatePipe(&impl->hOutputRead, &impl->hOutputWrite, &sa, 0))
	{
		std::cerr << "\033[31mPyright Windows:\033[0m Failed to create pipes"
				  << std::endl;
		return false;
	}

	// Make sure the write handles are not inherited
	SetHandleInformation(impl->hInputWrite, HANDLE_FLAG_INHERIT, 0);
	SetHandleInformation(impl->hOutputRead, HANDLE_FLAG_INHERIT, 0);

	// Setup process creation
	STARTUPINFO si;
	PROCESS_INFORMATION pi;
	ZeroMemory(&si, sizeof(si));
	si.cb = sizeof(si);
	si.hStdError = impl->hOutputWrite;
	si.hStdOutput = impl->hOutputWrite;
	si.hStdInput = impl->hInputRead;
	si.dwFlags |= STARTF_USESTDHANDLES;

	// Build command line
	std::string cmdLine = "\"" + lspPath + "\" --stdio";
	std::vector<char> cmdLineBuf(cmdLine.begin(), cmdLine.end());
	cmdLineBuf.push_back('\0');

	std::cout << "\033[35mPyright Windows:\033[0m Executing: " << cmdLine << std::endl;

	if (!CreateProcess(NULL, cmdLineBuf.data(), NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi))
	{
		DWORD error = GetLastError();
		std::cerr
			<< "\033[31mPyright Windows:\033[0m Failed to start process. Error code: "
			<< error << std::endl;
		return false;
	}

	impl->hProcess = pi.hProcess;
	CloseHandle(pi.hThread);
	CloseHandle(impl->hInputRead);
	CloseHandle(impl->hOutputWrite);

	std::cout << "\033[35mPyright Windows:\033[0m Started pyright process successfully"
			  << std::endl;
	return true;
}

bool LSPAdapterPyrightWindows::sendInitializeRequest(const std::string &workspacePath)
{
	// Convert Windows path to proper URI format
	std::string workspaceUri = workspacePath;
	// Replace backslashes with forward slashes for URI
	for (char &c : workspaceUri)
	{
		if (c == '\\')
			c = '/';
	}

	// Create the initialize request
	std::string initRequest = R"({
		"jsonrpc": "2.0",
		"id": 1,
		"method": "initialize",
		"params": {
			"processId": null,
			"rootUri": "file:///)" +
							  workspaceUri + R"(",
			"capabilities": {
				"textDocument": {
					"hover": {
						"dynamicRegistration": true,
						"contentFormat": ["markdown", "plaintext"]
					},
					"synchronization": {
						"didSave": true
					}
				}
			},
			"initializationOptions": {
				"pyright": {
					"disableOrganizeImports": false,
					"disableLanguageServices": false
				}
			}
		}
	})";

	std::cout
		<< "\033[35mPyright Windows:\033[0m Sending initialize request for workspace: "
		<< workspacePath << std::endl;
	std::cout << "\033[36mPyright Windows Init Request:\033[0m\n"
			  << initRequest << "\n"
			  << std::endl;

	if (!sendRequest(initRequest))
	{
		std::cerr << "\033[31mPyright Windows:\033[0m Failed to send initialize request"
				  << std::endl;
		return false;
	}

	// Wait for initialization response - pyright sends log messages first, then the
	// actual response
	const int MAX_ATTEMPTS =
		10; // Reduced from 30 to minimize UI blocking
	for (int attempt = 0; attempt < MAX_ATTEMPTS; attempt++)
	{
		std::cout << "\033[35mPyright Windows:\033[0m Waiting for initialization "
					 "response (attempt "
				  << (attempt + 1) << ")" << std::endl;

		std::string response = readResponse(nullptr);
		if (response.empty())
		{
			std::cout << "\033[31mPyright Windows:\033[0m Empty response received"
					  << std::endl;
			Sleep(20); // Reduced from 100ms to minimize UI blocking
			continue;
		}

		std::cout << "\033[36mPyright Windows:\033[0m Received response: "
				  << response.substr(0, 200) << "..." << std::endl;

		// Check if this is the initialization response (has id:1 AND result field)
		if (response.find("\"id\":1") != std::string::npos &&
			response.find("\"result\":") != std::string::npos)
		{
			std::cout
				<< "\033[32mPyright Windows:\033[0m Received initialization response"
				<< std::endl;

			// Send the initialized notification
			std::string initializedNotification = R"({
				"jsonrpc": "2.0",
				"method": "initialized",
				"params": {}
			})";
			sendRequest(initializedNotification);
			std::cout << "\033[32mPyright Windows:\033[0m Sent initialized notification"
					  << std::endl;

			return true;
		}

		// Check for error responses
		if (response.find("\"id\":1") != std::string::npos &&
			response.find("\"error\":") != std::string::npos)
		{
			std::cerr << "\033[31mPyright Windows:\033[0m Initialization error: "
					  << response << std::endl;
			return false;
		}

		// Log messages and other notifications are normal - keep waiting
		if (response.find("window/logMessage") != std::string::npos)
		{
			std::cout
				<< "\033[33mPyright Windows:\033[0m Received log message, continuing..."
				<< std::endl;
		} else
		{
			std::cout
				<< "\033[33mPyright Windows:\033[0m Received other message, continuing..."
				<< std::endl;
		}

		Sleep(20); // Reduced from 100ms to minimize UI blocking
	}

	std::cerr << "\033[31mPyright Windows:\033[0m Failed to receive initialization "
				 "response after "
			  << MAX_ATTEMPTS << " attempts" << std::endl;
	return false;
}

bool LSPAdapterPyrightWindows::initialize(const std::string &workspacePath)
{
	if (initialized)
	{
		std::cout << "\033[35mPyright Windows:\033[0m Already initialized" << std::endl;
		return true;
	}

	// Start the pyright process
	if (!startPyrightProcess())
	{
		return false;
	}

	// Send initialize request and wait for response
	if (!sendInitializeRequest(workspacePath))
	{
		return false;
	}

	initialized = true;
	std::cout << "\033[32mPyright Windows:\033[0m Initialization completed successfully"
			  << std::endl;
	return true;
}

bool LSPAdapterPyrightWindows::sendRequest(const std::string &request)
{
	if (!impl || impl->hInputWrite == INVALID_HANDLE_VALUE)
	{
		std::cerr << "\033[31mPyright Windows:\033[0m Cannot send request - server not "
					 "initialized"
				  << std::endl;
		return false;
	}

	// Format LSP message with Content-Length header
	std::string message =
		"Content-Length: " + std::to_string(request.length()) + "\r\n\r\n" + request;

	DWORD bytesWritten;
	if (!WriteFile(
			impl->hInputWrite, message.c_str(), message.length(), &bytesWritten, NULL))
	{
		DWORD error = GetLastError();
		std::cerr << "\033[31mPyright Windows:\033[0m Failed to write to pipe. Error: "
				  << error << std::endl;
		return false;
	}

	FlushFileBuffers(impl->hInputWrite);
	return true;
}

std::string LSPAdapterPyrightWindows::readResponse(int *contentLength)
{
	if (!impl || impl->hOutputRead == INVALID_HANDLE_VALUE)
	{
		std::cerr << "\033[31mPyright Windows:\033[0m Cannot read response - server not "
					 "initialized"
				  << std::endl;
		return "";
	}

	// Read Content-Length header with timeout
	std::string header;
	char ch;
	DWORD bytesRead;
	const int TIMEOUT_MS = 1000; // 1 second timeout
	DWORD startTime = GetTickCount();

	// Read until we get the Content-Length line
	while (true)
	{
		// Check for timeout
		if (GetTickCount() - startTime > TIMEOUT_MS)
		{
			std::cerr << "\033[31mPyright Windows:\033[0m Timeout reading header"
					  << std::endl;
			return "";
		}

		// Check if data is available before reading
		DWORD bytesAvailable = 0;
		if (!PeekNamedPipe(impl->hOutputRead, NULL, 0, NULL, &bytesAvailable, NULL))
		{
			Sleep(5); // Reduced from 10ms to minimize UI blocking
			continue;
		}
		if (bytesAvailable == 0)
		{
			Sleep(5); // Reduced from 10ms to minimize UI blocking
			continue;
		}

		if (!ReadFile(impl->hOutputRead, &ch, 1, &bytesRead, NULL) || bytesRead == 0)
		{
			std::cerr << "\033[31mPyright Windows:\033[0m Failed to read header"
					  << std::endl;
			return "";
		}

		header += ch;
		if (header.find("Content-Length:") == 0 && ch == '\n')
		{
			break;
		}
		if (header.length() > 100)
		{
			// Reset if header gets too long
			header.clear();
		}
	}

	// Parse content length
	int length = 0;
	size_t colonPos = header.find(':');
	if (colonPos != std::string::npos)
	{
		std::string lengthStr = header.substr(colonPos + 1);
		length = std::stoi(lengthStr);
	}

	if (length <= 0)
	{
		std::cerr << "\033[31mPyright Windows:\033[0m Invalid content length: " << length
				  << std::endl;
		return "";
	}

	// Skip the empty line after headers with timeout
	DWORD skipStartTime = GetTickCount();
	while (true)
	{
		// Check for timeout
		if (GetTickCount() - skipStartTime > TIMEOUT_MS)
		{
			std::cerr << "\033[31mPyright Windows:\033[0m Timeout skipping header line"
					  << std::endl;
			return "";
		}

		// Check if data is available
		DWORD bytesAvailable = 0;
		if (!PeekNamedPipe(impl->hOutputRead, NULL, 0, NULL, &bytesAvailable, NULL) ||
			bytesAvailable == 0)
		{
			Sleep(5); // Reduced from 10ms to minimize UI blocking
			continue;
		}

		if (!ReadFile(impl->hOutputRead, &ch, 1, &bytesRead, NULL) || bytesRead == 0)
		{
			break;
		}
		if (ch == '\n')
		{
			break;
		}
	}

	// Read the actual content with timeout
	std::vector<char> buffer(length + 1);
	DWORD totalBytesRead = 0;
	DWORD contentStartTime = GetTickCount();

	while (totalBytesRead < length)
	{
		// Check for timeout
		if (GetTickCount() - contentStartTime > TIMEOUT_MS * 2)
		{ // Double timeout for content
			std::cerr << "\033[31mPyright Windows:\033[0m Timeout reading content"
					  << std::endl;
			break;
		}

		// Check if data is available
		DWORD bytesAvailable = 0;
		if (!PeekNamedPipe(impl->hOutputRead, NULL, 0, NULL, &bytesAvailable, NULL) ||
			bytesAvailable == 0)
		{
			Sleep(5); // Reduced from 10ms to minimize UI blocking
			continue;
		}

		DWORD currentBytesRead;
		DWORD bytesToRead = std::min(bytesAvailable, (DWORD)(length - totalBytesRead));
		if (!ReadFile(impl->hOutputRead,
					  buffer.data() + totalBytesRead,
					  bytesToRead,
					  &currentBytesRead,
					  NULL) ||
			currentBytesRead == 0)
		{
			break;
		}
		totalBytesRead += currentBytesRead;
	}

	buffer[totalBytesRead] = '\0';

	if (contentLength)
	{
		*contentLength = length;
	}

	return std::string(buffer.data(), totalBytesRead);
}

std::string LSPAdapterPyrightWindows::getLanguageId(const std::string &filePath) const
{
	// Get file extension
	size_t dot_pos = filePath.find_last_of(".");
	if (dot_pos == std::string::npos)
	{
		return "plaintext";
	}

	std::string ext = filePath.substr(dot_pos + 1);

	// Map Python extensions to language IDs
	if (ext == "py")
	{
		return "python";
	}

	// Default for unknown extensions
	return "plaintext";
}