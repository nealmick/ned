#include "agent_request.h"
#include <utf8.h>
#include "ai_open_router.h"
#include <iostream>
#include "mcp/mcp_manager.h"
#include "ai_agent.h"
#include "../lib/json.hpp"
#include <curl/curl.h>
#include <thread>
#include <chrono>

using json = nlohmann::json;

// External global MCP manager instance
extern MCP::Manager gMCPManager;

// External global AI agent instance
extern AIAgent gAIAgent;

AgentRequest::AgentRequest() {}
AgentRequest::~AgentRequest() { stopRequest(); }

void AgentRequest::stopRequest() {
    // std::cout << "DEBUG: stopRequest called" << std::endl;
    
    if (isStreaming.load()) {
        // std::cout << "DEBUG: Setting shouldCancelStreaming to true" << std::endl;
        shouldCancelStreaming.store(true);
        isStreaming.store(false);
        
        // Also set the global cancel flag for immediate CURL cancellation
        extern std::atomic<bool> g_should_cancel;
        g_should_cancel = true;
    }
    
    if (streamingThread.joinable()) {
        // std::cout << "DEBUG: Detaching streaming thread to avoid blocking" << std::endl;
        try { 
            streamingThread.detach(); // Use detach instead of join to avoid blocking
            // std::cout << "DEBUG: Streaming thread detached successfully" << std::endl;
        } catch (...) {
            // std::cout << "DEBUG: Exception during thread detach" << std::endl;
        }
    }
    
    // Don't reset the cancellation flags immediately - let the detached thread handle them
    // The flags will be reset when a new request starts
    // std::cout << "DEBUG: stopRequest completed" << std::endl;
}

void AgentRequest::sendMessage(const std::string& payload, const std::string& api_key,
                               StreamingCallback onStreamingToken,
                               CompleteCallback onComplete) {
    
    std::cout << "-----------------------------------" << std::endl;
    try {
        json payloadJson = json::parse(payload);
        std::cout << payloadJson.dump(4) << std::endl;
    } catch (const json::parse_error& e) {
        std::cout << "Error parsing JSON: " << e.what() << std::endl;
        std::cout << "Raw payload: " << payload << std::endl;
    }
    std::cout << "-----------------------------------" << std::endl;
    
    stopRequest();
    if (isStreaming.load()) return;
    isStreaming.store(true);
    shouldCancelStreaming.store(false);
    
    // Reset global cancel flag for new request
    extern std::atomic<bool> g_should_cancel;
    g_should_cancel = false;

    // Track the full response using shared pointer
    auto fullResponse = std::make_shared<std::string>();
    auto toolCallMarkers = std::make_shared<std::vector<std::string>>();

    streamingThread = std::thread([this, payload, api_key, onStreamingToken, onComplete, fullResponse, toolCallMarkers]() {
        try {
            // Parse the JSON payload
            json payloadJson;
            try {
                payloadJson = json::parse(payload);
            } catch (const json::parse_error& e) {
                std::cerr << "Error parsing JSON payload: " << e.what() << std::endl;
                isStreaming.store(false);
                if (onComplete) onComplete("Error: Invalid JSON payload", false);
                return;
            }
            

            // Track the full JSON response
            auto fullJsonResponse = std::make_shared<json>();
            
            // Use the new JSON payload streaming function with response callback
            bool streamSuccess = OpenRouter::jsonPayloadStreamWithResponse(payloadJson.dump(), api_key, 
                [this, onStreamingToken, fullResponse](const std::string& token) {
                    *fullResponse += token;
                    
                    // Filter out tool call JSON tokens to prevent them from appearing in the agent's text
                    // Tool call JSON tokens typically start with "{" and contain "function" or "tool_calls"
                    if (token.find("{") == 0 && (token.find("function") != std::string::npos || token.find("tool_calls") != std::string::npos)) {
                        // This is likely a tool call JSON token, skip it
                        return;
                    }
                    
                    if (onStreamingToken) onStreamingToken(token);
                },
                [fullJsonResponse](const json& response) {
                    *fullJsonResponse = response;
                },
                &shouldCancelStreaming);
            
            if (!streamSuccess) {
                // std::cerr << "DEBUG: Streaming failed" << std::endl;
                isStreaming.store(false);
                if (onComplete) onComplete("Error: Streaming failed", false);
                return;
            }
            
            isStreaming.store(false);
            
            // Print the full JSON response object
            std::cout << "=== FULL JSON RESPONSE OBJECT ===" << std::endl;
            if (!fullJsonResponse->is_null()) {
                std::cout << fullJsonResponse->dump(4) << std::endl;
                
                // Check if there are tool calls in the response
                if (fullJsonResponse->contains("choices") && 
                    (*fullJsonResponse)["choices"].is_array() && 
                    !(*fullJsonResponse)["choices"].empty()) {
                    
                    const auto& choice = (*fullJsonResponse)["choices"][0];
                    if (choice.contains("message") && 
                        choice["message"].contains("tool_calls") && 
                        choice["message"]["tool_calls"].is_array()) {
                        
                        const auto& toolCalls = choice["message"]["tool_calls"];
                        std::cout << "=== PROCESSING TOOL CALLS ===" << std::endl;
                        
                        for (const auto& toolCall : toolCalls) {
                            if (toolCall.contains("function") && 
                                toolCall["function"].contains("name") && 
                                toolCall["function"].contains("arguments")) {
                                
                                std::string toolName = toolCall["function"]["name"];
                                std::string argumentsStr = toolCall["function"]["arguments"];
                                
                                std::cout << "Tool call: " << toolName << std::endl;
                                std::cout << "Arguments: " << argumentsStr << std::endl;
                                
                                // Parse the arguments JSON
                                try {
                                    json argumentsJson = json::parse(argumentsStr);
                                    std::unordered_map<std::string, std::string> parameters;
                                    
                                    for (auto it = argumentsJson.begin(); it != argumentsJson.end(); ++it) {
                                        parameters[it.key()] = it.value().get<std::string>();
                                    }
                                    
                                    // Execute the tool call
                                    std::string result = gMCPManager.executeToolCall(toolName, parameters);
                                    
                                    std::cout << "=== TOOL CALL RESULT ===" << std::endl;
                                    std::cout << result << std::endl;
                                    std::cout << "=== END TOOL CALL RESULT ===" << std::endl;
                                    
                                } catch (const json::parse_error& e) {
                                    std::cerr << "Error parsing tool call arguments: " << e.what() << std::endl;
                                }
                            }
                        }
                        std::cout << "=== END PROCESSING TOOL CALLS ===" << std::endl;
                    }
                }
            } else {
                std::cout << "No JSON response object captured" << std::endl;
            }
            std::cout << "=== END JSON RESPONSE ===" << std::endl;
            
            return;
            
            
        } catch (const std::exception& e) {
            std::cerr << "Exception in streaming thread: " << e.what() << std::endl;
            isStreaming.store(false);
            if (onComplete) onComplete("Error: Exception occurred during streaming", false);
        } catch (...) {
            std::cerr << "Unknown exception in streaming thread" << std::endl;
            isStreaming.store(false);
            if (onComplete) onComplete("Error: Unknown error occurred during streaming", false);
        }
    });
}
