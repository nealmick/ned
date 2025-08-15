#include "agent_request.h"
#include "../lib/json.hpp"
#include "ai_agent.h"
#include "ai_open_router.h"
#include "mcp/mcp_manager.h"
#include <chrono>
#include <curl/curl.h>
#include <iostream>
#include <mutex>
#include <thread>
#ifdef _WIN32
// Fix for Windows UTF-8 library assert macro conflict
#include <cassert>
#ifdef assert
#undef assert
#endif
#include <utf8.h>
#ifdef _WIN32
#define assert(expr) ((void)0)
#endif
#else
#include <utf8.h>
#endif

using json = nlohmann::json;

// External global MCP manager instance
extern MCP::Manager gMCPManager;

// External global AI agent instance
extern AIAgent gAIAgent;

AgentRequest::AgentRequest() {}
AgentRequest::~AgentRequest() { stopRequest(); }

void AgentRequest::stopRequest()
{
	// std::cout << "DEBUG: stopRequest called" << std::endl;

	if (isStreaming.load())
	{
		// std::cout << "DEBUG: Setting shouldCancelStreaming to true" << std::endl;
		shouldCancelStreaming.store(true);
		isStreaming.store(false);

		// Also set the global cancel flag for immediate CURL cancellation
		extern std::atomic<bool> g_should_cancel;
		g_should_cancel = true;
	}

	if (streamingThread.joinable())
	{
		// std::cout << "DEBUG: Detaching streaming thread to avoid blocking" <<
		// std::endl;
		try
		{
			streamingThread.detach(); // Use detach instead of join to avoid blocking
			// std::cout << "DEBUG: Streaming thread detached successfully" <<
			// std::endl;
		} catch (...)
		{
			// std::cout << "DEBUG: Exception during thread detach" << std::endl;
		}
	}

	// Don't reset the cancellation flags immediately - let the detached thread
	// handle them The flags will be reset when a new request starts std::cout
	// << "DEBUG: stopRequest completed" << std::endl;
}

void AgentRequest::sendMessage(const std::string &payload,
							   const std::string &api_key,
							   StreamingCallback onStreamingToken,
							   CompleteCallback onComplete)
{

	std::cout << "=== AGENT REQUEST: STARTING NEW REQUEST ===" << std::endl;
	std::cout << "API Key length: " << api_key.length()
			  << " (first 10 chars: " << api_key.substr(0, 10) << "...)" << std::endl;
	std::cout << "Payload length: " << payload.length() << " bytes" << std::endl;

	std::cout << "-----------------------------------" << std::endl;
	try
	{
		json payloadJson = json::parse(payload);
		std::cout << payloadJson.dump(4) << std::endl;
	} catch (const json::parse_error &e)
	{
		std::cout << "Error parsing JSON: " << e.what() << std::endl;
		std::cout << "Raw payload: " << payload << std::endl;
	}
	std::cout << "-----------------------------------" << std::endl;

	stopRequest();
	if (isStreaming.load())
	{
		std::cout << "ERROR: Request already in progress, aborting" << std::endl;
		return;
	}

	std::cout << "Setting up streaming state..." << std::endl;
	isStreaming.store(true);
	shouldCancelStreaming.store(false);

	// Reset global cancel flag for new request
	extern std::atomic<bool> g_should_cancel;
	g_should_cancel = false;

	// Track the full response using shared pointer
	auto fullResponse = std::make_shared<std::string>();
	auto toolCallMarkers = std::make_shared<std::vector<std::string>>();

	streamingThread = std::thread([this,
								   payload,
								   api_key,
								   onStreamingToken,
								   onComplete,
								   fullResponse,
								   toolCallMarkers]() {
		std::cout << "=== STREAMING THREAD: STARTED ===" << std::endl;
		try
		{
			// Parse the JSON payload
			json payloadJson;
			try
			{
				payloadJson = json::parse(payload);
				std::cout << "JSON payload parsed successfully" << std::endl;
			} catch (const json::parse_error &e)
			{
				std::cerr << "ERROR: Failed to parse JSON payload: " << e.what()
						  << std::endl;
				std::cerr << "Raw payload: " << payload << std::endl;
				isStreaming.store(false);
				if (onComplete)
					onComplete("Error: Invalid JSON payload", false);
				return;
			}

			// Track the full JSON response
			auto fullJsonResponse = std::make_shared<json>();

			std::cout << "Calling OpenRouter::jsonPayloadStreamWithResponse..."
					  << std::endl;
			// Use the new JSON payload streaming function with response callback
			auto streamResult = OpenRouter::jsonPayloadStreamWithResponse(
				payloadJson.dump(),
				api_key,
				[this, onStreamingToken, fullResponse](const std::string &token) {
					*fullResponse += token;

					// Filter out tool call JSON tokens to prevent them from
					// appearing in the agent's text Tool call JSON tokens typically
					// start with "{" and contain "function" or "tool_calls"
					if (token.find("{") == 0 &&
						(token.find("function") != std::string::npos ||
						 token.find("tool_calls") != std::string::npos))
					{
						// This is likely a tool call JSON token, skip it
						return;
					}

					if (onStreamingToken)
						onStreamingToken(token);
				},
				[fullJsonResponse](const json &response) { *fullJsonResponse = response; },
				&shouldCancelStreaming);

			bool streamSuccess = streamResult.first;
			std::string errorDetails = streamResult.second;

			if (!streamSuccess)
			{
				isStreaming.store(false);

				// Clean up any empty streaming messages that failed
				{
					std::lock_guard<std::mutex> lock(gAIAgent.messagesMutex);
					if (!gAIAgent.messages.empty() &&
						gAIAgent.messages.back().role == "assistant" &&
						gAIAgent.messages.back().isStreaming &&
						gAIAgent.messages.back().text.empty())
					{
						gAIAgent.messages.pop_back();
						gAIAgent.messageDisplayLinesDirty = true;
						std::cout << "Removed empty streaming message due to failure"
								  << std::endl;
					}
				}

				if (onComplete)
					onComplete("Error: " + errorDetails, false); // Pass detailed error
				return;
			}

			std::cout << "Streaming completed successfully, processing response..."
					  << std::endl;
			isStreaming.store(false);

			// Print the full JSON response object
			std::cout << "=== FULL JSON RESPONSE OBJECT ===" << std::endl;
			if (!fullJsonResponse->is_null())
			{
				std::cout << fullJsonResponse->dump(4) << std::endl;

				// Check if there are tool calls in the response
				if (fullJsonResponse->contains("choices") &&
					(*fullJsonResponse)["choices"].is_array() &&
					!(*fullJsonResponse)["choices"].empty())
				{

					const auto &choices = (*fullJsonResponse)["choices"];
					const auto &choice = choices[0];

					if (choice.contains("message") &&
						choice["message"].contains("tool_calls"))
					{
						std::cout << "=== PROCESSING TOOL CALLS ===" << std::endl;

						// Update the assistant message with tool calls
						{
							std::lock_guard<std::mutex> lock(gAIAgent.messagesMutex);
							if (!gAIAgent.messages.empty())
							{
								Message &lastMsg = gAIAgent.messages.back();
								if (lastMsg.role == "assistant" && lastMsg.isStreaming)
								{
									lastMsg.tool_calls = choice["message"]["tool_calls"];
									lastMsg.isStreaming = false;
									// Handle null content properly - don't set
									// empty string for null content
									if (choice["message"].contains("content") &&
										!choice["message"]["content"].is_null())
									{
										lastMsg.text = choice["message"]["content"]
														   .get<std::string>();
									} else
									{
										// Keep the text as is, don't set to
										// empty string for null content This
										// will be handled properly when
										// building the payload
									}
									gAIAgent.messageDisplayLinesDirty = true;
									std::cout << "=== DEBUG: Updated assistant "
												 "message with tool "
												 "calls ==="
											  << std::endl;
									std::cout << "Tool calls count: "
											  << lastMsg.tool_calls.size() << std::endl;
									std::cout << "=== END DEBUG ===" << std::endl;
								}
							}
						}

						// Process each tool call
						const auto &toolCalls = choice["message"]["tool_calls"];
						for (const auto &toolCall : toolCalls)
						{
							// Log the tool call for debugging DeepSeek issues
							std::cout << "Raw tool call JSON: " << toolCall.dump()
									  << std::endl;

							if (toolCall.contains("function") &&
								toolCall["function"].contains("name") &&
								!toolCall["function"]["name"].is_null())
							{
								std::string toolName =
									toolCall["function"]["name"].get<std::string>();
								std::string argumentsStr;

								if (!toolCall["function"].contains("arguments") ||
									toolCall["function"]["arguments"].is_null())
								{
									argumentsStr = "{}";
									std::cout << "No arguments or arguments is null, "
												 "using empty object"
											  << std::endl;
								} else
								{
									argumentsStr = toolCall["function"]["arguments"]
													   .get<std::string>();
								}

								std::cout << "Executing tool call: " << toolName
										  << std::endl;
								std::cout << "Arguments: " << argumentsStr << std::endl;

								std::string result;
								bool toolCallSucceeded = false;

								try
								{
									// Simple validation - just check if arguments is not
									// empty Let JSON parser handle validation instead of
									// custom checks
									json argumentsJson = json::parse(argumentsStr);
									std::unordered_map<std::string, std::string> parameters;

									for (auto it = argumentsJson.begin();
										 it != argumentsJson.end();
										 ++it)
									{
										if (it.value().is_null())
										{
											parameters[it.key()] = "";
										} else
										{
											parameters[it.key()] =
												it.value().get<std::string>();
										}
									}

									// Execute the tool call
									result =
										gMCPManager.executeToolCall(toolName, parameters);
									toolCallSucceeded = true;

									std::cout << "=== TOOL CALL RESULT ===" << std::endl;
									std::cout << result << std::endl;
									std::cout << "=== END TOOL CALL RESULT ==="
											  << std::endl;

								} catch (const json::parse_error &e)
								{
									std::cerr << "Error parsing tool call arguments: "
											  << e.what() << std::endl;
									result = "ERROR: Failed to parse tool call "
											 "arguments: " +
											 std::string(e.what());
									toolCallSucceeded = false;
								} catch (const std::exception &e)
								{
									std::cerr << "Error executing tool call: " << e.what()
											  << std::endl;
									result = "ERROR: Tool execution failed: " +
											 std::string(e.what());
									toolCallSucceeded = false;
								}

								// Always add a tool result message, even if the
								// tool call failed
								{
									std::lock_guard<std::mutex> lock(
										gAIAgent.messagesMutex);
									Message toolMsg;
									toolMsg.text = result;
									toolMsg.role = "tool";
									toolMsg.isStreaming = false;
									toolMsg.hide_message = false;
									toolMsg.timestamp = std::chrono::system_clock::now();

									// Set the tool_call_id to match the tool
									// call ID
									if (toolCall.contains("id") &&
										!toolCall["id"].is_null())
									{
										toolMsg.tool_call_id =
											toolCall["id"].get<std::string>();
									}

									gAIAgent.messages.push_back(toolMsg);
									gAIAgent.messageDisplayLinesDirty = true;

									std::cout << "=== DEBUG: Added tool "
												 "message to agent ==="
											  << std::endl;
									std::cout
										<< "Message count: " << gAIAgent.messages.size()
										<< std::endl;
									std::cout << "Display dirty flag: "
											  << gAIAgent.messageDisplayLinesDirty.load()
											  << std::endl;
									std::cout << "Tool message text: "
											  << toolMsg.text.substr(0, 100) << "..."
											  << std::endl;
									std::cout << "Tool call ID: " << toolMsg.tool_call_id
											  << std::endl;
									std::cout << "Tool call succeeded: "
											  << (toolCallSucceeded ? "YES" : "NO")
											  << std::endl;
									std::cout << "=== END DEBUG ===" << std::endl;

									// If the tool call failed due to malformed
									// JSON, log it
									if (!toolCallSucceeded &&
										(result.find("ERROR: Incomplete tool "
													 "call arguments") !=
											 std::string::npos ||
										 result.find("ERROR: Failed to parse "
													 "tool call arguments") !=
											 std::string::npos ||
										 result.find("ERROR: Malformed tool "
													 "call arguments") !=
											 std::string::npos))
									{
										std::cout << "Tool call failed for " << toolName
												  << " due to malformed JSON"
												  << std::endl;
									}
								}
							}
						}
						std::cout << "=== END PROCESSING TOOL CALLS ===" << std::endl;

						// Set flag to trigger follow-up message after tool calls
						gAIAgent.needsFollowUpMessage = true;

						// Call the completion callback to indicate tool calls
						// were processed
						if (onComplete)
						{
							onComplete("",
									   true); // Empty result, but had tool call
						}

						return;
					}
				}
			} else
			{
				std::cout << "No JSON response object captured" << std::endl;
			}
			std::cout << "=== END JSON RESPONSE ===" << std::endl;

			// If we get here, there were no tool calls, so call the completion
			// callback
			if (onComplete)
			{
				onComplete(*fullResponse, false); // Full response, no tool call
			}

			return;

		} catch (const std::exception &e)
		{
			std::cerr << "EXCEPTION in streaming thread: " << e.what() << std::endl;
			std::cerr << "Exception type: " << typeid(e).name() << std::endl;
			isStreaming.store(false);

			// Clean up any empty streaming messages that failed
			{
				std::lock_guard<std::mutex> lock(gAIAgent.messagesMutex);
				if (!gAIAgent.messages.empty() &&
					gAIAgent.messages.back().role == "assistant" &&
					gAIAgent.messages.back().isStreaming &&
					gAIAgent.messages.back().text.empty())
				{
					gAIAgent.messages.pop_back();
					gAIAgent.messageDisplayLinesDirty = true;
					std::cout << "Removed empty streaming message due to exception"
							  << std::endl;
				}
			}

			if (onComplete)
				onComplete("Error: Exception occurred during streaming", false);
		} catch (...)
		{
			std::cerr << "UNKNOWN EXCEPTION in streaming thread" << std::endl;
			isStreaming.store(false);

			// Clean up any empty streaming messages that failed
			{
				std::lock_guard<std::mutex> lock(gAIAgent.messagesMutex);
				if (!gAIAgent.messages.empty() &&
					gAIAgent.messages.back().role == "assistant" &&
					gAIAgent.messages.back().isStreaming &&
					gAIAgent.messages.back().text.empty())
				{
					gAIAgent.messages.pop_back();
					gAIAgent.messageDisplayLinesDirty = true;
					std::cout << "Removed empty streaming message due to "
								 "unknown exception"
							  << std::endl;
				}
			}

			if (onComplete)
				onComplete("Error: Unknown error occurred during streaming", false);
		}
	});

	std::cout << "=== AGENT REQUEST: THREAD STARTED ===" << std::endl;
}
