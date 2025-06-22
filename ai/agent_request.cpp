#include "agent_request.h"
#include <utf8.h>
#include "ai_open_router.h"
#include <iostream>

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
        if (onComplete) onComplete();
    });
}
