#include "lsp_adapter_clangd.h"
#include <cstdio>
#include <iostream>
#include <sstream>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

class LSPAdapterClangd::ClangdImpl
{
  public:
	ClangdImpl() : input(nullptr), output(nullptr) {}
	~ClangdImpl()
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

LSPAdapterClangd::LSPAdapterClangd() : impl(new ClangdImpl()), initialized(false)
{
	// Default path for clangd
	lspPath = "/usr/bin/clangd";
}

LSPAdapterClangd::~LSPAdapterClangd() = default;

bool LSPAdapterClangd::initialize(const std::string &workspacePath)
{
	if (initialized)
	{
		std::cout << "\033[35mClangd:\033[0m Already initialized" << std::endl;
		return true;
	}

	try
	{
		std::cout << "\033[35mClangd:\033[0m Starting LSP server with path: " << lspPath
				  << std::endl;

		// Create pipes for communication
		int inPipe[2], outPipe[2];
		if (pipe(inPipe) < 0 || pipe(outPipe) < 0)
		{
			std::cerr << "\033[31mClangd:\033[0m Failed to create pipes" << std::endl;
			return false;
		}

		// Fork process
		pid_t pid = fork();
		if (pid < 0)
		{
			std::cerr << "\033[31mClangd:\033[0m Fork failed" << std::endl;
			return false;
		}

		if (pid == 0)
		{ // Child process
			std::cout << "\033[35mClangd:\033[0m Starting clangd process" << std::endl;

			dup2(outPipe[0], STDIN_FILENO);
			dup2(inPipe[1], STDOUT_FILENO);

			close(inPipe[0]);
			close(inPipe[1]);
			close(outPipe[0]);
			close(outPipe[1]);

			execl(lspPath.c_str(), "clangd", "--log=error", nullptr);

			std::cerr << "\033[31mClangd:\033[0m Failed to start clangd" << std::endl;
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
			std::cerr << "\033[31mClangd:\033[0m Failed to open pipes" << std::endl;
			return false;
		}

		std::cout << "\033[35mClangd:\033[0m Sending initialize request for "
					 "workspace: "
				  << workspacePath << std::endl;

		// Create the initialize request string
		std::string initRequest = std::string(R"({
            "jsonrpc": "2.0",
            "id": 1,
            "method": "initialize",
            "params": {
                "processId": null,
                "rootUri": "file://)") +
								  workspacePath + R"(",
                "capabilities": {
                    "textDocument": {
                        "definition": {
                            "dynamicRegistration": true
                        },
                        "completion": {
                            "completionItem": {
                                "sortTextSupport": true
                            }
                        }
                    }
                }
            }
        })";

		// Send the initialization request
		sendRequest(initRequest);
		std::cout << "\033[35mClangd:\033[0m Waiting for initialization response" << std::endl;
		std::string initResponse = readResponse();
		std::cout << "\033[35mClangd:\033[0m Received initialization response: "
				  << initResponse.substr(0, 100) << "..." << std::endl;

		// Send the initialized notification
		std::string initializedNotification = R"({
            "jsonrpc": "2.0",
            "method": "initialized",
            "params": {}
        })";
		sendRequest(initializedNotification);
		std::cout << "\033[32mClangd:\033[0m Sent initialized notification" << std::endl;

		initialized = true;
		return true;
	} catch (const std::exception &e)
	{
		std::cerr << "\033[31mClangd:\033[0m Initialization failed: " << e.what() << std::endl;
		return false;
	}
}

bool LSPAdapterClangd::sendRequest(const std::string &request)
{
	if (!impl->input)
	{
		std::cerr << "\033[31mClangd:\033[0m Cannot send request - server not "
					 "initialized"
				  << std::endl;
		return false;
	}

	fprintf(impl->input, "Content-Length: %zu\r\n\r\n%s", request.length(), request.c_str());
	fflush(impl->input);
	return true;
}

std::string LSPAdapterClangd::readResponse(int *contentLength)
{
	if (!impl->output)
	{
		std::cerr << "\033[31mClangd:\033[0m Cannot read response - server not "
					 "initialized"
				  << std::endl;
		return "";
	}

	char header[1024];
	if (!fgets(header, sizeof(header), impl->output))
	{
		std::cerr << "\033[31mClangd:\033[0m Failed to read response header" << std::endl;
		return "";
	}

	int length = 0;
	if (sscanf(header, "Content-Length: %d\r\n", &length) != 1)
	{
		std::cerr << "\033[31mClangd:\033[0m Invalid header format" << std::endl;
		return "";
	}

	// Skip the empty line after the header
	fgets(header, sizeof(header), impl->output);

	// Read the response body
	std::vector<char> buffer(length + 1);
	size_t bytes_read = fread(buffer.data(), 1, length, impl->output);
	buffer[bytes_read] = '\0';

	if (contentLength)
	{
		*contentLength = length;
	}

	return std::string(buffer.data(), bytes_read);
}

std::string LSPAdapterClangd::getLanguageId(const std::string &filePath) const
{
	// Get file extension
	size_t dot_pos = filePath.find_last_of(".");
	if (dot_pos == std::string::npos)
	{
		return "plaintext";
	}

	std::string ext = filePath.substr(dot_pos + 1);

	// Map C/C++ extensions to language IDs
	if (ext == "cpp" || ext == "c" || ext == "cc" || ext == "cxx" || ext == "hpp" || ext == "h")
	{
		return "cpp";
	}

	// Default for unknown extensions
	return "plaintext";
}