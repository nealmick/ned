#include "ai_open_router.h"
#include "../lib/json.hpp"
#include "../util/settings.h"
#include <curl/curl.h>
#include <iostream>
#include <atomic>
#include <thread>
#include <mutex>
#include <map>

using json = nlohmann::json;

extern std::atomic<bool> g_should_cancel;

// Global variable to hold the current token callback with mutex protection
static std::function<void(const std::string&)> g_currentTokenCallback = nullptr;
static std::mutex g_callbackMutex;

// Mutex to protect CURL initialization
static std::mutex g_curlInitMutex;

// Initialize the CURL initialization state
std::atomic<bool> OpenRouter::curl_initialized(false);
std::mutex OpenRouter::g_curlInitMutex;

// Global variables for tool call accumulation
static std::map<std::string, json> g_accumulatedToolCalls;
static std::mutex g_toolCallMutex;

// Global mapping from index to id for tool call accumulation
static std::map<int, std::string> g_indexToIdMapping;
static std::mutex g_indexMappingMutex;

bool OpenRouter::initializeCURL() {
	std::lock_guard<std::mutex> lock(g_curlInitMutex);
	
	if (curl_initialized.load()) {
		return true; // Already initialized
	}
	
	CURLcode res = curl_global_init(CURL_GLOBAL_ALL);
	if (res != CURLE_OK) {
		return false;
	}
	
	curl_initialized.store(true);
	return true;
}

void OpenRouter::cleanupCURL() {
	std::lock_guard<std::mutex> lock(g_curlInitMutex);
	
	if (curl_initialized.load()) {
		curl_global_cleanup();
		curl_initialized.store(false);
	}
}

size_t OpenRouter::WriteData(void *ptr, size_t size, size_t nmemb, std::string *data)
{
	if (g_should_cancel) {
		return 0; // Stop receiving data if cancelled
	}
	data->append((char *)ptr, size * nmemb);
	return size * nmemb;
}

std::string OpenRouter::request(const std::string &prompt, const std::string &api_key)
{
	if (g_should_cancel) {
		return ""; // Return immediately if cancelled
	}

	// Ensure CURL is initialized
	if (!initializeCURL()) {
		return "";
	}

	CURL *curl = curl_easy_init();
	std::string response;
	long http_code = 0;

	if (curl)//testomg 
	{
		// Build JSON payload properly with escaping
		json payload = {{"model", gSettings.getCompletionModel()},
						{"messages",
						 {{{"role", "system"},
						   {"content", "Respond ONLY with code completion. No explanations."}},
						  {{"role", "user"}, {"content", prompt}}}},
						{"temperature", 0.2},
						{"max_tokens", 150}};

		struct curl_slist *headers = nullptr;
		headers = curl_slist_append(headers, "Content-Type: application/json");
		headers = curl_slist_append(headers, ("Authorization: Bearer " + api_key).c_str());
		headers = curl_slist_append(headers, "HTTP-Referer: my-text-editor");

		std::string json_str = payload.dump();

		curl_easy_setopt(curl, CURLOPT_URL, "https://openrouter.ai/api/v1/chat/completions");
		curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
		curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_str.c_str());
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteData);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
		curl_easy_setopt(curl, CURLOPT_TIMEOUT, 3L); // 3 second timeout
		curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 2L); // 2 second connect timeout
		curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L); // Don't use signals

		// Create a multi handle
		CURLM *multi_handle = curl_multi_init();
		curl_multi_add_handle(multi_handle, curl);

		// Start the request
		int still_running = 1;
		while (still_running && !g_should_cancel) {
			CURLMcode mc = curl_multi_perform(multi_handle, &still_running);
			if (mc != CURLM_OK) {
				break;
			}

			// Wait for activity, timeout or "nothing"
			int numfds;
			mc = curl_multi_wait(multi_handle, nullptr, 0, 10, &numfds);
			if (mc != CURLM_OK) {
				break;
			}

			// Check for completed transfers
			CURLMsg *msg;
			int msgs_left;
			while ((msg = curl_multi_info_read(multi_handle, &msgs_left))) {
				if (msg->msg == CURLMSG_DONE) {
					curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
					break;
				}
			}
		}

		// Cleanup
		curl_multi_remove_handle(multi_handle, curl);
		curl_multi_cleanup(multi_handle);
		curl_easy_cleanup(curl);
		curl_slist_free_all(headers);

		if (g_should_cancel) {
			return ""; // Return empty string if cancelled
		}

		if (http_code != 200)
		{
			return "HTTP error " + std::to_string(http_code) + ": " + response;
		}

		try
		{
			if (response.empty()) {
				return ""; // Return empty string if response is empty
			}
			json result = json::parse(response);
			if (!result.contains("choices") || !result["choices"].is_array() || result["choices"].empty()) {
				return ""; // Return empty string if response is invalid
			}
			std::string raw_content = result["choices"][0]["message"]["content"].get<std::string>();
			if (raw_content.empty()) {
				return ""; // Return empty string if content is empty
			}
			return sanitize_completion(raw_content);
		} catch (const json::exception &e)
		{
			return ""; // Return empty string on parse error
		}
	}

	return ""; // Return empty string on initialization failure
}

std::string OpenRouter::promptRequest(const std::string &prompt, const std::string &api_key)
{
	if (g_should_cancel) {
		return ""; // Return immediately if cancelled
	}

	// Ensure CURL is initialized
	if (!initializeCURL()) {
		return "";
	}

	CURL *curl = curl_easy_init();
	std::string response;
	long http_code = 0;

	if (curl)
	{
		// Build JSON payload for general conversation (no code completion system prompt)
		json payload = {{"model", gSettings.getAgentModel()},
						{"messages",
						 {{{"role", "user"}, {"content", prompt}}}},
						{"temperature", 0.7},
						{"max_tokens", 500}};

		struct curl_slist *headers = nullptr;
		headers = curl_slist_append(headers, "Content-Type: application/json");
		headers = curl_slist_append(headers, ("Authorization: Bearer " + api_key).c_str());
		headers = curl_slist_append(headers, "HTTP-Referer: my-text-editor");

		std::string json_str = payload.dump();

		curl_easy_setopt(curl, CURLOPT_URL, "https://openrouter.ai/api/v1/chat/completions");
		curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
		curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_str.c_str());
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteData);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
		curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L); // 10 second timeout for longer conversations
		curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 3L); // 3 second connect timeout
		curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L); // Don't use signals

		// Create a multi handle
		CURLM *multi_handle = curl_multi_init();
		curl_multi_add_handle(multi_handle, curl);

		// Start the request
		int still_running = 1;
		while (still_running && !g_should_cancel) {
			CURLMcode mc = curl_multi_perform(multi_handle, &still_running);
			if (mc != CURLM_OK) {
				break;
			}

			// Wait for activity, timeout or "nothing"
			int numfds;
			mc = curl_multi_wait(multi_handle, nullptr, 0, 10, &numfds);
			if (mc != CURLM_OK) {
				break;
			}

			// Check for completed transfers
			CURLMsg *msg;
			int msgs_left;
			while ((msg = curl_multi_info_read(multi_handle, &msgs_left))) {
				if (msg->msg == CURLMSG_DONE) {
					curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
					break;
				}
			}
		}

		// Cleanup
		curl_multi_remove_handle(multi_handle, curl);
		curl_multi_cleanup(multi_handle);
		curl_easy_cleanup(curl);
		curl_slist_free_all(headers);

		if (g_should_cancel) {
			return ""; // Return empty string if cancelled
		}

		if (http_code != 200)
		{
			return "HTTP error " + std::to_string(http_code) + ": " + response;
		}

		try
		{
			if (response.empty()) {
				return ""; // Return empty string if response is empty
			}
			json result = json::parse(response);
			if (!result.contains("choices") || !result["choices"].is_array() || result["choices"].empty()) {
				return ""; // Return empty string if response is invalid
			}
			std::string raw_content = result["choices"][0]["message"]["content"].get<std::string>();
			if (raw_content.empty()) {
				return ""; // Return empty string if content is empty
			}
			return raw_content; // Return raw content without sanitization for conversations
		} catch (const json::exception &e)
		{
			return ""; // Return empty string on parse error
		}
	}

	return ""; // Return empty string on initialization failure
}

size_t OpenRouter::WriteDataStream(void *ptr, size_t size, size_t nmemb, std::string *data)
{
	try {
		if (g_should_cancel) {
			return 0; // Stop receiving data if cancelled
		}
		
		const char* charPtr = static_cast<const char*>(ptr);
		if (!charPtr) {
			return 0;
		}
		
		std::string chunk(charPtr, size * nmemb);
		std::cout << "DEBUG: WriteDataStream received chunk: [" << chunk << "]" << std::endl;
		
		// Process Server-Sent Events (SSE) format
		std::string buffer = chunk;
		size_t pos = 0;
		while ((pos = buffer.find('\n', pos)) != std::string::npos) {
			std::string line = buffer.substr(0, pos);
			buffer.erase(0, pos + 1);
			pos = 0;
			
			if (line.empty()) continue;
			std::cout << "DEBUG: Processing line: [" << line << "]" << std::endl;
			if (line.substr(0, 5) == "data:") {
				std::string jsonData = line.substr(5);
				std::cout << "DEBUG: JSON data: [" << jsonData << "]" << std::endl;
				if (jsonData == "[DONE]") {
					std::cout << "DEBUG: Stream done" << std::endl;
					return size * nmemb; // End of stream
				}
				try {
					json result = json::parse(jsonData);
					std::cout << "DEBUG: Parsed JSON: " << result.dump() << std::endl;
					if (result.contains("choices") && result["choices"].is_array() && !result["choices"].empty()) {
						const auto& choice = result["choices"][0];
						if (choice.contains("delta") && choice["delta"].contains("content") && !choice["delta"]["content"].is_null()) {
							std::string content = choice["delta"]["content"].get<std::string>();
							if (!content.empty()) {
								std::cout << "DEBUG: Sending content: [" << content << "]" << std::endl;
								std::lock_guard<std::mutex> lock(g_callbackMutex);
								if (g_currentTokenCallback) {
									g_currentTokenCallback(content);
								}
							}
						}
						if (choice.contains("delta") && choice["delta"].contains("tool_calls")) {
							// Accumulate tool call fragments instead of sending them immediately
							std::lock_guard<std::mutex> lock(g_toolCallMutex);
							
							const auto& toolCalls = choice["delta"]["tool_calls"];
							for (const auto& toolCall : toolCalls) {
								// Get the tool call ID
								std::string toolCallId;
								if (toolCall.contains("id")) {
									toolCallId = toolCall["id"].get<std::string>();
								} else if (toolCall.contains("index")) {
									toolCallId = "index_" + std::to_string(toolCall["index"].get<int>());
								} else {
									continue; // Skip fragments without ID or index
								}
								
								// Initialize or update the accumulated tool call
								if (g_accumulatedToolCalls.find(toolCallId) == g_accumulatedToolCalls.end()) {
									g_accumulatedToolCalls[toolCallId] = json::object();
									g_accumulatedToolCalls[toolCallId]["id"] = toolCallId;
									g_accumulatedToolCalls[toolCallId]["type"] = "function";
									g_accumulatedToolCalls[toolCallId]["function"] = json::object();
									g_accumulatedToolCalls[toolCallId]["function"]["arguments"] = "";
								}
								
								// Merge the fragment into the accumulated tool call
								if (toolCall.contains("function")) {
									const auto& func = toolCall["function"];
									if (func.contains("name")) {
										g_accumulatedToolCalls[toolCallId]["function"]["name"] = func["name"];
									}
									if (func.contains("arguments")) {
										// Append arguments (they come in fragments)
										std::string currentArgs = g_accumulatedToolCalls[toolCallId]["function"]["arguments"].get<std::string>();
										std::string newArgs = func["arguments"].get<std::string>();
										
										// Simply concatenate the fragments - they are JSON fragments that need to be assembled
										g_accumulatedToolCalls[toolCallId]["function"]["arguments"] = currentArgs + newArgs;
									}
								}
								
								// Copy other fields
								if (toolCall.contains("index")) {
									g_accumulatedToolCalls[toolCallId]["index"] = toolCall["index"];
								}
								if (toolCall.contains("type")) {
									g_accumulatedToolCalls[toolCallId]["type"] = toolCall["type"];
								}
							}
						}
						if (choice.contains("finish_reason")) {
							std::string finishReason = choice["finish_reason"].get<std::string>();
							if (finishReason == "tool_calls") {
								std::cout << "DEBUG: Tool calls finished, sending complete tool calls" << std::endl;
								std::lock_guard<std::mutex> lock(g_callbackMutex);
								if (g_currentTokenCallback) {
									// Send all accumulated tool calls
									std::lock_guard<std::mutex> toolLock(g_toolCallMutex);
									for (const auto& [id, toolCall] : g_accumulatedToolCalls) {
										std::string toolCallJson = toolCall.dump();
										std::cout << "DEBUG: Sending complete tool call: [" << toolCallJson << "]" << std::endl;
										g_currentTokenCallback("TOOL_CALL:" + toolCallJson);
									}
									g_currentTokenCallback("\n[TOOL_CALLS_DETECTED]\n");
									// Clear accumulated tool calls
									g_accumulatedToolCalls.clear();
								}
							}
						}
					}
				} catch (const json::exception &e) {
					std::cout << "DEBUG: JSON parse error: " << e.what() << std::endl;
					// Ignore JSON parsing errors for individual chunks
				}
			}
		}
		return size * nmemb;
	} catch (const std::exception& e) {
		std::cout << "DEBUG: WriteDataStream exception: " << e.what() << std::endl;
		return 0;
	} catch (...) {
		std::cout << "DEBUG: WriteDataStream unknown exception" << std::endl;
		return 0;
	}
}

bool OpenRouter::promptRequestStream(const std::string &prompt, const std::string &api_key, 
									std::function<void(const std::string&)> tokenCallback,
									std::atomic<bool>* cancelFlag)
{
	try {
		if (cancelFlag && cancelFlag->load()) {
			return false; // Return immediately if cancelled
		}

		// Clear any previous tool call state
		{
			std::lock_guard<std::mutex> toolLock(g_toolCallMutex);
			std::lock_guard<std::mutex> indexLock(g_indexMappingMutex);
			g_accumulatedToolCalls.clear();
			g_indexToIdMapping.clear();
			std::cout << "DEBUG: Cleared tool call state for new request" << std::endl;
		}

		// Ensure CURL is initialized
		if (!initializeCURL()) {
			return false;
		}

		CURL *curl = curl_easy_init();
		if (!curl) {
			return false;
		}
		
		long http_code = 0;
		
		// Set the global callback with mutex protection
		{
			std::lock_guard<std::mutex> lock(g_callbackMutex);
			g_currentTokenCallback = tokenCallback;
		}
		
		// Build JSON payload for streaming conversation
		json payload = {{"model", gSettings.getAgentModel()},
						{"messages",
						 {{{"role", "user"}, {"content", prompt}}}},
						{"temperature", 0.7},
						{"max_tokens", 500},
						{"stream", true}}; // Enable streaming

		struct curl_slist *headers = nullptr;
		headers = curl_slist_append(headers, "Content-Type: application/json");
		headers = curl_slist_append(headers, ("Authorization: Bearer " + api_key).c_str());
		headers = curl_slist_append(headers, "HTTP-Referer: my-text-editor");
		headers = curl_slist_append(headers, "Accept: text/event-stream");

		std::string json_str = payload.dump();
		
		curl_easy_setopt(curl, CURLOPT_URL, "https://openrouter.ai/api/v1/chat/completions");
		curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
		curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_str.c_str());
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteDataStream);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, nullptr); // Not used in our implementation
		curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L); // 30 second timeout for streaming
		curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 5L); // 5 second connect timeout
		curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L); // Don't use signals
		
		CURLcode res = curl_easy_perform(curl);
		if (res != CURLE_OK) {
			curl_easy_cleanup(curl);
			curl_slist_free_all(headers);
			// Clear the global callback with mutex protection
			{
				std::lock_guard<std::mutex> lock(g_callbackMutex);
				g_currentTokenCallback = nullptr;
			}
			return false;
		}
		
		curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);

		// Cleanup
		curl_easy_cleanup(curl);
		curl_slist_free_all(headers);
		
		// Clear the global callback with mutex protection
		{
			std::lock_guard<std::mutex> lock(g_callbackMutex);
			g_currentTokenCallback = nullptr;
		}

		return http_code == 200;
	} catch (const std::exception& e) {
		return false;
	} catch (...) {
		return false;
	}
}

bool OpenRouter::jsonPayloadStream(const std::string &jsonPayload, const std::string &api_key, 
								   std::function<void(const std::string&)> tokenCallback,
								   std::atomic<bool>* cancelFlag)
{
	try {
		if (cancelFlag && cancelFlag->load()) {
			return false; // Return immediately if cancelled
		}

		// Clear any previous tool call state
		{
			std::lock_guard<std::mutex> toolLock(g_toolCallMutex);
			std::lock_guard<std::mutex> indexLock(g_indexMappingMutex);
			g_accumulatedToolCalls.clear();
			g_indexToIdMapping.clear();
			std::cout << "DEBUG: Cleared tool call state for new request" << std::endl;
		}

		// Ensure CURL is initialized
		if (!initializeCURL()) {
			return false;
		}

		CURL *curl = curl_easy_init();
		if (!curl) {
			return false;
		}
		
		long http_code = 0;
		
		// Set the global callback with mutex protection
		{
			std::lock_guard<std::mutex> lock(g_callbackMutex);
			g_currentTokenCallback = tokenCallback;
			std::cout << "DEBUG: Set g_currentTokenCallback for jsonPayloadStream" << std::endl;
		}
		
		// Parse the JSON payload and add streaming
		json payload;
		try {
			payload = json::parse(jsonPayload);
		} catch (const json::parse_error& e) {
			curl_easy_cleanup(curl);
			{
				std::lock_guard<std::mutex> lock(g_callbackMutex);
				g_currentTokenCallback = nullptr;
			}
			return false;
		}
		
		// Add streaming to the payload
		payload["stream"] = true;

		struct curl_slist *headers = nullptr;
		headers = curl_slist_append(headers, "Content-Type: application/json");
		headers = curl_slist_append(headers, ("Authorization: Bearer " + api_key).c_str());
		headers = curl_slist_append(headers, "HTTP-Referer: my-text-editor");
		headers = curl_slist_append(headers, "Accept: text/event-stream");

		std::string json_str = payload.dump();
		
		curl_easy_setopt(curl, CURLOPT_URL, "https://openrouter.ai/api/v1/chat/completions");
		curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
		curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_str.c_str());
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteDataStream);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, nullptr); // Not used in our implementation
		curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L); // 30 second timeout for streaming
		curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 5L); // 5 second connect timeout
		curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L); // Don't use signals
		
		CURLcode res = curl_easy_perform(curl);
		if (res != CURLE_OK) {
			std::cerr << "DEBUG: CURL error: " << curl_easy_strerror(res) << std::endl;
			curl_easy_cleanup(curl);
			curl_slist_free_all(headers);
			// Clear the global callback with mutex protection
			{
				std::lock_guard<std::mutex> lock(g_callbackMutex);
				g_currentTokenCallback = nullptr;
				std::cout << "DEBUG: Cleared g_currentTokenCallback at end of jsonPayloadStream (error)" << std::endl;
			}
			return false;
		}
		
		curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
		std::cout << "DEBUG: HTTP response code: " << http_code << std::endl;

		// Cleanup
		curl_easy_cleanup(curl);
		curl_slist_free_all(headers);
		
		// Clear the global callback with mutex protection
		{
			std::lock_guard<std::mutex> lock(g_callbackMutex);
			g_currentTokenCallback = nullptr;
			std::cout << "DEBUG: Cleared g_currentTokenCallback at end of jsonPayloadStream (success)" << std::endl;
		}

		return http_code == 200;
	} catch (const std::exception& e) {
		std::cerr << "DEBUG: Exception in jsonPayloadStream: " << e.what() << std::endl;
		return false;
	} catch (...) {
		std::cerr << "DEBUG: Unknown exception in jsonPayloadStream" << std::endl;
		return false;
	}
}

std::string OpenRouter::sanitize_completion(const std::string &completion)
{
	if (completion.empty()) {
		return "";
	}

	// Remove markdown code blocks
	size_t code_start = completion.find("```");
	if (code_start != std::string::npos)
	{
		size_t content_start = completion.find('\n', code_start);
		if (content_start == std::string::npos) {
			return completion; // Return original if no newline found
		}
		content_start++;
		size_t code_end = completion.rfind("```");
		if (code_end != std::string::npos && code_end > content_start)
		{
			return completion.substr(content_start, code_end - content_start);
		}
	}

	// Trim whitespace
	auto front = completion.find_first_not_of(" \t\n\r");
	if (front == std::string::npos) {
		return completion; // Return original if only whitespace
	}
	auto back = completion.find_last_not_of(" \t\n\r");
	return completion.substr(front, back - front + 1);
}