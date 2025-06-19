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
    void sendMessage(const std::string& msg);

private:
    std::vector<Message> messages;
    std::string inputBuffer;
};
