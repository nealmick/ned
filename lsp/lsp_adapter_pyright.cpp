#include "lsp_adapter_pyright.h"
#include <cstdio>
#include <iostream>
#include <sstream>
#include <sys/select.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

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
    int pid;      // LSP server process ID
};

LSPAdapterPyright::LSPAdapterPyright() : impl(new PyrightImpl()), initialized(false)
{
    // Default path for pyright-langserver - this is the correct binary for LSP
    // Pyright itself doesn't have LSP support, but it comes with pyright-langserver
    lspPath = "/opt/homebrew/bin/pyright-langserver";
}

LSPAdapterPyright::~LSPAdapterPyright() = default;
bool LSPAdapterPyright::initialize(const std::string &workspacePath)
{
    if (initialized) {
        std::cout << "\033[35mPyright:\033[0m Already initialized" << std::endl;
        return true;
    }

    try {
        std::cout << "\033[35mPyright:\033[0m Starting LSP server with path: " << lspPath << std::endl;

        // Create pipes for communication
        int inPipe[2], outPipe[2];
        if (pipe(inPipe) < 0 || pipe(outPipe) < 0) {
            std::cerr << "\033[31mPyright:\033[0m Failed to create pipes" << std::endl;
            return false;
        }

        // Fork process
        pid_t pid = fork();
        if (pid < 0) {
            std::cerr << "\033[31mPyright:\033[0m Fork failed" << std::endl;
            return false;
        }

        if (pid == 0) { // Child process
            std::cout << "\033[35mPyright:\033[0m Starting pyright-langserver process" << std::endl;

            dup2(outPipe[0], STDIN_FILENO);
            dup2(inPipe[1], STDOUT_FILENO);

            close(inPipe[0]);
            close(inPipe[1]);
            close(outPipe[0]);
            close(outPipe[1]);

            // Pyright langserver needs --stdio flag
            execl(lspPath.c_str(), "pyright-langserver", "--stdio", nullptr);

            std::cerr << "\033[31mPyright:\033[0m Failed to start pyright-langserver" << std::endl;
            exit(1);
        }

        // Parent process
        close(inPipe[1]);
        close(outPipe[0]);

        impl->input = fdopen(outPipe[1], "w");
        impl->output = fdopen(inPipe[0], "r");
        impl->pid = pid;

        if (!impl->input || !impl->output) {
            std::cerr << "\033[31mPyright:\033[0m Failed to open pipes" << std::endl;
            return false;
        }

        std::cout << "\033[35mPyright:\033[0m Sending initialize request for workspace: " << workspacePath << std::endl;

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
                                "snippetSupport": true
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
        std::cout << "\033[32mPyright:\033[0m Initialize request sent successfully" << std::endl;

        // Wait for initialization response with timeout
        const int MAX_ATTEMPTS = 10;
        for (int attempt = 0; attempt < MAX_ATTEMPTS; attempt++) {
            std::cout << "\033[35mPyright:\033[0m Waiting for initialization response (attempt " << (attempt + 1) << ")" << std::endl;

            std::string response = readResponse(nullptr);
            if (response.empty()) {
                std::cout << "\033[31mPyright:\033[0m Empty response received" << std::endl;
                continue;
            }

            // Check if this is the initialization response
            if (response.find("\"id\":1") != std::string::npos && response.find("\"result\":") != std::string::npos) {
                std::cout << "\033[32mPyright:\033[0m Received initialization response" << std::endl;

                // Send the initialized notification
                std::string initializedNotification = R"({
                    "jsonrpc": "2.0",
                    "method": "initialized",
                    "params": {}
                })";
                sendRequest(initializedNotification);
                std::cout << "\033[32mPyright:\033[0m Sent initialized notification" << std::endl;

                initialized = true;
                return true;
            }

            // If we got a response but it's not the initialization response, keep trying
            std::cout << "\033[33mPyright:\033[0m Received message but not initialization response, continuing..." << std::endl;
        }

        std::cerr << "\033[31mPyright:\033[0m Failed to receive initialization response after " << MAX_ATTEMPTS << " attempts" << std::endl;
        return false;

    } catch (const std::exception &e) {
        std::cerr << "\033[31mPyright:\033[0m Initialization failed: " << e.what() << std::endl;
        return false;
    }
}

bool LSPAdapterPyright::sendRequest(const std::string &request)
{
    if (!impl->input) {
        std::cerr << "\033[31mPyright:\033[0m Cannot send request - server not initialized" << std::endl;
        return false;
    }

    // Print request for debugging
    std::cout << "\033[36mPyright Request:\033[0m " << request << std::endl;

    fprintf(impl->input, "Content-Length: %zu\r\n\r\n%s", request.length(), request.c_str());
    fflush(impl->input);
    return true;
}
std::string LSPAdapterPyright::readResponse(int *contentLength)
{
    if (!impl->output) {
        std::cerr << "\033[31mPyright:\033[0m Cannot read response - server not initialized" << std::endl;
        return "";
    }

    char header[1024];
    if (!fgets(header, sizeof(header), impl->output)) {
        std::cerr << "\033[31mPyright:\033[0m Failed to read response header" << std::endl;
        return "";
    }

    // If header starts with { it's a JSON response, not a Content-Length header
    if (header[0] == '{') {
        std::cerr << "\033[31mPyright:\033[0m Invalid header format, but contains JSON: " << header << std::endl;

        // Extract just the JSON part (before Content-Length if present)
        std::string response = header;
        size_t contentPos = response.find("Content-Length:");
        if (contentPos != std::string::npos) {
            response = response.substr(0, contentPos);
        }

        std::cout << "\033[32mPyright:\033[0m Extracted JSON: " << response << std::endl;
        return response;
    }

    // Regular header processing
    int length = 0;
    if (sscanf(header, "Content-Length: %d\r\n", &length) != 1) {
        std::cerr << "\033[31mPyright:\033[0m Invalid header format: " << header << std::endl;
        return "";
    }

    // Skip the empty line after the header
    fgets(header, sizeof(header), impl->output);

    // Read the response body
    std::vector<char> buffer(length + 1);
    size_t bytes_read = fread(buffer.data(), 1, length, impl->output);
    buffer[bytes_read] = '\0';

    if (contentLength) {
        *contentLength = length;
    }

    return std::string(buffer.data(), bytes_read);
}

std::string LSPAdapterPyright::getLanguageId(const std::string &filePath) const
{
    // Get file extension
    size_t dot_pos = filePath.find_last_of(".");
    if (dot_pos == std::string::npos) {
        return "plaintext";
    }

    std::string ext = filePath.substr(dot_pos + 1);

    // Map Python extensions to language IDs
    if (ext == "py") {
        return "python";
    }

    // Default for unknown extensions
    return "plaintext";
}