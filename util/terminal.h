#pragma once
#include "imgui.h"
#include <string>
#include <vector>
#include <deque>
#include <thread>
#include <mutex>

class Terminal {
public:
    Terminal();
    ~Terminal();
    
    void toggleVisibility() {
        isVisible = !isVisible;
    }
    
    bool isTerminalVisible() const {
        return isVisible;
    }
    
    void render();
    void setWorkingDirectory(const std::string& path);
    void updateTerminalSize();
private:
    bool isVisible;
    int ptyFd;              // File descriptor for the PTY
    int childPid;           // PID of the shell process
    bool shellStarted;      // Flag to track if shell is already running
    bool shouldScrollToBottom = false;
    std::string terminalOutput;  // Complete terminal output
    static const size_t INPUT_BUFFER_SIZE = 2048;
    char inputBuffer[INPUT_BUFFER_SIZE];
    std::mutex bufferMutex;
    std::thread readThread;
    bool shouldTerminate;
    
    void startShell();
    void readOutput();
    void processInput(const std::string& input);
    std::string stripAnsiCodes(const std::string& input);
    void initScreenBuffer();  // Add this
    void writeToScreenBuffer(const char* data, size_t length);  // And this
    int scrollOffset = 0;  // How many lines we've scrolled up
    int maxScrollback = 1000;  // How many lines of history to keep


    struct ScreenCell {
        char ch = ' ';
        bool dirty = false;
        ImVec4 fg = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);  // Default white text
        ImVec4 bg = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);  // Default transparent background
        bool bold = false;
        bool italic = false;
    };
    std::vector<std::vector<ScreenCell>> screenBuffer;
    int cursorX = 0;
    int cursorY = 0;
    int bufferWidth = 80;   // Default terminal width
    int bufferHeight = 24;  // Default terminal height
};

extern Terminal gTerminal;