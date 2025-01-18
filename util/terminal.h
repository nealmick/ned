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
    // Cell data structure
    struct Cell {
        char32_t ch = ' ';
        ImVec4 fg = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
        ImVec4 bg = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
        uint16_t attrs = 0;  // Bold, italic, etc
    };

    // Buffer state
    std::vector<std::vector<Cell>> buffer;
    std::vector<std::vector<Cell>> altBuffer;
    std::vector<std::vector<Cell>> historyBuffer;
    const int bufferWidth = 120;
    const int bufferHeight = 30;
    const int maxHistoryLines = 1000;

    // Cursor state
    int cursorX = 0;
    int cursorY = 0;
    float cursorBlinkTime = 0;
    int lastLineLength = 0;
    int promptEndX = 0;
    int promptEndY = 0;
    int scrollRegionTop = 0;
    int scrollRegionBottom = bufferHeight;

    // PTY state
    bool isVisible = false;
    bool needsFocus = false;
    int ptyFd = -1;
    pid_t childPid = -1;
    bool shellStarted = false;
    bool set_dir = false;

    // Selection state
    struct {
        bool active = false;
        int startX = 0, startY = 0;
        int endX = 0, endY = 0;
    } selection;

    // ANSI state
    struct {
        bool inEscape = false;
        bool inCSI = false;
        std::string currentSequence;
        ImVec4 currentFg = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
        ImVec4 currentBg = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
        uint16_t currentAttrs = 0;
        bool skipNext = false;
        
        // Terminal modes
        bool applicationCursorKeys = false;
        bool lineWrap = true;
        bool cursorBlink = true; 
        bool cursorVisible = true;
        bool bracketedPaste = false;
        bool insertMode = false;
        bool automaticNewline = false;
        bool mouseTracking = false;
        bool mouseButtonTracking = false;
        bool mouseAnyEvent = false;
        // Saved cursor position
        int savedCursorX = 0;
        int savedCursorY = 0;
    } ansiState;

    // Scroll state  
    float scrollPosition = 0.0f;
    float maxScrollPosition = 0.0f;
    bool autoScroll = true;
    bool needsScroll = false;
    bool isTyping = false;
    float typeIdleTime = 0.0f;
    static constexpr float TYPE_TIMEOUT = 1.0f;


    
    // Thread management
    std::mutex bufferMutex;
    std::thread readThread;
    bool shouldTerminate = false;

    // Core operations
    void startShell();
    void readOutput();
    void processInput(const std::string& input);
    void processChar(char32_t c);
    void writeToBuffer(const char* data, size_t length);
    void scrollBuffer(int lines);
    void clearRange(int startX, int startY, int endX, int endY);
    void copySelection();

    // ANSI handling
    void handleCSI(const std::string& seq);
    void setCursorPos(int x, int y);
    ImVec4 convert256ToRGB(int color);

    // Rendering
    void renderBuffer(const ImVec2& pos, float charWidth, float lineHeight);
    void renderCursor(const ImVec2& pos, float charWidth, float lineHeight);
    void screenToTerminal(const ImVec2& screenPos, const ImVec2& terminalPos,
                         float charWidth, float lineHeight,
                         int* termX, int* termY);


    struct CSIParams {
        std::vector<int> params;
        bool isPrivateMode;
        std::string paramStr;
        char command;
        bool isSoftReset;
    };

    CSIParams parseCSIParams(const std::string& seq);
    void handleCursorMovement(const CSIParams& params);
    void handleEraseOperations(const CSIParams& params);
    void handleLineOperations(const CSIParams& params);
    void handleModeSettings(const CSIParams& params);
    void handleGraphicsRendition(const std::vector<int>& params);
    void handleScrollRegion(const CSIParams& params);

};

extern Terminal gTerminal;