#include "ai_agent.h"

void AIAgent::render(float agentPaneWidth) {
    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 0.0f);
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.2f, 0.2f, 0.2f, 0.0f));

    ImGui::BeginChild("Agent Pane", ImVec2(agentPaneWidth, -1), true);
    ImGui::Text("Agent");
    ImGui::EndChild();

    ImGui::PopStyleColor();
    ImGui::PopStyleVar();
}
