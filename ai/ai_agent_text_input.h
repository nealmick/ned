// ai_agent_text_input.h
#pragma once
#include <functional>
#include <imgui.h>
#include <string>

// Forward declaration for ImGui internal type
struct ImGuiInputTextState;

class AIAgentTextInput
{
  public:
	AIAgentTextInput();
	~AIAgentTextInput() = default;

	// Main render function for the text input box
	void render(const ImVec2 &textBoxSize, float textBoxWidth, float horizontalPadding);

	// Set the input buffer and its size
	void setInputBuffer(char *buffer, size_t bufferSize);

	// Set callback for when a message should be sent
	void setSendMessageCallback(std::function<void(const char *, bool)> callback);

	// Set callback for checking if processing
	void setIsProcessingCallback(std::function<bool()> callback);

	// Set callback for showing notifications
	void setNotificationCallback(std::function<void(const char *, float)> callback);

	// Set callback for clearing conversation
	void setClearConversationCallback(std::function<void()> callback);

	// Set callback for toggling history
	void setToggleHistoryCallback(std::function<void()> callback);

	// Set callback for blocking editor input
	void setBlockInputCallback(std::function<void(bool)> callback);

	// Set callback for stopping the current request
	void setStopRequestCallback(std::function<void()> callback);

  private:
	char *inputBuffer;
	size_t bufferSize;

	// Callbacks
	std::function<void(const char *, bool)> sendMessageCallback;
	std::function<bool()> isProcessingCallback;
	std::function<void(const char *, float)> notificationCallback;
	std::function<void()> clearConversationCallback;
	std::function<void()> toggleHistoryCallback;
	std::function<void(bool)> blockInputCallback;
	std::function<void()> stopRequestCallback;

	// Internal state
	bool shouldRestoreFocus;

	// Helper functions
	static bool HandleWordBreakAndWrap(char *inputBuffer,
									   size_t bufferSize,
									   ImGuiInputTextState *state,
									   float max_width,
									   float text_box_x);
	void renderInputBox(const ImVec2 &textBoxSize, float textBoxWidth);
	void renderButtons(float textBoxWidth);
	void handleInputLogic();
	void renderHintText(float textBoxWidth);
	void renderSpinner(const ImVec2 &position, float size, float time);
};