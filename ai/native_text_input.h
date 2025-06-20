#pragma once
#include <imgui.h>
#include <string>
#include <functional>

#ifdef __APPLE__
// Forward declarations for Objective-C classes
#ifdef __OBJC__
@class NSTextView;
@class NSWindow;
#else
typedef struct objc_object NSTextView;
typedef struct objc_object NSWindow;
#endif

class NativeTextInput {
public:
    NativeTextInput();
    ~NativeTextInput();
    
    // Render the native text input widget
    bool render(const ImVec2& size, const char* label = "##NativeTextInput");
    
    // Get the current text content
    std::string getText() const;
    
    // Set the text content
    void setText(const std::string& text);
    
    // Check if the widget is focused
    bool isFocused() const;
    void setNativeFocused(bool focused);
    
    // Set callback for when text changes
    void setTextChangedCallback(std::function<void(const std::string&)> callback);
    
    // Set callback for when Enter is pressed
    void setEnterPressedCallback(std::function<void()> callback);
    
    // Clear the text
    void clear();
    
    // Focus the widget
    void focus();
    
    // Unfocus the widget
    void unfocus();
    
    // Handle text change (called from Objective-C)
    void handleTextChange();

    // Ensure the native view is created (deferred init)
    void ensureInitialized();

private:
    void* textView; // Use void* to avoid type conflicts
    NSWindow* parentWindow;
    bool isActive;
    bool nativeFocused;
    std::string currentText;
    std::function<void(const std::string&)> textChangedCallback;
    std::function<void()> enterPressedCallback;
    
    // Cache the last frame to avoid redundant updates
    float lastFrameX = -1.0f;
    float lastFrameY = -1.0f;
    float lastFrameW = -1.0f;
    float lastFrameH = -1.0f;
    
    float lastFontSize = -1.0f;
    
    void updatePosition(const ImVec2& pos, const ImVec2& size);
    void syncText();
};

#else
// Fallback for non-macOS platforms - use ImGui InputTextMultiline
class NativeTextInput {
public:
    NativeTextInput() : bufferSize(1024) {
        buffer = new char[bufferSize];
        buffer[0] = '\0';
    }
    
    ~NativeTextInput() {
        delete[] buffer;
    }
    
    bool render(const ImVec2& size, const char* label = "##NativeTextInput") {
        return ImGui::InputTextMultiline(label, buffer, bufferSize, size);
    }
    
    std::string getText() const {
        return std::string(buffer);
    }
    
    void setText(const std::string& text) {
        strncpy(buffer, text.c_str(), bufferSize - 1);
        buffer[bufferSize - 1] = '\0';
    }
    
    bool isFocused() const {
        return ImGui::IsItemActive();
    }
    
    void setTextChangedCallback(std::function<void(const std::string&)> callback) {
        textChangedCallback = callback;
    }
    
    void setEnterPressedCallback(std::function<void()> callback) {
        enterPressedCallback = callback;
    }
    
    void clear() {
        buffer[0] = '\0';
    }
    
    void focus() {
        // ImGui doesn't have a direct focus method
    }
    
    void unfocus() {
        // ImGui doesn't have a direct unfocus method
    }
    
    void handleTextChange() {
        // Not used in fallback implementation
    }

private:
    char* buffer;
    size_t bufferSize;
    std::function<void(const std::string&)> textChangedCallback;
    std::function<void()> enterPressedCallback;
};

#endif 