#include "ai_tab.h"
#include "../editor/editor.h"
#include "../files/files.h"
#include "ai_open_router.h"
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

    // Reduced context window for better focus (10 lines before, 5 after)
    const int total_lines = editor_state.editor_content_lines.size();
    const int start_line = std::max(0, current_line - 10);
    const int end_line = std::min(total_lines - 1, current_line + 5);

    // Get line start/end indices
    const int context_start = editor_state.editor_content_lines[start_line];
    int context_end = editor_state.editor_content_lines[end_line];
    if (end_line + 1 < editor_state.editor_content_lines.size()) {
        context_end = editor_state.editor_content_lines[end_line + 1];
    }

    // Extract the context string
    std::string context = editor_state.fileContent.substr(context_start, context_end - context_start);

    // Insert unambiguous cursor marker with newlines for visibility
    const size_t cursor_pos_in_context = cursor_pos - context_start;
    context.insert(cursor_pos_in_context, "\n [[CURSOR]]\n");

    // Format prompt with clear structure
    std::string prompt = "You are a code completion assistant. Replace [[CURSOR]] with appropriate code.\n"
                         "File: " +
                         gFileExplorer.currentFile +
                         "\n"
                         "Code:\n"
                         "```" +
                         get_file_extension() + "\n" + context +
                         "\n```\n"
                         "Respond only with the code that should replace [[CURSOR]]. No markdown, no explanations.\n";

    std::cout << "\n=== Generated Prompt ===\n" << prompt << "\n=======================\n";

    return prompt;
}

// Helper to get file extension for syntax highlighting
std::string AITab::get_file_extension() const
{
    size_t dot_pos = gFileExplorer.currentFile.find_last_of('.');
    if (dot_pos != std::string::npos) {
        return gFileExplorer.currentFile.substr(dot_pos + 1);
    }
    return "txt";
}

void AITab::update()
{
    if (worker.joinable() && request_done) {
        worker.join();

        std::cout << "\n=== AI Response ===" << std::endl;
        std::cout << response << std::endl;
        std::cout << "===================\n";

        if (!response.empty() && response.find("error") == std::string::npos) {
            insert(response);
        } else {
            std::cerr << "Error in AI response: " << response << "\n";
        }

        request_done = false;
    }
}
void AITab::insert(const std::string &code)
{
    if (code.empty()) {
        std::cerr << "No code to insert\n";
        return;
    }

    // Remove trailing newline if present
    std::string cleaned_code = code;
    if (!cleaned_code.empty() && cleaned_code.back() == '\n') {
        cleaned_code.pop_back();
    }

    const int original_pos = editor_state.cursor_index;
    int insert_pos = original_pos;
    int new_cursor_pos = insert_pos + cleaned_code.size();

    // Handle selection replacement
    if (editor_state.selection_start != editor_state.selection_end) {
        int start = std::min(editor_state.selection_start, editor_state.selection_end);
        int end = std::max(editor_state.selection_start, editor_state.selection_end);

        editor_state.fileContent.replace(start, end - start, cleaned_code);
        editor_state.fileColors.erase(editor_state.fileColors.begin() + start, editor_state.fileColors.begin() + end);
        insert_pos = start;
        new_cursor_pos = start + cleaned_code.size();
    } else {
        editor_state.fileContent.insert(insert_pos, cleaned_code);
        editor_state.fileColors.insert(editor_state.fileColors.begin() + insert_pos, cleaned_code.size(), ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
    }

    // Update editor state with selection
    editor_state.selection_start = insert_pos;
    editor_state.selection_end = new_cursor_pos;
    editor_state.selection_active = true;
    editor_state.cursor_index = new_cursor_pos;

    // Force editor to show the inserted text
    editor_state.text_changed = true;

    // Trigger syntax highlighting
    gEditorHighlight.highlightContent();
}
