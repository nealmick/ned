#pragma once
#include "imgui.h"
#include <string>
#include <vector>
#include <thread>
#include <mutex>

class Terminal {
public:
    Terminal();
    ~Terminal();
    
    void toggleVisibility();
    bool isTerminalVisible() const { return isVisible; }
    void render();
    void setWorkingDirectory(const std::string& path);

private:
    // Core terminal state
    bool isVisible = false;
    bool needsFocus = false;
    int ptyFd = -1;
    pid_t childPid = -1;
    bool shellStarted = false;
    bool set_dir = false;
    
    // Selection operations
    void copySelection();
    
    // Buffer management
    struct Cell {
        char32_t ch = ' ';
        ImVec4 fg = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
        ImVec4 bg = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
        uint16_t attrs = 0;  // Bold, italic, etc
    };
    
    std::vector<std::vector<Cell>> buffer;
    int bufferWidth = 80;
    int bufferHeight = 24;
    int cursorX = 0;
    int cursorY = 0;
    float cursorBlinkTime = 0;
    int lastLineLength = 0;  
    int promptEndX = 0;
    int promptEndY = 0;
    // Selection state
    struct {
        bool active = false;
        int startX = 0;
        int startY = 0;
        int endX = 0;
        int endY = 0;
    } selection;
    
    // ANSI state
    struct {
        bool inEscape = false;
        bool inCSI = false;
        std::string currentSequence;
        ImVec4 currentFg = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
        ImVec4 currentBg = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
        uint16_t currentAttrs = 0;
        bool skipNext = false;  // signal to not render char
    } ansiState;

    // Thread management
    std::mutex bufferMutex;
    std::thread readThread;
    bool shouldTerminate = false;

    // Core terminal operations
    void startShell();
    void readOutput();
    void processInput(const std::string& input);
    void processChar(char32_t c);
    void updateTerminalSize();

    // Buffer operations
    void writeToBuffer(const char* data, size_t length);
    void scrollBuffer(int lines);
    void clearRange(int startX, int startY, int endX, int endY);
    
    // Selection handling
    void screenToTerminal(const ImVec2& screenPos, const ImVec2& terminalPos,
                         float charWidth, float lineHeight,
                         int* termX, int* termY);
    
    // ANSI handling
    void handleEscapeSequence();
    void handleCSI(const std::string& seq);
    void setCursorPos(int x, int y);
    
    // Rendering
    void renderBuffer(const ImVec2& pos, float charWidth, float lineHeight);
    void renderCursor(const ImVec2& pos, float charWidth, float lineHeight);


    std::vector<std::vector<Cell>> historyBuffer;  // Stores scrolled-off lines
    int maxHistoryLines = 1000;                    // Maximum history size
    float scrollPosition = 0.0f;                   // Current scroll position
    float maxScrollPosition = 0.0f;                // Maximum scroll position
    bool autoScroll = true;                        // Whether to auto-scroll on new output


    bool isTyping = false;            // Tracks if user is currently typing
    float typeIdleTime = 0.0f;        // Time since last keystroke
    const float TYPE_TIMEOUT = 1.0f;   // Time before considering typing finished
    bool needsScroll = false;  // Flag to indicate scroll needed

    //alternate buffer rendering
    int savedCursorX = 0;
    int savedCursorY = 0;
    int scrollRegionTop = 0;
    int scrollRegionBottom = bufferHeight - 1;

    


};

extern Terminal gTerminal;