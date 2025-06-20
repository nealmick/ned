#include "ai_agent.h"
#include <imgui.h>
#include <iostream>
#include <cstring>
#include "editor/editor.h" // for editor_state
#include "util/settings.h"

AIAgent::AIAgent() {
    nativeInput = new NativeTextInput();
    nativeInput->setTextChangedCallback([this](const std::string& text) {
        this->onTextChanged(text);
    });
    nativeInput->setEnterPressedCallback([this]() {
        this->onEnterPressed();
    });
}

AIAgent::~AIAgent() {
    if (nativeInput) {
        delete nativeInput;
        nativeInput = nullptr;
    }
}

void AIAgent::render(float agentPaneWidth) {
    float inputWidth = ImGui::GetContentRegionAvail().x;
    float windowHeight = ImGui::GetWindowHeight();
    float horizontalPadding = 16.0f;
    float textBoxWidth = inputWidth; // Do NOT subtract padding here
    if (textBoxWidth < 50.0f) textBoxWidth = 50.0f;

    ImGui::BeginGroup();
    ImGui::Dummy(ImVec2(horizontalPadding-10, 0)); // Left padding
    ImGui::SameLine();

    ImGui::BeginGroup(); // Vertical stack for message history, button, and input

    // Render message history above the send button and input
    float historyHeight = windowHeight * 0.6f;
    ImVec2 historySize = ImVec2(textBoxWidth - 2 * horizontalPadding, historyHeight);
    renderMessageHistory(historySize);
    ImGui::Spacing();

    // Style the Send button to match the input box
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(8, 8));
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.3f, 0.3f, 0.3f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(
        gSettings.getSettings()["backgroundColor"][0].get<float>() * 0.8f,
        gSettings.getSettings()["backgroundColor"][1].get<float>() * 0.8f,
        gSettings.getSettings()["backgroundColor"][2].get<float>() * 0.8f,
        1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(
        gSettings.getSettings()["backgroundColor"][0].get<float>() * 0.95f,
        gSettings.getSettings()["backgroundColor"][1].get<float>() * 0.95f,
        gSettings.getSettings()["backgroundColor"][2].get<float>() * 0.95f,
        1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(
        gSettings.getSettings()["backgroundColor"][0].get<float>() * 0.7f,
        gSettings.getSettings()["backgroundColor"][1].get<float>() * 0.7f,
        gSettings.getSettings()["backgroundColor"][2].get<float>() * 0.7f,
        1.0f));

    if (ImGui::Button("Send", ImVec2(60, 0))) {
        sendMessage(currentInputText.c_str());
        currentInputText.clear();
        nativeInput->clear();
    }
    ImGui::PopStyleColor(4);
    ImGui::PopStyleVar(3);

    ImGui::Spacing();

    float lineHeight = ImGui::GetTextLineHeightWithSpacing();
    ImVec2 textBoxSize = ImVec2(textBoxWidth - 2 * horizontalPadding, lineHeight * 3 + 16.0f); // Subtract padding from the size, not the width
    AgentInput(textBoxSize, textBoxWidth - 2 * horizontalPadding, horizontalPadding);
    ImGui::EndGroup(); // End vertical group

    ImGui::SameLine();
    ImGui::Dummy(ImVec2(horizontalPadding, 0)); // Right padding
    ImGui::EndGroup();
}

void AIAgent::renderMessageHistory(const ImVec2& size) {
    ImGui::BeginChild("##AIAgentMessageHistory", size, false, ImGuiWindowFlags_HorizontalScrollbar);
    for (const auto& msg : messages) {
        ImGui::TextWrapped("%s%s", msg.isAgent ? "Agent: " : "User: ", msg.text.c_str());
    }
    if (scrollToBottom) {
        ImGui::SetScrollHereY(1.0f);
        scrollToBottom = false;
    }
    ImGui::EndChild();
}

void AIAgent::AgentInput(const ImVec2& textBoxSize, float textBoxWidth, float horizontalPadding) {
    static bool wasFocused = false;
    ImGui::PushItemWidth(textBoxWidth);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(8, 8));
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.3f, 0.3f, 0.3f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(
        gSettings.getSettings()["backgroundColor"][0].get<float>() * 0.8f,
        gSettings.getSettings()["backgroundColor"][1].get<float>() * 0.8f,
        gSettings.getSettings()["backgroundColor"][2].get<float>() * 0.8f,
        1.0f));

    // Use native text input widget instead of ImGui InputTextMultiline
    if (nativeInput) {
        nativeInput->render(textBoxSize, "##AIAgentInput");
        bool isFocused = nativeInput->isFocused();
        if (isFocused != wasFocused) {
            if (isFocused) {
                editor_state.block_input = true;
                std::cout << "focused" << std::endl;
            } else {
                editor_state.block_input = false;
                std::cout << "unfocused" << std::endl;
            }
            wasFocused = isFocused;
        }
        if (isFocused) {
            editor_state.block_input = true;
        }
        std::cout << editor_state.block_input << std::endl;
    }

    ImGui::PopStyleColor(2);
    ImGui::PopStyleVar(3);
    ImGui::PopItemWidth();
}

void AIAgent::printAllMessages() const {
    std::cout << "--- Message History ---" << std::endl;
    for (const auto& msg : messages) {
        std::cout << (msg.isAgent ? "Agent: " : "User: ") << msg.text << std::endl;
    }
    std::cout << "-----------------------" << std::endl;
}

void AIAgent::sendMessage(const char* msg) {
    messages.push_back({msg ? msg : "", false}); // User message, avoid null
    std::cout << "sending message: " << msg << std::endl;
    printAllMessages();
    scrollToBottom = true;
    // In the future, trigger agent response here
}

void AIAgent::onTextChanged(const std::string& text) {
    currentInputText = text;
}

void AIAgent::onEnterPressed() {
    if (!currentInputText.empty()) {
        sendMessage(currentInputText.c_str());
        currentInputText.clear();
        nativeInput->clear();
    }
}
