// Copyright 2024-2025 Aidan Sun and the ImGuiTextSelect contributors
// SPDX-License-Identifier: MIT

#define IMGUI_DEFINE_MATH_OPERATORS

#include "textselect.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <string>
#include <string_view>

#include <imgui.h>
#include <imgui_internal.h>
#include <utf8.h>

// Calculates the midpoint between two numbers.
template <typename T>
constexpr T midpoint(T a, T b) {
    return a + (b - a) / 2;
}

// Checks if a string view ends with the specified char suffix.
bool endsWith(std::string_view str, char suffix) {
    return !str.empty() && str.back() == suffix;
}

// Simple word boundary detection, accounts for Latin Unicode blocks only.
static bool isBoundary(char32_t c) {
    using Range = std::array<char32_t, 2>;
    std::array ranges{ Range{ 0x20, 0x2F }, Range{ 0x3A, 0x40 }, Range{ 0x5B, 0x60 }, Range{ 0x7B, 0xBF } };

    return std::find_if(ranges.begin(), ranges.end(), [c](const Range& r) { return c >= r[0] && c <= r[1]; })
        != ranges.end();
}

// Gets the number of UTF-8 characters (not bytes) in a string.
static std::size_t utf8Length(std::string_view s) {
    return utf8::unchecked::distance(s.begin(), s.end());
}

// Gets the display width of a substring.
static float substringSizeX(std::string_view s, std::size_t start, std::size_t length = std::string_view::npos) {
    // For an empty string, data() or begin() == end()
    if (s.empty()) {
        return 0;
    }

    // Convert char-based start and length into byte-based iterators
    auto stringStart = s.begin();
    utf8::unchecked::advance(stringStart, start);

    auto stringEnd = stringStart;
    if (length == std::string_view::npos) {
        stringEnd = s.end();
    } else {
        utf8::unchecked::advance(stringEnd, std::min(utf8Length(s), length));
    }

    // Dereferencing std::string_view::end() may be undefined behavior in some compilers,
    // because of that, we need to get the pointer value manually if stringEnd == s.end().
    const char* endPtr = stringEnd == s.end() ? s.data() + s.size() : &*stringEnd;

    // Calculate text size between start and end
    return ImGui::CalcTextSize(&*stringStart, endPtr).x;
}

// Gets the index of the character the mouse cursor is over.
static std::size_t getCharIndex(std::string_view s, float cursorPosX, std::size_t start, std::size_t end) {
    // Ignore cursor position when it is invalid
    if (cursorPosX < 0) {
        return 0;
    }

    // Check for exit conditions
    if (s.empty()) {
        return 0;
    }
    if (end < start) {
        return utf8Length(s);
    }

    // Midpoint of given string range
    std::size_t midIdx = midpoint(start, end);

    // Display width of the entire string up to the midpoint, gives the x-position where the (midIdx + 1)th char starts
    float widthToMid = substringSizeX(s, 0, midIdx + 1);

    // Same as above but exclusive, gives the x-position where the (midIdx)th char starts
    float widthToMidEx = substringSizeX(s, 0, midIdx);

    // Perform a recursive binary search to find the correct index
    // If the mouse position is between the (midIdx)th and (midIdx + 1)th character positions, the search ends
    if (cursorPosX < widthToMidEx) {
        return getCharIndex(s, cursorPosX, start, midIdx - 1);
    } else if (cursorPosX > widthToMid) {
        return getCharIndex(s, cursorPosX, midIdx + 1, end);
    } else {
        return midIdx;
    }
}

// Wrapper for getCharIndex providing the initial bounds.
static std::size_t getCharIndex(std::string_view s, float cursorPosX) {
    return getCharIndex(s, cursorPosX, 0, utf8Length(s));
}

// Gets the scroll delta for the given cursor position and window bounds.
static float getScrollDelta(float v, float min, float max) {
    const float deltaScale = 10.0f * ImGui::GetIO().DeltaTime;
    const float maxDelta = 100.0f;

    if (v < min) {
        return std::max(-(min - v), -maxDelta) * deltaScale;
    } else if (v > max) {
        return std::min(v - max, maxDelta) * deltaScale;
    }

    return 0.0f;
}

TextSelect::Selection TextSelect::getSelection() const {
    // Start and end may be out of order (ordering is based on Y position)
    bool startBeforeEnd = selectStart.y < selectEnd.y || (selectStart.y == selectEnd.y && selectStart.x < selectEnd.x);

    // Reorder X points if necessary
    std::size_t startX = startBeforeEnd ? selectStart.x : selectEnd.x;
    std::size_t endX = startBeforeEnd ? selectEnd.x : selectStart.x;

    // Get min and max Y positions for start and end
    std::size_t startY = std::min(selectStart.y, selectEnd.y);
    std::size_t endY = std::max(selectStart.y, selectEnd.y);

    return { startX, startY, endX, endY };
}

//
// Taken from imgui_draw.cpp
//
// Trim trailing space and find beginning of next line
static inline const char* CalcWordWrapNextLineStartA(const char* text, const char* text_end) {
    while (text < text_end && ImCharIsBlankA(*text)) text++;
    if (*text == '\n') text++;
    return text;
}

// Split `text` that does not fit in `wrapWidth` into multiple lines.
// result.size() is never 0.
static ImVector<std::string_view> wrapText(std::string_view text, float wrapWidth, ImFont* font) {
    ImVector<std::string_view> result;

    const char* textEnd = text.data() + text.size();
    const char* wrappedLineStart = text.data();
    const char* wrappedLineEnd = text.data();
    while (wrappedLineEnd != textEnd) {
        wrappedLineStart = wrappedLineEnd;
        wrappedLineEnd = font->CalcWordWrapPositionA(1, wrappedLineStart, textEnd, wrapWidth);

        if (wrappedLineEnd - wrappedLineStart != 0) {
            result.push_back({ wrappedLineStart, static_cast<std::size_t>(wrappedLineEnd - wrappedLineStart) });
        }

        wrappedLineEnd = CalcWordWrapNextLineStartA(wrappedLineEnd, textEnd);
    }

    // Treat empty text as one empty line.
    if (result.size() == 0) {
        result.push_back({ text.data(), 0 });
    }

    return result;
}

ImVector<TextSelect::SubLine> TextSelect::getSubLines() const {
    ImVector<SubLine> result;

    std::size_t numLines = getNumLines();

    // There will be at minimum `numLines` sublines.
    result.reserve(numLines);

    ImGuiWindow* window = ImGui::GetCurrentWindow();
    ImFont* font = ImGui::GetCurrentContext()->Font;
    const float wrapWidth = ImGui::CalcWrapWidthForPos(window->DC.CursorPos, 0);

    for (std::size_t i = 0; i < numLines; ++i) {
        auto wholeLine = getLineAtIdx(i);
        if (enableWordWrap) {
            auto subLines = wrapText(wholeLine, wrapWidth, font);
            for (auto subLine : subLines) {
                result.push_back({ subLine, i });
            }
        } else {
            result.push_back({ wholeLine, i });
        }
    }
    return result;
}

void TextSelect::handleMouseDown(const ImVector<TextSelect::SubLine>& subLines, const ImVec2& cursorPosStart) {
    if (subLines.size() == 0) {
        return;
    }

    ImGuiWindow* window = ImGui::GetCurrentWindow();
    const float wrapWidth = ImGui::CalcWrapWidthForPos(window->DC.CursorPos, 0);
    const float textHeight = ImGui::GetTextLineHeight();
    const float itemSpacing = ImGui::GetCurrentContext()->Style.ItemSpacing.y;
    ImVec2 mousePos = ImGui::GetMousePos() - cursorPosStart;

    // Find the index of the sub line under the cursor.
    std::size_t subY = 0;
    float accumulatedHeight = textHeight;
    for (std::size_t i = 1; i < subLines.size(); ++i) {
        if (mousePos.y < accumulatedHeight) {
            break;
        }
        ++subY;
        accumulatedHeight += textHeight;
        // Don't add spacing between sublines, only between whole lines.
        if (subLines[i].wholeLineIndex != subLines[i - 1].wholeLineIndex) {
            accumulatedHeight += itemSpacing;
        }
    }

    std::string_view currentSubLine = subLines[subY].string;

    std::size_t wholeY = subLines[subY].wholeLineIndex;

    std::string_view currentWholeLine = getLineAtIdx(wholeY);

    std::size_t charsInWholeLineBeforeSubLine = utf8Length(
        { currentWholeLine.data(), static_cast<std::size_t>(currentSubLine.data() - currentWholeLine.data()) });

    std::size_t wholeX = getCharIndex(currentSubLine, mousePos.x) + charsInWholeLineBeforeSubLine;

    // Get mouse click count and determine action
    if (int mouseClicks = ImGui::GetMouseClickedCount(ImGuiMouseButton_Left); mouseClicks > 0) {
        if (mouseClicks % 3 == 0) {
            // Triple click - select whole line

            // Find the first sub line in the whole line
            std::size_t firstSubLineIndex = subY;
            while (firstSubLineIndex > 0
                && subLines[firstSubLineIndex - 1].wholeLineIndex == subLines[firstSubLineIndex].wholeLineIndex) {
                --firstSubLineIndex;
            }

            // Find the last sub line in the whole line
            std::size_t lastSubLineIndex = subY;
            while (lastSubLineIndex < subLines.size() - 1
                && subLines[lastSubLineIndex + 1].wholeLineIndex == subLines[lastSubLineIndex].wholeLineIndex) {
                ++lastSubLineIndex;
            }

            bool atLastLine = wholeY == (getNumLines() - 1);
            selectStart = { 0, wholeY };
            selectEnd = { atLastLine ? utf8Length(currentWholeLine) : 0, atLastLine ? wholeY : wholeY + 1 };
        } else if (mouseClicks % 2 == 0) {
            // Double click - select word
            // Initialize start and end iterators to current cursor position
            utf8::unchecked::iterator startIt{ currentWholeLine.data() };
            utf8::unchecked::iterator endIt{ currentWholeLine.data() };
            for (std::size_t i = 0; i < wholeX; i++) {
                startIt++;
                endIt++;
            }

            bool isCurrentBoundary = isBoundary(*startIt);

            // Scan to left until a word boundary is reached
            for (std::size_t startInv = 0; startInv <= wholeX; startInv++) {
                if (isBoundary(*startIt) != isCurrentBoundary) {
                    break;
                }
                selectStart = { wholeX - startInv, wholeY };
                startIt--;
            }

            // Scan to right until a word boundary is reached
            for (std::size_t end = wholeX; end <= utf8Length(currentWholeLine); end++) {
                selectEnd = { end, wholeY };
                if (isBoundary(*endIt) != isCurrentBoundary) {
                    break;
                }
                endIt++;
            }
        } else if (ImGui::IsKeyDown(ImGuiMod_Shift)) {
            // Single click with shift - select text from start to click
            // The selection starts from the beginning if no start position exists
            if (selectStart.isInvalid()) {
                selectStart = { 0, 0 };
            }

            selectEnd = { wholeX, wholeY };
        } else {
            // Single click - set start position, invalidate end position
            selectStart = { wholeX, wholeY };
            selectEnd = { std::string_view::npos, std::string_view::npos };
        }
    } else if (ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
        // Mouse dragging - set end position
        selectEnd = { wholeX, wholeY };
    }
}

void TextSelect::handleScrolling() const {
    // Window boundaries
    ImVec2 windowMin = ImGui::GetWindowPos();
    ImVec2 windowMax = windowMin + ImGui::GetWindowSize();

    // Get current and active window information from Dear ImGui state
    ImGuiWindow* currentWindow = ImGui::GetCurrentWindow();
    const ImGuiWindow* activeWindow = GImGui->ActiveIdWindow;

    ImGuiID scrollXID = ImGui::GetWindowScrollbarID(currentWindow, ImGuiAxis_X);
    ImGuiID scrollYID = ImGui::GetWindowScrollbarID(currentWindow, ImGuiAxis_Y);
    ImGuiID activeID = ImGui::GetActiveID();
    bool scrollbarsActive = activeID == scrollXID || activeID == scrollYID;

    // Do not handle scrolling if:
    // - There is no active window
    // - The current window is not active
    // - The user is scrolling via the scrollbars
    if (activeWindow == nullptr || activeWindow->ID != currentWindow->ID || scrollbarsActive) {
        return;
    }

    // Get scroll deltas from mouse position
    ImVec2 mousePos = ImGui::GetMousePos();
    float scrollXDelta = getScrollDelta(mousePos.x, windowMin.x, windowMax.x);
    float scrollYDelta = getScrollDelta(mousePos.y, windowMin.y, windowMax.y);

    // If there is a nonzero delta, scroll in that direction
    if (std::abs(scrollXDelta) > 0.0f) {
        ImGui::SetScrollX(ImGui::GetScrollX() + scrollXDelta);
    }
    if (std::abs(scrollYDelta) > 0.0f) {
        ImGui::SetScrollY(ImGui::GetScrollY() + scrollYDelta);
    }
}

static void drawSelectionRect(const ImVec2& cursorPosStart, float minX, float minY, float maxX, float maxY) {
    // Get rectangle corner points offset from the cursor's start position in the window
    ImVec2 rectMin = cursorPosStart + ImVec2{ minX, minY };
    ImVec2 rectMax = cursorPosStart + ImVec2{ maxX, maxY };

    // Draw the rectangle
    ImU32 color = ImGui::GetColorU32(ImGuiCol_TextSelectedBg);
    ImGui::GetWindowDrawList()->AddRectFilled(rectMin, rectMax, color);
}

void TextSelect::drawSelection(const ImVector<TextSelect::SubLine>& subLines, const ImVec2& cursorPosStart) const {
    if (!hasSelection()) {
        return;
    }

    // Start and end positions
    auto [startX, startY, endX, endY] = getSelection();

    std::size_t numLines = getNumLines();
    if (startY >= numLines || endY >= numLines) {
        return;
    }

    float accumulatedHeight = 0;

    ImGuiContext* context = ImGui::GetCurrentContext();
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    ImFont* font = context->Font;
    const float wrapWidth = ImGui::CalcWrapWidthForPos(window->DC.CursorPos, 0);
    const float newlineWidth = ImGui::CalcTextSize(" ").x;
    const float textHeight = context->FontSize;
    const float itemSpacing = context->Style.ItemSpacing.y;

    for (auto subLine : subLines) {
        auto wholeLine = getLineAtIdx(subLine.wholeLineIndex);

        auto wholeLineEnd = wholeLine.data() + wholeLine.size();

        const char* subLineStart = subLine.string.data();
        const char* subLineEnd = subLine.string.data() + subLine.string.size();

        // Indices of sub-line bounds relative to the start of the whole line.
        std::size_t subLineStartX = utf8::distance(wholeLine.data(), subLineStart);
        std::size_t subLineEndX = utf8::distance(wholeLine.data(), subLineEnd);

        float minY = accumulatedHeight;
        accumulatedHeight += textHeight;
        // Item spacing is not applied between sub-lines
        if (subLineEnd == wholeLineEnd) {
            // We are rendering last sub-line.
            accumulatedHeight += itemSpacing;
        }
        float maxY = accumulatedHeight;

        // Skip whole/sub lines before selection.
        if (startY > subLine.wholeLineIndex || subLine.wholeLineIndex == startY && startX >= subLineEndX) {
            continue;
        }
        // Skip whole/sub lines after selection.
        if (endY < subLine.wholeLineIndex || subLine.wholeLineIndex == endY && endX < subLineStartX) {
            break;
        }

        // The first and last rectangles should only extend to the selection boundaries
        // The middle rectangles (if any) enclose the entire line + some extra width for the newline.
        bool isStartSubLine = subLine.wholeLineIndex == startY && subLineStartX <= startX && startX <= subLineEndX;
        bool isEndSubLine = subLine.wholeLineIndex == endY && subLineStartX <= endX && endX <= subLineEndX;

        float minX = isStartSubLine ? substringSizeX(subLine.string, 0, startX - std::min(subLineStartX, startX)) : 0;
        float maxX = isEndSubLine ? substringSizeX(subLine.string, 0, endX - std::min(subLineStartX, endX))
                                  : substringSizeX(subLine.string, 0) + newlineWidth;

        drawSelectionRect(cursorPosStart, minX, minY, maxX, maxY);
    }
}

void TextSelect::copy() const {
    if (!hasSelection()) {
        return;
    }

    auto [startX, startY, endX, endY] = getSelection();

    // Collect selected text in a single string
    std::string selectedText;

    for (std::size_t i = startY; i <= endY; i++) {
        // Similar logic to drawing selections
        std::size_t subStart = i == startY ? startX : 0;
        std::string_view line = getLineAtIdx(i);

        auto stringStart = line.begin();
        utf8::unchecked::advance(stringStart, subStart);

        auto stringEnd = stringStart;
        if (i == endY) {
            utf8::unchecked::advance(stringEnd, endX - subStart);
        } else {
            stringEnd = line.end();
        }

        std::string_view lineToAdd = line.substr(stringStart - line.begin(), stringEnd - stringStart);
        selectedText += lineToAdd;

        // If lines before the last line don't already end with newlines, add them in
        if (!endsWith(lineToAdd, '\n') && i < endY) {
            selectedText += '\n';
        }
    }

    ImGui::SetClipboardText(selectedText.c_str());
}

void TextSelect::selectAll() {
    std::size_t lastLineIdx = getNumLines() - 1;
    std::string_view lastLine = getLineAtIdx(lastLineIdx);

    // Set the selection range from the beginning to the end of the last line
    selectStart = { 0, 0 };
    selectEnd = { utf8Length(lastLine), lastLineIdx };
}

void TextSelect::update() {
    // ImGui::GetCursorStartPos() is in window coordinates so it is added to the window position
    ImVec2 cursorPosStart = ImGui::GetWindowPos() + ImGui::GetCursorStartPos();

    // Switch cursors if the window is hovered
    bool hovered = ImGui::IsWindowHovered();
    if (hovered) {
        ImGui::SetMouseCursor(ImGuiMouseCursor_TextInput);
    }

    // Split whole lines by wrap width (if enabled).
    auto subLines = getSubLines();

    // Handle mouse events
    if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
        if (hovered) {
            shouldHandleMouseDown = true;
        }
    }

    if (ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
        shouldHandleMouseDown = false;
    }

    if (ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
        if (shouldHandleMouseDown) {
            handleMouseDown(subLines, cursorPosStart);
        }
        if (!hovered) {
            handleScrolling();
        }
    }

    drawSelection(subLines, cursorPosStart);

    // Keyboard shortcuts
    if (ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_A)) {
        selectAll();
    } else if (ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_C)) {
        copy();
    }
}
