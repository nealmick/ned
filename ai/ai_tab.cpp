#include "ai_tab.h"
#include "ai_open_router.h"
#include "editor.h"
#include <algorithm>
#include <fstream>
#include <iostream>

AITab gAITab;

AITab::AITab()
{
    key_loaded = load_key();
    if (key_loaded) {
        std::cout << "API key loaded successfully\n";
    }
}

AITab::~AITab()
{
    if (worker.joinable()) {
        worker.join();
        std::cout << "Worker thread cleaned up\n";
    }
}

bool AITab::load_key()
{
    const char *home_dir = getenv("HOME");
    if (!home_dir) {
        std::cerr << "Error: HOME environment variable not set\n";
        return false;
    }

    std::string key_path = std::string(home_dir) + "/ned/.openrouter";
    std::ifstream key_file(key_path);

    if (!key_file.is_open()) {
        std::cerr << "Error: Could not open OpenRouter key file at " << key_path << "\n";
        return false;
    }

    api_key = std::string((std::istreambuf_iterator<char>(key_file)), std::istreambuf_iterator<char>());

    // Trim whitespace
    api_key.erase(api_key.find_last_not_of(" \n\r\t") + 1);

    if (api_key.empty()) {
        std::cerr << "Error: API key is empty\n";
        return false;
    }

    return true;
}

void AITab::tab_complete()
{
    if (!key_loaded) {
        std::cerr << "Cannot complete - API key not loaded\n";
        return;
    }
    if (request_active) { // Changed check
        std::cout << "Request already in progress\n";
        return;
    }

    if (worker.joinable()) {
        if (request_done) {
            worker.join();
            request_done = false;
        } else {
            std::cout << "Request already in progress\n";
            return;
        }
    }

    std::cout << "Starting AI request...\n";
    request_active = true;
    worker = std::thread([this]() {
        std::string prompt = collect_context();
        response = OpenRouter::request(prompt, api_key);
        request_done = true;
        request_active = false;
    });
}

std::string AITab::collect_context() const
{
    const int cursor_pos = editor_state.cursor_index;

    // Find current line number
    int current_line = 0;
    for (size_t i = 1; i < editor_state.editor_content_lines.size(); i++) {
        if (cursor_pos < editor_state.editor_content_lines[i]) {
            current_line = i - 1;
            break;
        }
    }

    // Calculate line range (100 lines before/after)
    const int total_lines = editor_state.editor_content_lines.size();
    const int start_line = std::max(0, current_line - 100);
    const int end_line = std::min(total_lines - 1, current_line + 100);

    // Get line start/end indices
    const int context_start = editor_state.editor_content_lines[start_line];
    int context_end = editor_state.editor_content_lines[end_line];
    if (end_line + 1 < editor_state.editor_content_lines.size()) {
        context_end = editor_state.editor_content_lines[end_line + 1];
    }

    // Extract the context string
    std::string context = editor_state.fileContent.substr(context_start, context_end - context_start);

    // Build prompt with cursor marker
    std::string prompt = "Complete the code after the CURSOR marker.\n"
                         "Context:\n```\n" +
                         context.substr(0, cursor_pos - context_start) + "/* CURSOR */" + context.substr(cursor_pos - context_start) + "\n```\nProvide only the completion code without explanations.";

    std::cout << "\n=== Generated Prompt ===\n" << prompt << "\n=======================\n";

    return prompt;
}

void AITab::update()
{
    if (worker.joinable() && request_done) {
        worker.join();
        std::cout << "\n=== AI Response ===" << std::endl;
        std::cout << response << std::endl;
        std::cout << "===================\n" << std::endl;
        request_done = false;
    }
}