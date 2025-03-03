#include "editor_highlight.h"
#include "../files/files.h"
#include <algorithm>
#include <filesystem>
#include <iostream>

// Global instance
EditorHighlight gEditorHighlight;

EditorHighlight::EditorHighlight() : highlightingInProgress(false), cancelHighlightFlag(false) {}

void EditorHighlight::cancelHighlighting()
{
    cancelHighlightFlag = true;
    if (highlightFuture.valid()) {
        highlightFuture.wait();
    }
    cancelHighlightFlag = false;
}

void EditorHighlight::forceColorUpdate()
{
    pythonLexer.forceColorUpdate();
    cppLexer.forceColorUpdate();
    htmlLexer.forceColorUpdate();
    jsxLexer.forceColorUpdate();
}

bool EditorHighlight::validateHighlightContentParams(const std::string &content, const std::vector<ImVec4> &colors, int start_pos, int end_pos)
{
    if (content.empty()) {
        std::cerr << "Error: Empty content in highlightContent" << std::endl;
        return false;
    }
    if (colors.empty()) {
        std::cerr << "Error: Empty colors vector in highlightContent" << std::endl;
        return false;
    }
    if (content.size() != colors.size()) {
        std::cerr << "Error: Mismatch between content and colors size "
                     "in highlightContent"
                  << std::endl;
        return false;
    }
    if (start_pos < 0 || start_pos >= static_cast<int>(content.size())) {
        std::cerr << "Error: Invalid start_pos in highlightContent" << std::endl;
        return false;
    }
    if (end_pos < start_pos || end_pos > static_cast<int>(content.size())) {
        std::cerr << "Error: Invalid end_pos in highlightContent" << std::endl;
        return false;
    }
    return true;
}

void EditorHighlight::highlightContent(const std::string &content, std::vector<ImVec4> &colors, int start_pos, int end_pos)
{
    std::lock_guard<std::mutex> lock(highlight_mutex);
    std::cout << "\033[36mEditorHighlight:\033[0m Highlight Content. content size: " << content.size() << std::endl;

    // Cancel any ongoing highlighting first
    cancelHighlighting();

    // For large files (>100KB), just use default color
    const size_t LARGE_FILE_THRESHOLD = 100 * 1024;
    if (content.size() > LARGE_FILE_THRESHOLD) {
        // Pre-allocate colors vector to match content size
        colors.resize(content.size(), ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
        return;
    }

    // Validate inputs
    if (!validateHighlightContentParams(content, colors, start_pos, end_pos))
        return;

    // Pre-allocate vectors to avoid reallocation during async operation
    std::string content_copy = content;
    std::vector<ImVec4> colors_copy;
    colors_copy.reserve(content.size()); // Reserve space before copying
    colors_copy = colors;

    std::string currentFile = gFileExplorer.getCurrentFile();

    highlightingInProgress = true;
    cancelHighlightFlag = false;

    std::string extension = fs::path(currentFile).extension().string();

    // Launch highlighting task - properly capture colors by reference
    highlightFuture = std::async(std::launch::async, [this, content_copy, &colors, colors_copy = std::move(colors_copy), currentFile, start_pos, end_pos, extension]() mutable {
        try {
            if (cancelHighlightFlag) {
                highlightingInProgress = false;
                return;
            }

            // Apply highlighting
            if (extension == ".cpp" || extension == ".h" || extension == ".hpp") {
                cppLexer.applyHighlighting(content_copy, colors_copy, 0);
            } else if (extension == ".py") {
                pythonLexer.applyHighlighting(content_copy, colors_copy, 0);
            } else if (extension == ".html") {
                htmlLexer.applyHighlighting(content_copy, colors_copy, 0);
            } else if (extension == ".js" || extension == ".jsx") {
                jsxLexer.applyHighlighting(content_copy, colors_copy, 0);
            } else {
                std::fill(colors_copy.begin() + start_pos, colors_copy.begin() + end_pos, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
            }

            if (!cancelHighlightFlag && currentFile == gFileExplorer.getCurrentFile()) {
                std::lock_guard<std::mutex> colorsLock(colorsMutex);
                // Note: colors is captured by reference now
                if (colors.size() == colors_copy.size()) {
                    colors = std::move(colors_copy);
                }
            }
        } catch (const std::exception &e) {
            std::cerr << "Error in highlighting: " << e.what() << std::endl;
        }
        highlightingInProgress = false;
    });
}

void EditorHighlight::setTheme(const std::string &themeName) { loadTheme(themeName); }

void EditorHighlight::loadTheme(const std::string &themeName)
{
    auto &settings = gSettings.getSettings();
    if (settings.contains("themes") && settings["themes"].contains(themeName)) {
        auto &theme = settings["themes"][themeName];
        for (const auto &[key, value] : theme.items()) {
            themeColors[key] = ImVec4(value[0], value[1], value[2], value[3]);
        }
    } else {
        // Set default colors if theme not found
        themeColors["keyword"] = ImVec4(0.0f, 0.4f, 1.0f, 1.0f);
        themeColors["function"] = ImVec4(0.0f, 0.4f, 1.0f, 1.0f);
        themeColors["string"] = ImVec4(0.87f, 0.87f, 0.0f, 1.0f);
        themeColors["number"] = ImVec4(0.0f, 0.8f, 0.8f, 1.0f);
        themeColors["comment"] = ImVec4(0.5f, 0.5f, 0.5f, 1.0f);
    }
}
