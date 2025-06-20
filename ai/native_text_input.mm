#include "native_text_input.h"
#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_COCOA
#include <GLFW/glfw3native.h>
#include <iostream>

#ifdef __APPLE__
#import <Cocoa/Cocoa.h>

// Forward declaration
@interface NativeTextInputView : NSTextView
@property (nonatomic, assign) BOOL textChanged;
@property (nonatomic, assign) BOOL enterPressed;
@property (nonatomic, assign) BOOL focused;
@end

@implementation NativeTextInputView {
    NativeTextInput* _owner;
}

- (instancetype)initWithFrame:(NSRect)frame owner:(NativeTextInput*)owner {
    self = [super initWithFrame:frame];
    if (self) {
        _owner = owner;
        _textChanged = NO;
        _enterPressed = NO;
        _focused = NO;
        
        // Configure the text view for word wrapping
        [self setRichText:NO];
        [self setEditable:YES];
        [self setSelectable:YES];
        [self setImportsGraphics:NO];
        [self setAllowsUndo:YES];
        
        // Enable word wrapping
        [[self textContainer] setWidthTracksTextView:YES];
        [[self textContainer] setContainerSize:NSMakeSize(FLT_MAX, FLT_MAX)];
        [self setMaxSize:NSMakeSize(FLT_MAX, FLT_MAX)];
        
        // Configure appearance
        [self setBackgroundColor:[NSColor clearColor]];
        [self setTextColor:[NSColor textColor]];
        // Set font size to match ImGui font size
        float imguiFontSize = 13.0f;
        #ifdef __cplusplus
        imguiFontSize = ImGui::GetFontSize();
        imguiFontSize = imguiFontSize * .8;
        #endif
        [self setFont:[NSFont systemFontOfSize:imguiFontSize]];
        
        // Remove borders and make it look like a native input
        [self setDrawsBackground:NO];
        [self setFocusRingType:NSFocusRingTypeNone];
        
        // Ensure we can receive mouse events
        [self setAcceptsTouchEvents:YES];
    }
    return self;
}

- (void)mouseDown:(NSEvent*)event {
    // Make this view the first responder when clicked
    std::cout << "NativeTextInputView mouseDown received" << std::endl;
    [[self window] makeFirstResponder:self];
    [super mouseDown:event];
}

- (BOOL)becomeFirstResponder {
    std::cout << "NativeTextInputView becomeFirstResponder called" << std::endl;
    BOOL result = [super becomeFirstResponder];
    self.focused = YES;
    if (_owner) {
        _owner->setNativeFocused(true);
    }
    std::cout << "NativeTextInputView becomeFirstResponder result: " << (result ? "YES" : "NO") << std::endl;
    return result;
}

- (BOOL)resignFirstResponder {
    std::cout << "NativeTextInputView resignFirstResponder called" << std::endl;
    BOOL result = [super resignFirstResponder];
    self.focused = NO;
    if (_owner) {
        _owner->setNativeFocused(false);
    }
    std::cout << "NativeTextInputView resignFirstResponder result: " << (result ? "YES" : "NO") << std::endl;
    return result;
}

- (void)textDidChange:(NSNotification*)notification {
    self.textChanged = YES;
    if (_owner) {
        _owner->handleTextChange();
    }
}

- (void)keyDown:(NSEvent*)event {
    // Check for Cmd+A
    if (([event modifierFlags] & NSEventModifierFlagCommand) && [[event charactersIgnoringModifiers] isEqualToString:@"a"]) {
        [self selectAll:nil];
        return;
    }
    if ([event keyCode] == 36) { // Enter key
        if ([event modifierFlags] & NSEventModifierFlagShift) {
            // Shift+Enter: insert newline
            [super keyDown:event];
        } else {
            // Enter: trigger callback and don't insert newline
            self.enterPressed = YES;
            if (_owner) {
                _owner->handleTextChange();
            }
            return;
        }
    } else {
        [super keyDown:event];
    }
}

- (BOOL)textView:(NSTextView*)textView doCommandBySelector:(SEL)commandSelector {
    if (commandSelector == @selector(insertNewline:)) {
        // Handle Enter key
        self.enterPressed = YES;
        if (_owner) {
            _owner->handleTextChange();
        }
        return YES;
    }
    return NO;
}

- (void)setFrame:(NSRect)frame {
    [super setFrame:frame];
    
    // Update text container size for word wrapping
    NSTextContainer* container = [self textContainer];
    [container setContainerSize:NSMakeSize(frame.size.width, FLT_MAX)];
}

- (void)doCommandBySelector:(SEL)commandSelector {
    if (commandSelector == @selector(insertNewline:)) {
        self.enterPressed = YES;
        if (_owner) {
            _owner->handleTextChange();
        }
        // Don't call super for Enter, since you handle it
        return;
    }
    [super doCommandBySelector:commandSelector];
}

@end

// C++ implementation
NativeTextInput::NativeTextInput() 
    : textView(nil), parentWindow(nil), isActive(false), nativeFocused(false) {
    std::cout << "NativeTextInput constructor called" << std::endl;
    // Do not create the view here; defer until ensureInitialized()
}

void NativeTextInput::ensureInitialized() {
    if (textView && parentWindow) return;
    // Get the GLFW window and convert to NSWindow
    GLFWwindow* glfwWindow = glfwGetCurrentContext();
    if (glfwWindow) {
        parentWindow = glfwGetCocoaWindow(glfwWindow);
        std::cout << "NativeTextInput: Got parent window" << std::endl;
    } else {
        std::cout << "NativeTextInput: No GLFW window found" << std::endl;
    }
    if (parentWindow) {
        // Create the text view
        NSRect frame = NSMakeRect(0, 0, 200, 100);
        textView = (__bridge_retained void*)[[NativeTextInputView alloc] initWithFrame:frame owner:this];
        // Add to the window's content view
        NSView* contentView = [parentWindow contentView];
        [contentView addSubview:(__bridge NSView*)textView];
        // Keep the view visible from the start
        std::cout << "NativeTextInput: Text view created and added to window" << std::endl;
    } else {
        std::cout << "NativeTextInput: No parent window, text view not created" << std::endl;
    }
}

NativeTextInput::~NativeTextInput() {
    if (textView) {
        [(__bridge NSView*)textView removeFromSuperview];
        CFRelease(textView);
        textView = nil;
    }
}

void NativeTextInput::setNativeFocused(bool focused) {
    nativeFocused = focused;
    std::cout << "NativeTextInput focus changed to: " << (focused ? "true" : "false") << std::endl;
}

bool NativeTextInput::render(const ImVec2& size, const char* label) {
    ensureInitialized();
    std::cout << "NativeTextInput::render called, textView: " << (textView ? "exists" : "nil") << std::endl;
    
    if (!textView || !parentWindow) {
        // Fallback to ImGui if native widget is not available
        static char buffer[1024] = {0};
        std::cout << "NativeTextInput::render - using ImGui fallback" << std::endl;
        return ImGui::InputTextMultiline(label, buffer, sizeof(buffer), size);
    }
    
    // Dynamically update font size to match ImGui
    float imguiFontSize = ImGui::GetFontSize();
    if (imguiFontSize != lastFontSize) {
        [(__bridge NativeTextInputView*)textView setFont:[NSFont systemFontOfSize:imguiFontSize]];
        lastFontSize = imguiFontSize * .8;;
    }
    
    // Get the current ImGui cursor position
    ImVec2 cursorPos = ImGui::GetCursorScreenPos();
    
    // Update the native widget position and size
    updatePosition(cursorPos, size);
    
    // Check if ImGui wants to focus this widget
    ImGuiID id = ImGui::GetID(label);
    bool isHovered = ImGui::IsItemHovered();
    bool isClicked = ImGui::IsMouseClicked(0) && isHovered;
    
    std::cout << "NativeTextInput::render - isHovered: " << (isHovered ? "true" : "false") << ", isClicked: " << (isClicked ? "true" : "false") << std::endl;
    
    // Handle focus changes - only manage focus, don't hide/show the view
    if (isClicked && !isActive) {
        focus();
    } else if (!isHovered && ImGui::IsMouseClicked(0)) {
        unfocus();
    }
    
    // Handle text changes and callbacks
    if (textView) {
        NativeTextInputView* nativeView = (__bridge NativeTextInputView*)textView;
        if (nativeView.textChanged) {
            syncText();
            if (textChangedCallback) {
                textChangedCallback(currentText);
            }
            nativeView.textChanged = NO;
        }
        
        if (nativeView.enterPressed) {
            if (enterPressedCallback) {
                enterPressedCallback();
            }
            nativeView.enterPressed = NO;
        }
    }
    
    // Use an invisible button for hit-testing
    ImGui::InvisibleButton(label, size);
    
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    ImVec2 min = ImGui::GetItemRectMin();
    ImVec2 max = ImGui::GetItemRectMax();
    draw_list->AddRect(min, max, IM_COL32(77, 77, 77, 255), 4.0f, 0, 2.0f);
    
    // Keep the native widget visible - don't hide/show it
    [(__bridge NSView*)textView setHidden:NO];
    
    return false; // We handle the input ourselves
}

std::string NativeTextInput::getText() const {
    if (textView) {
        NSString* text = [(__bridge NativeTextInputView*)textView string];
        return std::string([text UTF8String]);
    }
    return currentText;
}

void NativeTextInput::setText(const std::string& text) {
    currentText = text;
    if (textView) {
        NSString* nsText = [NSString stringWithUTF8String:text.c_str()];
        [(__bridge NativeTextInputView*)textView setString:nsText];
    }
}

bool NativeTextInput::isFocused() const {
    return nativeFocused;
}

void NativeTextInput::setTextChangedCallback(std::function<void(const std::string&)> callback) {
    textChangedCallback = callback;
}

void NativeTextInput::setEnterPressedCallback(std::function<void()> callback) {
    enterPressedCallback = callback;
}

void NativeTextInput::clear() {
    currentText.clear();
    if (textView) {
        [(__bridge NativeTextInputView*)textView setString:@""];
    }
}

void NativeTextInput::focus() {
    std::cout << "NativeTextInput::focus() called" << std::endl;
    if (textView && !isActive) {
        isActive = true;
        // Don't hide/show the view - just manage focus
        [[(__bridge NSView*)textView window] makeFirstResponder:(__bridge NSView*)textView];
        std::cout << "NativeTextInput::focus() - made first responder" << std::endl;
    }
}

void NativeTextInput::unfocus() {
    std::cout << "NativeTextInput::unfocus() called" << std::endl;
    if (textView && isActive) {
        isActive = false;
        // Don't hide the view - just remove focus
        [[(__bridge NSView*)textView window] makeFirstResponder:nil];
        std::cout << "NativeTextInput::unfocus() - removed first responder" << std::endl;
    }
}

void NativeTextInput::updatePosition(const ImVec2& pos, const ImVec2& size) {
    if (!textView || !parentWindow) return;
    
    // Convert ImGui screen coordinates to window coordinates
    NSView* contentView = [parentWindow contentView];
    NSRect windowFrame = [contentView frame];
    
    // Calculate position relative to the window
    float x = pos.x;
    float y = windowFrame.size.height - pos.y - size.y; // Flip Y coordinate
    
    // Only update the frame if it has changed
    if (x != lastFrameX || y != lastFrameY || size.x != lastFrameW || size.y != lastFrameH) {
        NSRect frame = NSMakeRect(x, y, size.x, size.y);
        std::cout << "NativeTextInput::updatePosition frame: x=" << x << ", y=" << y << ", w=" << size.x << ", h=" << size.y << std::endl;
        [(__bridge NSView*)textView setFrame:frame];
        // Update text container for word wrapping
        NSTextContainer* container = [(__bridge NativeTextInputView*)textView textContainer];
        [container setContainerSize:NSMakeSize(size.x, FLT_MAX)];
        // Cache the new frame
        lastFrameX = x;
        lastFrameY = y;
        lastFrameW = size.x;
        lastFrameH = size.y;
    }
}

void NativeTextInput::syncText() {
    if (textView) {
        NSString* text = [(__bridge NativeTextInputView*)textView string];
        currentText = std::string([text UTF8String]);
    }
}

void NativeTextInput::handleTextChange() {
    syncText();
}

#endif 