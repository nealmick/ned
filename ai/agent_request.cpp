#include "agent_request.h"
#include <utf8.h>
#include "ai_open_router.h"
#include <iostream>
#include "mcp/mcp_manager.h"
#include "ai_agent.h"
#include "../lib/json.hpp"
#include <curl/curl.h>

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
            
            // Check if this is a modern API call with tools
            bool hasTools = payloadJson.contains("tools") && !payloadJson["tools"].empty();
            
            if (hasTools) {
                // Modern tool calling approach
                // std::cout << "DEBUG: Using modern tool calling API" << std::endl;
                
                // Use the new JSON payload streaming function
                bool streamSuccess = OpenRouter::jsonPayloadStream(payloadJson.dump(), api_key, [this, onStreamingToken, fullResponse](const std::string& token) {
                    *fullResponse += token;
                    if (onStreamingToken) onStreamingToken(token);
                }, &shouldCancelStreaming);
                
                if (!streamSuccess) {
                    // std::cerr << "DEBUG: Streaming failed" << std::endl;
                    isStreaming.store(false);
                    if (onComplete) onComplete("Error: Streaming failed", false);
                    return;
                }
                
                isStreaming.store(false);
                // std::cout << "DEBUG: Modern API streaming completed" << std::endl;
                // std::cout << "DEBUG: Full response: " << *fullResponse << std::endl;
                
                // After streaming is done, ensure all tool call markers are in fullResponse
                for (const auto& marker : *toolCallMarkers) {
                    if (fullResponse->find(marker) == std::string::npos) {
                        *fullResponse += marker;
                        // std::cout << "DEBUG: Appended missing TOOL_CALL marker after streaming: " << marker << std::endl;
                    }
                }
                
                // Parse for tool calls in the streamed response
                std::string finalResult = *fullResponse;
                bool hadToolCall = false;
                
                // std::cout << "DEBUG: About to parse tool calls in fullResponse: [" << *fullResponse << "]" << std::endl;
                
                // Check if we have tool call markers in the response
                if (fullResponse->find("TOOL_CALL:") != std::string::npos) {
                    // std::cout << "DEBUG: Tool call markers found in response" << std::endl;
                    hadToolCall = true;
                    finalResult = gMCPManager.processToolCalls(*fullResponse);
                    // std::cout << "DEBUG: Tool call result: " << finalResult << std::endl;
                } else {
                    // Fallback to legacy tool call detection
                    hadToolCall = gMCPManager.hasToolCalls(*fullResponse);
                    if (hadToolCall) {
                        // std::cout << "DEBUG: Tool call detected in streamed response" << std::endl;
                        finalResult = gMCPManager.processToolCalls(*fullResponse);
                        // std::cout << "DEBUG: Tool call result: " << finalResult << std::endl;
                    }
                }
                
                if (onComplete) onComplete(finalResult, hadToolCall);
                return;
                
            } else {
                // Fallback to legacy format for backward compatibility
                // std::cout << "DEBUG: Using legacy prompt format" << std::endl;
                
                // Convert the modern format back to the old format for compatibility
                std::string legacyPrompt = "";
                
                if (payloadJson.contains("messages") && payloadJson["messages"].is_array()) {
                    for (const auto& msg : payloadJson["messages"]) {
                        std::string role = msg.value("role", "user");
                        std::string content = msg.value("content", "");
                        
                        if (role == "system") {
                            legacyPrompt += "SYSTEM: " + content + "\n\n";
                        } else if (role == "user") {
                            legacyPrompt += "#User: " + content + "\n";
                        } else if (role == "assistant") {
                            legacyPrompt += "#Assistant: " + content + "\n";
                        } else if (role == "tool") {
                            legacyPrompt += "TOOL_RESULT: " + content + "\n";
                        }
                    }
                }
                
                // std::cout << "DEBUG: Converted to legacy prompt format:" << std::endl;
                // std::cout << legacyPrompt << std::endl;
                
                OpenRouter::promptRequestStream(legacyPrompt, api_key, [this, onStreamingToken, fullResponse](const std::string& token) {
                    std::string combined = utf8_buffer + token;
                    auto valid_end = combined.begin();
                    try { valid_end = utf8::find_invalid(combined.begin(), combined.end()); }
                    catch (...) { valid_end = combined.begin(); }
                    std::string validToken = std::string(combined.begin(), valid_end);
                    *fullResponse += validToken;
                    if (onStreamingToken) onStreamingToken(validToken);
                    utf8_buffer = std::string(valid_end, combined.end());
                }, &shouldCancelStreaming);
                
                isStreaming.store(false);
                // std::cout << "fgot final repsoine here for ya boss:" << std::endl;
                // std::cout << *fullResponse << std::endl;
                
                // Parse for tool calls (legacy method)
                std::string finalResult = *fullResponse;
                // std::cout << "checking for tool calls" << std::endl;
                // std::cout << "Response content: " << *fullResponse << std::endl;
                bool hadToolCall = gMCPManager.hasToolCalls(*fullResponse);
                // std::cout << "hasToolCalls returned: " << (hadToolCall ? "true" : "false") << std::endl;
                if (hadToolCall) {
                    // std::cout << "Tool call detected in response!" << std::endl;
                    finalResult = gMCPManager.processToolCalls(*fullResponse);
                    // std::cout << "Tool call result: " << finalResult << std::endl;
                }
                
                if (onComplete) onComplete(finalResult, hadToolCall);
            }
            
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
