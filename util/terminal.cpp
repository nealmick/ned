#include "util/terminal.h"
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <signal.h>
#include <iostream>
#include <cmath>

Terminal gTerminal;


Terminal::Terminal() {
    buffer.resize(bufferHeight, std::vector<Cell>(bufferWidth));
}

Terminal::~Terminal() {
    shouldTerminate = true;
    if (readThread.joinable()) {
        readThread.join();
    }
    if (ptyFd >= 0) {
        close(ptyFd);
    }
    if (childPid > 0) {
        kill(childPid, SIGTERM);
    }
}

void Terminal::startShell() {
    if (shellStarted) return;
    shellStarted = true;
    
    ptyFd = posix_openpt(O_RDWR | O_NOCTTY);
    if (ptyFd < 0) {
        std::cerr << "Failed to open PTY" << std::endl;
        return;
    }

    if (grantpt(ptyFd) < 0 || unlockpt(ptyFd) < 0) {
        close(ptyFd);
        std::cerr << "Failed to grant/unlock PTY" << std::endl;
        return;
    }

    char* slaveName = ptsname(ptyFd);
    if (slaveName == NULL) {
        close(ptyFd);
        std::cerr << "Failed to get PTY slave name" << std::endl;
        return;
    }

    childPid = fork();
    if (childPid < 0) {
        close(ptyFd);
        std::cerr << "Fork failed" << std::endl;
        return;
    }

    if (childPid == 0) {  // Child process
        if (setsid() < 0) {
            std::cerr << "setsid failed" << std::endl;
            exit(1);
        }

        close(ptyFd);  // Close master side
        int slaveFd = open(slaveName, O_RDWR);
        if (slaveFd < 0) {
            std::cerr << "Failed to open slave PTY" << std::endl;
            exit(1);
        }

        if (ioctl(slaveFd, TIOCSCTTY, 0) < 0) {
            std::cerr << "Failed to set controlling terminal" << std::endl;
            exit(1);
        }

        dup2(slaveFd, STDIN_FILENO);
        dup2(slaveFd, STDOUT_FILENO);
        dup2(slaveFd, STDERR_FILENO);
        if (slaveFd > STDERR_FILENO) {
            close(slaveFd);
        }

        // Configure terminal
        struct termios tios;
        if (tcgetattr(STDIN_FILENO, &tios) < 0) {
            std::cerr << "Failed to get terminal attributes" << std::endl;
            exit(1);
        }
        // Configure terminal for full operation
        tios.c_iflag = ICRNL | IXON | IXANY | IMAXBEL | BRKINT;
        tios.c_oflag = OPOST | ONLCR;
        tios.c_cflag = CREAD | CS8 | HUPCL;
        tios.c_lflag = ICANON | ISIG | IEXTEN | ECHO | ECHOE | ECHOK | ECHOCTL | ECHOKE | ECHOPRT;

        // Set control characters
        tios.c_cc[VMIN] = 1;
        tios.c_cc[VTIME] = 0;
        tios.c_cc[VINTR] = 0x03;    // Ctrl-C
        tios.c_cc[VQUIT] = 0x1c;    // Ctrl-backslash
        tios.c_cc[VERASE] = 0x7f;   // Delete
        tios.c_cc[VKILL] = 0x15;    // Ctrl-U
        tios.c_cc[VEOF] = 0x04;     // Ctrl-D
        tios.c_cc[VSTART] = 0x11;   // Ctrl-Q
        tios.c_cc[VSTOP] = 0x13;    // Ctrl-S
        tios.c_cc[VSUSP] = 0x1a;    // Ctrl-Z
        tios.c_cc[VEOL] = 0;
        tios.c_cc[VREPRINT] = 0x12; // Ctrl-R
        tios.c_cc[VDISCARD] = 0x0f; // Ctrl-O
        tios.c_cc[VWERASE] = 0x17;  // Ctrl-W
        tios.c_cc[VLNEXT] = 0x16;   // Ctrl-V
        tios.c_cc[VEOL2] = 0;


        if (tcsetattr(STDIN_FILENO, TCSANOW, &tios) < 0) {
            std::cerr << "Failed to set terminal attributes" << std::endl;
            exit(1);
        }

        const char* shell = getenv("SHELL");
        if (!shell) shell = "/bin/bash";
        
        char* const args[] = {(char*)shell, NULL};
        execvp(shell, args);
        std::cerr << "Failed to execute shell" << std::endl;
        exit(1);
    }

    // Parent process
    readThread = std::thread(&Terminal::readOutput, this);
}

void Terminal::readOutput() {
    char buffer[4096];
    while (!shouldTerminate) {
        ssize_t bytesRead = read(ptyFd, buffer, sizeof(buffer) - 1);
        if (bytesRead > 0) {
            std::lock_guard<std::mutex> lock(bufferMutex);
            writeToBuffer(buffer, bytesRead);
        } else if (bytesRead < 0 && errno != EINTR) {
            break;
        }
    }
}
void Terminal::writeToBuffer(const char* data, size_t length) {
    if (data[0] == '\033') {  // ESC character
        std::cerr << "Escape sequence received at buffer state=" 
                  << (useAlternateBuffer ? "alt" : "normal") << std::endl;
    }
    // Add these debug statements at the start
    std::cerr << "writeToBuffer called, useAlternateBuffer=" 
              << (useAlternateBuffer ? "true" : "false") << std::endl;
              
    if (useAlternateBuffer) {
        std::cerr << "VIM raw bytes (len=" << length << "): ";
        for (size_t i = 0; i < length; i++) {
            unsigned char c = data[i];
            if (c == '\033') std::cerr << "ESC";
            else if (c < 32) std::cerr << "^" << (char)(c + 64);
            else std::cerr << c;
        }
        std::cerr << std::endl;
    }
    for (size_t i = 0; i < length; ++i) {
        if (ansiState.inEscape) {
            if (ansiState.inCSI) {
                if (isalpha(data[i])) {
                    ansiState.currentSequence += data[i];
                    handleCSI(ansiState.currentSequence);
                    ansiState.inEscape = false;
                    ansiState.inCSI = false;
                    ansiState.currentSequence.clear();
                } else {
                    ansiState.currentSequence += data[i];
                }
            } else if (data[i] == '[') {
                ansiState.inCSI = true;
                ansiState.currentSequence.clear();
            } else if (data[i] == '(' || data[i] == ')') {
                // Skip the next character (usually 'B' for ASCII charset)
                ansiState.skipNext = true;
                ansiState.inEscape = false;
            } else {
                ansiState.inEscape = false;
            }
            continue;
        }

        if (ansiState.skipNext) {
            ansiState.skipNext = false;
            continue;
        }

        if (data[i] == '\033') {
            ansiState.inEscape = true;
            continue;
        }

        // Rest of the function stays the same
        if (data[i] == '\x08') {
            if (cursorX > 0) cursorX--;
            continue;
        }

        if (data[i] >= 32) {
            buffer[cursorY][cursorX].ch = data[i];
            buffer[cursorY][cursorX].fg = ansiState.currentFg;
            buffer[cursorY][cursorX].bg = ansiState.currentBg;
            buffer[cursorY][cursorX].attrs = ansiState.currentAttrs;
            cursorX++;
            if (cursorX > lastLineLength) lastLineLength = cursorX;
        } else {
            processChar(data[i]);
        }
    }
    
    if (autoScroll) needsScroll = true;
}
void Terminal::processChar(char32_t c) {
    bool needScroll = false;
    
    if (c == '\r') {
        cursorX = 0;
        lastLineLength = 0;
    } else if (c == '\n') {
        cursorX = 0;
        lastLineLength = 0;
        cursorY++;
        
        // If we're within a scroll region and hit the bottom
        if (scrollRegionBottom != (bufferHeight - 1) && cursorY > scrollRegionBottom) {
            // Scroll just the region
            std::cerr << "Scrolling region " << scrollRegionTop << " to " << scrollRegionBottom << std::endl;
            for (int y = scrollRegionTop; y < scrollRegionBottom; y++) {
                buffer[y] = std::move(buffer[y + 1]);
            }
            buffer[scrollRegionBottom] = std::vector<Cell>(bufferWidth);
            cursorY = scrollRegionBottom;
        }
        // Normal scrolling behavior
        else if (cursorY >= bufferHeight) {
            if (!useAlternateBuffer) {
                scrollBuffer(1);
            }
            cursorY = bufferHeight - 1;
            needScroll = true;
        }
        
        // After newline, next output will be shell prompt
        promptEndX = 0;
        promptEndY = cursorY;
    } else if (c == '\t') {
        int newX = (cursorX + 8) & ~7;
        if (newX >= bufferWidth) {
            cursorX = 0;
            lastLineLength = 0;
            cursorY++;
            if (cursorY >= bufferHeight) {
                if (!useAlternateBuffer) {
                    scrollBuffer(1);
                }
                cursorY = bufferHeight - 1;
                needScroll = true;
            }
        } else {
            cursorX = newX;
            lastLineLength = std::max(lastLineLength, cursorX);
        }
        // Update prompt end position if we're still receiving shell output
        if (!isTyping) {
            promptEndX = cursorX;
            promptEndY = cursorY;
        }
    } else if (c >= 32) {
        if (cursorX >= bufferWidth) {
            cursorX = 0;
            lastLineLength = 0;
            cursorY++;
            if (cursorY >= bufferHeight) {
                if (!useAlternateBuffer) {
                    scrollBuffer(1);
                }
                cursorY = bufferHeight - 1;
                needScroll = true;
            }
        }

        buffer[cursorY][cursorX].ch = c;
        buffer[cursorY][cursorX].fg = ansiState.currentFg;
        buffer[cursorY][cursorX].bg = ansiState.currentBg;
        buffer[cursorY][cursorX].attrs = ansiState.currentAttrs;
        cursorX++;
        lastLineLength = std::max(lastLineLength, cursorX);
        
        // Update prompt end position if we're still receiving shell output
        if (!isTyping) {
            promptEndX = cursorX;
            promptEndY = cursorY;
        }
    }
    
    if (needScroll && autoScroll) {
        needsScroll = true;
    }
}


void Terminal::scrollBuffer(int lines) {

    if (lines <= 0) return;
    if (lines <= 0 || useAlternateBuffer) return;
    // Add the top lines to history before scrolling
    for (int i = 0; i < lines && i < bufferHeight; i++) {
        historyBuffer.push_back(buffer[i]);
        if (historyBuffer.size() > maxHistoryLines) {
            historyBuffer.erase(historyBuffer.begin());
        }
    }

    // Move lines up in the visible buffer
    for (int y = 0; y < bufferHeight - lines; y++) {
        buffer[y] = std::move(buffer[y + lines]);
    }

    // Clear new lines at bottom
    for (int y = bufferHeight - lines; y < bufferHeight; y++) {
        buffer[y] = std::vector<Cell>(bufferWidth);
    }

    // Update scroll position if auto-scroll is enabled
    if (autoScroll) {
        scrollPosition = maxScrollPosition;
    }
}
void Terminal::handleCSI(const std::string& seq) {
    if (seq.empty()) return;
    
    // Debug logging
    std::cerr << "CSI seq: '" << seq << "' (";
    for (char c : seq) {
        if (c < 32) std::cerr << "^" << (char)(c + 64);
        else std::cerr << c;
    }
    std::cerr << ")" << std::endl;
    
    char cmd = seq.back();
    std::vector<int> params;
    
    // Extract numeric parameters
    std::string numStr;
    bool isPrivateMode = false;
    
    for (size_t i = 0; i < seq.length() - 1; i++) {
        if (i == 0 && seq[i] == '?') {
            isPrivateMode = true;
            continue;
        }
        if (isdigit(seq[i])) {
            numStr += seq[i];
        } else if (seq[i] == ';') {
            params.push_back(numStr.empty() ? 0 : std::stoi(numStr));
            numStr.clear();
        }
    }
    if (!numStr.empty()) {
        params.push_back(std::stoi(numStr));
    }

    // Handle private mode sequences first
    if (isPrivateMode) {
        switch (cmd) {
            case 'h':
                if (isPrivateMode && !params.empty()) {
                    std::cerr << "Private mode set: " << params[0] << std::endl;
                    if (params[0] == 1049 || params[0] == 1047 || params[0] == 47) {
                        std::cerr << "Enabling alternate buffer via private mode" << std::endl;
                        useAlternateBuffer = true;
                    }
                }
                break;
            case 'l': // Disable mode
                if (!params.empty()) {
                    std::cerr << "Private mode handler: " << params[0] << " cmd: " << cmd << std::endl;
                    switch (params[0]) {
                        case 47:   // Original alternate buffer
                        case 1047: // Alternate screen buffer
                        case 1049: // Alternate screen buffer + clear
                            if (cmd == 'h') {
                                std::cerr << "Enabling alternate buffer mode" << std::endl;
                                clearRange(0, 0, bufferWidth - 1, bufferHeight - 1);
                                useAlternateBuffer = true;
                            } else {
                                std::cerr << "Disabling alternate buffer mode" << std::endl;
                                useAlternateBuffer = false;
                            }
                            break;
                    }
                }
                return;
        }
        return;
    }
    // Handle regular CSI sequences
    switch (cmd) {
        case 'H': // Cursor Position
        case 'f': // Alternative Cursor Position
            if (params.size() >= 2) {
                setCursorPos(params[1] - 1, params[0] - 1);
            } else {
                setCursorPos(0, 0);
            }
            break;
            
        case 'A': // Cursor Up
            cursorY = std::max(0, cursorY - (params.empty() ? 1 : params[0]));
            break;
            
        case 'B': // Cursor Down
            cursorY = std::min(bufferHeight - 1, cursorY + (params.empty() ? 1 : params[0]));
            break;
        case 'c': // Reset terminal
            if (seq[0] == '>') {  // Device Attributes request
                std::cerr << "Device attributes request - preserving state" << std::endl;
            } else {
                std::cerr << "Full terminal reset requested" << std::endl;
                ansiState = {};
                clearRange(0, 0, bufferWidth - 1, bufferHeight - 1);
                cursorX = cursorY = 0;
                scrollRegionTop = 0;
                scrollRegionBottom = bufferHeight - 1;
            }
            break;
        case 'C': // Cursor Forward
            cursorX = std::min(bufferWidth - 1, cursorX + (params.empty() ? 1 : params[0]));
            break;
            
        case 'D': // Cursor Back
            cursorX = std::max(0, cursorX - (params.empty() ? 1 : params[0]));
            break;
            
        case 'G': // Cursor horizontal absolute
            if (!params.empty()) {
                cursorX = std::min(bufferWidth - 1, std::max(0, params[0] - 1));
            } else {
                cursorX = 0;
            }
            break;
            

        case 'J': // Erase in Display
            if (params.empty() || params[0] == 2) {
                clearRange(0, 0, bufferWidth - 1, bufferHeight - 1);
                cursorX = 0;
                cursorY = 0;
            } else if (params[0] == 0) {
                // Clear from cursor to end of screen
                clearRange(cursorX, cursorY, bufferWidth - 1, cursorY);
                for (int y = cursorY + 1; y < bufferHeight; y++) {
                    clearRange(0, y, bufferWidth - 1, y);
                }
            } else if (params[0] == 1) {
                // Clear from beginning of screen to cursor
                for (int y = 0; y < cursorY; y++) {
                    clearRange(0, y, bufferWidth - 1, y);
                }
                clearRange(0, cursorY, cursorX, cursorY);
            }
            break;

        case 'K': // Erase in Line
            if (params.empty() || params[0] == 0) {
                // Clear from cursor to end of line
                clearRange(cursorX, cursorY, bufferWidth - 1, cursorY);
            } else if (params[0] == 1) {
                // Clear from beginning of line to cursor
                clearRange(0, cursorY, cursorX, cursorY);
            } else if (params[0] == 2) {
                // Clear entire line
                clearRange(0, cursorY, bufferWidth - 1, cursorY);
            }
            break;

        case 'S': // Scroll up
            if (params.empty()) params = {1};
            {
                int count = params[0];
                for (int y = 0; y < bufferHeight - count; y++) {
                    buffer[y] = std::move(buffer[y + count]);
                }
                for (int y = bufferHeight - count; y < bufferHeight; y++) {
                    buffer[y] = std::vector<Cell>(bufferWidth);
                }
            }
            break;

        case 'T': // Scroll down
            if (params.empty()) params = {1};
            {
                int count = params[0];
                for (int y = bufferHeight - 1; y >= count; y--) {
                    buffer[y] = std::move(buffer[y - count]);
                }
                for (int y = 0; y < count; y++) {
                    buffer[y] = std::vector<Cell>(bufferWidth);
                }
            }
            break;

        case 'P': // Delete characters
            if (params.empty() || params[0] == 0) params = {1};
            {
                int count = params[0];
                for (int x = cursorX; x < bufferWidth - count; x++) {
                    buffer[cursorY][x] = buffer[cursorY][x + count];
                }
                for (int x = bufferWidth - count; x < bufferWidth; x++) {
                    buffer[cursorY][x] = Cell{};
                }
            }
            break;

        case '@': // Insert characters
            if (params.empty() || params[0] == 0) params = {1};
            {
                int count = params[0];
                for (int x = bufferWidth - 1; x >= cursorX + count; x--) {
                    buffer[cursorY][x] = buffer[cursorY][x - count];
                }
                for (int x = cursorX; x < cursorX + count && x < bufferWidth; x++) {
                    buffer[cursorY][x] = Cell{};
                }
            }
            break;

        case 'L': // Insert lines
            if (params.empty() || params[0] == 0) params = {1};
            {
                int count = std::min(params[0], bufferHeight - cursorY);
                for (int y = bufferHeight - 1; y >= cursorY + count; y--) {
                    buffer[y] = buffer[y - count];
                }
                for (int y = cursorY; y < cursorY + count; y++) {
                    buffer[y] = std::vector<Cell>(bufferWidth);
                }
            }
            break;

        case 'M': // Delete lines
            if (params.empty() || params[0] == 0) params = {1};
            {
                int count = std::min(params[0], bufferHeight - cursorY);
                for (int y = cursorY; y < bufferHeight - count; y++) {
                    buffer[y] = buffer[y + count];
                }
                for (int y = bufferHeight - count; y < bufferHeight; y++) {
                    buffer[y] = std::vector<Cell>(bufferWidth);
                }
            }
            break;

            
        case 'u': // Restore cursor position
            cursorX = savedCursorX;
            cursorY = savedCursorY;
            break;

        case 'r': // Set scrolling region
            if (params.size() >= 2) {
                std::cerr << "Setting scroll region " << params[0] << " to " << params[1] << std::endl;
                // vim specific startup sequence
                if ((params[0] == 1 && params[1] == 22) || 
                    (params[0] == 1 && params[1] == 24)) {
                    std::cerr << "Detected vim startup sequence" << std::endl;
                    useAlternateBuffer = true;
                }
                scrollRegionTop = std::max(0, params[0] - 1);
                scrollRegionBottom = std::min(bufferHeight - 1, params[1] - 1);
            }
            break;
            
        case 'm': // SGR (Select Graphic Rendition)
            if (params.empty() || params[0] == 0) {
                ansiState.currentFg = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
                ansiState.currentBg = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
                ansiState.currentAttrs = 0;
                break;
            }
            for (int param : params) {
                switch (param) {
                    case 1: ansiState.currentAttrs |= 1; break;  // Bold
                    case 30: ansiState.currentFg = ImVec4(0.0f, 0.0f, 0.0f, 1.0f); break;
                    case 31: ansiState.currentFg = ImVec4(1.0f, 0.0f, 0.0f, 1.0f); break;
                    case 32: ansiState.currentFg = ImVec4(0.0f, 1.0f, 0.0f, 1.0f); break;
                    case 33: ansiState.currentFg = ImVec4(1.0f, 1.0f, 0.0f, 1.0f); break;
                    case 34: ansiState.currentFg = ImVec4(0.0f, 0.0f, 1.0f, 1.0f); break;
                    case 35: ansiState.currentFg = ImVec4(1.0f, 0.0f, 1.0f, 1.0f); break;
                    case 36: ansiState.currentFg = ImVec4(0.0f, 1.0f, 1.0f, 1.0f); break;
                    case 37: ansiState.currentFg = ImVec4(1.0f, 1.0f, 1.0f, 1.0f); break;
                    case 40: ansiState.currentBg = ImVec4(0.0f, 0.0f, 0.0f, 1.0f); break;
                    case 41: ansiState.currentBg = ImVec4(1.0f, 0.0f, 0.0f, 1.0f); break;
                    case 42: ansiState.currentBg = ImVec4(0.0f, 1.0f, 0.0f, 1.0f); break;
                    case 43: ansiState.currentBg = ImVec4(1.0f, 1.0f, 0.0f, 1.0f); break;
                    case 44: ansiState.currentBg = ImVec4(0.0f, 0.0f, 1.0f, 1.0f); break;
                    case 45: ansiState.currentBg = ImVec4(1.0f, 0.0f, 1.0f, 1.0f); break;
                    case 46: ansiState.currentBg = ImVec4(0.0f, 1.0f, 1.0f, 1.0f); break;
                    case 47: ansiState.currentBg = ImVec4(1.0f, 1.0f, 1.0f, 1.0f); break;
                }
            }
            break;
    }
}

void Terminal::handleEscapeSequence() {
    ansiState.inEscape = false;
    ansiState.currentSequence.clear();
}

void Terminal::setCursorPos(int x, int y) {
    cursorX = std::max(0, std::min(x, bufferWidth - 1));
    cursorY = std::max(0, std::min(y, bufferHeight - 1));
}

void Terminal::clearRange(int startX, int startY, int endX, int endY) {
    for (int y = startY; y <= endY; y++) {
        for (int x = (y == startY ? startX : 0); 
             x <= (y == endY ? endX : bufferWidth - 1); x++) {
            buffer[y][x] = Cell{};
        }
    }
}
void Terminal::processInput(const std::string& input) {
    if (ptyFd >= 0) {
        int promptLen = 0;
        for (int x = 0; x < bufferWidth; x++) {
            if (buffer[cursorY][x].ch == '$' || buffer[cursorY][x].ch == '#' || buffer[cursorY][x].ch == '>') {
                promptLen = x + 2;
                break;
            }
        }
        if (input == "\t") { // tab completions
            write(ptyFd, "\t", 1);  
            return;
        }
        if (input == "\x7f" && cursorX > promptLen) { // Backspace
            // Shift buffer left
            for (int x = cursorX - 1; x < lastLineLength - 1; x++) {
                buffer[cursorY][x] = buffer[cursorY][x + 1];
            }
            buffer[cursorY][lastLineLength - 1] = Cell{};
            lastLineLength--;
        }

        if (cursorX >= promptLen || input == "\r\n") {
            write(ptyFd, input.c_str(), input.length());
        }

        isTyping = true;
        typeIdleTime = 0.0f;
        
        if (autoScroll) {
            needsScroll = true;
        }
    }
}

void Terminal::toggleVisibility() { 
    isVisible = !isVisible; 
    if (isVisible) {
        needsFocus = true;
    }
}


void Terminal::updateTerminalSize() {
    if (ptyFd >= 0) {
        struct winsize ws = {};
        ws.ws_row = bufferHeight;
        ws.ws_col = bufferWidth;
        
        if (ioctl(ptyFd, TIOCSWINSZ, &ws) < 0) {
            std::cerr << "Failed to set terminal size" << std::endl;
        }
    }
}
void Terminal::render() {
    if (!isVisible) return;
    
    if (!shellStarted) {
        startShell();
    }
    float deltaTime = ImGui::GetIO().DeltaTime;
    if (isTyping) {
        typeIdleTime += deltaTime;
        if (typeIdleTime >= TYPE_TIMEOUT) {
            isTyping = false;
            typeIdleTime = 0.0f;
        }
    }


    // Create main window
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);
    if (!ImGui::Begin("Terminal", nullptr, 
        ImGuiWindowFlags_NoDecoration | 
        ImGuiWindowFlags_NoMove | 
        ImGuiWindowFlags_NoResize)) {
        ImGui::End();
        return;
    }

    // Create scrollable content area
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.1f, 0.1f, 0.1f, 1.0f));
    if (needsFocus) {
        ImGui::SetNextWindowFocus();
    }
    ImGui::BeginChild("TerminalContent", ImGui::GetContentRegionAvail(), true);
    if (needsScroll) {
        float windowHeight = ImGui::GetContentRegionAvail().y;
        float lineHeight = ImGui::GetTextLineHeight();
        int visibleLines = static_cast<int>(windowHeight / lineHeight);
        int totalLines = historyBuffer.size() + bufferHeight;
        maxScrollPosition = std::max(0.0f, static_cast<float>(totalLines - visibleLines));
        scrollPosition = maxScrollPosition;
        needsScroll = false;
    }
    // Handle scrolling
    if (ImGui::IsWindowHovered()) {
        float wheel = ImGui::GetIO().MouseWheel;
        if (wheel != 0) {
            // Only change autoScroll if we're not typing
            if (!isTyping) {
                // Disable auto-scroll when scrolling up
                if (wheel > 0) {
                    autoScroll = false;
                }
                
                // Update scroll position
                scrollPosition -= wheel * 3.0f;  // 3 lines per scroll tick
                
                // Calculate max scroll position
                float windowHeight = ImGui::GetContentRegionAvail().y;
                float lineHeight = ImGui::GetTextLineHeight();
                int visibleLines = static_cast<int>(windowHeight / lineHeight);
                int totalLines = historyBuffer.size() + bufferHeight;
                maxScrollPosition = std::max(0.0f, 
                    static_cast<float>(totalLines - visibleLines));
                
                // Clamp scroll position
                scrollPosition = std::max(0.0f, 
                    std::min(scrollPosition, maxScrollPosition));
                
                // Re-enable auto-scroll if we scroll to bottom
                if (scrollPosition >= maxScrollPosition) {
                    autoScroll = true;
                }
            }
        }
    }

    float maxScrollY = std::max(0.0f, bufferHeight * ImGui::GetTextLineHeight() - ImGui::GetWindowHeight());
    ImGui::SetScrollY(std::max(0.0f, std::min(ImGui::GetScrollY(), maxScrollY)));

    ImVec2 pos = ImGui::GetCursorScreenPos();
    float charWidth = ImGui::GetFont()->GetCharAdvance('M');
    float lineHeight = ImGui::GetTextLineHeight();

    // Handle mouse selection
    if (ImGui::IsWindowHovered()) {
        if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
            ImVec2 mousePos = ImGui::GetMousePos();
            int termX, termY;
            screenToTerminal(mousePos, pos, charWidth, lineHeight, &termX, &termY);
            selection.startX = selection.endX = termX;
            selection.startY = selection.endY = termY;
            selection.active = false; // Don't activate until we drag
        }
        else if (ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
            ImVec2 mousePos = ImGui::GetMousePos();
            int termX, termY;
            screenToTerminal(mousePos, pos, charWidth, lineHeight, &termX, &termY);
            selection.endX = termX;
            selection.endY = termY;
            
            // Only activate selection if we've moved more than one character
            int dx = abs(selection.endX - selection.startX);
            int dy = abs(selection.endY - selection.startY);
            if (dx > 1 || dy > 0) {
                selection.active = true;
            }
        }
        else if (ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
            // Clear selection if it wasn't activated
            if (!selection.active) {
                selection.startX = selection.endX = 0;
                selection.startY = selection.endY = 0;
            }
        }
    }

    renderBuffer(pos, charWidth, lineHeight);
    renderCursor(pos, charWidth, lineHeight);

    // Handle keyboard input
    if (ImGui::IsWindowFocused()) {
        ImGuiIO& io = ImGui::GetIO();
        
        for (int i = 0; i < io.InputQueueCharacters.Size; i++) {
            char c = (char)io.InputQueueCharacters[i];
            if (c != 0) {
                processInput(std::string(1, c));
            }
        }
        if (ImGui::IsKeyPressed(ImGuiKey_Tab)) {
            processInput("\t");
        }
        if (io.KeyCtrl) {
            if (ImGui::IsKeyPressed(ImGuiKey_C)) {
                if (selection.active) {
                    copySelection();
                    selection.active = false;
                } else {
                    processInput("\x03");  // Ctrl-C SIGINT
                }
            }
            else if (ImGui::IsKeyPressed(ImGuiKey_X)) {
                processInput("\x18");  // Ctrl-X for nano
            }
            else if (ImGui::IsKeyPressed(ImGuiKey_V)) {
                const char* clipText = ImGui::GetClipboardText();
                if (clipText) processInput(clipText);
            }
        }

        if (ImGui::IsKeyPressed(ImGuiKey_Enter)) processInput("\r\n");
        else if (ImGui::IsKeyPressed(ImGuiKey_Backspace)) processInput("\x7f");
        else if (ImGui::IsKeyPressed(ImGuiKey_UpArrow)) processInput("\x1b[A");
        else if (ImGui::IsKeyPressed(ImGuiKey_DownArrow)) processInput("\x1b[B");
        else if (ImGui::IsKeyPressed(ImGuiKey_RightArrow)) processInput("\x1b[C");
        else if (ImGui::IsKeyPressed(ImGuiKey_LeftArrow)) processInput("\x1b[D");
    }
    if (needsFocus && ImGui::IsWindowFocused(ImGuiFocusedFlags_ChildWindows)) {
        needsFocus = false;
    }
    ImGui::EndChild();
    ImGui::PopStyleColor();
    ImGui::End();

}
void Terminal::renderBuffer(const ImVec2& pos, float charWidth, float lineHeight) {
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    std::lock_guard<std::mutex> lock(bufferMutex);
    
    int visibleLines = static_cast<int>(ImGui::GetContentRegionAvail().y / lineHeight);
    int totalLines = historyBuffer.size() + bufferHeight;
    int startLine = static_cast<int>(scrollPosition);
    
    // Calculate selection bounds for highlighting
    int selStartX = std::min(selection.startX, selection.endX);
    int selEndX = std::max(selection.startX, selection.endX);
    int selStartY = std::min(selection.startY, selection.endY);
    int selEndY = std::max(selection.startY, selection.endY);
    
    for (int displayLine = 0; displayLine < visibleLines; displayLine++) {
        int sourceLine = startLine + displayLine;
        
        const std::vector<Cell>* lineToRender = nullptr;
        if (sourceLine < historyBuffer.size()) {
            lineToRender = &historyBuffer[sourceLine];
        } else if (sourceLine < totalLines) {
            int bufferLine = sourceLine - historyBuffer.size();
            if (bufferLine < bufferHeight) {
                lineToRender = &buffer[bufferLine];
            }
        }
        
        if (lineToRender) {
            float yOffset = displayLine * lineHeight;
            
            // Render selection highlight if this line is within selection
            if (selection.active && sourceLine >= selStartY && sourceLine <= selEndY) {
                int highlightStart = (sourceLine == selStartY) ? selStartX : 0;
                int highlightEnd = (sourceLine == selEndY) ? selEndX : bufferWidth - 1;
                
                ImVec2 highlightPos(
                    pos.x + highlightStart * charWidth,
                    pos.y + yOffset
                );
                ImVec2 highlightEndPos(
                    pos.x + (highlightEnd + 1) * charWidth,
                    pos.y + yOffset + lineHeight
                );
                
                drawList->AddRectFilled(
                    highlightPos,
                    highlightEndPos,
                    ImGui::ColorConvertFloat4ToU32(ImVec4(0.3f, 0.3f, 0.7f, 0.3f))
                );
            }

            // Render characters
            for (int x = 0; x < bufferWidth; x++) {
                const Cell& cell = (*lineToRender)[x];
                ImVec2 cellPos(pos.x + x * charWidth, pos.y + yOffset);
                
                if (cell.bg.w > 0.0f) {
                    drawList->AddRectFilled(
                        cellPos,
                        ImVec2(cellPos.x + charWidth, cellPos.y + lineHeight),
                        ImGui::ColorConvertFloat4ToU32(cell.bg)
                    );
                }
                
                if (cell.ch != ' ') {
                    char text[2] = {(char)cell.ch, 0};
                    drawList->AddText(cellPos, 
                        ImGui::ColorConvertFloat4ToU32(cell.fg), text);
                }
            }
        }
    }
}

void Terminal::renderCursor(const ImVec2& pos, float charWidth, float lineHeight) {
    // Don't render cursor if it's outside visible area
    int cursorBufferLine = cursorY;
    int visibleLines = static_cast<int>(ImGui::GetContentRegionAvail().y / lineHeight);
    int totalLines = historyBuffer.size() + bufferHeight;
    int startLine = static_cast<int>(scrollPosition);
    
    // Convert cursor position to screen space
    int cursorScreenLine = cursorBufferLine + historyBuffer.size() - startLine;
    
    // Only render if cursor is in visible area
    if (cursorScreenLine >= 0 && cursorScreenLine < visibleLines) {
        cursorBlinkTime += ImGui::GetIO().DeltaTime;
        float alpha = (sinf(cursorBlinkTime * 4.0f) + 1.0f) * 0.5f;
        
        ImDrawList* drawList = ImGui::GetWindowDrawList();
        ImVec2 cursorPos(
            pos.x + cursorX * charWidth,
            pos.y + cursorScreenLine * lineHeight
        );
        
        drawList->AddRectFilled(
            cursorPos,
            ImVec2(cursorPos.x + charWidth, cursorPos.y + lineHeight),
            ImGui::ColorConvertFloat4ToU32(ImVec4(1.0f, 1.0f, 1.0f, alpha * 0.5f))
        );
    }
}
void Terminal::screenToTerminal(const ImVec2& screenPos, const ImVec2& terminalPos,
                         float charWidth, float lineHeight,
                         int* termX, int* termY) {
    *termX = static_cast<int>((screenPos.x - terminalPos.x) / charWidth);
    int displayY = static_cast<int>((screenPos.y - terminalPos.y) / lineHeight);
    
    // Convert display Y to buffer position including scroll and history
    *termY = static_cast<int>(scrollPosition) + displayY;
    
    // Clamp to valid range
    *termX = std::max(0, std::min(*termX, bufferWidth - 1));
    *termY = std::max(0, std::min(*termY, static_cast<int>(historyBuffer.size()) + bufferHeight - 1));
}


void Terminal::copySelection() {
    if (!selection.active) return;
    
    std::string selectedText;
    int startX = std::min(selection.startX, selection.endX);
    int endX = std::max(selection.startX, selection.endX);
    int startY = std::min(selection.startY, selection.endY);
    int endY = std::max(selection.startY, selection.endY);

    for (int y = startY; y <= endY; y++) {
        if (y > startY) selectedText += '\n';
        
        const std::vector<Cell>* line;
        if (y < historyBuffer.size()) {
            line = &historyBuffer[y];
        } else {
            int bufferY = y - historyBuffer.size();
            if (bufferY >= bufferHeight) continue;
            line = &buffer[bufferY];
        }
        
        for (int x = (y == startY ? startX : 0);
             x <= (y == endY ? endX : bufferWidth - 1); x++) {
            selectedText += static_cast<char>((*line)[x].ch);
        }
    }
    
    // Trim trailing whitespace from each line
    size_t pos = selectedText.find_last_not_of(" \t\n");
    if (pos != std::string::npos) {
        selectedText = selectedText.substr(0, pos + 1);
    }
    
    ImGui::SetClipboardText(selectedText.c_str());
}


void Terminal::setWorkingDirectory(const std::string& path) {
    if(set_dir == false){
        if (!path.empty()) {
            processInput("cd " + path + "\n");
        }
        set_dir = true;
    }

}