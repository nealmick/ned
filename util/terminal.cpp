#include "util/terminal.h"
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>
#include <regex>

Terminal gTerminal;

Terminal::Terminal() : isVisible(false), ptyFd(-1), childPid(-1), 
                      shouldTerminate(false), shellStarted(false) {
    memset(inputBuffer, 0, INPUT_BUFFER_SIZE);
    initScreenBuffer();  
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
std::string Terminal::stripAnsiCodes(const std::string& input) {
    static const std::regex ansiPattern(
        "\033\\["                // ESC[
        "[\\d;]*"               // Zero or more digits or semicolons
        "[A-Za-z]"              // Ending with any letter
    );
    
    std::string result = std::regex_replace(input, ansiPattern, "");
    
    // Process carriage returns properly
    std::string processedResult;
    size_t lineStart = 0;
    for (size_t i = 0; i < result.size(); ++i) {
        if (result[i] == '\r') {
            if (i + 1 < result.size() && result[i + 1] != '\n') {
                processedResult.erase(processedResult.find_last_of('\n') + 1);
            }
            continue;
        }
        processedResult += result[i];
    }
    
    // Only remove certain control characters
    processedResult.erase(
        std::remove_if(processedResult.begin(), processedResult.end(),
            [](char c) { 
                // Keep important control chars
                if (c == '\n' || c == '\t' || c >= 32) return false;
                return true;
            }),
        processedResult.end()
    );
    
    return processedResult;
}

void Terminal::initScreenBuffer() {
    screenBuffer.resize(bufferHeight);
    for (auto& row : screenBuffer) {
        row.resize(bufferWidth, ScreenCell());
    }
    cursorX = 0;
    cursorY = 0;
}
void Terminal::writeToScreenBuffer(const char* data, size_t length) {
    ImVec4 currentFg = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
    ImVec4 currentBg = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
    bool currentBold = false;
    bool currentItalic = false;

    for (size_t i = 0; i < length; i++) {
        // Check for ANSI escape sequence
        if (data[i] == '\033' && i + 1 < length && data[i + 1] == '[') {
            i += 2;  // Skip ESC[
            std::string params;
            // Collect parameters
            while (i < length && !isalpha(data[i])) {
                params += data[i++];
            }
            if (i < length) {
                char cmd = data[i];
                // Parse parameters
                std::vector<int> nums;
                
                // Handle empty params string
                if (params.empty()) {
                    nums.push_back(0);  // Default value
                } else {
                    size_t start = 0;
                    size_t end;
                    while (start < params.length()) {
                        end = params.find(';', start);
                        std::string numStr = params.substr(start, end - start);
                        // Handle empty parameter (consecutive semicolons)
                        if (numStr.empty()) {
                            nums.push_back(0);  // Default value
                        } else {
                            try {
                                nums.push_back(std::stoi(numStr));
                            } catch (const std::exception&) {
                                nums.push_back(0);  // Default value on error
                            }
                        }
                        if (end == std::string::npos) break;
                        start = end + 1;
                    }
                }

                // Handle commands
                switch (cmd) {
                    case 'm': // SGR (Select Graphic Rendition)
                        if (nums.empty()) nums.push_back(0);  // Default to reset if no parameters
                        for (int num : nums) {
                            switch (num) {
                                case 0:  // Reset
                                    currentFg = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
                                    currentBg = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
                                    currentBold = false;
                                    currentItalic = false;
                                    break;
                                case 1:  // Bold
                                    currentBold = true;
                                    break;
                                case 3:  // Italic
                                    currentItalic = true;
                                    break;
                                case 30: // Black foreground
                                    currentFg = ImVec4(0.0f, 0.0f, 0.0f, 1.0f);
                                    break;
                                case 31: // Red foreground
                                    currentFg = ImVec4(1.0f, 0.0f, 0.0f, 1.0f);
                                    break;
                                case 32: // Green foreground
                                    currentFg = ImVec4(0.0f, 1.0f, 0.0f, 1.0f);
                                    break;
                                case 33: // Yellow foreground
                                    currentFg = ImVec4(1.0f, 1.0f, 0.0f, 1.0f);
                                    break;
                                case 34: // Blue foreground
                                    currentFg = ImVec4(0.0f, 0.0f, 1.0f, 1.0f);
                                    break;
                                case 35: // Magenta foreground
                                    currentFg = ImVec4(1.0f, 0.0f, 1.0f, 1.0f);
                                    break;
                                case 36: // Cyan foreground
                                    currentFg = ImVec4(0.0f, 1.0f, 1.0f, 1.0f);
                                    break;
                                case 37: // White foreground
                                    currentFg = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
                                    break;
                                // Background colors
                                case 40: // Black background
                                    currentBg = ImVec4(0.0f, 0.0f, 0.0f, 1.0f);
                                    break;
                                case 41: // Red background
                                    currentBg = ImVec4(1.0f, 0.0f, 0.0f, 1.0f);
                                    break;
                                case 42: // Green background
                                    currentBg = ImVec4(0.0f, 1.0f, 0.0f, 1.0f);
                                    break;
                                case 43: // Yellow background
                                    currentBg = ImVec4(1.0f, 1.0f, 0.0f, 1.0f);
                                    break;
                                case 44: // Blue background
                                    currentBg = ImVec4(0.0f, 0.0f, 1.0f, 1.0f);
                                    break;
                                case 45: // Magenta background
                                    currentBg = ImVec4(1.0f, 0.0f, 1.0f, 1.0f);
                                    break;
                                case 46: // Cyan background
                                    currentBg = ImVec4(0.0f, 1.0f, 1.0f, 1.0f);
                                    break;
                                case 47: // White background
                                    currentBg = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
                                    break;
                            }
                        }
                        break;
                    case 'H': // Cursor Home or Position
                        if (nums.size() >= 2) {
                            cursorY = std::max(0, nums[0] - 1);  // Convert from 1-based to 0-based
                            cursorX = std::max(0, nums[1] - 1);
                        } else {
                            cursorX = 0;
                            cursorY = 0;
                        }
                        break;
                    case 'J': // Clear Screen
                        if (nums.empty() || nums[0] == 2) {
                            // Clear entire screen
                            for (auto& row : screenBuffer) {
                                for (auto& cell : row) {
                                    cell = ScreenCell();
                                }
                            }
                        }
                        break;
                    case 'K': // Clear Line
                        if (nums.empty() || nums[0] == 0) {
                            // Clear from cursor to end of line
                            for (int x = cursorX; x < bufferWidth; x++) {
                                screenBuffer[cursorY][x] = ScreenCell();
                            }
                        }
                        break;
                    case 'A': // Cursor Up
                        cursorY = std::max(0, cursorY - (nums.empty() ? 1 : nums[0]));
                        break;
                    case 'B': // Cursor Down
                        cursorY = std::min(bufferHeight - 1, cursorY + (nums.empty() ? 1 : nums[0]));
                        break;
                    case 'C': // Cursor Forward
                        cursorX = std::min(bufferWidth - 1, cursorX + (nums.empty() ? 1 : nums[0]));
                        break;
                    case 'D': // Cursor Back
                        cursorX = std::max(0, cursorX - (nums.empty() ? 1 : nums[0]));
                        break;
                }
            }
            continue;
        }
        char c = data[i];
        
        // Handle special characters
        if (c == '\n') {
            cursorX = 0;
            cursorY++;
            if (cursorY >= bufferHeight) {
                // Scroll the buffer up
                screenBuffer.erase(screenBuffer.begin());
                screenBuffer.push_back(std::vector<ScreenCell>(bufferWidth, ScreenCell()));
                cursorY = bufferHeight - 1;
            }
        } 
        else if (c == '\r') {
            cursorX = 0;
        }
        else if (c == '\t') {
            // Move to next tab stop (every 8 columns)
            cursorX = (cursorX + 8) & ~7;
            if (cursorX >= bufferWidth) {
                cursorX = 0;
                cursorY++;
            }
        }
        if (c >= 32) {
            if (cursorX >= bufferWidth) {
                cursorX = 0;
                cursorY++;
            }
            if (cursorY >= bufferHeight) {
                screenBuffer.erase(screenBuffer.begin());
                screenBuffer.push_back(std::vector<ScreenCell>(bufferWidth, ScreenCell()));
                cursorY = bufferHeight - 1;
            }
            
            screenBuffer[cursorY][cursorX].ch = c;
            screenBuffer[cursorY][cursorX].dirty = true;
            screenBuffer[cursorY][cursorX].fg = currentFg;
            screenBuffer[cursorY][cursorX].bg = currentBg;
            screenBuffer[cursorY][cursorX].bold = currentBold;
            screenBuffer[cursorY][cursorX].italic = currentItalic;
            cursorX++;
        }
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

    // Get the slave name BEFORE forking
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
        // Create new session and process group
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

        // Set as controlling terminal
        if (ioctl(slaveFd, TIOCSCTTY, 0) < 0) {
            std::cerr << "Failed to set controlling terminal" << std::endl;
            exit(1);
        }

        // Set up standard file descriptors
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

        // Enable job control and signals
        tios.c_lflag |= (ECHO | ICANON | ISIG | IEXTEN);
        tios.c_iflag |= (ICRNL | IXON);
        tios.c_oflag |= OPOST;
        tios.c_cc[VINTR] = 3;    // Ctrl-C
        tios.c_cc[VSUSP] = 26;   // Ctrl-Z
        tios.c_cc[VQUIT] = 28;   // Ctrl-\

        if (tcsetattr(STDIN_FILENO, TCSANOW, &tios) < 0) {
            std::cerr << "Failed to set terminal attributes" << std::endl;
            exit(1);
        }

        // Set initial terminal size
        struct winsize ws;
        ws.ws_row = 24;
        ws.ws_col = 80;
        ws.ws_xpixel = 0;
        ws.ws_ypixel = 0;
        ioctl(STDIN_FILENO, TIOCSWINSZ, &ws);

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
    char buffer[1024];
    while (!shouldTerminate) {
        ssize_t bytesRead = read(ptyFd, buffer, sizeof(buffer) - 1);
        if (bytesRead > 0) {
            buffer[bytesRead] = '\0';
            std::lock_guard<std::mutex> lock(bufferMutex);
            writeToScreenBuffer(buffer, bytesRead);
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

void Terminal::processInput(const std::string& input) {
    if (ptyFd >= 0) {
        {
            std::lock_guard<std::mutex> lock(bufferMutex);
            writeToScreenBuffer(input.c_str(), input.length());
        }
        write(ptyFd, input.c_str(), input.length());
        shouldScrollToBottom = true;
    }
}


void Terminal::setWorkingDirectory(const std::string& path) {
    if (!path.empty()) {
        std::string cmd = "cd " + path + "\n";
        processInput(cmd);
    }
}
void Terminal::updateTerminalSize() {
    if (ptyFd >= 0) {
        ImVec2 size = ImGui::GetContentRegionAvail();
        float charWidth = ImGui::GetFont()->GetCharAdvance('M');
        float charHeight = ImGui::GetTextLineHeight();
        
        struct winsize ws = {};  // Initialize to zero
        ws.ws_row = std::max(1, static_cast<int>(size.y / charHeight));
        ws.ws_col = std::max(1, static_cast<int>(size.x / charWidth));
        ws.ws_xpixel = static_cast<unsigned short>(size.x);
        ws.ws_ypixel = static_cast<unsigned short>(size.y);

        std::cout << "Setting terminal size: " << ws.ws_col << "x" << ws.ws_row << std::endl;

        // Set size on master PTY
        if (ioctl(ptyFd, TIOCSWINSZ, &ws) < 0) {
            std::cerr << "Failed to set window size: " << strerror(errno) << std::endl;
        }

        // Send SIGWINCH signal to notify of size change
        if (childPid > 0) {
            pid_t pgid = getpgid(childPid);
            if (pgid > 0) {
                std::cout << "Sending SIGWINCH to process group " << pgid << std::endl;
                killpg(pgid, SIGWINCH);
            }
        }
    }
}
void Terminal::render() {
    if (!isVisible) return;
    
    if (!shellStarted) {
        startShell();
    }
    
    // Handle Ctrl+C
    bool ctrl_pressed = ImGui::GetIO().KeyCtrl;
    if (ctrl_pressed && ImGui::IsKeyPressed(ImGuiKey_C)) {
        std::cout << "Ctrl+C detected, sending to terminal" << std::endl;
        if (ptyFd >= 0) {
            char c = 3;  // ASCII code for Ctrl+C
            write(ptyFd, &c, 1);
        }
    }
    ImVec2 currentSize = ImGui::GetContentRegionAvail();
    static ImVec2 lastSize(0, 0);
    
    if (currentSize.x != lastSize.x || currentSize.y != lastSize.y) {
        updateTerminalSize();
        lastSize = currentSize;
    }
    
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);
    ImGui::Begin("Terminal", nullptr, 
        ImGuiWindowFlags_NoDecoration | 
        ImGuiWindowFlags_NoMove | 
        ImGuiWindowFlags_NoResize);

    // Output area with dark background
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.1f, 0.1f, 0.1f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_TextSelectedBg, ImVec4(0.3f, 0.3f, 0.7f, 0.5f));
    const float footerHeightToReserve = ImGui::GetStyle().ItemSpacing.y + ImGui::GetFrameHeightWithSpacing();
    ImGui::BeginChild("ScrollingRegion", ImVec2(0, -footerHeightToReserve), false);
    if (ImGui::GetIO().MouseWheel != 0 && ImGui::IsWindowHovered()) {
        scrollOffset -= (int)ImGui::GetIO().MouseWheel;
        scrollOffset = std::max(0, std::min(scrollOffset, maxScrollback));
    }
     {
        std::lock_guard<std::mutex> lock(bufferMutex);
        
        ImDrawList* draw_list = ImGui::GetWindowDrawList();
        ImVec2 pos = ImGui::GetCursorScreenPos();
        float char_width = ImGui::GetFont()->GetCharAdvance('M');
        float line_height = ImGui::GetTextLineHeight();

        // Calculate visible area
        int visibleRows = (int)(ImGui::GetContentRegionAvail().y / line_height);
        int startRow = std::max(0, bufferHeight - visibleRows - scrollOffset);
        int endRow = std::min(bufferHeight, startRow + visibleRows);

        // Draw background
        ImVec2 term_size(char_width * bufferWidth, line_height * visibleRows);
        draw_list->AddRectFilled(
            pos,
            ImVec2(pos.x + term_size.x, pos.y + term_size.y),
            ImGui::ColorConvertFloat4ToU32(ImVec4(0.1f, 0.1f, 0.1f, 1.0f))
        );

        // Draw visible portion of buffer
        for (int y = startRow; y < endRow; y++) {
            for (int x = 0; x < bufferWidth; x++) {
                const auto& cell = screenBuffer[y][x];
                if (cell.ch != ' ') {
                    char text[2] = {cell.ch, 0};
                    ImVec4 color = cell.fg;
                    if (cell.bold) {
                        // Make color brighter for bold text
                        color.x = std::min(1.0f, color.x * 1.2f);
                        color.y = std::min(1.0f, color.y * 1.2f);
                        color.z = std::min(1.0f, color.z * 1.2f);
                    }
                    draw_list->AddText(
                        ImVec2(pos.x + x * char_width, 
                               pos.y + (y - startRow) * line_height),
                        ImGui::ColorConvertFloat4ToU32(color),
                        text
                    );
                }
            }
        }

        // Draw cursor only if not scrolled
        if (scrollOffset == 0) {
            draw_list->AddRectFilled(
                ImVec2(pos.x + cursorX * char_width, 
                       pos.y + (cursorY - startRow) * line_height),
                ImVec2(pos.x + (cursorX + 1) * char_width, 
                       pos.y + (cursorY - startRow + 1) * line_height),
                IM_COL32(255, 255, 255, 127)
            );
        }
    }

    ImGui::EndChild();
    ImGui::PopStyleColor(2);

    // Input line with proper spacing
    ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.1f, 0.1f, 0.1f, 1.0f));
    ImGui::PushItemWidth(-1);
    
    bool inputActive = ImGui::InputText("##TerminalInput", inputBuffer, INPUT_BUFFER_SIZE, 
        ImGuiInputTextFlags_EnterReturnsTrue);
    
    if (inputActive) {
        std::string cmd(inputBuffer);
        cmd += "\n";
        processInput(cmd);
        memset(inputBuffer, 0, INPUT_BUFFER_SIZE);
    }
    
    ImGui::PopItemWidth();
    ImGui::PopStyleColor();

    // Auto-focus input only when appropriate
    if (ImGui::IsWindowFocused() && !ImGui::IsAnyItemActive() && 
        !ImGui::IsMouseDragging(0) && !ImGui::GetIO().MouseDown[0]) {
        ImGui::SetKeyboardFocusHere(-1);
    }

    ImGui::End();
}