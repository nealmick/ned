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

void Terminal::copySelection() {
    if (!selection.active) return;
    
    std::string selectedText;
    int startX = std::min(selection.startX, selection.endX);
    int endX = std::max(selection.startX, selection.endX);
    int startY = std::min(selection.startY, selection.endY);
    int endY = std::max(selection.startY, selection.endY);
    
    for (int y = startY; y <= endY; y++) {
        if (y > startY) selectedText += '\n';
        for (int x = (y == startY ? startX : 0); 
             x <= (y == endY ? endX : bufferWidth - 1); x++) {
            selectedText += static_cast<char>(buffer[y][x].ch);
        }
    }
    
    ImGui::SetClipboardText(selectedText.c_str());
}

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
        tios.c_lflag = ICANON | ISIG | IEXTEN | ECHO | ECHOE | ECHOK | ECHOCTL | ECHOKE;

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
            } else {
                handleEscapeSequence();
                ansiState.inEscape = false;
            }
            continue;
        }

        if (data[i] == '\033') {
            ansiState.inEscape = true;
            continue;
        }

        if (data[i] == '\x08') {  // Backspace
            if (cursorX > 0) {
                cursorX--;
            }
            continue;
        }

        if (data[i] >= 32) {  // Printable characters
            buffer[cursorY][cursorX].ch = data[i];
            buffer[cursorY][cursorX].fg = ansiState.currentFg;
            buffer[cursorY][cursorX].bg = ansiState.currentBg;
            buffer[cursorY][cursorX].attrs = ansiState.currentAttrs;
            cursorX++;
            
            // Update lastLineLength if we're extending the line
            if (cursorX > lastLineLength) {
                lastLineLength = cursorX;
            }
        } else {
            processChar(data[i]);  // Handle other control characters
        }
    }
    
    if (autoScroll) {
        needsScroll = true;
    }
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
        if (cursorY >= bufferHeight) {
            scrollBuffer(1);
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
                scrollBuffer(1);
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
                scrollBuffer(1);
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
    
    char cmd = seq.back();
    std::vector<int> params;
    
    size_t start = 0;
    size_t end = 0;
    
    // Safely parse parameters
    while ((end = seq.find(';', start)) != std::string::npos) {
        std::string paramStr = seq.substr(start, end - start);
        try {
            if (paramStr.empty()) {
                params.push_back(0);  // Default value for empty parameter
            } else {
                params.push_back(std::stoi(paramStr));
            }
        } catch (const std::exception&) {
            params.push_back(0);  // Default value on conversion error
        }
        start = end + 1;
    }
    
    // Handle the last parameter before the command character
    if (start < seq.length() - 1) {
        std::string paramStr = seq.substr(start, seq.length() - 1 - start);
        try {
            if (paramStr.empty()) {
                params.push_back(0);
            } else {
                params.push_back(std::stoi(paramStr));
            }
        } catch (const std::exception&) {
            params.push_back(0);
        }
    }
    
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
        case 'C': // Cursor Forward
            cursorX = std::min(bufferWidth - 1, cursorX + (params.empty() ? 1 : params[0]));
            break;
        case 'D': // Cursor Back
            if (cursorX > 0) {
                cursorX = std::max(0, cursorX - (params.empty() ? 1 : params[0]));
            }
            break;
        case 'J': // Erase in Display
            if (params.empty() || params[0] == 2) {
                clearRange(0, 0, bufferWidth - 1, bufferHeight - 1);
            } else if (params[0] == 0) {
                // Clear from cursor to end of screen
                clearRange(cursorX, cursorY, bufferWidth - 1, cursorY);
                if (cursorY < bufferHeight - 1) {
                    clearRange(0, cursorY + 1, bufferWidth - 1, bufferHeight - 1);
                }
            } else if (params[0] == 1) {
                // Clear from start to cursor
                clearRange(0, 0, bufferWidth - 1, cursorY - 1);
                clearRange(0, cursorY, cursorX, cursorY);
            }
            break;
        case 'K': // Erase in Line
            if (params.empty() || params[0] == 0) {
                clearRange(cursorX, cursorY, bufferWidth - 1, cursorY); // Clear to end of line
            } else if (params[0] == 1) {
                clearRange(0, cursorY, cursorX, cursorY); // Clear to start of line
            } else if (params[0] == 2) {
                clearRange(0, cursorY, bufferWidth - 1, cursorY); // Clear entire line
            }
            break;
        case 'm': // Select Graphic Rendition (SGR)
            if (params.empty()) {
                ansiState.currentFg = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
                ansiState.currentBg = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
                ansiState.currentAttrs = 0;
            }
            for (int param : params) {
                switch (param) {
                    case 0: // Reset
                        ansiState.currentFg = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
                        ansiState.currentBg = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
                        ansiState.currentAttrs = 0;
                        break;
                    case 1: // Bold
                        ansiState.currentAttrs |= 1;
                        break;
                    case 31: // Red foreground
                        ansiState.currentFg = ImVec4(1.0f, 0.0f, 0.0f, 1.0f);
                        break;
                    case 32: // Green foreground
                        ansiState.currentFg = ImVec4(0.0f, 1.0f, 0.0f, 1.0f);
                        break;
                    case 33: // Yellow foreground
                        ansiState.currentFg = ImVec4(1.0f, 1.0f, 0.0f, 1.0f);
                        break;
                    case 34: // Blue foreground
                        ansiState.currentFg = ImVec4(0.0f, 0.0f, 1.0f, 1.0f);
                        break;
                    case 35: // Magenta foreground
                        ansiState.currentFg = ImVec4(1.0f, 0.0f, 1.0f, 1.0f);
                        break;
                    case 36: // Cyan foreground
                        ansiState.currentFg = ImVec4(0.0f, 1.0f, 1.0f, 1.0f);
                        break;
                    case 37: // White foreground
                        ansiState.currentFg = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
                        break;
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
        // Count the prompt length (e.g. "neal@computer:~/$ ")
        int promptLen = 0;
        for (int x = 0; x < bufferWidth; x++) {
            if (buffer[cursorY][x].ch == '$' || buffer[cursorY][x].ch == '#' || buffer[cursorY][x].ch == '>') {
                promptLen = x + 2;  // +2 to account for the symbol and space after it
                break;
            }
        }

        // Just send input to PTY and let writeToBuffer handle cursor movement
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
            selection.active = true;
            selection.startX = selection.endX = termX;
            selection.startY = selection.endY = termY;
        }
        else if (ImGui::IsMouseDown(ImGuiMouseButton_Left) && selection.active) {
            ImVec2 mousePos = ImGui::GetMousePos();
            int termX, termY;
            screenToTerminal(mousePos, pos, charWidth, lineHeight, &termX, &termY);
            selection.endX = termX;
            selection.endY = termY;
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
        
        if (io.KeyCtrl) {
            if (ImGui::IsKeyPressed(ImGuiKey_C)) {
                if (selection.active) {
                    copySelection();
                    selection.active = false;
                } else {
                    processInput("\x03");
                }
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
    
    // Calculate which lines to display
    for (int displayLine = 0; displayLine < visibleLines; displayLine++) {
        int sourceLine = startLine + displayLine;
        
        // Get the line of cells to render
        const std::vector<Cell>* lineToRender = nullptr;
        if (sourceLine < historyBuffer.size()) {
            // Render from history buffer
            lineToRender = &historyBuffer[sourceLine];
        } else if (sourceLine < totalLines) {
            // Render from current buffer
            int bufferLine = sourceLine - historyBuffer.size();
            if (bufferLine < bufferHeight) {
                lineToRender = &buffer[bufferLine];
            }
        }
        
        if (lineToRender) {
            float yOffset = displayLine * lineHeight;
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
    // Convert screen coordinates to terminal coordinates
    *termX = static_cast<int>((screenPos.x - terminalPos.x) / charWidth);
    *termY = static_cast<int>((screenPos.y - terminalPos.y) / lineHeight);
    
    // Clamp to terminal bounds
    *termX = std::max(0, std::min(*termX, bufferWidth - 1));
    *termY = std::max(0, std::min(*termY, bufferHeight - 1));
}

void Terminal::setWorkingDirectory(const std::string& path) {
    if (!path.empty()) {
        processInput("cd " + path + "\n");
    }
}