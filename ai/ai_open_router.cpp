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

// Global variables for full response accumulation
static json g_fullResponse;
static std::mutex g_responseMutex;
static std::function<void(const json&)> g_responseCallback = nullptr;

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
		
		// Process Server-Sent Events (SSE) format
		std::string buffer = chunk;
		size_t pos = 0;
		while ((pos = buffer.find('\n', pos)) != std::string::npos) {
			std::string line = buffer.substr(0, pos);
			buffer.erase(0, pos + 1);
			pos = 0;
			
			if (line.empty()) continue;
			if (line.substr(0, 5) == "data:") {
				std::string jsonData = line.substr(5);
				if (jsonData == "[DONE]") {
					return size * nmemb; // End of stream
				}
				try {
					json result = json::parse(jsonData);
					if (result.contains("choices") && result["choices"].is_array() && !result["choices"].empty()) {
						const auto& choice = result["choices"][0];
						if (choice.contains("delta") && choice["delta"].contains("content") && !choice["delta"]["content"].is_null()) {
							std::string content = choice["delta"]["content"].get<std::string>();
							if (!content.empty()) {
								std::lock_guard<std::mutex> lock(g_callbackMutex);
								if (g_currentTokenCallback) {
									g_currentTokenCallback(content);
								}
							}
						}
						if (choice.contains("delta") && choice["delta"].contains("tool_calls")) {
							// Accumulate tool call fragments
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
										
										// Simply concatenate the fragments
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
							std::cout << "DEBUG: Found finish_reason in streaming: " << finishReason << std::endl;
							if (finishReason == "tool_calls") {
								std::lock_guard<std::mutex> lock(g_callbackMutex);
								if (g_currentTokenCallback) {
									std::lock_guard<std::mutex> toolLock(g_toolCallMutex);
									for (const auto& [id, toolCall] : g_accumulatedToolCalls) {
										std::string toolCallJson = toolCall.dump();
										g_currentTokenCallback("TOOL_CALL:" + toolCallJson);
									}
									g_accumulatedToolCalls.clear();
								}
							}
							
							// Call the response callback when we have a finish_reason
							if (!finishReason.empty() && finishReason != "null") {
								std::cout << "DEBUG: Finish reason detected: " << finishReason << ", calling response callback" << std::endl;
								std::lock_guard<std::mutex> responseLock(g_responseMutex);
								if (g_responseCallback && !g_fullResponse.is_null()) {
									std::cout << "DEBUG: Full response accumulated: " << g_fullResponse.dump(2) << std::endl;
									g_responseCallback(g_fullResponse);
								} else {
									std::cout << "DEBUG: No response callback or null response" << std::endl;
								}
							}
						}
					}
				} catch (const json::exception &e) {
					// Ignore JSON parsing errors for individual chunks
				}
			}
		}
		return size * nmemb;
	} catch (const std::exception& e) {
		return 0;
	} catch (...) {
		return 0;
	}
}

size_t OpenRouter::WriteDataStreamWithResponse(void *ptr, size_t size, size_t nmemb, std::string *data)
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
		
		// Process Server-Sent Events (SSE) format
		std::string buffer = chunk;
		size_t pos = 0;
		while ((pos = buffer.find('\n', pos)) != std::string::npos) {
			std::string line = buffer.substr(0, pos);
			buffer.erase(0, pos + 1);
			pos = 0;
			
			if (line.empty()) continue;
			if (line.substr(0, 5) == "data:") {
				std::string jsonData = line.substr(5);
				if (jsonData == "[DONE]") {
					std::lock_guard<std::mutex> responseLock(g_responseMutex);
					if (g_responseCallback && !g_fullResponse.is_null()) {
						g_responseCallback(g_fullResponse);
					} else {
						std::cout << "DEBUG: No response callback or null response" << std::endl;
					}
					return size * nmemb; // End of stream
				}
				try {
					json result = json::parse(jsonData);
					
					// Accumulate the full response
					{
						std::lock_guard<std::mutex> responseLock(g_responseMutex);
						if (g_fullResponse.is_null()) {
							// Initialize the full response structure
							g_fullResponse = json::object();
							g_fullResponse["id"] = result.value("id", "");
							g_fullResponse["object"] = result.value("object", "");
							g_fullResponse["created"] = result.value("created", 0);
							g_fullResponse["model"] = result.value("model", "");
							g_fullResponse["choices"] = json::array();
							g_fullResponse["usage"] = result.value("usage", json::object());
						} else {
						}
						
						// Merge choices
						if (result.contains("choices") && result["choices"].is_array()) {
							for (size_t i = 0; i < result["choices"].size(); ++i) {
								if (i >= g_fullResponse["choices"].size()) {
									g_fullResponse["choices"].push_back(json::object());
								}
								
								const auto& choice = result["choices"][i];
								auto& fullChoice = g_fullResponse["choices"][i];
								
								// Merge delta into the full choice
								if (choice.contains("delta")) {
									const auto& delta = choice["delta"];
									
									// Merge content
									if (delta.contains("content") && !delta["content"].is_null()) {
										if (!fullChoice.contains("message")) {
											fullChoice["message"] = json::object();
											fullChoice["message"]["role"] = "assistant";
											fullChoice["message"]["content"] = "";
										}
										fullChoice["message"]["content"] = fullChoice["message"]["content"].get<std::string>() + delta["content"].get<std::string>();
									}
									
									// Merge tool calls
									if (delta.contains("tool_calls")) {
										if (!fullChoice.contains("message")) {
											fullChoice["message"] = json::object();
											fullChoice["message"]["role"] = "assistant";
											fullChoice["message"]["content"] = nullptr;
										}
										if (!fullChoice["message"].contains("tool_calls")) {
											fullChoice["message"]["tool_calls"] = json::array();
										}
										
										// Merge tool call fragments
										for (const auto& toolCall : delta["tool_calls"]) {
											if (toolCall.contains("index")) {
												int index = toolCall["index"].get<int>();
												while (fullChoice["message"]["tool_calls"].size() <= index) {
													fullChoice["message"]["tool_calls"].push_back(json::object());
												}
												
												auto& fullToolCall = fullChoice["message"]["tool_calls"][index];
												
												// Merge tool call fields
												if (toolCall.contains("id")) {
													fullToolCall["id"] = toolCall["id"];
												}
												if (toolCall.contains("type")) {
													fullToolCall["type"] = toolCall["type"];
												}
												if (toolCall.contains("function")) {
													if (!fullToolCall.contains("function")) {
														fullToolCall["function"] = json::object();
														fullToolCall["function"]["arguments"] = "";
													}
													
													const auto& func = toolCall["function"];
													if (func.contains("name")) {
														fullToolCall["function"]["name"] = func["name"];
													}
													if (func.contains("arguments")) {
														fullToolCall["function"]["arguments"] = fullToolCall["function"]["arguments"].get<std::string>() + func["arguments"].get<std::string>();
													}
												}
											}
										}
									}
								}
								
								// Copy finish_reason
								if (choice.contains("finish_reason")) {
									fullChoice["finish_reason"] = choice["finish_reason"];
								}
							}
						}
						
						// Update usage
						if (result.contains("usage")) {
							g_fullResponse["usage"] = result["usage"];
						}
					}
					
					// Process content tokens and tool calls for streaming (same as original function)
					if (result.contains("choices") && result["choices"].is_array() && !result["choices"].empty()) {
						const auto& choice = result["choices"][0];
						if (choice.contains("delta") && choice["delta"].contains("content") && !choice["delta"]["content"].is_null()) {
							std::string content = choice["delta"]["content"].get<std::string>();
							if (!content.empty()) {
								std::lock_guard<std::mutex> lock(g_callbackMutex);
								if (g_currentTokenCallback) {
									g_currentTokenCallback(content);
								}
							}
						}
						
						// Handle tool calls (same as original function)
						if (choice.contains("delta") && choice["delta"].contains("tool_calls")) {
							std::lock_guard<std::mutex> lock(g_toolCallMutex);
							
							const auto& toolCalls = choice["delta"]["tool_calls"];
							for (const auto& toolCall : toolCalls) {
								std::string toolCallId;
								if (toolCall.contains("id")) {
									toolCallId = toolCall["id"].get<std::string>();
								} else if (toolCall.contains("index")) {
									toolCallId = "index_" + std::to_string(toolCall["index"].get<int>());
								} else {
									continue;
								}
								
								if (g_accumulatedToolCalls.find(toolCallId) == g_accumulatedToolCalls.end()) {
									g_accumulatedToolCalls[toolCallId] = json::object();
									g_accumulatedToolCalls[toolCallId]["id"] = toolCallId;
									g_accumulatedToolCalls[toolCallId]["type"] = "function";
									g_accumulatedToolCalls[toolCallId]["function"] = json::object();
									g_accumulatedToolCalls[toolCallId]["function"]["arguments"] = "";
								}
								
								if (toolCall.contains("function")) {
									const auto& func = toolCall["function"];
									if (func.contains("name")) {
										g_accumulatedToolCalls[toolCallId]["function"]["name"] = func["name"];
									}
									if (func.contains("arguments")) {
										std::string currentArgs = g_accumulatedToolCalls[toolCallId]["function"]["arguments"].get<std::string>();
										std::string newArgs = func["arguments"].get<std::string>();
										g_accumulatedToolCalls[toolCallId]["function"]["arguments"] = currentArgs + newArgs;
									}
								}
								
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
							std::cout << "DEBUG: Found finish_reason in streaming: " << finishReason << std::endl;
							if (finishReason == "tool_calls") {
								std::lock_guard<std::mutex> lock(g_callbackMutex);
								if (g_currentTokenCallback) {
									std::lock_guard<std::mutex> toolLock(g_toolCallMutex);
									for (const auto& [id, toolCall] : g_accumulatedToolCalls) {
										std::string toolCallJson = toolCall.dump();
										g_currentTokenCallback(toolCallJson);
									}
									g_accumulatedToolCalls.clear();
								}
							}
							
							// Call the response callback when we have a finish_reason
							if (!finishReason.empty() && finishReason != "null") {
								std::lock_guard<std::mutex> responseLock(g_responseMutex);
								if (g_responseCallback && !g_fullResponse.is_null()) {
									g_responseCallback(g_fullResponse);
								} else {
								}
							}
						} else {
							std::cout << "DEBUG: No finish_reason found in choice" << std::endl;
						}
					} else {
						std::cout << "DEBUG: No choices found or choices array is empty" << std::endl;
					}
				} catch (const json::exception &e) {
				}
			}
		}
		return size * nmemb;
	} catch (const std::exception& e) {
		return 0;
	} catch (...) {
		return 0;
	}
}

bool OpenRouter::promptRequestStream(const std::string &prompt, const std::string &api_key, 
									std::function<void(const std::string&)> tokenCallback,
									std::atomic<bool>* cancelFlag)
{
	try {
		if (cancelFlag && cancelFlag->load()) {
			return false; 
		}

		{
			std::lock_guard<std::mutex> toolLock(g_toolCallMutex);
			std::lock_guard<std::mutex> indexLock(g_indexMappingMutex);
			g_accumulatedToolCalls.clear();
			g_indexToIdMapping.clear();
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
		curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L); // Reduced timeout for faster cancellation
		curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 5L); // 5 second connect timeout
		curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L); // Don't use signals
		
		// Add progress callback to check cancellation more frequently
		curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);
		curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, [](void* clientp, curl_off_t dltotal, curl_off_t dlnow, curl_off_t ultotal, curl_off_t ulnow) -> int {
			if (g_should_cancel) {
				return 1; // Return non-zero to abort the transfer
			}
			return 0; // Continue the transfer
		});
		
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
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, nullptr);
		curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L); // Reduced timeout for faster cancellation
		curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 5L);
		curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);
		
		// Add progress callback to check cancellation more frequently
		curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);
		curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, [](void* clientp, curl_off_t dltotal, curl_off_t dlnow, curl_off_t ultotal, curl_off_t ulnow) -> int {
			if (g_should_cancel) {
				return 1; // Return non-zero to abort the transfer
			}
			return 0; // Continue the transfer
		});
		
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

bool OpenRouter::jsonPayloadStreamWithResponse(const std::string &jsonPayload, const std::string &api_key,
											   std::function<void(const std::string&)> tokenCallback,
											   std::function<void(const json&)> responseCallback,
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
		}

		// Clear previous response state
		{
			std::lock_guard<std::mutex> responseLock(g_responseMutex);
			g_fullResponse = json::value_t::null;
			g_responseCallback = responseCallback;
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
			{
				std::lock_guard<std::mutex> responseLock(g_responseMutex);
				g_responseCallback = nullptr;
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
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteDataStreamWithResponse);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, nullptr);
		curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L); // Reduced timeout for faster cancellation
		curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 5L);
		curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);
		
		// Add progress callback to check cancellation more frequently
		curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);
		curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, [](void* clientp, curl_off_t dltotal, curl_off_t dlnow, curl_off_t ultotal, curl_off_t ulnow) -> int {
			if (g_should_cancel) {
				return 1; // Return non-zero to abort the transfer
			}
			return 0; // Continue the transfer
		});
		
		CURLcode res = curl_easy_perform(curl);
		if (res != CURLE_OK) {
			curl_easy_cleanup(curl);
			curl_slist_free_all(headers);
			// Clear the global callbacks with mutex protection
			{
				std::lock_guard<std::mutex> lock(g_callbackMutex);
				g_currentTokenCallback = nullptr;
			}
			{
				std::lock_guard<std::mutex> responseLock(g_responseMutex);
				g_responseCallback = nullptr;
			}
			return false;
		}
		
		curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);

		// Cleanup
		curl_easy_cleanup(curl);
		curl_slist_free_all(headers);
		
		// Clear the global callbacks with mutex protection
		{
			std::lock_guard<std::mutex> lock(g_callbackMutex);
			g_currentTokenCallback = nullptr;
		}
		{
			std::lock_guard<std::mutex> responseLock(g_responseMutex);
			g_responseCallback = nullptr;
		}

		return http_code == 200;
	} catch (const std::exception& e) {
		return false;
	} catch (...) {
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