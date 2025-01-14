#include "util/terminal.h"
#include "util/settings.h"
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
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
    shellStarted = true;
    std::cerr << "\033[38;5;21mTerminal:\033[0m  starting shell" << std::endl;

    // Open PTY master
    ptyFd = posix_openpt(O_RDWR);
    grantpt(ptyFd);
    unlockpt(ptyFd);
    
    char* slaveName = ptsname(ptyFd);
   
    childPid = fork();


    if (childPid == 0) {  // Child process
        setsid();// Create new session and process group
        close(ptyFd);  // Close master side

        // Open slave PTY
        int slaveFd = open(slaveName, O_RDWR);
        // Make it our controlling terminal
        ioctl(slaveFd, TIOCSCTTY, 0);
        // Set up standard file descriptors
        dup2(slaveFd, STDIN_FILENO);
        dup2(slaveFd, STDOUT_FILENO);
        dup2(slaveFd, STDERR_FILENO);

        if (slaveFd > STDERR_FILENO) {
            close(slaveFd);
        }

        // Configure terminal modes
        struct termios tios;
        tcgetattr(STDIN_FILENO, &tios);

        tios.c_iflag = ICRNL | IXON | IXANY | IMAXBEL | BRKINT;
        tios.c_oflag = OPOST | ONLCR;
        tios.c_cflag = CREAD | CS8 | HUPCL;
        tios.c_lflag = ICANON | ISIG | IEXTEN | ECHO | ECHOE | ECHOK | ECHOCTL | ECHOKE;

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
            exit(1);
        }

        // Set window size
        struct winsize ws = {};
        ws.ws_row = bufferHeight;
        ws.ws_col = bufferWidth;
        
        if (ioctl(STDIN_FILENO, TIOCSWINSZ, &ws) < 0) {
            char msg[] = "Failed to set terminal size\n";
            write(STDERR_FILENO, msg, strlen(msg));
        } else {
            char msg[100];
            snprintf(msg, sizeof(msg), "Set terminal size to: %dx%d\n", ws.ws_row, ws.ws_col);
            write(STDERR_FILENO, msg, strlen(msg));
        }

        setenv("TERM", "xterm-256color", 1);
        
        const char* shell = getenv("SHELL");
        if (!shell) shell = "/bin/bash";
        
        char* const args[] = {(char*)shell, NULL};
        execvp(shell, args);
        char msg[] = "Failed to execute shell\n";
        write(STDERR_FILENO, msg, strlen(msg));
        exit(1);
    }

    // Parent process - just start the read thread
    readThread = std::thread(&Terminal::readOutput, this);
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
       ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(
        gSettings.getSettings()["backgroundColor"][0].get<float>(),
        gSettings.getSettings()["backgroundColor"][1].get<float>(),
        gSettings.getSettings()["backgroundColor"][2].get<float>(),
        0.0f  // Force alpha to 1.0 (fully opaque)
    ));

    if (needsFocus) {
        ImGui::SetNextWindowFocus();
    }
    ImGui::BeginChild("TerminalContent", ImGui::GetContentRegionAvail(), false);
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
                float oldScrollPos = scrollPosition;  // Store old position
                // Update scroll position
                scrollPosition -= wheel * 3.0f;  // 3 lines per scroll tick
                
                // Calculate max scroll position
                float windowHeight = ImGui::GetContentRegionAvail().y;
                float lineHeight = ImGui::GetTextLineHeight();
                int visibleLines = static_cast<int>(windowHeight / lineHeight);
                int totalLines = historyBuffer.size() + bufferHeight;
                maxScrollPosition = std::max(0.0f, static_cast<float>(totalLines - visibleLines));
                
                // Clamp scroll position
                scrollPosition = std::max(0.0f, std::min(scrollPosition, maxScrollPosition));
                
               
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
        if (ImGui::IsKeyPressed(ImGuiKey_Escape)) {
            processInput("\033");  // \033 is the ASCII/ANSI escape character
        }
        if (ImGui::IsKeyPressed(ImGuiKey_Tab)) {
            processInput("\t");
        } 
        if (io.KeyCtrl) {
            if (ImGui::IsKeyPressed(ImGuiKey_A)) {
                std::cerr << "\033[38;5;21mTerminal:\033[0m sending ctrl a  " <<std::endl;
                // Send Ctrl-A to screen (\x01)
                processInput("\x01");
            }
            else if (ImGui::IsKeyPressed(ImGuiKey_C)) {
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
            // Add other Ctrl key combinations that screen uses
            else {
                // Forward any other Ctrl key combinations
                for (int key = ImGuiKey_A; key <= ImGuiKey_Z; key++) {
                    if (ImGui::IsKeyPressed((ImGuiKey)key)) {
                        char ctrlChar = (char)((key - ImGuiKey_A) + 1);  // Convert to Ctrl code (1-26)
                        processInput(std::string(1, ctrlChar));
                    }
                }
            }
        }
        if (ImGui::IsKeyPressed(ImGuiKey_Enter)) processInput("\r\n");
        else if (ImGui::IsKeyPressed(ImGuiKey_Backspace)) processInput("\x7f");
        else if (ImGui::IsKeyPressed(ImGuiKey_UpArrow)) processInput("\033OA");
        else if (ImGui::IsKeyPressed(ImGuiKey_DownArrow)) processInput("\033OB");
        else if (ImGui::IsKeyPressed(ImGuiKey_RightArrow)) processInput("\033OC");
        else if (ImGui::IsKeyPressed(ImGuiKey_LeftArrow)) processInput("\033OD");
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
                    ImGui::ColorConvertFloat4ToU32(ImVec4(1.0f, 0.1f, 0.7f, 0.3f))  // Green with 50% alpha
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
        
        if (cursorY >= bufferHeight - 1) {
            scrollBuffer(1);
            needScroll = true;
        } else {
            cursorY++;
        }

        if (cursorY > scrollRegionBottom) {
            for (int y = scrollRegionTop; y < scrollRegionBottom; y++) {
                buffer[y] = std::move(buffer[y + 1]);
            }
            buffer[scrollRegionBottom] = std::vector<Cell>(bufferWidth);
            cursorY = scrollRegionBottom;
        }
        
        promptEndX = 0;
        promptEndY = cursorY;
    }else if (c == '\t') {
        int newX = (cursorX + 8) & ~7;
        if (newX >= bufferWidth) {
            cursorX = 0;
            lastLineLength = 0;
            cursorY++;
            // Only scroll if no scroll region is active
            if (scrollRegionBottom == 0 && scrollRegionTop == 0 &&
                cursorY >= bufferHeight - 1) {
                scrollBuffer(1);
                cursorY = bufferHeight - 2;
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
            // Only scroll if no scroll region is active
            if (scrollRegionBottom == 0 && scrollRegionTop == 0 &&
                cursorY >= bufferHeight - 1) {
                scrollBuffer(1);
                cursorY = bufferHeight - 2;
                needScroll = true;
            }
        }

        buffer[cursorY][cursorX].ch = c;
        buffer[cursorY][cursorX].fg = ansiState.currentFg;
        buffer[cursorY][cursorX].bg = ansiState.currentBg;
        buffer[cursorY][cursorX].attrs = ansiState.currentAttrs;
        cursorX++;
        lastLineLength = std::max(lastLineLength, cursorX);
        
        if (!isTyping) {
            promptEndX = cursorX;
            promptEndY = cursorY;
        }
    }
    
    if (needScroll && autoScroll) {
        needsScroll = true;
    }
}

void Terminal::processInput(const std::string& input) { 
     std::cerr << "\033[38;5;21mTerminal:\033[0m input :  " << input <<std::endl;

    if (ptyFd >= 0) {
        // Debug PTY state before write
        struct stat statbuf;
        int fstat_result = fstat(ptyFd, &statbuf);

        // Also check PTY state
        int error = 0;
        socklen_t len = sizeof(error);
        int retval = getsockopt(ptyFd, SOL_SOCKET, SO_ERROR, &error, &len);

        // Check if child process is still alive
        int status;
        pid_t result = waitpid(childPid, &status, WNOHANG);
        

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
            ssize_t bytes = write(ptyFd, input.c_str(), input.length());
            
        }else{
            ssize_t bytes = write(ptyFd, input.c_str(), input.length());
        }
        isTyping = true;
        typeIdleTime = 0.0f;
        
        if (autoScroll) {
            needsScroll = true;
        }
    } else {
        std::cerr << "\033[38;5;21mTerminal:\033[0m PTY not open (fd < 0)" << std::endl;

    }
}
void Terminal::handleCSI(const std::string& seq) {
    if (seq.empty()) return;
    
    
    char cmd = seq.back();
    std::vector<int> params;
    
    // Extract numeric parameters
    std::string numStr;
    bool isPrivateMode = false;
    
    std::string paramStr = seq.substr(0, seq.length() - 1);  // Get everything except the command char
    
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
    bool handled = true;  
    switch (cmd) {
        case 'r': // Set scrolling region
            if (params.size() >= 2) {
                int top = std::max(0, params[0] - 1);
                int bottom = std::min(bufferHeight - 1, params[1] - 1);
                scrollRegionTop = top;
                scrollRegionBottom = bottom;

            }
            break;
        
        case 'H': // Cursor Position
            if (params.size() >= 2) {
                int row = params[0] - 1;
                int col = params[1] - 1;
                
                // Update the cursor position based on Vim's request, but clamp to the buffer dimensions
                setCursorPos(col, row);
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
            cursorX = std::max(0, cursorX - (params.empty() ? 1 : params[0]));
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
                //std::cerr << "Terminal: Clear line at Y=" << cursorY << " mode=0" << std::endl;
                clearRange(cursorX, cursorY, bufferWidth - 1, cursorY);
            }break;

        

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
            std::cerr << "Terminal: inserting lines at y=" << cursorY << std::endl;
            if (params.empty() || params[0] == 0) params = {1};
            {
                // Only insert within scroll region if defined
                int effectiveBottom = (scrollRegionBottom > scrollRegionTop) ? 
                    scrollRegionBottom : (bufferHeight - 1);
                int effectiveTop = (scrollRegionBottom > scrollRegionTop) ?
                    scrollRegionTop : 0;
                
                // Only proceed if cursor is within scroll region
                if (cursorY >= effectiveTop && cursorY <= effectiveBottom) {
                    int count = std::min(params[0], effectiveBottom - cursorY + 1);
                    
                    // Move lines down within the scroll region
                    for (int y = effectiveBottom; y >= cursorY + count; y--) {
                        buffer[y] = buffer[y - count];
                    }
                    
                    // Clear the newly inserted lines
                    for (int y = cursorY; y < cursorY + count && y <= effectiveBottom; y++) {
                        buffer[y] = std::vector<Cell>(bufferWidth);
                    }
                }
            }
            break;

        case 'M': // Delete lines
            {
                std::cerr << "Terminal: deleteing..." << std::endl;
                if (params.empty() || params[0] == 0) params = {1};
                
                // Only delete within scroll region if defined
                int effectiveBottom = (scrollRegionBottom > scrollRegionTop) ? 
                    scrollRegionBottom : (bufferHeight - 1);
                int effectiveTop = (scrollRegionBottom > scrollRegionTop) ?
                    scrollRegionTop : 0;
                
                // Only proceed if cursor is within scroll region
                if (cursorY >= effectiveTop && cursorY <= effectiveBottom) {
                    int count = std::min(params[0], effectiveBottom - cursorY + 1);
                    
                    // Move lines up within the scroll region
                    for (int y = cursorY; y <= effectiveBottom - count; y++) {
                        buffer[y] = std::move(buffer[y + count]);
                    }
                    
                    // Clear the newly exposed lines at the bottom of the region
                    for (int y = effectiveBottom - count + 1; y <= effectiveBottom; y++) {
                        buffer[y] = std::vector<Cell>(bufferWidth);
                    }
                } else {
                    std::cerr << "Terminal: ignored delete lines - cursor outside scroll region" << std::endl;
                }
            }
            break;
        case 'd': // Move to line N
            if (!params.empty()) {
                cursorY = std::max(0, std::min(params[0] - 1, bufferHeight - 1));
            }
            break;

        case 'G': // Move to column N
            if (!params.empty()) {
                cursorX = std::max(0, std::min(params[0] - 1, bufferWidth - 1));
            }
            break;

        case 'h': // Set mode
        case 'l': // Reset mode
            if (isPrivateMode) {
                if (!params.empty()) {
                    bool enable = (cmd == 'h');  // 'h' enables, 'l' disables
                    switch (params[0]) {
                        case 1:
                            // Application cursor keys mode
                            ansiState.applicationCursorKeys = enable;
                            break;
                        case 7:
                            // Line wrap mode
                            ansiState.lineWrap = enable;
                            break;
                        case 12:
                            // Cursor blink
                            ansiState.cursorBlink = enable;
                            break;
                        case 25:
                            // Show/hide cursor
                            ansiState.cursorVisible = enable;
                            break;
                        case 1049:
                            // Alternative screen buffer
                            if (enable) {
                                // Save current buffer state
                                altBuffer = buffer;
                                buffer = std::vector<std::vector<Cell>>(bufferHeight, 
                                        std::vector<Cell>(bufferWidth));
                            } else {
                                // Restore main buffer
                                buffer = altBuffer;
                            }
                            break;
                        case 2004:
                            // Bracketed paste mode
                            ansiState.bracketedPaste = enable;
                            break;
                        default:
                            std::cerr << "Terminal: Unknown private mode: " << params[0] 
                                     << (enable ? "h" : "l") << std::endl;
                            break;
                    }
                }
            } else {
                if (!params.empty()) {
                    switch(params[0]) {
                        case 4:  // Reset Insert Mode/Set Replace Mode
                            // We should probably track this in ansiState
                            ansiState.insertMode = false;
                            break;
                        default:
                            std::cerr << "Terminal: Non-private mode: " << paramStr << cmd << std::endl;
                            break;
                    }
                }
            }
            break;
        case 'm':  // SGR
            std::cerr << "Terminal: Color sequence: ";
            for (int p : params) {
                std::cerr << p << " ";
            }
            std::cerr << std::endl;
            if (params.empty() || params[0] == 0) {
                ansiState.currentFg = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);  // Reset to white
                ansiState.currentBg = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);  // Reset to transparent black
                ansiState.currentAttrs = 0;
                break;
            }
            for (size_t i = 0; i < params.size(); i++) {
                int param = params[i];
                switch (param) {
                    case 0:  // Reset all
                        ansiState.currentFg = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
                        ansiState.currentBg = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
                        ansiState.currentAttrs = 0;
                        break;
                    case 1: ansiState.currentAttrs |= 1; break;  // Bold
                    case 2: ansiState.currentAttrs |= 2; break;  // Dim
                    case 3: ansiState.currentAttrs |= 4; break;  // Italic
                    case 4: ansiState.currentAttrs |= 8; break;  // Underline
                    case 7: // Reverse video
                        {
                            ImVec4 temp = ansiState.currentFg;
                            ansiState.currentFg = ansiState.currentBg;
                            ansiState.currentBg = temp;
                        }
                        break;
                    case 39: // Default foreground
                        ansiState.currentFg = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
                        break;
                    case 49: // Default background
                        ansiState.currentBg = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
                        break;
                    
                    // Standard colors (30-37)
                    case 30: ansiState.currentFg = ImVec4(0.0f, 0.0f, 0.0f, 1.0f); break;     // Black
                    case 31: ansiState.currentFg = ImVec4(0.8f, 0.0f, 0.0f, 1.0f); break;     // Red
                    case 32: ansiState.currentFg = ImVec4(0.0f, 0.8f, 0.0f, 1.0f); break;     // Green
                    case 33: ansiState.currentFg = ImVec4(0.8f, 0.8f, 0.0f, 1.0f); break;     // Yellow
                    case 34: ansiState.currentFg = ImVec4(0.0f, 0.0f, 0.8f, 1.0f); break;     // Blue
                    case 35: ansiState.currentFg = ImVec4(0.8f, 0.0f, 0.8f, 1.0f); break;     // Magenta
                    case 36: ansiState.currentFg = ImVec4(0.0f, 0.8f, 0.8f, 1.0f); break;     // Cyan
                    case 37: ansiState.currentFg = ImVec4(0.8f, 0.8f, 0.8f, 1.0f); break;     // White

                    // Bright colors (90-97)
                    case 90: ansiState.currentFg = ImVec4(0.4f, 0.4f, 0.4f, 1.0f); break;     // Bright Black
                    case 91: ansiState.currentFg = ImVec4(1.0f, 0.0f, 0.0f, 1.0f); break;     // Bright Red
                    case 92: ansiState.currentFg = ImVec4(0.0f, 1.0f, 0.0f, 1.0f); break;     // Bright Green
                    case 93: ansiState.currentFg = ImVec4(1.0f, 1.0f, 0.0f, 1.0f); break;     // Bright Yellow
                    case 94: ansiState.currentFg = ImVec4(0.0f, 0.0f, 1.0f, 1.0f); break;     // Bright Blue
                    case 95: ansiState.currentFg = ImVec4(1.0f, 0.0f, 1.0f, 1.0f); break;     // Bright Magenta
                    case 96: ansiState.currentFg = ImVec4(0.0f, 1.0f, 1.0f, 1.0f); break;     // Bright Cyan
                    case 97: ansiState.currentFg = ImVec4(1.0f, 1.0f, 1.0f, 1.0f); break;     // Bright White

                    // Background colors (40-47)
                    case 40: ansiState.currentBg = ImVec4(0.0f, 0.0f, 0.0f, 1.0f); break;     // Black
                    case 41: ansiState.currentBg = ImVec4(0.8f, 0.0f, 0.0f, 1.0f); break;     // Red
                    case 42: ansiState.currentBg = ImVec4(0.0f, 0.8f, 0.0f, 1.0f); break;     // Green
                    case 43: ansiState.currentBg = ImVec4(0.8f, 0.8f, 0.0f, 1.0f); break;     // Yellow
                    case 44: ansiState.currentBg = ImVec4(0.0f, 0.0f, 0.8f, 1.0f); break;     // Blue
                    case 45: ansiState.currentBg = ImVec4(0.8f, 0.0f, 0.8f, 1.0f); break;     // Magenta
                    case 46: ansiState.currentBg = ImVec4(0.0f, 0.8f, 0.8f, 1.0f); break;     // Cyan
                    case 47: ansiState.currentBg = ImVec4(0.8f, 0.8f, 0.8f, 1.0f); break;     // White

                    // 256 color support
                    case 38: // Foreground
                        if (i + 2 < params.size() && params[i + 1] == 5) {
                            int color = params[i + 2];
                            ansiState.currentFg = convert256ToRGB(color);
                            i += 2; // Skip the next two parameters
                        }
                        break;
                    case 48: // Background
                        if (i + 2 < params.size() && params[i + 1] == 5) {
                            int color = params[i + 2];
                            ansiState.currentBg = convert256ToRGB(color);
                            i += 2; // Skip the next two parameters
                        }
                        break;
                }
            }
            break;
        default:
            handled = false;
            break;

    }
     if (!handled) {
        std::cerr << "Terminal: Unknown CSI sequence: ESC[" 
                  << (isPrivateMode ? "?" : "")
                  << paramStr << cmd 
                  << " (hex: " << std::hex << (int)cmd << std::dec << ")"
                  << std::endl;
    }
    
}


void Terminal::setCursorPos(int x, int y) {
    //std::cerr << "Terminal: Set Cursor Pos x"<< x<<" y"<<y << std::endl;
    cursorX = std::max(0, std::min(x, bufferWidth - 1));
    cursorY = std::max(0, std::min(y, bufferHeight - 1));
}


void Terminal::clearRange(int startX, int startY, int endX, int endY) {
    //std::cerr << "Terminal: Clearing Range " << std::endl;
    for (int y = startY; y <= endY; y++) {
        for (int x = (y == startY ? startX : 0); 
             x <= (y == endY ? endX : bufferWidth - 1); x++) {
            buffer[y][x] = Cell{};
        }
    }
}


void Terminal::toggleVisibility() {
    isVisible = !isVisible; 
    std::cerr << "\033[38;5;21mTerminal:\033[0m toggle Visibility " << isVisible <<std::endl;
    if (isVisible) {
        needsFocus = true;
    }
}

void Terminal::renderCursor(const ImVec2& pos, float charWidth, float lineHeight) {
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


ImVec4 Terminal::convert256ToRGB(int color) {
    // Handle standard colors (0-15)
    if (color < 16) {
        bool bright = color > 7;
        float intensity = bright ? 1.0f : 0.8f;
        color = color % 8;
        
        switch (color) {
            case 0: return ImVec4(0.0f, 0.0f, 0.0f, 1.0f);  // Black
            case 1: return ImVec4(intensity, 0.0f, 0.0f, 1.0f);  // Red
            case 2: return ImVec4(0.0f, intensity, 0.0f, 1.0f);  // Green
            case 3: return ImVec4(intensity, intensity, 0.0f, 1.0f);  // Yellow
            case 4: return ImVec4(0.0f, 0.0f, intensity, 1.0f);  // Blue
            case 5: return ImVec4(intensity, 0.0f, intensity, 1.0f);  // Magenta
            case 6: return ImVec4(0.0f, intensity, intensity, 1.0f);  // Cyan
            case 7: return ImVec4(intensity, intensity, intensity, 1.0f);  // White
        }
    }
    
    // Handle 216 colors (16-231)
    if (color < 232) {
        int adjusted = color - 16;
        float r = (adjusted / 36) * (1.0f / 5.0f);
        float g = ((adjusted / 6) % 6) * (1.0f / 5.0f);
        float b = (adjusted % 6) * (1.0f / 5.0f);
        return ImVec4(r, g, b, 1.0f);
    }
    
    // Handle grayscale (232-255)
    float gray = (color - 232) * (1.0f / 23.0f);
    return ImVec4(gray, gray, gray, 1.0f);
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