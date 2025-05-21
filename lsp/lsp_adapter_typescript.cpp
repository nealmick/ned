#include "lsp_adapter_typescript.h"
#include <cerrno> // For errno
#include <cstdio>
#include <cstring> // For strncmp, strcmp, strerror
#include <iostream>
#include <sstream>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h> // For pipe, fork, dup2, execl, close, usleep
#include <vector>	// For readResponse buffer

// PImpl class definition
class LSPAdapterTypescript::TypescriptImpl
{
  public:
	TypescriptImpl() : input(nullptr), output(nullptr), pid(0) {}
	// Destructor for TypescriptImpl: FILE* are managed by LSPAdapterTypescript's destructor
	~TypescriptImpl() = default;

	FILE *input;  // To LSP server (child's stdin)
	FILE *output; // From LSP server (child's stdout)
	pid_t pid;	  // LSP server process ID
};

LSPAdapterTypescript::LSPAdapterTypescript() : impl(new TypescriptImpl()), initialized(false)
{
	// Path to the typescript-language-server executable
	lspPath = "/opt/homebrew/bin/typescript-language-server"; // User provided path
}

LSPAdapterTypescript::~LSPAdapterTypescript()
{
	// Ensure input stream is valid before sending requests
	if (impl && impl->input)
	{
		if (initialized)
		{ // Only send shutdown/exit if successfully initialized
			std::cout << "\033[35mTypescript:\033[0m Sending shutdown request." << std::endl;
			std::string shutdownRequest =
				R"({"jsonrpc":"2.0","id":9998,"method":"shutdown","params":null})";
			// Best effort: ignore result of sendRequest during shutdown
			sendRequest(shutdownRequest);
			// Note: Some servers might close connection after shutdown req, before exit
			// notification is sent/processed.

			std::cout << "\033[35mTypescript:\033[0m Sending exit notification." << std::endl;
			std::string exitNotification = R"({"jsonrpc":"2.0","method":"exit","params":null})";
			sendRequest(exitNotification);
		}
		// fclose will also close the underlying file descriptor for the pipe.
		fclose(impl->input);
		impl->input = nullptr;
	}

	if (impl && impl->output)
	{
		fclose(impl->output);
		impl->output = nullptr;
	}

	if (impl && impl->pid > 0)
	{
		std::cout << "\033[35mTypescript:\033[0m Waiting for LSP server process (PID: " << impl->pid
				  << ") to exit." << std::endl;
		int status;
		if (waitpid(impl->pid, &status, 0) == -1)
		{
			std::cerr << "\033[31mTypescript:\033[0m Error waiting for LSP server process: "
					  << strerror(errno) << std::endl;
		} else
		{
			if (WIFEXITED(status))
			{
				std::cout << "\033[32mTypescript:\033[0m LSP server exited with status "
						  << WEXITSTATUS(status) << std::endl;
			} else if (WIFSIGNALED(status))
			{
				std::cout << "\033[33mTypescript:\033[0m LSP server terminated by signal "
						  << WTERMSIG(status) << std::endl;
			}
		}
		impl->pid = 0; // Mark as waited for
	}
	// The unique_ptr impl will delete the TypescriptImpl object, calling its destructor.
}

bool LSPAdapterTypescript::initialize(const std::string &workspacePath)
{
	if (initialized)
	{
		std::cout << "\033[35mTypescript:\033[0m Already initialized" << std::endl;
		return true;
	}
	// Ensure PImpl is created
	if (!impl)
	{
		impl = std::make_unique<TypescriptImpl>();
	}

	std::cout << "\033[35mTypescript:\033[0m Starting LSP server with path: " << lspPath
			  << std::endl;

	int toServerPipeFDs[2];	  // Parent writes to toServerPipeFDs[1], Child reads from
							  // toServerPipeFDs[0] (child's stdin)
	int fromServerPipeFDs[2]; // Child writes to fromServerPipeFDs[1] (child's stdout), Parent reads
							  // from fromServerPipeFDs[0]

	if (pipe(toServerPipeFDs) < 0 || pipe(fromServerPipeFDs) < 0)
	{
		std::cerr << "\033[31mTypescript:\033[0m Failed to create pipes: " << strerror(errno)
				  << std::endl;
		return false;
	}

	impl->pid = fork();
	if (impl->pid < 0)
	{ // Fork failed
		std::cerr << "\033[31mTypescript:\033[0m Fork failed: " << strerror(errno) << std::endl;
		close(toServerPipeFDs[0]);
		close(toServerPipeFDs[1]);
		close(fromServerPipeFDs[0]);
		close(fromServerPipeFDs[1]);
		impl->pid = 0; // Reset PID
		return false;
	}

	if (impl->pid == 0)
	{ // Child process
		// Redirect child's stdin to read from toServerPipeFDs[0]
		if (dup2(toServerPipeFDs[0], STDIN_FILENO) == -1)
		{ /* error handling */
			exit(1);
		}
		// Redirect child's stdout to write to fromServerPipeFDs[1]
		if (dup2(fromServerPipeFDs[1], STDOUT_FILENO) == -1)
		{ /* error handling */
			exit(1);
		}
		// stderr could be redirected to a log file or /dev/null if desired
		// e.g., int devNull = open("/dev/null", O_WRONLY); dup2(devNull, STDERR_FILENO);
		// close(devNull);

		// Close all original pipe FDs in child, as they are now duplicated to stdin/stdout
		close(toServerPipeFDs[0]);
		close(toServerPipeFDs[1]);
		close(fromServerPipeFDs[0]);
		close(fromServerPipeFDs[1]);

		// Execute the language server
		execl(lspPath.c_str(), "typescript-language-server", "--stdio", (char *)nullptr);

		// If execl returns, it means an error occurred
		std::cerr << "\033[31mTypescript Child Error:\033[0m Failed to start "
					 "typescript-language-server (execl failed: "
				  << strerror(errno) << ")" << std::endl;
		exit(1); // Critical error, child process must exit
	}

	// Parent process:
	// Close unused pipe ends
	close(toServerPipeFDs[0]);	 // Parent does not read from this end
	close(fromServerPipeFDs[1]); // Parent does not write to this end

	// Open FILE streams for communication. These FDs are now owned by the FILE* streams.
	impl->input =
		fdopen(toServerPipeFDs[1], "w"); // Parent writes to server's stdin via this pipe end
	impl->output =
		fdopen(fromServerPipeFDs[0], "r"); // Parent reads from server's stdout via this pipe end

	if (!impl->input || !impl->output)
	{
		std::cerr << "\033[31mTypescript:\033[0m Failed to fdopen pipes: " << strerror(errno)
				  << std::endl;
		// If fdopen failed, the FDs might still be open. Close them directly.
		// If FILE* were successfully created, fclose would handle the FD.
		if (impl->input)
			fclose(impl->input);
		else
			close(toServerPipeFDs[1]);
		if (impl->output)
			fclose(impl->output);
		else
			close(fromServerPipeFDs[0]);
		impl->input = nullptr;
		impl->output = nullptr; // Nullify to prevent use in destructor logic

		int status; // Reap the child if fork succeeded but setup failed
		waitpid(impl->pid, &status, 0);
		impl->pid = 0;
		return false;
	}

	// Set streams to be unbuffered for more responsive I/O.
	// This is crucial for request-response patterns like LSP.
	setvbuf(impl->input, nullptr, _IONBF, 0);
	setvbuf(impl->output, nullptr, _IONBF, 0);

	std::cout << "\033[35mTypescript:\033[0m Sending initialize request for workspace: "
			  << workspacePath << std::endl;
	// Using a comprehensive set of capabilities for typescript-language-server
	std::string initRequest = R"({
        "jsonrpc": "2.0",
        "id": 1,
        "method": "initialize",
        "params": {
            "processId": null,
            "clientInfo": { "name": "MyTextEditor", "version": "0.1" },
            "rootUri": "file://)" +
							  workspacePath + R"(",
            "capabilities": {
                "textDocument": {
                    "synchronization": { "dynamicRegistration": true, "willSave": true, "willSaveWaitUntil": true, "didSave": true },
                    "completion": { "dynamicRegistration": true, "completionItem": { "snippetSupport": true, "commitCharactersSupport": true, "documentationFormat": ["markdown", "plaintext"], "deprecatedSupport": true, "preselectSupport": true, "tagSupport": {"valueSet": [1]}, "insertReplaceSupport": true, "resolveSupport": {"properties": ["documentation", "detail", "additionalTextEdits"]}, "insertTextModeSupport": {"valueSet": [1,2]}}, "completionItemKind": {"valueSet": [1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25]}, "contextSupport": true },
                    "hover": { "dynamicRegistration": true, "contentFormat": ["markdown", "plaintext"] },
                    "signatureHelp": { "dynamicRegistration": true, "signatureInformation": { "documentationFormat": ["markdown", "plaintext"], "parameterInformation": { "labelOffsetSupport": true }, "activeParameterSupport": true }, "contextSupport": true },
                    "definition": { "dynamicRegistration": true, "linkSupport": true },
                    "references": { "dynamicRegistration": true },
                    "documentHighlight": { "dynamicRegistration": true },
                    "documentSymbol": { "dynamicRegistration": true, "symbolKind": { "valueSet": [1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26]}, "hierarchicalDocumentSymbolSupport": true, "tagSupport": {"valueSet": [1]}, "labelSupport": true },
                    "codeAction": { "dynamicRegistration": true, "codeActionLiteralSupport": { "codeActionKind": { "valueSet": ["", "quickfix", "refactor", "refactor.extract", "refactor.inline", "refactor.rewrite", "source", "source.organizeImports"]}}, "isPreferredSupport": true, "disabledSupport": true, "dataSupport": true, "resolveSupport": {"properties": ["edit"]}, "honorsChangeAnnotations": false },
                    "formatting": { "dynamicRegistration": true },
                    "rangeFormatting": { "dynamicRegistration": true },
                    "rename": { "dynamicRegistration": true, "prepareSupport": true, "prepareSupportDefaultBehavior": 1, "honorsChangeAnnotations": true }
                },
                "workspace": {
                    "applyEdit": true,
                    "workspaceEdit": { "documentChanges": true, "resourceOperations": ["create", "rename", "delete"], "failureHandling": "abort", "normalizesLineEndings": true, "changeAnnotationSupport": {"groupsOnLabel": true} },
                    "didChangeConfiguration": { "dynamicRegistration": true },
                    "didChangeWatchedFiles": { "dynamicRegistration": true, "relativePatternSupport": true },
                    "symbol": { "dynamicRegistration": true, "symbolKind": { "valueSet": [1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26]}, "tagSupport": {"valueSet": [1]}, "resolveSupport": {"properties": ["location.range"]} },
                    "executeCommand": { "dynamicRegistration": true },
                    "workspaceFolders": true,
                    "configuration": true
                }
            }
        }
    })";

	if (!sendRequest(initRequest))
	{
		std::cerr << "\033[31mTypescript:\033[0m Failed to send initialize request" << std::endl;
		// Destructor path will handle cleanup of PID and any opened FILE* or FDs.
		return false;
	}
	std::cout << "\033[32mTypescript:\033[0m Initialize request sent successfully" << std::endl;

	const int MAX_INIT_ATTEMPTS = 20; // Increased attempts for potentially slower servers
	bool initResponseReceived = false;
	for (int attempt = 0; attempt < MAX_INIT_ATTEMPTS; ++attempt)
	{
		std::cout << "\033[35mTypescript:\033[0m Waiting for initialization response (attempt "
				  << (attempt + 1) << ")" << std::endl;

		int responseLength = 0;
		std::string response = readResponse(&responseLength);

		if (responseLength == -1)
		{ // Error indicated by readResponse
			std::cerr << "\033[31mTypescript:\033[0m Error reading response during initialization."
					  << std::endl;
			return false; // Destructor handles cleanup
		}
		if (response.empty() && responseLength == 0)
		{ // Valid empty JSON body, but not for init response.
			std::cout << "\033[33mTypescript:\033[0m Empty JSON body received, expected init "
						 "response. Retrying..."
					  << std::endl;
			usleep(200000); // 200ms
			continue;
		}
		if (response.empty())
		{ // No header / content at all
			std::cout << "\033[33mTypescript:\033[0m No response or empty header, retrying..."
					  << std::endl;
			usleep(500000); // 500ms, server might be slow starting
			continue;
		}

		// Check if this is the initialization response (id: 1)
		if (response.find("\"id\":1") != std::string::npos)
		{
			if (response.find("\"result\":") != std::string::npos)
			{
				std::cout << "\033[32mTypescript:\033[0m Received initialization response."
						  << std::endl;
				// You can log part of the response if needed for debugging:
				// std::cout << response.substr(0, std::min((size_t)400, response.length())) <<
				// (response.length() > 400 ? "..." : "") << std::endl;
				initResponseReceived = true;
				break;
			} else if (response.find("\"error\":") != std::string::npos)
			{
				std::cerr
					<< "\033[31mTypescript:\033[0m Initialization failed with error from server: "
					<< response << std::endl;
				return false; // Destructor handles cleanup
			}
		}

		std::cout << "\033[33mTypescript:\033[0m Received non-init message or unrelated response: "
				  << response.substr(0, std::min((size_t)100, response.length())) << "..."
				  << std::endl;
		// The server might send notifications before the init response.
	}

	if (!initResponseReceived)
	{
		std::cerr << "\033[31mTypescript:\033[0m Failed to receive initialization response after "
				  << MAX_INIT_ATTEMPTS << " attempts." << std::endl;
		return false; // Destructor handles cleanup
	}

	// Send the initialized notification
	std::string initializedNotification =
		R"({"jsonrpc": "2.0", "method": "initialized", "params": {}})";
	if (!sendRequest(initializedNotification))
	{
		std::cerr << "\033[31mTypescript:\033[0m Failed to send initialized notification"
				  << std::endl;
		return false; // Destructor handles cleanup
	}
	std::cout << "\033[32mTypescript:\033[0m Sent initialized notification" << std::endl;

	initialized = true;
	return true;
}

bool LSPAdapterTypescript::sendRequest(const std::string &request)
{
	if (!impl || !impl->input)
	{
		std::cerr << "\033[31mTypescript:\033[0m Cannot send request - server not initialized or "
					 "input pipe closed"
				  << std::endl;
		return false;
	}

	// LSP messages require Content-Length header
	// For debugging: std::cout << "DEBUG TS SEND: Content-Length: " << request.length() <<
	// "\r\n\r\n" << request << std::endl;
	if (fprintf(impl->input, "Content-Length: %zu\r\n\r\n%s", request.length(), request.c_str()) <
		0)
	{
		std::cerr << "\033[31mTypescript:\033[0m fprintf failed to write request. Error: "
				  << strerror(errno) << std::endl;
		// This often means the pipe is broken (e.g., server crashed).
		// Consider this a fatal error for the adapter's current session.
		// initialized = false; // Could mark as uninitialized to trigger re-init later.
		return false;
	}
	// fflush(impl->input) is implicitly handled by _IONBF or explicitly if needed,
	// but for _IONBF, fprintf should send data immediately.
	return true;
}

std::string LSPAdapterTypescript::readResponse(int *outContentLength)
{
	if (outContentLength)
		*outContentLength = -1; // Default to error state for length

	if (!impl || !impl->output)
	{
		std::cerr << "\033[31mTypescript:\033[0m Cannot read response - server not initialized or "
					 "output pipe closed"
				  << std::endl;
		return "";
	}

	char header_line[1024];
	int content_length = -1;
	bool headers_ended = false;

	// Read headers line by line
	while (fgets(header_line, sizeof(header_line), impl->output) != nullptr)
	{
		// For debugging: std::cout << "DEBUG TS HEADER LINE: " << header_line;
		if (strncmp(header_line, "Content-Length:", 15) == 0)
		{
			if (sscanf(header_line + 15, " %d", &content_length) != 1)
			{
				std::cerr
					<< "\033[31mTypescript:\033[0m Failed to parse Content-Length value from: "
					<< header_line << std::endl;
				return ""; // Malformed header
			}
		} else if (strcmp(header_line, "\r\n") == 0)
		{ // Empty line signifies end of headers
			headers_ended = true;
			break;
		}
		// Other headers are ignored for now.
	}

	if (!headers_ended)
	{ // fgets returned nullptr before finding an empty line
		if (feof(impl->output))
		{
			std::cerr << "\033[33mTypescript:\033[0m EOF reached while reading header (server "
						 "likely closed connection)."
					  << std::endl;
		} else
		{ // ferror
			std::cerr << "\033[31mTypescript:\033[0m Failed to read response header line. Error: "
					  << strerror(errno) << std::endl;
		}
		return ""; // Error or EOF
	}

	if (content_length < 0)
	{
		std::cerr << "\033[31mTypescript:\033[0m Invalid or missing Content-Length header after "
					 "reading all headers."
				  << std::endl;
		return "";
	}

	if (outContentLength)
		*outContentLength = content_length;

	if (content_length == 0)
	{
		// For debugging: std::cout << "DEBUG TS RECV (empty content)" << std::endl;
		return ""; // Valid case: empty JSON body (e.g., for some notifications if they were
				   // responses)
	}

	std::vector<char> buffer(content_length);
	size_t total_bytes_read = 0;
	while (total_bytes_read < static_cast<size_t>(content_length))
	{
		size_t bytes_to_read = static_cast<size_t>(content_length) - total_bytes_read;
		size_t current_bytes_read =
			fread(buffer.data() + total_bytes_read, 1, bytes_to_read, impl->output);

		if (current_bytes_read == 0)
		{ // Error or EOF
			if (feof(impl->output))
			{
				std::cerr << "\033[31mTypescript:\033[0m EOF reached prematurely while reading "
							 "message body. Expected "
						  << content_length << ", got " << total_bytes_read << "." << std::endl;
			} else
			{ // ferror
				std::cerr << "\033[31mTypescript:\033[0m Failed to read message body. Expected "
						  << content_length << ", got " << total_bytes_read
						  << ". Error: " << strerror(errno) << std::endl;
			}
			if (outContentLength)
				*outContentLength = -1; // Ensure error signaled for length
			// Return what was read, even if partial, but caller checks outContentLength
			return std::string(buffer.data(), total_bytes_read);
		}
		total_bytes_read += current_bytes_read;
	}

	std::string response_body(buffer.data(), total_bytes_read);
	// For debugging: std::cout << "DEBUG TS RECV: " << response_body << std::endl;
	return response_body;
}

std::string LSPAdapterTypescript::getLanguageId(const std::string &filePath) const
{
	size_t dot_pos = filePath.find_last_of(".");
	if (dot_pos == std::string::npos)
	{
		return "plaintext";
	}

	std::string ext = filePath.substr(dot_pos + 1);

	if (ext == "ts")
	{
		return "typescript";
	} else if (ext == "tsx")
	{
		return "typescriptreact";
	} else if (ext == "js")
	{
		return "javascript";
	} else if (ext == "jsx")
	{
		return "javascriptreact";
	}
	// You could add ".json" -> "json" or ".mjs", ".cjs" if desired

	return "plaintext"; // Default for unknown extensions
}