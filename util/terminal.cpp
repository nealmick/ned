#include "util/terminal.h"
#include "util/settings.h"
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <signal.h>
#include <iostream>



Terminal gTerminal;
Terminal::Terminal() {
    // Initialize with safe default size
    state.row = 24;
    state.col = 80;
    state.bot = state.row - 1;
    

    sel.mode = SEL_IDLE;
    sel.type = SEL_REGULAR;
    sel.snap = 0;
    sel.ob.x = -1;
    sel.ob.y = -1;
    sel.oe.x = -1;
    sel.oe.y = -1;
    sel.nb.x = -1;
    sel.nb.y = -1;
    sel.ne.x = -1;
    sel.ne.y = -1;
    sel.alt = 0;
    // Initialize screen buffer
    state.lines.resize(state.row, std::vector<Glyph>(state.col));
    state.altLines.resize(state.row, std::vector<Glyph>(state.col));
    state.dirty.resize(state.row, true);
    
    // Initialize tab stops (every 8 columns)
    state.tabs.resize(state.col, false);
    for (int i = 8; i < state.col; i += 8) {
        state.tabs[i] = true;
    }
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
    // Open PTY master
    ptyFd = posix_openpt(O_RDWR);
    grantpt(ptyFd);
    unlockpt(ptyFd);
    
    char* slaveName = ptsname(ptyFd);
    childPid = fork();

    if (childPid == 0) {  // Child process
        setsid(); // Create new session and process group
        close(ptyFd);  // Close master side

        // Open slave PTY
        int slaveFd = open(slaveName, O_RDWR);
        ioctl(slaveFd, TIOCSCTTY, 0);
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

        if (tcsetattr(STDIN_FILENO, TCSANOW, &tios) < 0) {
            exit(1);
        }

        // Set window size
        struct winsize ws = {};
        ws.ws_row = state.row;
        ws.ws_col = state.col;
        ioctl(STDIN_FILENO, TIOCSWINSZ, &ws);

        setenv("TERM", "xterm-256color", 1);
        
        const char* shell = getenv("SHELL");
        if (!shell) shell = "/bin/bash";
        
        char* const args[] = {(char*)shell, NULL};
        execvp(shell, args);
        exit(1);
    }

    // Parent process - start read thread
    readThread = std::thread(&Terminal::readOutput, this);
}
void Terminal::render() {
    if (!isVisible) return;
    
    if (ptyFd < 0) {
        startShell();
    }

    // Basic ImGui window setup
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);
    ImGui::Begin("Terminal", nullptr, 
        ImGuiWindowFlags_NoDecoration | 
        ImGuiWindowFlags_NoMove | 
        ImGuiWindowFlags_NoResize);
    // Get actual content size excluding window borders/padding
    ImVec2 contentSize = ImGui::GetContentRegionAvail();
    float charWidth = ImGui::GetFont()->GetCharAdvance('M');
    float lineHeight = ImGui::GetTextLineHeight();

    // Calculate new terminal dimensions
    int new_cols = std::max(1, static_cast<int>(contentSize.x / charWidth));
    int new_rows = std::max(1, static_cast<int>(contentSize.y / lineHeight));

    // Resize if dimensions changed
    if (new_cols != state.col || new_rows != state.row) {
        resize(new_cols, new_rows);
    }

    renderBuffer();


    // Handle mouse wheel for scrollback
    if (ImGui::IsWindowFocused() && ImGui::IsWindowHovered() && !(state.mode & MODE_ALTSCREEN)) {
        ImGuiIO& io = ImGui::GetIO();
        if (io.MouseWheel != 0.0f) {
            int maxScroll = std::max(0, (int)(scrollbackBuffer.size() + state.row) - new_rows);
            scrollOffset -= static_cast<int>(io.MouseWheel * 3);
            scrollOffset = std::clamp(scrollOffset, 0, maxScroll);
        }
    }
    // Handle mouse input
    ImGuiIO& io = ImGui::GetIO();
    if (ImGui::IsWindowFocused() && ImGui::IsWindowHovered()) {
        ImVec2 mousePos = ImGui::GetMousePos();
        ImVec2 winPos = ImGui::GetWindowPos();
        float charWidth = ImGui::GetFont()->GetCharAdvance('M');
        float lineHeight = ImGui::GetTextLineHeight();

        // Convert mouse position to terminal cell coordinates
        int cellX = static_cast<int>((mousePos.x - winPos.x) / charWidth);
        int cellY = static_cast<int>((mousePos.y - winPos.y) / lineHeight);

        // Handle selection
        if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
            // Start new selection
            selectionStart(cellX, cellY);
        }
        else if (ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
            // Extend selection
            selectionExtend(cellX, cellY);
        }
        else if (ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
            if (sel.mode != SEL_IDLE) {
                // Copy selection to clipboard on mouse release
                copySelection();
                sel.mode = SEL_IDLE;
            }
        }

        // Handle right-click paste
        if (ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
            pasteFromClipboard();
        }

        // Handle clipboard shortcuts
        if (io.KeyCtrl) {
            if (ImGui::IsKeyPressed(ImGuiKey_Y, false)) {
                copySelection();
            }
            if (ImGui::IsKeyPressed(ImGuiKey_W, false)) {
                copySelection();
            }
            if (ImGui::IsKeyPressed(ImGuiKey_V, false)) {
                std::cout << "Cmd+V pressed" << std::endl;
                pasteFromClipboard();
            }

        }
    }
    // Improved input handling
    if (ImGui::IsWindowFocused()) {
        ImGuiIO& io = ImGui::GetIO();
        
        // Handle special keys
        if (ImGui::IsKeyPressed(ImGuiKey_Enter)) {
            processInput("\r");
        }
        else if (ImGui::IsKeyPressed(ImGuiKey_Backspace)) {
            processInput("\x7f");  // Delete character
        }
       else if (ImGui::IsKeyPressed(ImGuiKey_UpArrow)) {
            if (state.mode & MODE_APPCURSOR){
                processInput("\033OA");
            }else{
                std::cout << "sending [A" << std::endl;
                processInput("\033[A");
            }
                
        }
        else if (ImGui::IsKeyPressed(ImGuiKey_DownArrow)) {
            if (state.mode & MODE_APPCURSOR){
                processInput("\033OB");
            }else{
                std::cout << "sending [B" << std::endl;
                processInput("\033[B");
            }
                
        }
        else if (ImGui::IsKeyPressed(ImGuiKey_RightArrow)) {
            if (state.mode & MODE_APPCURSOR)
                processInput("\033OC");
            else
                processInput("\033[C");
        }
        else if (ImGui::IsKeyPressed(ImGuiKey_LeftArrow)) {
            if (io.KeyCtrl) {
                // Ctrl+Left: Move word left
                processInput("\033[1;5D");
            } else if (io.KeyShift) {
                // Shift+Left: Selection
                processInput("\033[1;2D");
            } else if (state.mode & MODE_APPCURSOR) {
                processInput("\033OD");
            } else {
                // Normal left arrow
                processInput("\033[D");
            }
        }
        // More special keys
        else if (ImGui::IsKeyPressed(ImGuiKey_Home)) {
            processInput("\033[H");
        }
        else if (ImGui::IsKeyPressed(ImGuiKey_End)) {
            processInput("\033[F");
        }
        else if (ImGui::IsKeyPressed(ImGuiKey_Delete)) {
            processInput("\033[3~");
        }
        else if (ImGui::IsKeyPressed(ImGuiKey_PageUp)) {
            processInput("\033[5~");
        }
        else if (ImGui::IsKeyPressed(ImGuiKey_PageDown)) {
            processInput("\033[6~");
        }
        else if (ImGui::IsKeyPressed(ImGuiKey_Tab)) {
            processInput("\t");
        }
        else if (ImGui::IsKeyPressed(ImGuiKey_Escape)) {
            processInput("\033");
        }

        // Handle Ctrl key combinations
        if (io.KeyCtrl || io.KeySuper) {
            // Map common control combinations explicitly
            if (ImGui::IsKeyPressed(ImGuiKey_A)) processInput("\x01");
            if (ImGui::IsKeyPressed(ImGuiKey_B)) processInput("\x02");
            if (ImGui::IsKeyPressed(ImGuiKey_C)) processInput("\x03");
            if (ImGui::IsKeyPressed(ImGuiKey_D)) processInput("\x04");
            if (ImGui::IsKeyPressed(ImGuiKey_E)) processInput("\x05");
            if (ImGui::IsKeyPressed(ImGuiKey_F)) processInput("\x06");
            if (ImGui::IsKeyPressed(ImGuiKey_G)) processInput("\x07");
            if (ImGui::IsKeyPressed(ImGuiKey_H)) processInput("\x08");
            if (ImGui::IsKeyPressed(ImGuiKey_I)) processInput("\x09");
            if (ImGui::IsKeyPressed(ImGuiKey_J)) processInput("\x0A");
            if (ImGui::IsKeyPressed(ImGuiKey_K)) processInput("\x0B");
            if (ImGui::IsKeyPressed(ImGuiKey_L)) processInput("\x0C");
            if (ImGui::IsKeyPressed(ImGuiKey_M)) processInput("\x0D");
            if (ImGui::IsKeyPressed(ImGuiKey_N)) processInput("\x0E");
            if (ImGui::IsKeyPressed(ImGuiKey_O)) processInput("\x0F");
            if (ImGui::IsKeyPressed(ImGuiKey_P)) processInput("\x10");
            if (ImGui::IsKeyPressed(ImGuiKey_Q)) processInput("\x11");
            if (ImGui::IsKeyPressed(ImGuiKey_R)) processInput("\x12");
            if (ImGui::IsKeyPressed(ImGuiKey_S)) processInput("\x13");
            if (ImGui::IsKeyPressed(ImGuiKey_T)) processInput("\x14");
            if (ImGui::IsKeyPressed(ImGuiKey_U)) processInput("\x15");
            //if (ImGui::IsKeyPressed(ImGuiKey_V)) processInput("\x16");//removed for cmd v
            if (ImGui::IsKeyPressed(ImGuiKey_W)) processInput("\x17");
            if (ImGui::IsKeyPressed(ImGuiKey_X)) processInput("\x18");
            if (ImGui::IsKeyPressed(ImGuiKey_Y)) processInput("\x19");
            if (ImGui::IsKeyPressed(ImGuiKey_Z)) processInput("\x1A");
        }
        
        // Regular text input - only if no control key is pressed
        if (!io.KeySuper && !io.KeyCtrl && !io.KeyAlt) {
            for (int i = 0; i < io.InputQueueCharacters.Size; i++) {
                char c = (char)io.InputQueueCharacters[i];
                if (c != 0) {
                    processInput(std::string(1, c));
                }
            }
        }
    }

    ImGui::End();
}
void Terminal::renderBuffer() {
    std::lock_guard<std::mutex> lock(bufferMutex);
    
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    ImVec2 pos = ImGui::GetCursorScreenPos();
    float charWidth = ImGui::GetFont()->GetCharAdvance('M');
    float lineHeight = ImGui::GetTextLineHeight();
    bool altScreen = state.mode & MODE_ALTSCREEN;

    if (altScreen) {
        // Alternate screen rendering (vim/htop)
        // Draw selection highlight
        if (sel.mode != SEL_IDLE && sel.ob.x != -1) {
            for (int y = 0; y < state.row; y++) {
                for (int x = 0; x < state.col; x++) {
                    if (selectedText(x, y)) {
                        ImVec2 highlightPos(
                            pos.x + x * charWidth,
                            pos.y + y * lineHeight
                        );
                        drawList->AddRectFilled(
                            highlightPos,
                            ImVec2(highlightPos.x + charWidth, highlightPos.y + lineHeight),
                            ImGui::ColorConvertFloat4ToU32(ImVec4(0.3f, 0.3f, 0.7f, 0.3f))
                        );
                    }
                }
            }
        }

        // Draw alt screen characters
        for (int y = 0; y < state.row; y++) {
            for (int x = 0; x < state.col; x++) {
                const Glyph& glyph = state.lines[y][x];
                if (glyph.mode & ATTR_WDUMMY) continue;

                char text[UTF_SIZ] = {0};
                char* p = text;
                utf8Encode(glyph.u, p);

                ImVec2 charPos(pos.x + x * charWidth, pos.y + y * lineHeight);
                ImVec4 fg = glyph.fg;
                ImVec4 bg = glyph.bg;

                // Handle true color
                if (glyph.colorMode == COLOR_TRUE) {
                    uint32_t tc = glyph.trueColorFg;
                    fg = ImVec4(
                        ((tc >> 16) & 0xFF) / 255.0f,
                        ((tc >> 8) & 0xFF) / 255.0f,
                        (tc & 0xFF) / 255.0f,
                        1.0f
                    );
                }

                // Handle reverse video
                if (glyph.mode & ATTR_REVERSE) std::swap(fg, bg);

                // Handle bold
                if (glyph.mode & ATTR_BOLD && glyph.colorMode == COLOR_BASIC) {
                    fg.x = std::min(1.0f, fg.x * 1.5f);
                    fg.y = std::min(1.0f, fg.y * 1.5f);
                    fg.z = std::min(1.0f, fg.z * 1.5f);
                }

                // Draw background
                if (bg.x != 0 || bg.y != 0 || bg.z != 0 || (glyph.mode & ATTR_REVERSE)) {
                    drawList->AddRectFilled(
                        charPos,
                        ImVec2(charPos.x + charWidth, charPos.y + lineHeight),
                        ImGui::ColorConvertFloat4ToU32(bg)
                    );
                }

                // Draw character
                if (glyph.u != ' ' && glyph.u != 0) {
                    drawList->AddText(
                        charPos, 
                        ImGui::ColorConvertFloat4ToU32(fg),
                        text
                    );
                }

                // Draw underline
                if (glyph.mode & ATTR_UNDERLINE) {
                    drawList->AddLine(
                        ImVec2(charPos.x, charPos.y + lineHeight - 1),
                        ImVec2(charPos.x + charWidth, charPos.y + lineHeight - 1),
                        ImGui::ColorConvertFloat4ToU32(fg)
                    );
                }

                if (glyph.mode & ATTR_WIDE) x++;
            }
        }

        // Draw alt screen cursor
        if (ImGui::IsWindowFocused()) {
            ImVec2 cursorPos(
                pos.x + state.c.x * charWidth,
                pos.y + state.c.y * lineHeight
            );
            
            const Glyph& cursorCell = state.lines[state.c.y][state.c.x];
            float time = ImGui::GetTime();
            float alpha = (sin(time * 3.14159f) * 0.3f) + 0.5f;

            if (state.mode & MODE_INSERT) {
                // Vertical bar cursor
                drawList->AddRectFilled(
                    cursorPos,
                    ImVec2(cursorPos.x + 2, cursorPos.y + lineHeight),
                    ImGui::ColorConvertFloat4ToU32(ImVec4(0.7f, 0.7f, 0.7f, alpha))
                );
            } else {
                if (cursorCell.u != 0) {
                    char text[UTF_SIZ] = {0};
                    utf8Encode(cursorCell.u, text);

                    // Inverse colors
                    ImVec4 bg = cursorCell.fg;
                    ImVec4 fg = cursorCell.bg;

                    drawList->AddRectFilled(
                        cursorPos,
                        ImVec2(cursorPos.x + charWidth, cursorPos.y + lineHeight),
                        ImGui::ColorConvertFloat4ToU32(ImVec4(bg.x, bg.y, bg.z, alpha))
                    );

                    drawList->AddText(
                        cursorPos,
                        ImGui::ColorConvertFloat4ToU32(fg),
                        text
                    );
                } else {
                    // Empty cell cursor
                    drawList->AddRectFilled(
                        cursorPos,
                        ImVec2(cursorPos.x + charWidth, cursorPos.y + lineHeight),
                        ImGui::ColorConvertFloat4ToU32(ImVec4(0.7f, 0.7f, 0.7f, alpha))
                    );
                }
            }
        }
    } else {
        // Main screen with scrollback
        ImVec2 contentSize = ImGui::GetContentRegionAvail();
        int visibleRows = std::max(1, static_cast<int>(contentSize.y / lineHeight));
        int totalLines = scrollbackBuffer.size() + state.row;
        
        // Clamp scroll offset
        int maxScroll = std::max(0, totalLines - visibleRows);
        scrollOffset = std::clamp(scrollOffset, 0, maxScroll);
        int startLine = std::max(0, totalLines - visibleRows - scrollOffset);

        // Draw selection highlight
        if (sel.mode != SEL_IDLE && sel.ob.x != -1) {
            for (int visY = 0; visY < visibleRows; visY++) {
                int currentLine = startLine + visY;
                if (currentLine >= scrollbackBuffer.size()) {
                    int screenY = currentLine - scrollbackBuffer.size();
                    if (screenY >= 0 && screenY < state.row) {
                        for (int x = 0; x < state.col; x++) {
                            if (selectedText(x, screenY)) {
                                ImVec2 highlightPos(
                                    pos.x + x * charWidth,
                                    pos.y + visY * lineHeight
                                );
                                drawList->AddRectFilled(
                                    highlightPos,
                                    ImVec2(highlightPos.x + charWidth, highlightPos.y + lineHeight),
                                    ImGui::ColorConvertFloat4ToU32(ImVec4(0.3f, 0.3f, 0.7f, 0.3f))
                                );
                            }
                        }
                    }
                }
            }
        }

        // Draw scrollback + main screen
        for (int visY = 0; visY < visibleRows; visY++) {
            int currentLine = startLine + visY;
            const std::vector<Glyph>* line = nullptr;
            
            if (currentLine < scrollbackBuffer.size()) {
                line = &scrollbackBuffer[currentLine];
            } else {
                int screenY = currentLine - scrollbackBuffer.size();
                if (screenY >= 0 && screenY < state.row) {
                    line = &state.lines[screenY];
                }
            }

            if (!line) continue;

            for (int x = 0; x < state.col; x++) {
                if (x >= line->size()) break;
                const Glyph& glyph = (*line)[x];
                if (glyph.mode & ATTR_WDUMMY) continue;

                char text[UTF_SIZ] = {0};
                char* p = text;
                utf8Encode(glyph.u, p);

                ImVec2 charPos(pos.x + x * charWidth, pos.y + visY * lineHeight);
                ImVec4 fg = glyph.fg;
                ImVec4 bg = glyph.bg;

                // Handle true color
                if (glyph.colorMode == COLOR_TRUE) {
                    uint32_t tc = glyph.trueColorFg;
                    fg = ImVec4(
                        ((tc >> 16) & 0xFF) / 255.0f,
                        ((tc >> 8) & 0xFF) / 255.0f,
                        (tc & 0xFF) / 255.0f,
                        1.0f
                    );
                }

                // Handle reverse video
                if (glyph.mode & ATTR_REVERSE) std::swap(fg, bg);

                // Handle bold
                if (glyph.mode & ATTR_BOLD && glyph.colorMode == COLOR_BASIC) {
                    fg.x = std::min(1.0f, fg.x * 1.5f);
                    fg.y = std::min(1.0f, fg.y * 1.5f);
                    fg.z = std::min(1.0f, fg.z * 1.5f);
                }

                // Draw background
                if (bg.x != 0 || bg.y != 0 || bg.z != 0 || (glyph.mode & ATTR_REVERSE)) {
                    drawList->AddRectFilled(
                        charPos,
                        ImVec2(charPos.x + charWidth, charPos.y + lineHeight),
                        ImGui::ColorConvertFloat4ToU32(bg)
                    );
                }

                // Draw character
                if (glyph.u != ' ' && glyph.u != 0) {
                    drawList->AddText(
                        charPos, 
                        ImGui::ColorConvertFloat4ToU32(fg),
                        text
                    );
                }

                // Draw underline
                if (glyph.mode & ATTR_UNDERLINE) {
                    drawList->AddLine(
                        ImVec2(charPos.x, charPos.y + lineHeight - 1),
                        ImVec2(charPos.x + charWidth, charPos.y + lineHeight - 1),
                        ImGui::ColorConvertFloat4ToU32(fg)
                    );
                }

                if (glyph.mode & ATTR_WIDE) x++;
            }
        }

        // Draw main screen cursor (only when not scrolled)
        if (ImGui::IsWindowFocused() && scrollOffset == 0) {
            ImVec2 cursorPos(
                pos.x + state.c.x * charWidth,
                pos.y + (visibleRows - (totalLines - scrollbackBuffer.size()) + state.c.y) * lineHeight
            );

            const Glyph& cursorCell = state.lines[state.c.y][state.c.x];
            float time = ImGui::GetTime();
            float alpha = (sin(time * 3.14159f) * 0.3f) + 0.5f;

            if (state.mode & MODE_INSERT) {
                drawList->AddRectFilled(
                    cursorPos,
                    ImVec2(cursorPos.x + 2, cursorPos.y + lineHeight),
                    ImGui::ColorConvertFloat4ToU32(ImVec4(0.7f, 0.7f, 0.7f, alpha))
                );
            } else {
                if (cursorCell.u != 0) {
                    char text[UTF_SIZ] = {0};
                    utf8Encode(cursorCell.u, text);

                    ImVec4 bg = cursorCell.fg;
                    ImVec4 fg = cursorCell.bg;

                    drawList->AddRectFilled(
                        cursorPos,
                        ImVec2(cursorPos.x + charWidth, cursorPos.y + lineHeight),
                        ImGui::ColorConvertFloat4ToU32(ImVec4(bg.x, bg.y, bg.z, alpha))
                    );

                    drawList->AddText(
                        cursorPos,
                        ImGui::ColorConvertFloat4ToU32(fg),
                        text
                    );
                } else {
                    drawList->AddRectFilled(
                        cursorPos,
                        ImVec2(cursorPos.x + charWidth, cursorPos.y + lineHeight),
                        ImGui::ColorConvertFloat4ToU32(ImVec4(0.7f, 0.7f, 0.7f, alpha))
                    );
                }
            }
        }
    }
}


    
void Terminal::toggleVisibility() {
    isVisible = !isVisible;
}


void Terminal::writeToBuffer(const char* data, size_t length) {
    static char utf8buf[UTF_SIZ];
    static size_t utf8len = 0;
    
    for (size_t i = 0; i < length; ++i) {
        unsigned char c = data[i];

        // Existing STR sequence handling
        if (state.esc & ESC_STR) {
            if (c == '\a' || c == 030 || c == 032 || c == 033 || ISCONTROLC1(c)) {
                state.esc &= ~(ESC_START | ESC_STR);
                state.esc |= ESC_STR_END;
                strparse();
                handleStringSequence();
                state.esc = 0;
                continue;
            }
            
            if (strescseq.len < 256) {
                strescseq.buf += c;
                strescseq.len++;
            }
            continue;
        }

        // Escape sequence start
        if (c == '\033') {
            state.esc = ESC_START;
            csiescseq.len = 0;
            strescseq.buf.clear();
            strescseq.len = 0;
            utf8len = 0;  // Reset UTF-8 buffer
            continue;
        }

        // Ongoing escape sequence processing
        if (state.esc & ESC_START) {
            if (state.esc & ESC_CSI) {
                if (csiescseq.len < sizeof(csiescseq.buf) - 1) {
                    csiescseq.buf[csiescseq.len++] = c;
                    if (BETWEEN(c, 0x40, 0x7E)) {
                        csiescseq.buf[csiescseq.len] = '\0';
                        csiescseq.mode[0] = c;
                        parseCSIParam(csiescseq);
                        handleCSI(csiescseq);
                        state.esc = 0;
                        csiescseq.len = 0;
                    }
                }
                continue;
            }

            if (eschandle(c)) {
                state.esc = 0;
            }
            continue;
        }

        // Control character handling
        if (ISCONTROL(c)) {
            utf8len = 0;  // Reset UTF-8 buffer
            handleControlCode(c);
            continue;
        }


        
        if (state.mode & MODE_UTF8) {
            if (utf8len == 0) {
                if ((c & 0x80) == 0) {
                    writeChar(c);
                } 
                else if ((c & 0xE0) == 0xC0 ||   // 2-byte start
                         (c & 0xF0) == 0xE0 ||   // 3-byte start
                         (c & 0xF8) == 0xF0) {   // 4-byte start
                    utf8buf[utf8len++] = c;
                    
                    // If it's a 3-byte sequence (box drawing characters), 
                    // we want to immediately look for the next two bytes
                    if ((c & 0xF0) == 0xE0) {
                        // Look ahead for the next two bytes
                        if (i + 2 < length) {
                            utf8buf[utf8len++] = data[i + 1];
                            utf8buf[utf8len++] = data[i + 2];
                            
                            Rune u;
                            size_t decoded = utf8Decode(utf8buf, &u, utf8len);
                            if (decoded > 0) {
                                writeChar(u);
                            }
                            
                            // Skip the next two bytes since we've processed them
                            i += 2;
                            utf8len = 0;
                        }
                    }
                } 
                else {
                    // Unexpected start byte
                    utf8buf[utf8len++] = c;
                    utf8buf[utf8len] = '\0';
                    writeChar(0xFFFD);
                }
            }
            else {
                // This block is now less likely to be used due to the changes above
                if ((c & 0xC0) == 0x80) {
                    utf8buf[utf8len++] = c;
                    
                    size_t expected_len = 
                        ((utf8buf[0] & 0xE0) == 0xC0) ? 2 :
                        ((utf8buf[0] & 0xF0) == 0xE0) ? 3 :
                        ((utf8buf[0] & 0xF8) == 0xF0) ? 4 : 0;

                    if (utf8len == expected_len) {
                        Rune u;
                        size_t decoded = utf8Decode(utf8buf, &u, utf8len);
                        if (decoded > 0) {
                            writeChar(u);
                        }
                        utf8len = 0;
                    }
                } 
                else {
                    std::cerr << "Invalid continuation byte: 0x" 
                              << std::hex << (int)c << std::dec << std::endl;
                    utf8len = 0;
                }
            }
        } 
        else {
            writeChar(c);
        }
    }
}

void Terminal::writeChar(Rune u) {
    

    // Your existing box drawing character mapping logic
    auto it = boxDrawingChars.find(u);
    if (it != boxDrawingChars.end()) {
        
        u = it->second;
    } else {
        // Log unmapped characters
        if (u >= 0x2500 && u <= 0x257F) {
            std::cerr << "Unmapped box drawing character: U+" 
                      << std::hex << u << std::dec 
                      << " (hex: 0x" << std::hex << u << std::dec << ")" << std::endl;
        }
    }

    // Rest of your existing writeChar implementation
    Glyph g;
    g.u = u;
    g.mode = state.c.attrs;
    g.fg = state.c.fg;
    g.bg = state.c.bg;
    g.colorMode = state.c.colorMode;

    writeGlyph(g, state.c.x, state.c.y);
    state.c.x++;
    state.lastc = u;
}

int Terminal::eschandle(unsigned char ascii) {
    switch (ascii) {
        case '[':
            state.esc |= ESC_CSI;
            return 0;
            
        // Handle O sequence directly for cursor moves
        case 'O':
            return 0;  // Keep processing to handle next char directly
            
        case 'A': // Cursor Up
            if (state.esc == ESC_START) { // Direct O-sequence
                tmoveto(state.c.x, state.c.y - 1);
                return 1;
            }
            break;
            
        case 'B': // Cursor Down 
            if (state.esc == ESC_START) {
                tmoveto(state.c.x, state.c.y + 1); 
                return 1;
            }
            break;
        case 'z':  // DECID -- Identify Terminal
            processInput("\033[?6c");  // Respond as VT102
            break;
        case ']':
        case 'P':  
        case '_':
        case '^':
        case 'k':
            tstrsequence(ascii);
            return 0;
        case 'n':
            state.charset = 2;
            break;
        case 'o':
            state.charset = 3;
            break;
        case '(':
        case ')':
        case '*':
        case '+':
            state.icharset = ascii - '(';
            state.esc |= ESC_ALTCHARSET;
            return 0;
        case 'D':  // IND
            if (state.c.y == state.bot) 
                scrollUp(state.top, 1);
            else
                state.c.y++;
            break;
        case 'E': // NEL
            tnewline(1);
            break;
        case 'H':  // HTS
            state.tabs[state.c.x] = true;
            break;
        case 'M':  // RI 
            if (state.c.y == state.top)
                scrollDown(state.top, 1);
            else 
                state.c.y--;
            break;
        case 'Z':  // DECID
            processInput("\033[?6c");
            break;
        case 'c':  // RIS
            reset();
            break;
        case '=':  // DECKPAM
            setMode(true, MODE_APPCURSOR); 
            break;
        case '>':  // DECKPNM
            setMode(false, MODE_APPCURSOR);
            break;
        case '7':  // DECSC
            cursorSave();
            break; 
        case '8':  // DECRC
            cursorLoad();
            break;
        case '\\':
            break;
        default:
            fprintf(stderr, "esc unhandled: ESC %c\n", ascii);
            break;
    }
    return 1;
}


void Terminal::tnewline(int first_col) {
    int y = state.c.y;

    if (y == state.bot) {
        scrollUp(state.top, 1);
    } else {
        y++;
    }
    moveTo(first_col ? 0 : state.c.x, y);
}
void Terminal::tstrsequence(unsigned char c) {
    // Handle different string sequences more comprehensively
    switch (c) {
        case 0x90:   // DCS - Device Control String
        case 0x9d:   // OSC - Operating System Command
        case 0x9e:   // PM - Privacy Message
        case 0x9f:   // APC - Application Program Command
            // TODO: Implement full handling of these sequences
            // This may involve buffering and processing multi-character sequences
            state.esc |= ESC_STR;
            break;
    }
}

void Terminal::tmoveto(int x, int y) {
    moveTo(x, y);  // Use the existing moveTo method
}
void Terminal::parseCSIParam(CSIEscape& csi) {
    char* p = csi.buf;
    csi.args.clear();
    
    // Check for private mode
    if (*p == '?') {
        csi.priv = 1;
        p++;
    } else {
        csi.priv = 0;
    }

    // Parse arguments
    while (p < csi.buf + csi.len) {
        // Parse numeric parameter
        int param = 0;
        while (p < csi.buf + csi.len && BETWEEN(*p, '0', '9')) {
            param = param * 10 + (*p - '0');
            p++;
        }
        csi.args.push_back(param);

        // Move to next parameter or end
        if (*p == ';') {
            p++;
        } else {
            break;
        }
    }
}
void Terminal::handleCSI(const CSIEscape& csi) {
    /*
    std::cout << "CSI sequence: " << csi.mode[0] << " args: ";
    for (auto arg : csi.args) {
        std::cout << arg << " ";
    }
    std::cout << std::endl;
    */

    switch (csi.mode[0]) {
        case '@': // ICH -- Insert <n> blank char
            {
                int n = csi.args.empty() ? 1 : csi.args[0];
                if (n < 1) n = 1;
                
                for (int i = state.col - 1; i >= state.c.x + n; i--)
                    state.lines[state.c.y][i] = state.lines[state.c.y][i-n];
                
                clearRegion(state.c.x, state.c.y, state.c.x + n - 1, state.c.y);
            }
            break;

        case 'A': // CUU -- Cursor <n> Up
            {
                int n = csi.args.empty() ? 1 : csi.args[0];

                if (n < 1) n = 1;
                moveTo(state.c.x, state.c.y - n);
            }
            break;

        case 'B': // CUD -- Cursor <n> Down
            {
                int n = csi.args.empty() ? 1 : csi.args[0];
                if (n < 1) n = 1;
                moveTo(state.c.x, state.c.y + n);
            }
            break;
        case 'e': // VPR -- Cursor <n> Down
            {
                int n = csi.args.empty() ? 1 : csi.args[0];
                if (n < 1) n = 1;
                moveTo(state.c.x, state.c.y + n);
            }
            break;

        case 'c': // DA -- Device Attributes
            if (csi.args.empty() || csi.args[0] == 0) {
                processInput("\033[?6c"); // VT102
            }
            break;

        case 'C': // CUF -- Cursor <n> Forward
        case 'a': // HPR -- Cursor <n> Forward
            {
                int n = csi.args.empty() ? 1 : csi.args[0];
                if (n < 1) n = 1;
                moveTo(state.c.x + n, state.c.y);
            }
            break;

        case 'D': // CUB -- Cursor <n> Backward
            {
                int n = csi.args.empty() ? 1 : csi.args[0];
                if (n < 1) n = 1;
                moveTo(state.c.x - n, state.c.y);
            }
            break;

        case 'E': // CNL -- Cursor <n> Down and first col
            {
                int n = csi.args.empty() ? 1 : csi.args[0];
                if (n < 1) n = 1;
                moveTo(0, state.c.y + n);
            }
            break;

        case 'F': // CPL -- Cursor <n> Up and first col
            {
                int n = csi.args.empty() ? 1 : csi.args[0];
                if (n < 1) n = 1;
                moveTo(0, state.c.y - n);
            }
            break;

        case 'g': // TBC -- Tabulation Clear
            switch (csi.args.empty() ? 0 : csi.args[0]) {
                case 0: // Clear current tab stop
                    state.tabs[state.c.x] = false;
                    break;
                case 3: // Clear all tab stops
                    std::fill(state.tabs.begin(), state.tabs.end(), false);
                    break;
            }
            break;

        case 'G': // CHA -- Cursor Character Absolute
        case '`': // HPA -- Horizontal Position Absolute
            if (!csi.args.empty()) {
                moveTo(csi.args[0] - 1, state.c.y);
            }
            break;

        case 'H': // CUP -- Move to <row> <col>
        case 'f': // HVP
            {
                int row = csi.args.size() > 0 ? csi.args[0] : 1;
                int col = csi.args.size() > 1 ? csi.args[1] : 1;
                tmoveato(col - 1, row - 1);
            }
            break;

        case 'I': // CHT -- Cursor Forward Tabulation <n>
            {
                int n = csi.args.empty() ? 1 : csi.args[0];
                if (n < 1) n = 1;
                tputtab(n);
            }
            break;

        case 'J': // ED -- Erase in Display
            switch (csi.args.empty() ? 0 : csi.args[0]) {
                case 0: // Below
                    clearRegion(state.c.x, state.c.y, state.col-1, state.c.y);
                    if (state.c.y < state.row-1)
                        clearRegion(0, state.c.y+1, state.col-1, state.row-1);
                    break;
                case 1: // Above
                    clearRegion(0, 0, state.col-1, state.c.y-1);
                    clearRegion(0, state.c.y, state.c.x, state.c.y);
                    break;
                case 2: // All
                    clearRegion(0, 0, state.col-1, state.row-1);
                    break;
            }
            break;

        case 'K': // EL -- Erase in Line
            switch (csi.args.empty() ? 0 : csi.args[0]) {
                case 0: // Right
                    clearRegion(state.c.x, state.c.y, state.col-1, state.c.y);
                    break;
                case 1: // Left
                    clearRegion(0, state.c.y, state.c.x, state.c.y);
                    break;
                case 2: // All
                    clearRegion(0, state.c.y, state.col-1, state.c.y);
                    break;
            }
            break;

        case 'L': // IL -- Insert <n> blank lines
            {
                int n = csi.args.empty() ? 1 : csi.args[0];
                if (n < 1) n = 1;
                if (BETWEEN(state.c.y, state.top, state.bot))
                    scrollDown(state.c.y, n);
            }
            break;

        case 'M': // DL -- Delete <n> lines
            {
                int n = csi.args.empty() ? 1 : csi.args[0];
                if (n < 1) n = 1;
                if (BETWEEN(state.c.y, state.top, state.bot))
                    scrollUp(state.c.y, n);
            }
            break;

        case 'P': // DCH -- Delete <n> char
            {
                int n = csi.args.empty() ? 1 : csi.args[0];
                if (n < 1) n = 1;
                
                for (int i = state.c.x; i + n < state.col && i < state.col; i++)
                    state.lines[state.c.y][i] = state.lines[state.c.y][i+n];
                
                clearRegion(state.col - n, state.c.y, state.col - 1, state.c.y);
            }
            break;

        case 'S': // SU -- Scroll <n> lines up
            {
                int n = csi.args.empty() ? 1 : csi.args[0];
                if (n < 1) n = 1;
                scrollUp(state.top, n);
            }
            break;

        case 'T': // SD -- Scroll <n> lines down
            {
                int n = csi.args.empty() ? 1 : csi.args[0];
                if (n < 1) n = 1;
                scrollDown(state.top, n);
            }
            break;

        case 'X': // ECH -- Erase <n> char
            {
                int n = csi.args.empty() ? 1 : csi.args[0];
                if (n < 1) n = 1;
                clearRegion(state.c.x, state.c.y, state.c.x + n - 1, state.c.y);
            }
            break;

        case 'Z': // CBT -- Cursor Backward Tabulation <n>
            {
                int n = csi.args.empty() ? 1 : csi.args[0];
                if (n < 1) n = 1;
                tputtab(-n);
            }
            break;

        case 'd': // VPA -- Move to <row>
            {
                int n = csi.args.empty() ? 1 : csi.args[0];
                tmoveato(state.c.x, n - 1);
            }
            break;

        case 'h': // SM -- Set Mode
            tsetmode(csi.priv, 1, csi.args);
            break;

        case 'l': // RM -- Reset Mode
            tsetmode(csi.priv, 0, csi.args);
            break;

        case 'm': // SGR -- Select Graphic Rendition
            handleSGR(csi.args);
            break;

        case 'n': // DSR -- Device Status Report
            switch (csi.args.empty() ? 0 : csi.args[0]) {
                case 5: // Operating status
                    processInput("\033[0n");
                    break;
                case 6: // Cursor position
                    {
                        char buf[40];
                        snprintf(buf, sizeof(buf), "\033[%i;%iR", 
                                state.c.y + 1, state.c.x + 1);
                        processInput(buf);
                    }
                    break;
            }
            break;

        case 'r': // DECSTBM -- Set Scrolling Region
            if (csi.args.size() >= 2) {
                int top = csi.args[0] - 1;
                int bot = csi.args[1] - 1;
                
                if (BETWEEN(top, 0, state.row-1) && BETWEEN(bot, 0, state.row-1) && top < bot) {
                    state.top = top;
                    state.bot = bot;
                    if (state.c.state & CURSOR_ORIGIN)
                        moveTo(0, state.top);
                }
            }
            break;

        case 's': // DECSC -- Save cursor position
            cursorSave();
            break;

        case 'u': // DECRC -- Restore cursor position
            cursorLoad();
            break;
    }
}


void Terminal::setMode(bool set, int mode) {
    if (mode == MODE_APPCURSOR) {
        std::cout << "APPCURSOR mode " << (set ? "enabled" : "disabled") << std::endl;
    }
    if (set)
        state.mode |= mode;
    else
        state.mode &= ~mode;
    switch (mode) {
        case 6:  // DECOM -- Origin Mode
            MODBIT(state.c.state, set, CURSOR_ORIGIN);
            if (set)
                moveTo(0, state.top);
            break;

        case MODE_WRAP:
            // Line wrapping mode
            break;
        case MODE_INSERT:
            // Toggle insert mode
            break;
        case MODE_ALTSCREEN:
            if (set) {
                state.altLines.swap(state.lines);
                state.mode |= MODE_ALTSCREEN;
                scrollOffset = 0; // Reset scroll on entering alt screen
            } else {
                state.altLines.swap(state.lines);
                state.mode &= ~MODE_ALTSCREEN;
                scrollOffset = 0; // Reset scroll on exiting alt screen
            }
            std::fill(state.dirty.begin(), state.dirty.end(), true);
            break;
        case MODE_CRLF:
            // Change line feed behavior
            break;
        case MODE_ECHO:
            // Local echo mode
            break;
    }
}
void Terminal::handleSGR(const std::vector<int>& args) {
    size_t i;
    int32_t idx;

    if (args.empty()) {
        // Reset all attributes if no parameters
        state.c.attrs = 0;
        state.c.fg = defaultColorMap[7];   // Default foreground
        state.c.bg = defaultColorMap[0];   // Default background
        state.c.colorMode = COLOR_BASIC;
        return;
    }

    for (i = 0; i < args.size(); i++) {
        int attr = args[i];
        switch (attr) {
            case 0: // Reset
                state.c.attrs = 0;
                state.c.fg = defaultColorMap[7];  // Default foreground
                state.c.bg = defaultColorMap[0];  // Default background
                state.c.colorMode = COLOR_BASIC;
                break;
            case 1: state.c.attrs |= ATTR_BOLD; break;
            case 2: state.c.attrs |= ATTR_FAINT; break;
            case 3: state.c.attrs |= ATTR_ITALIC; break;
            case 4: state.c.attrs |= ATTR_UNDERLINE; break;
            case 5: state.c.attrs |= ATTR_BLINK; break;
            case 7: state.c.attrs |= ATTR_REVERSE; break;
            case 8: state.c.attrs |= ATTR_INVISIBLE; break;
            case 9: state.c.attrs |= ATTR_STRUCK; break;
            
            case 22: state.c.attrs &= ~(ATTR_BOLD | ATTR_FAINT); break;
            case 23: state.c.attrs &= ~ATTR_ITALIC; break;
            case 24: state.c.attrs &= ~ATTR_UNDERLINE; break;
            case 25: state.c.attrs &= ~ATTR_BLINK; break;
            case 27: state.c.attrs &= ~ATTR_REVERSE; break;
            case 28: state.c.attrs &= ~ATTR_INVISIBLE; break;
            case 29: state.c.attrs &= ~ATTR_STRUCK; break;

            // Foreground color
            case 30 ... 37:
                state.c.fg = defaultColorMap[attr - 30];
                break;
            case 38:
                if (i + 2 < args.size()) {
                    if (args[i + 1] == 5) { // 256 colors
                        i += 2;
                        state.c.colorMode = COLOR_256;
                        if (args[i] < 16) {
                            state.c.fg = defaultColorMap[args[i]];
                        } else {
                            // Convert 256 color to RGB
                            uint8_t r = 0, g = 0, b = 0;
                            if (args[i] < 232) { // 216 colors: 16-231
                                uint8_t index = args[i] - 16;
                                r = (index / 36) * 51;
                                g = ((index / 6) % 6) * 51;
                                b = (index % 6) * 51;
                            } else { // Grayscale: 232-255
                                r = g = b = (args[i] - 232) * 11;
                            }
                            state.c.fg = ImVec4(r/255.0f, g/255.0f, b/255.0f, 1.0f);
                        }
                    } else if (args[i + 1] == 2 && i + 4 < args.size()) { // RGB
                        i += 4;
                        state.c.colorMode = COLOR_TRUE;
                        uint8_t r = args[i-2];
                        uint8_t g = args[i-1];
                        uint8_t b = args[i];
                        state.c.fg = ImVec4(r/255.0f, g/255.0f, b/255.0f, 1.0f);
                        state.c.trueColorFg = (r << 16) | (g << 8) | b;
                    }
                }
                break;
            case 39: // Default foreground
                state.c.fg = defaultColorMap[7];
                state.c.colorMode = COLOR_BASIC;
                break;

            // Background color
            case 40 ... 47:
                state.c.bg = defaultColorMap[attr - 40];
                break;
            case 48:
                if (i + 2 < args.size()) {
                    if (args[i + 1] == 5) { // 256 colors
                        i += 2;
                        if (args[i] < 16) {
                            state.c.bg = defaultColorMap[args[i]];
                        } else {
                            // Convert 256 color to RGB
                            uint8_t r = 0, g = 0, b = 0;
                            if (args[i] < 232) { // 216 colors: 16-231
                                uint8_t index = args[i] - 16;
                                r = (index / 36) * 51;
                                g = ((index / 6) % 6) * 51;
                                b = (index % 6) * 51;
                            } else { // Grayscale: 232-255
                                r = g = b = (args[i] - 232) * 11;
                            }
                            state.c.bg = ImVec4(r/255.0f, g/255.0f, b/255.0f, 1.0f);
                        }
                    } else if (args[i + 1] == 2 && i + 4 < args.size()) { // RGB
                        i += 4;
                        uint8_t r = args[i-2];
                        uint8_t g = args[i-1];
                        uint8_t b = args[i];
                        state.c.bg = ImVec4(r/255.0f, g/255.0f, b/255.0f, 1.0f);
                        state.c.trueColorBg = (r << 16) | (g << 8) | b;
                    }
                }
                break;
            case 49: // Default background
                state.c.bg = defaultColorMap[0];
                break;

            // Bright foreground colors
            case 90 ... 97:
                state.c.fg = defaultColorMap[(attr - 90) + 8];
                break;

            // Bright background colors
            case 100 ... 107:
                state.c.bg = defaultColorMap[(attr - 100) + 8];
                break;
        }
    }
}


void Terminal::writeGlyph(const Glyph& g, int x, int y) {
    if (x >= state.col || y >= state.row || x < 0 || y < 0) return;

    Glyph& cell = state.lines[y][x];
    cell = g;
    
    // Ensure we properly handle attribute clearing
    if (!(g.mode & (ATTR_REVERSE | ATTR_BOLD | ATTR_ITALIC | ATTR_BLINK | ATTR_UNDERLINE))) {
        cell.mode &= ~(ATTR_REVERSE | ATTR_BOLD | ATTR_ITALIC | ATTR_BLINK | ATTR_UNDERLINE);
    }
    
    cell.mode = (cell.mode & ~(ATTR_REVERSE | ATTR_BOLD | ATTR_ITALIC | ATTR_BLINK | ATTR_UNDERLINE)) 
              | (g.mode & (ATTR_REVERSE | ATTR_BOLD | ATTR_ITALIC | ATTR_BLINK | ATTR_UNDERLINE));
              
    cell.colorMode = g.colorMode;
    
    if (cell.mode & ATTR_REVERSE) {
        cell.fg = state.c.bg;
        cell.bg = state.c.fg;
        cell.trueColorFg = state.c.trueColorBg;
        cell.trueColorBg = state.c.trueColorFg;
    } else {
        cell.fg = state.c.fg;
        cell.bg = state.c.bg;
        cell.trueColorFg = state.c.trueColorFg;
        cell.trueColorBg = state.c.trueColorBg;
    }

    // Handle bold attribute affecting colors
    if (cell.mode & ATTR_BOLD && cell.colorMode == COLOR_BASIC) {
        // Make the color brighter for bold text
        if (cell.trueColorFg < 0x8) {  // If using standard colors
            cell.fg = defaultColorMap[cell.trueColorFg + 8];  // Use bright version
        }
    }
    if (cell.mode & ATTR_WIDE && x + 1 < state.col) {
        Glyph& nextCell = state.lines[y][x + 1];
        nextCell.u = ' ';  
        nextCell.mode = ATTR_WDUMMY;
        // Copy color/attrs from base cell
        nextCell.fg = cell.fg;
        nextCell.bg = cell.bg; 
    }
    
    state.dirty[y] = true;
}



void Terminal::handleEscape(char c) {
    switch(c) {
        case '7': // Save cursor position
            cursorSave();
            break;
        case '8': // Restore cursor position
            cursorLoad();
            break;
        case 'D': // IND - Index (move down and scroll if at bottom)
            if (state.c.y == state.bot) {
                scrollUp(state.top, 1);
            } else {
                state.c.y++;
            }
            break;
        case 'E': // NEL - Next Line
            state.c.x = 0;
            if (state.c.y == state.bot) {
                scrollUp(state.top, 1);
            } else {
                state.c.y++;
            }
            break;
        case 'H': // HTS - Horizontal Tab Set
            state.tabs[state.c.x] = true;
            break;
        case 'M': // RI - Reverse Index (move up and scroll if at top)
            if (state.c.y == state.top) {
                scrollDown(state.top, 1);
            } else {
                state.c.y--;
            }
            break;
       
        case '(': // G0 Character Set
        case ')': // G1 Character Set
        case '*': // G2 Character Set
        case '+': // G3 Character Set
            handleCharset(c);
            break;

        case 'n': // LS2 -- Locking shift 2
        case 'o': // LS3 -- Locking shift 3
            state.charset = 2 + (c - 'n');
            break;

        case 'A': // UK
            state.trantbl[state.charset] = CS_UK;
            break;
        case 'B': // US
            state.trantbl[state.charset] = CS_USA;
            break;
        case '0': // Special Graphics (Line Drawing)
            state.trantbl[state.charset] = CS_GRAPHIC0;
            break;
        case 'Z': // Device Control String (DCS)
            // Respond with terminal identification
            processInput("\033[?1;2c");
            break;

        case '~': // Keypad state
            // You can implement keypad mode handling here
            break;

        case 'c': // Full Reset
            reset();
            break;
    }
}


size_t Terminal::utf8Encode(Rune u, char* c) {
    size_t len = 0;
    
    if (u < 0x80) {
        c[0] = u;
        len = 1;
    }
    else if (u < 0x800) {
        c[0] = 0xC0 | (u >> 6);
        c[1] = 0x80 | (u & 0x3F);
        len = 2;
    }
    else if (u < 0x10000) {
        c[0] = 0xE0 | (u >> 12);
        c[1] = 0x80 | ((u >> 6) & 0x3F);
        c[2] = 0x80 | (u & 0x3F);
        len = 3;
    }
    else {
        c[0] = 0xF0 | (u >> 18);
        c[1] = 0x80 | ((u >> 12) & 0x3F);
        c[2] = 0x80 | ((u >> 6) & 0x3F);
        c[3] = 0x80 | (u & 0x3F);
        len = 4;
    }
    
    return len;
}



void Terminal::reset() {
    // Reset terminal to initial state
    state.mode = MODE_WRAP | MODE_UTF8;
    state.c = {}; // Reset cursor
    state.charset = 0;
    
    // Reset character set translation table
    for (int i = 0; i < 4; i++) {
        state.trantbl[i] = CS_USA;
    }

    // Clear screen and reset color/attributes
    clearRegion(0, 0, state.col - 1, state.row - 1);
    state.c.fg = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
    state.c.bg = ImVec4(0.0f, 0.0f, 0.0f, 1.0f);
}


void Terminal::clearRegion(int x1, int y1, int x2, int y2) {
    int temp;
    if (x1 > x2) {
        temp = x1;
        x1 = x2;
        x2 = temp;
    }
    if (y1 > y2) {
        temp = y1;
        y1 = y2;
        y2 = temp;
    }

    // Constrain to terminal size
    x1 = std::max(0, std::min(x1, state.col - 1));
    x2 = std::max(0, std::min(x2, state.col - 1));
    y1 = std::max(0, std::min(y1, state.row - 1));
    y2 = std::max(0, std::min(y2, state.row - 1));

    // Clear the cells and properly reset attributes
    for (int y = y1; y <= y2; y++) {
        for (int x = x1; x <= x2; x++) {
            Glyph& g = state.lines[y][x];
            g.u = ' ';
            g.mode = state.c.attrs & ~(ATTR_REVERSE | ATTR_BOLD | ATTR_ITALIC | ATTR_BLINK | ATTR_UNDERLINE);
            g.fg = state.c.fg;
            g.bg = state.c.bg;
            g.colorMode = state.c.colorMode;
            g.trueColorFg = state.c.trueColorFg;
            g.trueColorBg = state.c.trueColorBg;
        }
        state.dirty[y] = true;
    }
}

void Terminal::moveTo(int x, int y) {
    int miny, maxy;

    // Get scroll region bounds
    if (state.c.state & CURSOR_ORIGIN) {
        miny = state.top;
        maxy = state.bot;
    } else {
        miny = 0;
        maxy = state.row - 1;
    }

    int oldx = state.c.x;
    int oldy = state.c.y;

    // Constrain cursor position
    state.c.x = LIMIT(x, 0, state.col - 1);
    state.c.y = LIMIT(y, miny, maxy);

    // Reset wrap flag if moved
    if (oldx != state.c.x || oldy != state.c.y)
        state.c.state &= ~CURSOR_WRAPNEXT;
}


void Terminal::scrollUp(int orig, int n) {
    if (orig < 0 || orig >= state.row) return;
    if (n <= 0) return;

    n = std::min(n, state.bot - orig + 1);
    n = std::max(n, 0);

    // Add scrolled lines to scrollback when not in alt screen
    if (!(state.mode & MODE_ALTSCREEN)) {
        for (int i = orig; i < orig + n; ++i) {
            if (i < state.lines.size()) {
                addToScrollback(state.lines[i]);
            }
        }
    }

    // Existing scroll handling...
    for (int y = orig; y <= state.bot - n; ++y) {
        state.lines[y] = std::move(state.lines[y + n]);
    }

    // Clear revealed lines
    for (int i = state.bot - n + 1; i <= state.bot && i < state.row; i++) {
        state.lines[i].resize(state.col);
        for (int j = 0; j < state.col; j++) {
            Glyph& g = state.lines[i][j];
            g.u = ' ';
            g.mode = state.c.attrs;
            g.fg = state.c.fg;
            g.bg = state.c.bg;
        }
    }
}

void Terminal::scrollDown(int orig, int n) {
    if (orig < 0 || orig >= state.row) return;  // Safety check
    if (n <= 0) return;

    n = std::min(n, state.bot - orig + 1);
    n = std::max(n, 0);

    if (orig + n > state.row) return;

    // Mark lines as dirty
    for (int i = orig; i <= state.bot && i < state.row; i++) {
        state.dirty[i] = true;
    }

    // Move lines down with bounds checking
    for (int i = state.bot; i >= orig + n && i < state.row; i--) {
        state.lines[i] = std::move(state.lines[i - n]);
    }

    // Clear new lines
    for (int i = orig; i < orig + n && i < state.row; i++) {
        state.lines[i].resize(state.col);
        for (int j = 0; j < state.col; j++) {
            Glyph& g = state.lines[i][j];
            g.u = ' ';
            g.mode = state.c.attrs;
            g.fg = state.c.fg;
            g.bg = state.c.bg;
        }
    }
}


size_t Terminal::utf8Decode(const char* c, Rune* u, size_t clen) {
    *u = UTF_INVALID;
    size_t len = 0;
    Rune udecoded = 0;

    // Determine sequence length and initial byte decoding
    if ((c[0] & 0x80) == 0) {
        // ASCII character
        *u = c[0];
        return 1;
    } 
    else if ((c[0] & 0xE0) == 0xC0) {
        // 2-byte sequence
        len = 2;
        udecoded = c[0] & 0x1F;
    } 
    else if ((c[0] & 0xF0) == 0xE0) {
        // 3-byte sequence (used by box drawing characters)
        len = 3;
        udecoded = c[0] & 0x0F;
    } 
    else if ((c[0] & 0xF8) == 0xF0) {
        // 4-byte sequence
        len = 4;
        udecoded = c[0] & 0x07;
    } 
    else {
        std::cerr << "Invalid UTF-8 start byte: 0x" 
                  << std::hex << static_cast<int>(c[0]) 
                  << std::dec << std::endl;
        return 0;
    }

    // Validate sequence length
    if (clen < len) {
        std::cerr << "Incomplete UTF-8 sequence. Expected " << len 
                  << " bytes, got " << clen << std::endl;
        return 0;
    }

    // Process continuation bytes
    for (size_t i = 1; i < len; i++) {
        // Validate continuation byte
        if ((c[i] & 0xC0) != 0x80) {
            std::cerr << "Invalid continuation byte at position " << i 
                      << ": 0x" << std::hex << static_cast<int>(c[i]) 
                      << std::dec << std::endl;
            return 0;
        }
        
        // Shift and add continuation byte
        udecoded = (udecoded << 6) | (c[i] & 0x3F);
        
    }

    // Additional validation for decoded Unicode point
    if (!BETWEEN(udecoded, utfmin[len], utfmax[len]) || 
        BETWEEN(udecoded, 0xD800, 0xDFFF) || 
        udecoded > 0x10FFFF) {
        std::cerr << "Invalid Unicode code point: U+" 
                  << std::hex << udecoded << std::dec << std::endl;
        *u = UTF_INVALID;
        return 0;
    }



    *u = udecoded;
    return len;
}


void Terminal::handleCharset(char c) {
    static const struct {
        char code;
        Charset charset;
        const char* description;
    } charsetMap[] = {
        {'(', CS_USA, "US ASCII"},
        {')', CS_UK, "UK"},
        {'*', CS_MULTI, "Multilingual"},
        {'+', CS_GER, "German"},
        {'0', CS_GRAPHIC0, "Special Graphics"},
        {'A', CS_GER, "German"},
        {'B', CS_USA, "US ASCII"}
    };

    for (const auto& entry : charsetMap) {
        if (entry.code == c) {
            state.trantbl[state.charset] = entry.charset;
            break;
        }
    }
}

Terminal::Rune Terminal::utf8decodebyte(char c, size_t* i) {
    for (*i = 0; *i < 4; ++(*i))
        if ((unsigned char)c >= utfmask[*i] && (unsigned char)c < utfmask[*i + 1])
            return (unsigned char)c & ~utfmask[*i];

    return 0;
}


size_t Terminal::utf8validate(Rune* u, size_t i) {
    // Reject surrogate halves and out-of-range code points
    if (!BETWEEN(*u, utfmin[i], utfmax[i]) || 
        BETWEEN(*u, 0xD800, 0xDFFF) || 
        *u > 0x10FFFF) {
        *u = UTF_INVALID;
    }

    // Determine the correct UTF-8 sequence length
    for (i = 1; *u > utfmax[i]; ++i)
        ;

    return i;
}

void Terminal::handleDeviceStatusReport(const CSIEscape& csi) {
    switch (csi.args[0]) {
        case 5: // Operation Status Report
            processInput("\033[0n"); // OK
            break;
        case 6: // Cursor Position Report
            {
                char report[32];
                snprintf(report, sizeof(report), "\033[%d;%dR", 
                         state.c.y + 1, state.c.x + 1);
                processInput(report);
            }
            break;
        // Add more report types
    }
}

void Terminal::ringBell() {
    // Implement visual bell
    if (state.mode & MODE_VISUALBELL) {
        // Briefly invert screen colors
        for (auto& line : state.lines) {
            for (auto& glyph : line) {
                std::swap(glyph.fg, glyph.bg);
            }
        }
        // Schedule screen restoration
        std::thread([this]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            for (auto& line : state.lines) {
                for (auto& glyph : line) {
                    std::swap(glyph.fg, glyph.bg);
                }
            }
        }).detach();
    } else {
        // System bell or audio bell
        // Implement platform-specific bell
    }
}
void Terminal::resize(int cols, int rows) {
    std::lock_guard<std::mutex> lock(bufferMutex);
    
    // Ensure minimum dimensions
    cols = std::max(1, cols);
    rows = std::max(1, rows);
    
    // Don't resize if dimensions haven't changed
    if (cols == state.col && rows == state.row) {
        return;
    }
    
    try {
        // Create new buffers
        std::vector<std::vector<Glyph>> newLines(rows);
        std::vector<std::vector<Glyph>> newAltLines(rows);
        std::vector<bool> newDirty(rows, true);
        std::vector<bool> newTabs(cols, false);
        
        // Initialize the new lines
        for (int i = 0; i < rows; i++) {
            newLines[i].resize(cols);
            newAltLines[i].resize(cols);
            // Initialize with default glyphs if needed
            for (int j = 0; j < cols; j++) {
                newLines[i][j].u = ' ';
                newLines[i][j].mode = state.c.attrs;
                newLines[i][j].fg = state.c.fg;
                newLines[i][j].bg = state.c.bg;
            }
        }
        
        // Copy existing content
        int minRows = std::min(rows, state.row);
        int minCols = std::min(cols, state.col);
        
        for (int y = 0; y < minRows; y++) {
            for (int x = 0; x < minCols; x++) {
                if (y < state.lines.size() && x < state.lines[y].size()) {
                    newLines[y][x] = state.lines[y][x];
                }
            }
        }
        
        // Set new tab stops
        for (int i = 8; i < cols; i += 8) {
            newTabs[i] = true;
        }
        
        // Update terminal state
        state.row = rows;
        state.col = cols;
        state.bot = rows - 1;
        
        // Swap in new buffers
        state.lines = std::move(newLines);
        state.altLines = std::move(newAltLines);
        state.dirty = std::move(newDirty);
        state.tabs = std::move(newTabs);
        
        // Ensure cursor stays within bounds
        state.c.x = std::min(state.c.x, cols - 1);
        state.c.y = std::min(state.c.y, rows - 1);
        
        // Update PTY size if valid
        if (ptyFd >= 0) {
            struct winsize ws = {};
            ws.ws_row = rows;
            ws.ws_col = cols;
            ioctl(ptyFd, TIOCSWINSZ, &ws);
        }
    } catch (const std::exception& e) {
        std::cerr << "Error during resize: " << e.what() << std::endl;
    }
}



void Terminal::enableBracketedPaste() {
    // Send bracketed paste mode start sequence
    processInput("\033[?2004h");
    state.mode |= MODE_BRACKETPASTE;
}


void Terminal::disableBracketedPaste() {
    // Send bracketed paste mode end sequence
    processInput("\033[?2004l");
    state.mode &= ~MODE_BRACKETPASTE;
}

void Terminal::handlePastedContent(const std::string& content) {
    if (state.mode & MODE_BRACKETPASTE) {
        processInput("\033[200~"); // Start of paste
        processInput(content);
        processInput("\033[201~"); // End of paste
    } else {
        processInput(content);
    }
}
void Terminal::cursorSave() {
    savedCursor = state.c;
}

void Terminal::cursorLoad() {
    state.c = savedCursor;
    moveTo(state.c.x, state.c.y);
}


void Terminal::handleControlCode(unsigned char c) {
    switch(c) {
        case '\t':   // HT - Horizontal Tab
            tputtab(1);
            break;
        case '\b':   // BS - Backspace
            if (state.c.x > 0) {
                state.c.x--;
                state.c.state &= ~CURSOR_WRAPNEXT;
            }
            break;
        case '\r':   // CR - Carriage Return
            state.c.x = 0;
            state.c.state &= ~CURSOR_WRAPNEXT;
            break;
        case '\f':   // FF - Form Feed
        case '\v':   // VT - Vertical Tab
        case '\n':   // LF - Line Feed
            if (state.c.y == state.bot)
                scrollUp(state.top, 1);
            else
                state.c.y++;
            if (state.mode & MODE_CRLF)
                state.c.x = 0;
            state.c.state &= ~CURSOR_WRAPNEXT;
            break;
        case '\a':   // BEL - Bell
            ringBell();
            break;
        case 033:    // ESC - Escape
            state.esc = ESC_START;
            break;
    }
}

void Terminal::processInput(const std::string& input) {
    if (ptyFd < 0) return;
     if (state.mode & MODE_BRACKETPASTE) {
        if (input.substr(0, 4) == "\033[200~") {
            write(ptyFd, input.c_str(), input.length());
            return;
        }
        if (input.substr(0, 4) == "\033[201~") {
            write(ptyFd, input.c_str(), input.length());
            return;
        }
    }
    if (state.mode & MODE_APPCURSOR) {
        if (input == "\033[A") {
            write(ptyFd, "\033OA", 3); // Up
            return;
        }
        if (input == "\033[B") {
            write(ptyFd, "\033OB", 3); // Down
            return;
        }
        if (input == "\033[C") {
            write(ptyFd, "\033OC", 3); // Right
            return;
        }
        if (input == "\033[D") {
            write(ptyFd, "\033OD", 3); // Left
            return;
        }
    }

    if (input == "\r\n" || input == "\n") {
        write(ptyFd, "\r", 1);
        return;
    }
    
    if (input == "\b") {
        write(ptyFd, "\b \b", 3);
        return;
    }

    write(ptyFd, input.c_str(), input.length());
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
void Terminal::tputtab(int n) {
    uint x = state.c.x;

    if (n > 0) {
        while (x < state.col && n--) {
            // Find next tab stop
            do {
                x++;
            } while (x < state.col && !state.tabs[x]);
        }
    } else if (n < 0) {
        while (x > 0 && n++) {
            // Find previous tab stop
            do {
                x--;
            } while (x > 0 && !state.tabs[x]);
        }
    }

    state.c.x = LIMIT(x, 0, state.col - 1);
}


void Terminal::processStringSequence(const std::string& seq) {
    // Handle different types of string sequences
    if (seq.empty()) return;

    switch (seq[0]) {
        case ']':  // OSC - Operating System Command
            handleOSCSequence(seq);
            break;
        case 'P':  // DCS - Device Control String
            handleDCSSequence(seq);
            break;
        // Add handling for other sequence types
    }
}

void Terminal::handleOSCSequence(const std::string& seq) {
    // Example: Handle title setting
    if (seq.substr(0, 2) == "]0;" || seq.substr(0, 2) == "]2;") {
        // Extract title
        size_t titleEnd = seq.find('\007');
        if (titleEnd != std::string::npos) {
            std::string title = seq.substr(2, titleEnd - 2);
            // TODO: Set window title
            std::cout << "Window Title: " << title << std::endl;
        }
    }
}

void Terminal::handleDCSSequence(const std::string& seq) {
    // Placeholder for Device Control String handling
    // This can include things like terminal reports, device-specific commands
}


void Terminal::selectionInit() {
    sel.mode = SEL_IDLE;
    sel.snap = 0;
    sel.ob.x = -1;
}

void Terminal::selectionStart(int col, int row) {
    selectionClear();
    sel.mode = SEL_EMPTY;
    sel.type = SEL_REGULAR;
    sel.alt = state.mode & MODE_ALTSCREEN;
    sel.snap = 0;
    sel.oe.x = sel.ob.x = col;
    sel.oe.y = sel.ob.y = row;
    selectionNormalize();

    if (sel.snap != 0)
        sel.mode = SEL_READY;
}

void Terminal::selectionExtend(int col, int row) {
    if (sel.mode == SEL_IDLE)
        return;
    if (sel.mode == SEL_EMPTY) {
        sel.mode = SEL_SELECTING;
    }

    sel.oe.x = col;
    sel.oe.y = row;
    selectionNormalize();
}

void Terminal::selectionNormalize() {
    if (sel.type == SEL_REGULAR && sel.ob.y != sel.oe.y) {
        sel.nb.x = sel.ob.y < sel.oe.y ? sel.ob.x : sel.oe.x;
        sel.ne.x = sel.ob.y < sel.oe.y ? sel.oe.x : sel.ob.x;
    } else {
        sel.nb.x = std::min(sel.ob.x, sel.oe.x);
        sel.ne.x = std::max(sel.ob.x, sel.oe.x);
    }
    sel.nb.y = std::min(sel.ob.y, sel.oe.y);
    sel.ne.y = std::max(sel.ob.y, sel.oe.y);
}

void Terminal::selectionClear() {
    if (sel.ob.x == -1)
        return;
    sel.mode = SEL_IDLE;
    sel.ob.x = -1;
}

std::string Terminal::getSelection() {
    std::string str;
    if (sel.ob.x == -1)
        return str;
        
    for (int y = sel.nb.y; y <= sel.ne.y; y++) {
        int xstart = (y == sel.nb.y) ? sel.nb.x : 0;
        int xend = (y == sel.ne.y) ? sel.ne.x : state.col - 1;
        
        for (int x = xstart; x <= xend; x++) {
            if (state.lines[y][x].mode & ATTR_WDUMMY)
                continue;
            
            char buf[UTF_SIZ];
            size_t len = utf8Encode(state.lines[y][x].u, buf);
            str.append(buf, len);
        }
        
        if (y < sel.ne.y)
            str += '\n';
    }
    return str;
}


void Terminal::copySelection() {
    std::string selected = getSelection();
    if (!selected.empty()) {
        // Use ImGui's clipboard functions
        ImGui::SetClipboardText(selected.c_str());
    }
}
void Terminal::pasteFromClipboard() {
    const char* text = ImGui::GetClipboardText();
   
    // Minimal paste processing
    write(ptyFd, text, strlen(text));
}


bool Terminal::selectedText(int x, int y) {
    if (sel.mode == SEL_IDLE || sel.ob.x == -1 ||
        sel.alt != (state.mode & MODE_ALTSCREEN))
        return false;

    if (sel.type == SEL_RECTANGULAR)
        return BETWEEN(y, sel.nb.y, sel.ne.y) &&
               BETWEEN(x, sel.nb.x, sel.ne.x);

    return BETWEEN(y, sel.nb.y, sel.ne.y) &&
           (y != sel.nb.y || x >= sel.nb.x) &&
           (y != sel.ne.y || x <= sel.ne.x);
}



void Terminal::strparse() {
    // Parse string sequences into arguments
    strescseq.args.clear();
    std::string current;
    
    for (size_t i = 0; i < strescseq.len; i++) {
        char c = strescseq.buf[i];
        if (c == ';') {
            strescseq.args.push_back(current);
            current.clear();
        } else {
            current += c;
        }
    }
    if (!current.empty()) {
        strescseq.args.push_back(current);
    }
}

void Terminal::handleStringSequence() {
    if (strescseq.len == 0) return;

    switch (strescseq.type) {
        case ']': // OSC - Operating System Command
            if (strescseq.args.size() >= 2) {
                int cmd = std::atoi(strescseq.args[0].c_str());
                switch (cmd) {
                    case 0:  // Set window title and icon name
                    case 1:  // Set icon name
                    case 2:  // Set window title
                        // You would implement window title setting here
                        // For now, we'll just print it
                        std::cout << "Title: " << strescseq.args[1] << std::endl;
                        break;
                        
                    case 4: // Set/get color
                        handleOSCColor(strescseq.args);
                        break;
                        
                    case 52: // Manipulate selection data
                        handleOSCSelection(strescseq.args);
                        break;
                }
            }
            break;
            
        case 'P': // DCS - Device Control String
            handleDCS();
            break;
            
        case '_': // APC - Application Program Command
            // Not commonly used, implement if needed
            break;
            
        case '^': // PM - Privacy Message
            // Not commonly used, implement if needed
            break;
            
        case 'k': // Old title set compatibility
            // Set window title using old xterm sequence
            std::cout << "Old Title: " << strescseq.buf << std::endl;
            break;
    }
}

void Terminal::handleOSCColor(const std::vector<std::string>& args) {
    if (args.size() < 2) return;
    
    int index = std::atoi(args[1].c_str());
    if (args.size() > 2) {
        // Set color
        if (args[2][0] == '?') {
            // Color query - respond with current color
            char response[64];
            snprintf(response, sizeof(response), 
                    "\033]4;%d;rgb:%.2X/%.2X/%.2X\007",
                    index, 
                    (int)(state.c.fg.x * 255),
                    (int)(state.c.fg.y * 255),
                    (int)(state.c.fg.z * 255));
            processInput(response);
        } else {
            // Set color - parse color value (typically in rgb:RR/GG/BB format)
            // Implementation would go here
        }
    }
}

void Terminal::handleOSCSelection(const std::vector<std::string>& args) {
    if (args.size() < 3) return;
    
    // args[1] would contain the selection type (clipboard, primary, etc)
    // args[2] would contain the base64-encoded data
    
    // Example implementation:
    if (args[1] == "c") {  // clipboard
        std::string decoded; // You would implement base64 decoding
        ImGui::SetClipboardText(decoded.c_str());
    }
}



// Test sequence handler - DECALN alignment pattern
void Terminal::handleTestSequence(char c) {
    switch (c) {
        case '8': // DECALN - Screen Alignment Pattern
            // Fill screen with 'E'
            for (int y = 0; y < state.row; y++) {
                for (int x = 0; x < state.col; x++) {
                    Glyph g;
                    g.u = 'E';
                    g.mode = state.c.attrs;
                    g.fg = state.c.fg;
                    g.bg = state.c.bg;
                    writeGlyph(g, x, y);
                }
            }
            break;
    }
}
// Device Control String handler
void Terminal::handleDCS() {
    // Basic DCS sequence handling
    // This function is called when DCS sequences are received
    // For now, we'll only implement some basic DCS handling
    
    // Extract DCS sequence from strescseq
    if (strescseq.buf.empty()) return;

    // Example DCS sequence handling:
    // $q - DECRQSS (Request Status String)
    if (strescseq.buf.length() >= 2 && strescseq.buf.substr(0, 2) == "$q") {
        std::string param = strescseq.buf.substr(2);
        // Handle DECRQSS request
        if (param == "\"q") {  // DECSCA
            processInput("\033P1$r0\"q\033\\");  // Reply with default protection
        } else if (param == "r") {  // DECSTBM
            char response[40];
            snprintf(response, sizeof(response), "\033P1$r%d;%dr\033\\", 
                    state.top + 1, state.bot + 1);
            processInput(response);
        }
    }
}


void Terminal::tmoveato(int x, int y) {
    // Origin mode moves relative to scroll region
    if (state.c.state & CURSOR_ORIGIN)
        moveTo(x, y + state.top);
    else
        moveTo(x, y);
}

void Terminal::tsetmode(int priv, int set, const std::vector<int>& args) {
    // Mode setting per st.c
    int alt;
    
    for (int arg : args) {
        if (priv) {
            switch(arg) {
                case 1: // DECCKM -- Application cursor keys
                    setMode(set, MODE_APPCURSOR);
                    break;
                case 5: // DECSCNM -- Reverse video
                    // TODO: Implement screen reversal
                    break;
                case 6: // DECOM -- Origin
                    MODBIT(state.c.state, set, CURSOR_ORIGIN);
                    tmoveato(0, 0);
                    break;
                case 7: // DECAWM -- Auto wrap
                    MODBIT(state.mode, set, MODE_WRAP);
                    break;
                case 0:  // Error (IGNORED)
                case 2:  // DECANM -- ANSI/VT52 (IGNORED)
                case 3:  // DECCOLM -- Column  (IGNORED)
                case 4:  // DECSCLM -- Scroll (IGNORED)
                case 8:  // DECARM -- Auto repeat (IGNORED)
                    break;
                case 25: // DECTCEM -- Text Cursor Enable Mode
                    // Optional: handle cursor visibility
                    break;
                case 47: // swap screen
                case 1047: // alternate screen
                case 1049:
                    alt = (state.mode & MODE_ALTSCREEN) != 0;
                    if (set ^ alt) {
                        state.altLines.swap(state.lines);
                        state.mode ^= MODE_ALTSCREEN;
                    }
                case 1048:
                    (set) ? cursorSave() : cursorLoad();
                    break;
            }
        } else {
            switch(arg) {
                case 4:  // IRM -- Insertion-replacement
                    MODBIT(state.mode, set, MODE_INSERT);
                    break;
                case 20: // LNM -- Linefeed/new line
                    MODBIT(state.mode, set, MODE_CRLF);
                    break;
            }
        }
    }
}


void Terminal::addToScrollback(const std::vector<Glyph>& line) {
    scrollbackBuffer.push_back(line);
    if (scrollbackBuffer.size() > maxScrollbackLines) {
        scrollbackBuffer.erase(scrollbackBuffer.begin());
    }
}


