#include "lsp_adapter_pyright.h"
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <sstream>
#ifndef PLATFORM_WINDOWS
#include <sys/select.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#endif
class LSPAdapterPyright::PyrightImpl
{
  public:
	PyrightImpl() : input(nullptr), output(nullptr) {}
	~PyrightImpl()
	{
		if (input)
			pclose(input);
		if (output)
			pclose(output);
	}

	FILE *input;  // To LSP server
	FILE *output; // From LSP server
	int pid;	  // LSP server process ID
};

LSPAdapterPyright::LSPAdapterPyright() : impl(new PyrightImpl()), initialized(false)
{
	// Default path for pyright-langserver - this is the correct binary for LSP
	// Pyright itself doesn't have LSP support, but it comes with
	// pyright-langserver
	lspPath = "/opt/homebrew/bin/pyright-langserver";
}

LSPAdapterPyright::~LSPAdapterPyright() = default;
bool LSPAdapterPyright::initialize(const std::string &workspacePath)
{
	if (initialized)
	{
		std::cout << "\033[35mPyright:\033[0m Already initialized" << std::endl;
		return true;
	}

	// Check if the LSP server executable exists and is accessible
	// Use a more defensive approach to avoid permission issues
	try
	{
		if (access(lspPath.c_str(), X_OK) != 0)
		{
			std::cerr
				<< "\033[31mPyright:\033[0m LSP server not found or not executable at: "
				<< lspPath << std::endl;
			std::cerr << "\033[33mPyright:\033[0m Error: " << strerror(errno)
					  << std::endl;
			std::cerr
				<< "\033[33mPyright:\033[0m Pyright LSP support will be disabled for "
				   "this session"
				<< std::endl;
			return false;
		}
	} catch (...)
	{
		std::cerr << "\033[33mPyright:\033[0m Could not check LSP server accessibility - "
					 "disabling LSP support"
				  << std::endl;
		return false;
	}

	try
	{
		std::cout << "\033[35mPyright:\033[0m Starting LSP server with path: " << lspPath
				  << std::endl;

		// Create pipes for communication
		int inPipe[2], outPipe[2];
		if (pipe(inPipe) < 0 || pipe(outPipe) < 0)
		{
			std::cerr << "\033[31mPyright:\033[0m Failed to create pipes" << std::endl;
			return false;
		}

		// Fork process
		pid_t pid = fork();
		if (pid < 0)
		{
			std::cerr << "\033[31mPyright:\033[0m Fork failed" << std::endl;
			return false;
		}

		if (pid == 0)
		{ // Child process
			std::cout << "\033[35mPyright:\033[0m Starting pyright-langserver process"
					  << std::endl;

			dup2(outPipe[0], STDIN_FILENO);
			dup2(inPipe[1], STDOUT_FILENO);

			close(inPipe[0]);
			close(inPipe[1]);
			close(outPipe[0]);
			close(outPipe[1]);

			// Pyright langserver needs --stdio flag
			execl(lspPath.c_str(), "pyright-langserver", "--stdio", nullptr);

			std::cerr << "\033[31mPyright:\033[0m Failed to start pyright-langserver"
					  << std::endl;
			exit(1);
		}

		// Parent process
		close(inPipe[1]);
		close(outPipe[0]);

		impl->input = fdopen(outPipe[1], "w");
		impl->output = fdopen(inPipe[0], "r");
		impl->pid = pid;

		if (!impl->input || !impl->output)
		{
			std::cerr << "\033[31mPyright:\033[0m Failed to open pipes" << std::endl;
			return false;
		}

		std::cout << "\033[35mPyright:\033[0m Sending initialize request for "
					 "workspace: "
				  << workspacePath << std::endl;

		// Create the initialize request string
		// Pyright has some specific initialization parameters
		std::string initRequest = std::string(R"({
            "jsonrpc": "2.0",
            "id": 1,
            "method": "initialize",
            "params": {
                "processId": null,
                "rootUri": "file://)") +
								  workspacePath + "/" + R"(",
                "capabilities": {
                    "textDocument": {
                        "definition": {
                            "dynamicRegistration": true
                        },
                        "synchronization": {
                            "didSave": true,
                            "willSave": true
                        },
                        "completion": {
                            "completionItem": {
                                "snippetSupport": true,
                                "insertReplaceSupport": true
                            }
                        }
                    },
                    "workspace": {
                        "workspaceFolders": true
                    }
                },
                "initializationOptions": {
                    "pyright": {
                        "disableOrganizeImports": false,
                        "disableLanguageServices": false
                    },
                    "python": {
                        "analysis": {
                            "autoSearchPaths": true,
                            "useLibraryCodeForTypes": true,
                            "diagnosticMode": "workspace"
                        }
                    }
                }
            }
        })";

		sendRequest(initRequest);
		std::cout << "\033[32mPyright:\033[0m Initialize request sent successfully"
				  << std::endl;

		// Wait for initialization response with timeout
		const int MAX_ATTEMPTS = 10;
		for (int attempt = 0; attempt < MAX_ATTEMPTS; attempt++)
		{
			std::cout << "\033[35mPyright:\033[0m Waiting for initialization "
						 "response (attempt "
					  << (attempt + 1) << ")" << std::endl;

			std::string response = readResponse(nullptr);
			if (response.empty())
			{
				std::cout << "\033[31mPyright:\033[0m Empty response received"
						  << std::endl;
				continue;
			}

			// Check if this is the initialization response
			if (response.find("\"id\":1") != std::string::npos &&
				response.find("\"result\":") != std::string::npos)
			{
				std::cout << "\033[32mPyright:\033[0m Received initialization "
							 "response"
						  << std::endl;

				// Send the initialized notification
				std::string initializedNotification = R"({
                    "jsonrpc": "2.0",
                    "method": "initialized",
                    "params": {}
                })";
				sendRequest(initializedNotification);
				std::cout << "\033[32mPyright:\033[0m Sent initialized notification"
						  << std::endl;

				initialized = true;
				return true;
			}

			// If we got a response but it's not the initialization response,
			// keep trying
			std::cout << "\033[33mPyright:\033[0m Received message but not "
						 "initialization response, continuing..."
					  << std::endl;
		}

		std::cerr << "\033[31mPyright:\033[0m Failed to receive initialization "
					 "response after "
				  << MAX_ATTEMPTS << " attempts" << std::endl;
		return false;
	} catch (const std::exception &e)
	{
		std::cerr << "\033[31mPyright:\033[0m Initialization failed: " << e.what()
				  << std::endl;
		return false;
	}
}

bool LSPAdapterPyright::sendRequest(const std::string &request)
{
	if (!impl->input)
	{
		std::cerr << "\033[31mPyright:\033[0m Cannot send request - server not "
					 "initialized"
				  << std::endl;
		return false;
	}

	fprintf(
		impl->input, "Content-Length: %zu\r\n\r\n%s", request.length(), request.c_str());
	fflush(impl->input);
	return true;
}

std::string LSPAdapterPyright::readResponse(int *contentLength)
{
	if (!impl->output)
	{
		std::cerr << "\033[31mPyright:\033[0m Cannot read response" << std::endl;
		return "";
	}

	// Read until we get a valid Content-Length header
	while (true)
	{
		char header[1024] = {0};
		if (!fgets(header, sizeof(header), impl->output))
		{
			if (feof(impl->output))
			{
				std::cerr << "\033[33mPyright:\033[0m Server closed connection"
						  << std::endl;
				return "";
			}
			return "";
		}

		// Skip empty lines or non-header content
		if (strncmp(header, "Content-Length:", 15) == 0)
		{
			int length = 0;
			if (sscanf(header + 15, " %d", &length) == 1)
			{
				// Read the remaining header (until \r\n\r\n)
				while (fgets(header, sizeof(header), impl->output) &&
					   strcmp(header, "\r\n") != 0)
				{
				}

				// Read the actual content
				std::vector<char> buffer(length + 1);
				size_t bytes_read = fread(buffer.data(), 1, length, impl->output);
				buffer[bytes_read] = '\0';

				if (contentLength)
					*contentLength = length;
				return std::string(buffer.data(), bytes_read);
			}
		}
		// Handle JSON-RPC notifications (like diagnostics)
		else if (header[0] == '{')
		{
			std::string notification = header;
			// Read until we get the full notification
			while (!notification.empty() && notification.back() != '}')
			{
				int ch = fgetc(impl->output);
				if (ch == EOF)
					break;
				notification += static_cast<char>(ch);
			}
			std::cout << "\033[35mPyright Notification:\033[0m " << notification
					  << std::endl;
		}
	}
}
std::string LSPAdapterPyright::getLanguageId(const std::string &filePath) const
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
