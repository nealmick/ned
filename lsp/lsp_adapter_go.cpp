#include "lsp_adapter_go.h"
#include <cerrno>  // For errno
#include <cstdio>  // For FILE, fprintf, fgets, fread, sscanf, setvbuf, fclose
#include <cstring> // For strncmp, strcmp, strerror
#ifndef PLATFORM_WINDOWS
#include <fcntl.h>	  // For open (if redirecting child's stderr to a file)
#include <signal.h>	  // For kill, SIGTERM, SIGKILL
#include <sys/wait.h> // For waitpid, WNOHANG, WIFEXITED, WEXITSTATUS, WIFSIGNALED, WTERMSIG
#include <unistd.h>	  // For pipe, fork, dup2, execl, close, usleep, access, X_OK, R_OK
#endif
#include <iostream> // For std::cout, std::cerr
#include <sstream>	// For std::ostringstream
#include <vector>	// For std::vector<char> in readResponse

// PImpl class definition
class LSPAdapterGo::GoImpl
{
  public:
	GoImpl() : input(nullptr), output(nullptr), pid(0) {}

	FILE *input;
	FILE *output;
	pid_t pid;
};

LSPAdapterGo::LSPAdapterGo() : impl(new GoImpl()), initialized(false)
{
	const char *homeDir = getenv("HOME");
	if (homeDir)
	{
		lspPath = std::string(homeDir) + "/go/bin/gopls";
	} else
	{
		lspPath = "/Users/neal/go/bin/gopls"; // Your specific path as a fallback
		std::cerr << "\033[33mGo Adapter Constructor Warning:\033[0m HOME "
					 "environment variable not "
					 "set. Using hardcoded gopls path: "
				  << lspPath << std::endl;
	}

	if (access(lspPath.c_str(), X_OK) != 0)
	{
		std::cerr << "\033[31mGo Adapter Constructor Error:\033[0m gopls "
					 "executable at '"
				  << lspPath
				  << "' not found or not executable. Error: " << strerror(errno)
				  << std::endl;
	} else
	{
		std::cout << "\033[35mGo Adapter Constructor:\033[0m Using gopls executable: "
				  << lspPath << std::endl;
	}
}

LSPAdapterGo::~LSPAdapterGo()
{
	if (impl && impl->input)
	{
		if (initialized)
		{
			std::cout << "\033[35mGo Adapter Destructor:\033[0m Sending "
						 "shutdown request."
					  << std::endl;
			std::string shutdownRequest =
				R"({"jsonrpc":"2.0","id":9998,"method": "shutdown","params":null})"; // Note
																					 // space
			sendRequest(shutdownRequest);

			std::cout << "\033[35mGo Adapter Destructor:\033[0m Sending exit "
						 "notification."
					  << std::endl;
			std::string exitNotification =
				R"({"jsonrpc":"2.0","method": "exit","params":null})"; // Note space
			sendRequest(exitNotification);
		}
		if (fclose(impl->input) != 0)
		{ /* Optional: log error */
		}
		impl->input = nullptr;
	}

	if (impl && impl->output)
	{
		if (fclose(impl->output) != 0)
		{ /* Optional: log error */
		}
		impl->output = nullptr;
	}

	if (impl && impl->pid > 0)
	{
		std::cout << "\033[35mGo Adapter Destructor:\033[0m Waiting for LSP "
					 "server process (PID: "
				  << impl->pid << ") to exit." << std::endl;
		int status = 0;
		pid_t wait_ret = waitpid(impl->pid, &status, WNOHANG);

		if (wait_ret == 0)
		{
			usleep(200000);
			wait_ret = waitpid(impl->pid, &status, WNOHANG);
			if (wait_ret == 0)
			{
				std::cout << "\033[33mGo Adapter Destructor:\033[0m ...sending "
							 "SIGTERM."
						  << std::endl;
				kill(impl->pid, SIGTERM);
				usleep(500000);
				wait_ret = waitpid(impl->pid, &status, WNOHANG);
				if (wait_ret == 0)
				{
					std::cout << "\033[31mGo Adapter Destructor:\033[0m "
								 "...sending SIGKILL."
							  << std::endl;
					kill(impl->pid, SIGKILL);
					waitpid(impl->pid, &status, 0);
				}
			}
		}
		if (wait_ret > 0 && !(WIFEXITED(status) || WIFSIGNALED(status)))
		{
			waitpid(impl->pid, &status, 0);
		}

		if (WIFEXITED(status))
		{
			std::cout << "\033[32mGo Adapter Destructor:\033[0m LSP server (PID: "
					  << impl->pid << ") exited with status " << WEXITSTATUS(status)
					  << std::endl;
		} else if (WIFSIGNALED(status))
		{
			std::cout << "\033[33mGo Adapter Destructor:\033[0m LSP server (PID: "
					  << impl->pid << ") terminated by signal " << WTERMSIG(status)
					  << std::endl;
		} else if (wait_ret == -1 && errno == ECHILD)
		{
			std::cout << "\033[33mGo Adapter Destructor:\033[0m LSP server (PID: "
					  << impl->pid << ") already reaped." << std::endl;
		} else
		{
			std::cout << "\033[33mGo Adapter Destructor:\033[0m LSP server (PID: "
					  << impl->pid << ") exit status unknown (wait_ret=" << wait_ret
					  << ", errno=" << errno << ")" << std::endl;
		}
		impl->pid = 0;
	}
}

bool LSPAdapterGo::initialize(const std::string &workspacePath)
{
	if (initialized)
	{
		std::cout << "\033[35mGo Adapter:\033[0m Already initialized." << std::endl;
		return true;
	}
	if (!impl)
	{
		std::cerr << "\033[31mGo Adapter Initialize:\033[0m PImpl not created."
				  << std::endl;
		return false;
	}
	if (lspPath.empty() || access(lspPath.c_str(), X_OK) != 0)
	{
		std::cerr << "\033[31mGo Adapter Initialize:\033[0m gopls executable at '"
				  << lspPath
				  << "' not found or not executable. Error: " << strerror(errno)
				  << ". Cannot start gopls." << std::endl;
		return false;
	}

	std::cout << "\033[35mGo Adapter Initialize:\033[0m Starting LSP server using: "
			  << lspPath << std::endl;

	int toServerPipeFDs[2];
	int fromServerPipeFDs[2];

	if (pipe(toServerPipeFDs) < 0)
	{
		std::cerr << "\033[31mGo Adapter Initialize:\033[0m Failed to create "
					 "toServerPipe: "
				  << strerror(errno) << std::endl;
		return false;
	}
	if (pipe(fromServerPipeFDs) < 0)
	{
		std::cerr << "\033[31mGo Adapter Initialize:\033[0m Failed to create "
					 "fromServerPipe: "
				  << strerror(errno) << std::endl;
		close(toServerPipeFDs[0]);
		close(toServerPipeFDs[1]);
		return false;
	}

	impl->pid = fork();
	if (impl->pid < 0)
	{
		std::cerr << "\033[31mGo Adapter Initialize:\033[0m Fork failed: "
				  << strerror(errno) << std::endl;
		close(toServerPipeFDs[0]);
		close(toServerPipeFDs[1]);
		close(fromServerPipeFDs[0]);
		close(fromServerPipeFDs[1]);
		impl->pid = 0;
		return false;
	}

	if (impl->pid == 0)
	{ // Child process
		if (dup2(toServerPipeFDs[0], STDIN_FILENO) == -1)
		{
			fprintf(stderr,
					"Go Adapter Child Error: dup2 stdin failed: %s\n",
					strerror(errno));
			exit(errno);
		}
		if (dup2(fromServerPipeFDs[1], STDOUT_FILENO) == -1)
		{
			fprintf(stderr,
					"Go Adapter Child Error: dup2 stdout failed: %s\n",
					strerror(errno));
			exit(errno);
		}
		// Optional: Redirect child's STDERR (uncomment to use)
		// int err_fd = open("/tmp/gopls_stderr.log", O_WRONLY | O_CREAT |
		// O_TRUNC, 0644); if (err_fd != -1) {
		//     if (dup2(err_fd, STDERR_FILENO) == -1) { fprintf(stderr, "Go
		//     Adapter Child Error: dup2 stderr failed: %s\n", strerror(errno));
		//     } close(err_fd);
		// } else { fprintf(stderr, "Go Adapter Child Error: Could not open
		// /tmp/gopls_stderr.log: %s\n", strerror(errno)); }

		close(toServerPipeFDs[0]);
		close(toServerPipeFDs[1]);
		close(fromServerPipeFDs[0]);
		close(fromServerPipeFDs[1]);

		execl(lspPath.c_str(), "gopls", (char *)nullptr);

		fprintf(stderr,
				"Go Adapter Child Error: Failed to execute gopls at '%s'. "
				"Error: %s\n",
				lspPath.c_str(),
				strerror(errno));
		exit(127);
	}

	// Parent process
	close(toServerPipeFDs[0]);
	close(fromServerPipeFDs[1]);

	std::cout << "\033[35mGo Adapter Initialize:\033[0m Parent: About to "
				 "fdopen toServerPipeFDs[1] ("
			  << toServerPipeFDs[1] << ") for writing (impl->input)." << std::endl;
	impl->input = fdopen(toServerPipeFDs[1], "w");

	std::cout << "\033[35mGo Adapter Initialize:\033[0m Parent: About to "
				 "fdopen fromServerPipeFDs[0] ("
			  << fromServerPipeFDs[0] << ") for reading (impl->output)." << std::endl;
	impl->output = fdopen(fromServerPipeFDs[0], "r");

	if (!impl->input)
	{
		std::cerr << "\033[31mGo Adapter Initialize:\033[0m Failed to fdopen "
					 "impl->input "
					 "(toServerPipeFDs[1]="
				  << toServerPipeFDs[1] << "). Error: " << strerror(errno) << std::endl;
		if (impl->output)
			fclose(impl->output);
		else
			close(fromServerPipeFDs[0]);
		close(toServerPipeFDs[1]);
		int status;
		if (impl->pid > 0)
			waitpid(impl->pid, &status, 0);
		impl->pid = 0;
		return false;
	}
	if (!impl->output)
	{
		std::cerr << "\033[31mGo Adapter Initialize:\033[0m Failed to fdopen "
					 "impl->output "
					 "(fromServerPipeFDs[0]="
				  << fromServerPipeFDs[0] << "). Error: " << strerror(errno) << std::endl;
		fclose(impl->input);
		close(fromServerPipeFDs[0]);
		int status;
		if (impl->pid > 0)
			waitpid(impl->pid, &status, 0);
		impl->pid = 0;
		return false;
	}

	std::cout << "\033[35mGo Adapter Initialize:\033[0m Parent: fdopen "
				 "successful. Setting "
				 "unbuffered I/O."
			  << std::endl;
	setvbuf(impl->input, nullptr, _IONBF, 0);
	setvbuf(impl->output, nullptr, _IONBF, 0);

	std::cout << "\033[35mGo Adapter Initialize:\033[0m Sending initialize "
				 "request for workspace: "
			  << workspacePath << std::endl;
	std::string initRequest = R"({
        "jsonrpc": "2.0", "id": 1, "method": "initialize", 
        "params": {
            "processId": null, 
            "clientInfo": { "name": "NED-GoClient", "version": "0.1" },
            "rootUri": "file://)" +
							  workspacePath + R"(",
            "capabilities": {
                "textDocument": {
                    "synchronization": { "dynamicRegistration": true, "willSave": false, "willSaveWaitUntil": false, "didSave": true },
                    "completion": { "dynamicRegistration": true, "completionItem": { "snippetSupport": true } },
                    "hover": { "dynamicRegistration": true },
                    "signatureHelp": { "dynamicRegistration": true },
                    "definition": { "dynamicRegistration": true },
                    "references": { "dynamicRegistration": true },
                    "documentHighlight": { "dynamicRegistration": true },
                    "codeAction": { "dynamicRegistration": true, "codeActionLiteralSupport": { "codeActionKind": { "valueSet": ["", "quickfix", "refactor", "refactor.extract", "refactor.inline", "refactor.rewrite", "source", "source.organizeImports"] } } },
                    "formatting": { "dynamicRegistration": true },
                    "rangeFormatting": { "dynamicRegistration": true },
                    "rename": { "dynamicRegistration": true, "prepareSupport": true }
                },
                "workspace": {
                    "applyEdit": true,
                    "workspaceEdit": { "documentChanges": true },
                    "didChangeConfiguration": { "dynamicRegistration": true },
                    "symbol": { "dynamicRegistration": true, "symbolKind": { "valueSet": [1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26] } }
                }
            }
        }
    })";

	if (!sendRequest(initRequest))
	{
		std::cerr << "\033[31mGo Adapter Initialize:\033[0m Failed to send "
					 "initialize request."
				  << std::endl;
		return false;
	}
	std::cout << "\033[32mGo Adapter Initialize:\033[0m Initialize request "
				 "sent successfully."
			  << std::endl;

	const int MAX_INIT_ATTEMPTS = 60;
	const int INIT_RETRY_DELAY_US = 750000;
	bool initResponseReceived = false;

	for (int attempt = 0; attempt < MAX_INIT_ATTEMPTS; ++attempt)
	{
		std::cout << "\033[35mGo Adapter Initialize:\033[0m Waiting for "
					 "initialization response (attempt "
				  << (attempt + 1) << "/" << MAX_INIT_ATTEMPTS << ")" << std::endl;
		int responseLength = 0;
		std::string response = readResponse(&responseLength);

		if (responseLength == -1)
		{
			std::cerr << "\033[31mGo Adapter Initialize:\033[0m Error reading "
						 "response during "
						 "initialization."
					  << std::endl;
			return false;
		}
		if (response.empty() && attempt < MAX_INIT_ATTEMPTS - 1)
		{
			usleep(INIT_RETRY_DELAY_US);
			continue;
		}

		if (response.find("\"id\":1") != std::string::npos)
		{
			if (response.find("\"result\":") != std::string::npos)
			{
				std::cout << "\033[32mGo Adapter Initialize:\033[0m Received "
							 "initialization response."
						  << std::endl;
				initResponseReceived = true;
				break;
			} else if (response.find("\"error\":") != std::string::npos)
			{
				std::cerr << "\033[31mGo Adapter Initialize:\033[0m "
							 "Initialization failed with "
							 "error from server: "
						  << response << std::endl;
				return false;
			}
		}
		if (!response.empty())
		{
			std::cout << "\033[33mGo Adapter Initialize:\033[0m Received "
						 "non-init message: "
					  << response.substr(0, std::min((size_t)150, response.length()))
					  << "..." << std::endl;
		}
		usleep(INIT_RETRY_DELAY_US / 3);
	}

	if (!initResponseReceived)
	{
		std::cerr << "\033[31mGo Adapter Initialize:\033[0m Failed to receive "
					 "initialization "
					 "response after "
				  << MAX_INIT_ATTEMPTS << " attempts." << std::endl;
		return false;
	}

	// ******** FIX: Set initialized = true BEFORE sending "initialized"
	// notification ********
	initialized = true;
	std::cout << "\033[32mGo Adapter State:\033[0m Server handshake "
				 "successful. Marking adapter as "
				 "initialized BEFORE sending 'initialized' notification."
			  << std::endl;
	// ******** END FIX ********

	// The "initialized" notification does NOT use "method ":"initialized" with
	// a space, it's just "method":"initialized" because it's a fixed string not
	// taking variables. The sendRequest logic will correctly identify it as
	// non-special if needed, but since `initialized` is now true, it will pass
	// regardless.
	std::string initializedNotification =
		R"({"jsonrpc": "2.0", "method":"initialized", "params": {}})";
	if (!sendRequest(initializedNotification))
	{
		std::cerr << "\033[31mGo Adapter Initialize:\033[0m Failed to send "
					 "'initialized' notification."
				  << std::endl;
	} else
	{
		std::cout << "\033[32mGo Adapter Initialize:\033[0m Sent 'initialized' "
					 "notification successfully."
				  << std::endl;
	}

	// initialized = true; // This line was moved up
	std::cout << "\033[32mGo Adapter Initialize:\033[0m gopls LSP session "
				 "fully established for "
				 "workspace: "
			  << workspacePath << std::endl;
	return true;
}

bool LSPAdapterGo::sendRequest(const std::string &request)
{
	if (!impl || !impl->input)
	{
		std::cerr << "\033[31mGo Adapter SendRequest:\033[0m Cannot send "
					 "request - impl or "
					 "impl->input is null. Request: "
				  << request.substr(0, 50) << "..." << std::endl;
		return false;
	}

	// These requests are part of the core lifecycle and should always be
	// allowed if pipes are up. The "method": "initialize" (with space) matches
	// the dynamically generated initRequest. "method":"shutdown" and
	// "method":"exit" (no space) match the fixed strings in the destructor.
	bool is_initialize_req =
		(request.find("\"method\": \"initialize\"") != std::string::npos);
	bool is_shutdown_req = (request.find("\"method\":\"shutdown\"") !=
							std::string::npos); // Exact match for destructor
	bool is_exit_req = (request.find("\"method\":\"exit\"") !=
						std::string::npos); // Exact match for destructor

	bool is_allowed_pre_init = is_initialize_req || is_shutdown_req || is_exit_req;

	// The "initialized" notification is different. It's sent *after* the server
	// has responded to "initialize" and *after* our `initialized` flag is set
	// to true. So, it will pass the `!initialized` check below. All other
	// regular LSP requests (hover, definition, didOpen, etc.) must wait for
	// `initialized` to be true.

	if (!initialized && !is_allowed_pre_init)
	{
		std::cerr << "\033[31mGo Adapter SendRequest:\033[0m Server not fully "
					 "initialized. "
					 "Blocking request: "
				  << request.substr(0, 50) << "..." << std::endl;
		return false;
	}

	if (fprintf(impl->input,
				"Content-Length: %zu\r\n\r\n%s",
				request.length(),
				request.c_str()) < 0)
	{
		std::cerr << "\033[31mGo Adapter SendRequest:\033[0m fprintf failed: "
				  << strerror(errno) << ". Request: " << request.substr(0, 50) << "..."
				  << std::endl;
		return false;
	}
	return true;
}

std::string LSPAdapterGo::readResponse(int *outContentLength)
{
	if (outContentLength)
		*outContentLength = -1;

	if (!impl || !impl->output)
	{
		std::cerr << "\033[31mGo Adapter ReadResponse:\033[0m Cannot read - "
					 "impl or impl->output is null."
				  << std::endl;
		return "";
	}

	char header_line[1024];
	int content_length = -1;
	bool headers_ended = false;

	while (fgets(header_line, sizeof(header_line), impl->output) != nullptr)
	{
		if (strncmp(header_line, "Content-Length:", 15) == 0)
		{
			if (sscanf(header_line + 15, " %d", &content_length) != 1)
			{
				std::cerr << "\033[31mGo Adapter ReadResponse:\033[0m Failed "
							 "to parse "
							 "Content-Length from: "
						  << header_line << std::endl;
				return "";
			}
		} else if (strcmp(header_line, "\r\n") == 0)
		{
			headers_ended = true;
			break;
		}
	}

	if (!headers_ended)
	{
		if (feof(impl->output))
		{
			std::cerr << "\033[33mGo Adapter ReadResponse:\033[0m EOF reading "
						 "header (server closed)."
					  << std::endl;
		} else if (ferror(impl->output))
		{
			std::cerr << "\033[31mGo Adapter ReadResponse:\033[0m Error "
						 "reading header: "
					  << strerror(errno) << std::endl;
		} else
		{
			std::cerr << "\033[31mGo Adapter ReadResponse:\033[0m Unknown "
						 "issue reading header."
					  << std::endl;
		}
		return "";
	}

	if (content_length < 0)
	{
		std::cerr << "\033[31mGo Adapter ReadResponse:\033[0m Invalid/missing "
					 "Content-Length."
				  << std::endl;
		return "";
	}
	if (outContentLength)
		*outContentLength = content_length;
	if (content_length == 0)
	{
		return "";
	}

	std::vector<char> buffer(content_length);
	size_t total_bytes_read = 0;
	while (total_bytes_read < static_cast<size_t>(content_length))
	{
		size_t bytes_to_read_this_iteration =
			static_cast<size_t>(content_length) - total_bytes_read;
		size_t current_bytes_read = fread(buffer.data() + total_bytes_read,
										  1,
										  bytes_to_read_this_iteration,
										  impl->output);
		if (current_bytes_read == 0)
		{
			if (feof(impl->output))
			{
				std::cerr << "\033[31mGo Adapter ReadResponse:\033[0m EOF "
							 "prematurely reading "
							 "body. Expected "
						  << content_length << ", got " << total_bytes_read << "."
						  << std::endl;
			} else if (ferror(impl->output))
			{
				std::cerr << "\033[31mGo Adapter ReadResponse:\033[0m Error "
							 "reading body. Expected "
						  << content_length << ", got " << total_bytes_read
						  << ". Error: " << strerror(errno) << std::endl;
			} else
			{
				std::cerr << "\033[31mGo Adapter ReadResponse:\033[0m Unknown "
							 "issue reading body."
						  << std::endl;
			}
			if (outContentLength)
				*outContentLength = -1;
			return std::string(buffer.data(), total_bytes_read);
		}
		total_bytes_read += current_bytes_read;
	}
	return std::string(buffer.data(), total_bytes_read);
}

std::string LSPAdapterGo::getLanguageId(const std::string &filePath) const
{
	size_t dot_pos = filePath.find_last_of(".");
	if (dot_pos != std::string::npos)
	{
		std::string ext = filePath.substr(dot_pos + 1);
		if (ext == "go")
		{
			return "go";
		}
	}
	return "plaintext";
}