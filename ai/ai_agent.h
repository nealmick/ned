#pragma once
#include <imgui.h>
#include <vector>
#include <string>

class AIAgent {
public:
    struct Message {
        std::string text;
        bool isAgent;
    };

    void render(float agentPaneWidth);
    void sendMessage(const char* msg);
    void AgentInput(const ImVec2& textBoxSize, float textBoxWidth, float horizontalPadding);
    void printAllMessages() const;

private:
    std::vector<Message> messages;
    char inputBuffer[256] = {0};
    void renderMessageHistory(const ImVec2& size);
    bool scrollToBottom = false;
};
