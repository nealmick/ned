#include "ai_open_router.h"
#include "../lib/json.hpp"
#include <curl/curl.h>
#include <iostream>
#include <atomic>
#include <thread>
#include <mutex>

using json = nlohmann::json;

extern std::atomic<bool> g_should_cancel;

// Global variable to hold the current token callback with mutex protection
static std::function<void(const std::string&)> g_currentTokenCallback = nullptr;
static std::mutex g_callbackMutex;

// Mutex to protect CURL initialization
static std::mutex g_curlInitMutex;

// Initialize the CURL initialization state
std::atomic<bool> OpenRouter::curl_initialized(false);

bool OpenRouter::initializeCURL() {
	std::lock_guard<std::mutex> lock(g_curlInitMutex);
	
	if (curl_initialized.load()) {
		std::cout << "[OpenRouter] CURL already initialized" << std::endl;
		return true; // Already initialized
	}
	
	std::cout << "[OpenRouter] Initializing CURL..." << std::endl;
	
	CURLcode res = curl_global_init(CURL_GLOBAL_ALL);
	if (res != CURLE_OK) {
		std::cout << "[OpenRouter] curl_global_init() failed: " << curl_easy_strerror(res) << std::endl;
		return false;
	}
	
	curl_initialized.store(true);
	std::cout << "[OpenRouter] CURL global initialization successful" << std::endl;
	return true;
}

void OpenRouter::cleanupCURL() {
	std::lock_guard<std::mutex> lock(g_curlInitMutex);
	
	if (curl_initialized.load()) {
		std::cout << "[OpenRouter] Cleaning up CURL..." << std::endl;
		curl_global_cleanup();
		curl_initialized.store(false);
		std::cout << "[OpenRouter] CURL global cleanup completed" << std::endl;
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
		std::cout << "[OpenRouter] Failed to initialize CURL" << std::endl;
		return "";
	}

	//std::cout << api_key << std::endl;
	CURL *curl = curl_easy_init();
	std::string response;
	long http_code = 0;

	if (curl)//testomg 
	{
		// Build JSON payload properly with escaping
		json payload = {{"model", "meta-llama/llama-4-scout"},
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
		std::cout << "[OpenRouter] Failed to initialize CURL" << std::endl;
		return "";
	}

	CURL *curl = curl_easy_init();
	std::string response;
	long http_code = 0;

	if (curl)
	{
		// Build JSON payload for general conversation (no code completion system prompt)
		json payload = {{"model", "meta-llama/llama-4-scout"},
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
	if (g_should_cancel) {
		return 0; // Stop receiving data if cancelled
	}
	
	// Read the data properly
	const char* charPtr = static_cast<const char*>(ptr);
	std::string chunk(charPtr, size * nmemb);
	
	// Process Server-Sent Events (SSE) format
	std::string buffer = chunk;
	size_t pos = 0;
	while ((pos = buffer.find('\n', pos)) != std::string::npos) {
		std::string line = buffer.substr(0, pos);
		buffer.erase(0, pos + 1);
		pos = 0;
		
		// Skip empty lines
		if (line.empty()) continue;
		
		// Check for data line
		if (line.substr(0, 5) == "data:") {
			std::string jsonData = line.substr(5);
			
			// Check for [DONE] message
			if (jsonData == "[DONE]") {
				return size * nmemb; // End of stream
			}
			
			try {
				json result = json::parse(jsonData);
				if (result.contains("choices") && result["choices"].is_array() && !result["choices"].empty()) {
					if (result["choices"][0].contains("delta") && 
						result["choices"][0]["delta"].contains("content")) {
						std::string content = result["choices"][0]["delta"]["content"].get<std::string>();
						if (!content.empty()) {
							// Thread-safe callback access
							std::lock_guard<std::mutex> lock(g_callbackMutex);
							if (g_currentTokenCallback) {
								g_currentTokenCallback(content);
							}
						}
					}
				}
			} catch (const json::exception &e) {
				// JSON parse error - silently handle
			}
		}
	}
	
	return size * nmemb;
}

bool OpenRouter::promptRequestStream(const std::string &prompt, const std::string &api_key, 
									std::function<void(const std::string&)> tokenCallback,
									std::atomic<bool>* cancelFlag)
{
	try {
		if (cancelFlag && cancelFlag->load()) {
			return false; // Return immediately if cancelled
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
		json payload = {{"model", "meta-llama/llama-4-scout"},
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
			// curl_easy_perform failed - silently handle
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
		// Clear the global callback in case of exception
		try {
			std::lock_guard<std::mutex> lock(g_callbackMutex);
			g_currentTokenCallback = nullptr;
		} catch (...) {
			// Failed to clear callback after exception - silently handle
		}
		return false;
	} catch (...) {
		// Clear the global callback in case of exception
		try {
			std::lock_guard<std::mutex> lock(g_callbackMutex);
			g_currentTokenCallback = nullptr;
		} catch (...) {
			// Failed to clear callback after unknown exception - silently handle
		}
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