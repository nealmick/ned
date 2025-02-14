// debug_console.h
#pragma once
#include <sstream>
#include <streambuf>
#include <string>
#include <vector>
#include "imgui.h"
#include <deque>


class DebugConsole {
private:
    // Forward declare ConsoleBuf
    class ConsoleBuf;
    
public:
    static DebugConsole& getInstance() {
        static DebugConsole instance;
        return instance;
    }

    void render();
    void addLog(const std::string& message);
    void clear() { logs.clear(); }
    void toggleVisibility() { isVisible = !isVisible; }
    bool isConsoleVisible() const { return isVisible; }

private:
    // Constructor now properly initializes consoleBuf
    DebugConsole();
    std::deque<std::string> logs;

    bool isVisible;
    
    // Move ConsoleBuf class definition here
    class ConsoleBuf : public std::streambuf {
    public:
        ConsoleBuf(DebugConsole& console) : console(console) {}
    
    protected:
        virtual int_type overflow(int_type c = traits_type::eof()) {
            if (c != traits_type::eof()) {
                if (c == '\n') {
                    buffer += '\n';
                    console.addLog(buffer);
                    buffer.clear();
                } else {
                    buffer += static_cast<char>(c);
                }
            }
            return c;
        }

    private:
        std::string buffer;
        DebugConsole& console;
    };

    ConsoleBuf consoleBuf;
    std::streambuf* oldCoutBuf;

    friend class ConsoleBuf;
};

extern DebugConsole& gDebugConsole;