#include "agent_request.h"
#include <utf8.h>
#include "ai_open_router.h"
#include <iostream>
#include "mcp/mcp_manager.h"
#include "ai_agent.h"

// External global MCP manager instance
extern MCP::Manager gMCPManager;

// External global AI agent instance
extern AIAgent gAIAgent;

AgentRequest::AgentRequest() {}
AgentRequest::~AgentRequest() { stopRequest(); }

void AgentRequest::stopRequest() {
    if (isStreaming.load()) {
        shouldCancelStreaming.store(true);
        isStreaming.store(false);
    }
    if (streamingThread.joinable()) {
        try { streamingThread.join(); } catch (...) {}
    }
    shouldCancelStreaming.store(false);
}

void AgentRequest::sendMessage(const std::string& prompt, const std::string& api_key,
                               StreamingCallback onStreamingToken,
                               CompleteCallback onComplete) {
    stopRequest();
    if (isStreaming.load()) return;
    isStreaming.store(true);
    shouldCancelStreaming.store(false);

    // Track the full response using shared pointer
    auto fullResponse = std::make_shared<std::string>();

    streamingThread = std::thread([this, prompt, api_key, onStreamingToken, onComplete, fullResponse]() {
        try {
            OpenRouter::promptRequestStream(prompt, api_key, [this, onStreamingToken, fullResponse](const std::string& token) {
                std::string combined = utf8_buffer + token;
                auto valid_end = combined.begin();
                try { valid_end = utf8::find_invalid(combined.begin(), combined.end()); }
                catch (...) { valid_end = combined.begin(); }
                std::string validToken = std::string(combined.begin(), valid_end);
                *fullResponse += validToken;
                if (onStreamingToken) onStreamingToken(validToken);
                utf8_buffer = std::string(valid_end, combined.end());
            }, &shouldCancelStreaming);
        } catch (...) {}
        isStreaming.store(false);
        std::cout << "fgot final repsoine here for ya boss:" << std::endl;
        std::cout << *fullResponse << std::endl;
        
        // Parse for tool calls
        std::string finalResult = *fullResponse;
        std::cout << "checking for tool calls" << std::endl;
        std::cout << "Response content: " << *fullResponse << std::endl;
        bool hadToolCall = gMCPManager.hasToolCalls(*fullResponse);
        std::cout << "hasToolCalls returned: " << (hadToolCall ? "true" : "false") << std::endl;
        if (hadToolCall) {
            std::cout << "Tool call detected in response!" << std::endl;
            finalResult = gMCPManager.parseAndExecuteToolCalls(*fullResponse);
            std::cout << "Tool call result: " << finalResult << std::endl;
        }
        
        if (onComplete) onComplete(finalResult, hadToolCall);
    });
}
