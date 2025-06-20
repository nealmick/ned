#pragma once
#include <imgui.h>
#include <vector>
#include <string>
#include "native_text_input.h"

class AIAgent {
public:
    struct Message {
        std::string text;
        bool isAgent;
    };

    AIAgent();
    ~AIAgent();
    
    void render(float agentPaneWidth);
    void sendMessage(const char* msg);
    void AgentInput(const ImVec2& textBoxSize, float textBoxWidth, float horizontalPadding);
    void printAllMessages() const;

    // Expose nativeInput for focus checking
    NativeTextInput* getNativeInput() const { return nativeInput; }

private:
    std::vector<Message> messages;
    char inputBuffer[256] = {0};
    void renderMessageHistory(const ImVec2& size);
    bool scrollToBottom = false;
    
    // Native text input widget
    NativeTextInput* nativeInput;
    std::string currentInputText;
    
    // Callback handlers
    void onTextChanged(const std::string& text);
    void onEnterPressed();
};
