#pragma once
#include <stdint.h>
#include <sys/types.h>
#include "imgui.h"
#include <string>
#include <vector>
#include <thread>
#include <mutex>
#include <ctype.h>



#define MIN(a, b)       ((a) < (b) ? (a) : (b))
#define MAX(a, b)       ((a) < (b) ? (b) : (a))
#define BETWEEN(x, a, b) ((a) <= (x) && (x) <= (b))
#define LIMIT(x, a, b)   (x) = (x) < (a) ? (a) : (x) > (b) ? (b) : (x)
#define MODBIT(x, set, bit) ((set) ? ((x) |= (bit)) : ((x) &= ~(bit)))

#define ISCONTROLC0(c)     (BETWEEN(c, 0, 0x1f) || (c) == 0x7f)
#define ISCONTROLC1(c)     (BETWEEN(c, 0x80, 0x9f))
#define ISCONTROL(c)       (ISCONTROLC0(c) || ISCONTROLC1(c))

typedef unsigned char uchar;

static constexpr size_t UTF_SIZ = 4;




class Terminal {
public:
    // Common type definitions
    using Rune = uint_least32_t;
    
    // Glyph attributes (matching st's glyph_attribute)
    enum Attribute {
        ATTR_NULL       = 0,
        ATTR_BOLD       = 1 << 0,
        ATTR_FAINT      = 1 << 1,
        ATTR_ITALIC     = 1 << 2,
        ATTR_UNDERLINE  = 1 << 3,
        ATTR_BLINK      = 1 << 4,
        ATTR_REVERSE    = 1 << 5,
        ATTR_INVISIBLE  = 1 << 6,
        ATTR_STRUCK     = 1 << 7,
        ATTR_WRAP       = 1 << 8,
        ATTR_WIDE       = 1 << 9,
        ATTR_WDUMMY     = 1 << 10,
        ATTR_BOLD_FAINT = ATTR_BOLD | ATTR_FAINT,
    };

    // Terminal modes
    enum Mode {
        MODE_WRAP        = 1 << 0,
        MODE_INSERT      = 1 << 1,
        MODE_ALTSCREEN   = 1 << 2,
        MODE_CRLF        = 1 << 3,
        MODE_ECHO        = 1 << 4,
        MODE_PRINT       = 1 << 5,
        MODE_UTF8        = 1 << 6,
        MODE_SIXEL       = 1 << 7,
        MODE_BRACKETPASTE = 1 << 8,
        MODE_APPCURSOR    = 1 << 9,
        MODE_MOUSEBTN    = 1 << 10,
        MODE_MOUSESGR    = 1 << 11,
        MODE_MOUSEX10    = 1 << 12,
        MODE_MOUSEMANY   = 1 << 13,
        MODE_SMOOTHSCROLL = 1 << 14,
        MODE_VISUALBELL   = 1 << 15
    };

    struct STREscape {
        char type;                    // ESC type
        std::string buf;             // Raw string buffer
        size_t len{0};               // Raw string length
        size_t siz{256};             // Buffer size
        std::vector<std::string> args;  // Arguments
    };



    // Selection modes (matching st's selection_mode)
       
    enum SelectionMode {
        SEL_IDLE = 0,
        SEL_EMPTY = 1,
        SEL_READY = 2,
        SEL_SELECTING = 3
    };

    // Selection types
    enum SelectionType {
        SEL_REGULAR = 1,
        SEL_RECTANGULAR = 2
    };

    enum CursorState {
        CURSOR_DEFAULT  = 0,
        CURSOR_WRAPNEXT = 1,
        CURSOR_ORIGIN   = 2
    };

    enum EscapeState {
        ESC_START      = 1,
        ESC_CSI        = 2,
        ESC_STR        = 4,
        ESC_ALTCHARSET = 8,
        ESC_STR_END    = 16,
        ESC_UTF8       = 64,
        ESC_TEST       = 32,
        ESC_APPCURSOR  = 128
    };

    enum ColorMode {
        COLOR_BASIC = 0,
        COLOR_256 = 1,
        COLOR_TRUE = 2
    };

    // Core structures
    struct Glyph {
        Rune u{' '};           // character code
        uint16_t mode{0};      // attribute flags
        ImVec4 fg{1.0f, 1.0f, 1.0f, 1.0f};  // foreground color
        ImVec4 bg{0.0f, 0.0f, 0.0f, 1.0f};  // background color
        ColorMode colorMode{COLOR_BASIC};   // Color mode
        uint32_t trueColorFg{0xFFFFFFFF};   // True color foreground
        uint32_t trueColorBg{0xFF000000};   // True color background
    };
    struct TCursor {
        int x{0};
        int y{0};
        Glyph attr;
        uint16_t attrs{0};  // For ATTR_* flags
        uint8_t state{0}; 
        ImVec4 fg{1.0f, 1.0f, 1.0f, 1.0f};
        ImVec4 bg{0.0f, 0.0f, 0.0f, 1.0f};
        // Add these new members
        ColorMode colorMode{COLOR_BASIC};
        uint32_t trueColorFg{0xFFFFFFFF};
        uint32_t trueColorBg{0xFF000000};
    };

    struct Selection {
        SelectionMode mode{SEL_IDLE};
        SelectionType type{SEL_REGULAR};
        int snap{0};
        struct {
            int x, y;
        } nb, ne, ob, oe;  // normalized begin/end, original begin/end
        int alt{0};
    };

    // Constructor/Destructor
    Terminal();
    ~Terminal();

    // Public interface
    void render();
    void toggleVisibility();
    bool isTerminalVisible() const { return isVisible; }
    void resize(int cols, int rows);

    void pasteFromClipboard();
    bool selectedText(int x, int y);

private:
    // Terminal state
    struct TermState {
        TCursor c;                     // Current cursor
        int row{0};                    // number of rows
        int col{0};                    // number of columns
        int top{0};                    // scroll region top
        int bot{0};                    // scroll region bottom
        int icharset{0};               // Selected charset for sequence
        uint32_t mode{MODE_WRAP | MODE_UTF8};  // terminal mode flags
        int esc{0};                    // escape state flags
        char trantbl[4]{};            // charset table translation
        int charset{0};                // current charset
        Rune lastc{0};                // last printed char outside of sequence
        std::vector<std::vector<Glyph>> lines;  // screen
        std::vector<std::vector<Glyph>> altLines; // alternate screen
        std::vector<bool> dirty;       // dirtyness of lines
        std::vector<bool> tabs;        // Tab stops
    } state;


    struct CSIEscape {
        char buf[256];  // Raw string
        size_t len;     // Raw string length
        char priv;      // Private mode
        std::vector<int> args;  // Arguments
        char mode[2];   // Final character(s)
    };

    // Selection state
    Selection sel;

    // Thread and synchronization
    std::mutex bufferMutex;
    std::thread readThread;
    bool shouldTerminate{false};

    // Terminal configuration
    bool isVisible{false};
    int ptyFd{-1};
    pid_t childPid{-1};


    CSIEscape csiescseq;


    // Core functions
    void startShell();
    void readOutput();
    void writeToShell(const char* s, size_t len);
    void processInput(const std::string& input);
    
    // Terminal operations
    void clearRegion(int x1, int y1, int x2, int y2);
    void scrollUp(int orig, int n);
    void scrollDown(int orig, int n);
    void moveTo(int x, int y);
    void moveAbsolute(int x, int y);
    
    // Sequence handling
    void handleCSISequence(const std::string& seq);
    void handleSGR(const std::vector<int>& params);
    void handleEscape(char c);
    void handleControlCode(unsigned char c);
    
    // UTF-8 handling
    size_t utf8Decode(const char* c, Rune* u, size_t clen);
    size_t utf8Encode(Rune u, char* c);
    
    // Drawing
    void renderBuffer();
    void drawRegion(int x1, int y1, int x2, int y2);

    // Utility functions
    void reset();
    void dirty(int top, int bot);


    void writeGlyph(const Glyph& g, int x, int y);
    void writeToBuffer(const char* data, size_t length);
    void parseCSIParam(CSIEscape& csi); 
    void handleCSI(const CSIEscape& csi);

    static constexpr const unsigned char utfmask[5] = {0xC0, 0x80, 0xE0, 0xF0, 0xF8};
    static constexpr const Rune utfmin[5] = {0, 0, 0x80, 0x800, 0x10000};
    static constexpr const Rune utfmax[5] = {0x10FFFF, 0x7F, 0x7FF, 0xFFFF, 0x10FFFF};
    static const Rune UTF_INVALID = 0xFFFD;  // Unicode replacement character


    TCursor savedCursor;  // For cursor save/restore

    Rune utf8decodebyte(char c, size_t* i);
    size_t utf8validate(Rune* u, size_t i);
    void tputtab(int n);
    void cursorSave();
    void cursorLoad();

    void setMode(bool set, int mode);
    void handleCharset(char c);
    void handleGraphicCharset(Rune& u);


    int eschandle(unsigned char ascii);
    void tnewline(int first_col);
    void tstrsequence(unsigned char c);
    void tmoveto(int x, int y);


    void processStringSequence(const std::string& seq);
    void handleOSCSequence(const std::string& seq);
    void handleDCSSequence(const std::string& seq);



    void selectionInit();
    void selectionStart(int col, int row);
    void selectionExtend(int col, int row);
    void selectionClear();
    std::string getSelection();
    void copySelection();

    void handleMouseEvent(int button, int x, int y);
    void processMouseReport(int button, int x, int y);


    std::vector<std::vector<Glyph>> scrollbackBuffer;
    size_t maxScrollbackLines = 10000;

    void addToScrollback(const std::vector<Glyph>& line);
    void scrollbackScroll(int lines);




    void handleDeviceStatusReport(const CSIEscape& csi);
    void ringBell();
    void enableBracketedPaste();
    void disableBracketedPaste();
    void handlePastedContent(const std::string& content);




    enum Charset {
        CS_GRAPHIC0,
        CS_UK,
        CS_USA,
        CS_MULTI,
        CS_GER,
        CS_FIN
    };

    STREscape strescseq;
    
    void writeChar(unsigned char c);
    
    void handleTestSequence(char c);
    void handleDCS();
    void selectionNormalize();
    void strparse();
    void handleStringSequence();
    void handleOSCColor(const std::vector<std::string>& args);
    void handleOSCSelection(const std::vector<std::string>& args);
    

    void tmoveato(int x, int y);  // Absolute move with origin mode
    void tsetmode(int priv, int set, const std::vector<int>& args);


    static constexpr const ImVec4 defaultColorMap[16] = {
        // Standard colors
        ImVec4(0.0f, 0.0f, 0.0f, 1.0f),     // Black
        ImVec4(0.5f, 0.0f, 0.0f, 1.0f),     // Red
        ImVec4(0.0f, 0.5f, 0.0f, 1.0f),     // Green
        ImVec4(0.5f, 0.5f, 0.0f, 1.0f),     // Yellow
        ImVec4(0.0f, 0.0f, 0.5f, 1.0f),     // Blue
        ImVec4(0.5f, 0.0f, 0.5f, 1.0f),     // Magenta
        ImVec4(0.0f, 0.5f, 0.5f, 1.0f),     // Cyan
        ImVec4(0.75f, 0.75f, 0.75f, 1.0f),  // White

        // Bright colors
        ImVec4(0.5f, 0.5f, 0.5f, 1.0f),     // Bright Black (Gray)
        ImVec4(1.0f, 0.0f, 0.0f, 1.0f),     // Bright Red
        ImVec4(0.0f, 1.0f, 0.0f, 1.0f),     // Bright Green
        ImVec4(1.0f, 1.0f, 0.0f, 1.0f),     // Bright Yellow
        ImVec4(0.0f, 0.0f, 1.0f, 1.0f),     // Bright Blue
        ImVec4(1.0f, 0.0f, 1.0f, 1.0f),     // Bright Magenta
        ImVec4(0.0f, 1.0f, 1.0f, 1.0f),     // Bright Cyan
        ImVec4(1.0f, 1.0f, 1.0f, 1.0f)      // Bright White
    };

    


};

extern Terminal gTerminal;