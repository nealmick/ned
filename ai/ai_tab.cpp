#include "ai_tab.h"
#include "../editor/editor.h"
#include "../files/files.h"
#include "ai_open_router.h"
#include <algorithm>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <fstream>
#include <iostream>
#include <mutex>

AITab gAITab;
std::atomic<bool> g_should_cancel{false};

AITab::AITab()
	: key_loaded(load_key()), request_active(false), request_done(false),
	  has_ghost_text(false), ghost_text_start(0), ghost_text_end(0), should_cancel(false),
	  last_request_time(std::chrono::steady_clock::now()), pending_request(false),
	  active_thread_count(0)
{
}

AITab::~AITab()
{
	should_cancel = true;
	g_should_cancel = true;

	{
		std::lock_guard<std::mutex> lock(thread_mutex);
		if (debounce_thread.joinable())
		{
			debounce_thread.detach();
		}

		if (worker.joinable())
		{
			worker.detach();
		}
	}
}

bool AITab::load_key()
{
	std::string key = gSettingsFileManager.getOpenRouterKey();
	api_key = key;
	// std::cout << "API Key: " << api_key << std::endl;
	if (api_key.empty())
	{
		return false;
	}
	return true;
}

void AITab::tab_complete()
{
	if (!key_loaded)
	{
		return;
	}

	// Cancel any active request first
	if (request_active)
	{
		should_cancel = true;
		g_should_cancel = true;
		request_active = false;
		std::cout << "Request canceled\n";
	}

	last_request_time = std::chrono::steady_clock::now();
	pending_request = true;

	cleanup_old_threads();

	if (!can_start_new_thread())
	{
		return;
	}

	should_cancel = false;
	g_should_cancel = false;
	increment_thread_count();

	debounce_thread = std::thread([this]() {
		while (true)
		{
			auto now = std::chrono::steady_clock::now();
			auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
							   now - last_request_time)
							   .count();

			if (elapsed >= 500)
			{
				break;
			}
			std::this_thread::sleep_for(std::chrono::milliseconds(50));
		}

		if (pending_request && !should_cancel)
		{
			request_active = true;
			pending_request = false;

			worker = std::thread([this]() {
				if (should_cancel)
				{
					request_active = false;
					decrement_thread_count();
					return;
				}

				std::string prompt = collect_context();
				if (should_cancel)
				{
					request_active = false;
					decrement_thread_count();
					return;
				}

				std::cout << "Requesting AI completion...\n";
				std::string new_response = OpenRouter::request(prompt, api_key);

				if (should_cancel)
				{
					request_active = false;
					decrement_thread_count();
					return;
				}

				// Only process non-empty responses that don't start with "error"
				if (!new_response.empty() && new_response.find("error") != 0)
				{
					std::lock_guard<std::mutex> lock(thread_mutex);
					if (!should_cancel)
					{
						response = std::move(new_response);
						request_done = true;
					}
				}
				request_active = false;
				decrement_thread_count();
			});
		} else
		{
			decrement_thread_count();
		}
	});
}

std::string AITab::collect_context() const
{
	const int cursor_pos = editor_state.cursor_index;

	// Safety check for cursor position
	if (cursor_pos < 0 || cursor_pos > editor_state.fileContent.size())
	{
		return "";
	}

	int current_line = 0;
	for (size_t i = 1; i < editor_state.editor_content_lines.size(); i++)
	{
		if (cursor_pos < editor_state.editor_content_lines[i])
		{
			current_line = i - 1;
			break;
		}
	}

	const int total_lines = editor_state.editor_content_lines.size();
	const int start_line = std::max(0, current_line - 10);
	const int end_line = std::min(total_lines - 1, current_line + 5);

	// Safety check for line indices
	if (start_line >= total_lines || end_line < 0 || start_line > end_line)
	{
		return "";
	}

	const int context_start = editor_state.editor_content_lines[start_line];
	int context_end;

	// Handle the case where we're at the end of the file
	if (end_line + 1 < editor_state.editor_content_lines.size())
	{
		context_end = editor_state.editor_content_lines[end_line + 1];
	} else
	{
		context_end = editor_state.fileContent.size();
	}

	// Safety check for context bounds
	if (context_start < 0 || context_end > editor_state.fileContent.size() ||
		context_start >= context_end)
	{
		return "";
	}

	std::string context =
		editor_state.fileContent.substr(context_start, context_end - context_start);
	const size_t cursor_pos_in_context = cursor_pos - context_start;

	// Safety check for cursor position in context
	if (cursor_pos_in_context > context.size())
	{
		return "";
	}

	context.insert(cursor_pos_in_context, "\n [[CURSOR]]\n");

	std::string prompt = "You are a code completion assistant. Replace "
						 "[[CURSOR]] with appropriate code.\n"
						 "File: " +
						 gFileExplorer.currentFile +
						 "\n"
						 "Code:\n"
						 "```" +
						 get_file_extension() + "\n" + context +
						 "\n```\n"
						 "Respond only with the code that should replace "
						 "[[CURSOR]]. No markdown, no explanations.\n";

	return prompt;
}

std::string AITab::get_file_extension() const
{
	size_t dot_pos = gFileExplorer.currentFile.find_last_of('.');
	if (dot_pos != std::string::npos)
	{
		return gFileExplorer.currentFile.substr(dot_pos + 1);
	}
	return "txt";
}

void AITab::update()
{
	if (request_done)
	{
		std::string current_response;
		{
			std::lock_guard<std::mutex> lock(thread_mutex);
			current_response = std::move(response);
			request_done = false;
		}

		if (!current_response.empty() && current_response.find("error") != 0)
		{
			if (has_ghost_text)
			{
				dismiss_completion();
			}
			insert(current_response);
		}
	}
}

void AITab::insert(const std::string &code)
{
	if (code.empty())
		return;

	std::lock_guard<std::mutex> lock(thread_mutex);

	if (has_ghost_text)
	{
		dismiss_completion();
	}

	// Ensure cursor index is within bounds
	if (editor_state.cursor_index < 0 ||
		editor_state.cursor_index > editor_state.fileContent.size())
	{
		return;
	}

	ghost_text = code;
	ghost_text_start = editor_state.cursor_index;
	ghost_text_end = ghost_text_start + code.size();
	has_ghost_text = true;

	// Insert the code into the file content
	editor_state.fileContent.insert(editor_state.cursor_index, code);

	ImVec4 ghost_color = ImVec4(0.5f, 0.5f, 0.5f, 0.5f);

	// Ensure fileColors is properly sized and matches fileContent
	size_t new_size = editor_state.fileContent.size();
	if (editor_state.fileColors.size() != new_size)
	{
		editor_state.fileColors.resize(new_size, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
	}

	// Insert ghost colors at the cursor position
	editor_state.fileColors.insert(editor_state.fileColors.begin() +
									   editor_state.cursor_index,
								   code.size(),
								   ghost_color);

	editor_state.ghost_text_changed = true;
	gEditor.updateLineStarts();
}

void AITab::accept_completion()
{
	if (!has_ghost_text)
		return;

	for (int i = ghost_text_start; i < ghost_text_end; i++)
	{
		if (i < editor_state.fileColors.size())
		{
			editor_state.fileColors[i] = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
		}
	}

	editor_state.cursor_index = ghost_text_end - 1;
	editor_state.selection_start = editor_state.selection_end = editor_state.cursor_index;

	has_ghost_text = false;
	ghost_text.clear();
	ghost_text_start = 0;
	ghost_text_end = 0;

	editor_state.text_changed = true;
}

void AITab::dismiss_completion()
{
	if (!has_ghost_text)
		return;

	std::lock_guard<std::mutex> lock(thread_mutex);

	// Validate indices before accessing
	if (ghost_text_start < 0 || ghost_text_end > editor_state.fileContent.size() ||
		ghost_text_start >= editor_state.fileColors.size() ||
		ghost_text_end > editor_state.fileColors.size())
	{
		has_ghost_text = false;
		ghost_text.clear();
		ghost_text_start = 0;
		ghost_text_end = 0;
		return;
	}

	editor_state.fileContent.erase(ghost_text_start, ghost_text_end - ghost_text_start);
	editor_state.fileColors.erase(editor_state.fileColors.begin() + ghost_text_start,
								  editor_state.fileColors.begin() + ghost_text_end);

	has_ghost_text = false;
	ghost_text.clear();
	ghost_text_start = 0;
	ghost_text_end = 0;

	editor_state.ghost_text_changed = true;
	gEditor.updateLineStarts();
}

void AITab::handle_editor_operation()
{
	if (has_ghost_text)
	{
		std::lock_guard<std::mutex> lock(thread_mutex);

		bool should_dismiss = false;

		// Validate cursor index
		if (editor_state.cursor_index < 0 ||
			editor_state.cursor_index > editor_state.fileContent.size())
		{
			should_dismiss = true;
		} else if (editor_state.cursor_index < ghost_text_start ||
				   editor_state.cursor_index > ghost_text_end)
		{
			should_dismiss = true;
		}

		if (editor_state.fileContent.size() < ghost_text_end ||
			editor_state.fileColors.size() < ghost_text_end)
		{
			should_dismiss = true;
		}

		if (should_dismiss)
		{
			dismiss_completion();
		}
	}
}

void AITab::cleanup_old_threads()
{
	std::lock_guard<std::mutex> lock(thread_mutex);

	// Wait for threads to finish before detaching
	if (debounce_thread.joinable())
	{
		debounce_thread.detach();
	}
	if (worker.joinable())
	{
		worker.detach();
	}
}

bool AITab::can_start_new_thread()
{
	std::lock_guard<std::mutex> lock(thread_mutex);
	return active_thread_count < MAX_CONCURRENT_THREADS;
}

void AITab::increment_thread_count()
{
	std::lock_guard<std::mutex> lock(thread_mutex);
	active_thread_count++;
}

void AITab::decrement_thread_count()
{
	std::lock_guard<std::mutex> lock(thread_mutex);
	active_thread_count--;
	thread_cv.notify_one();
}

void AITab::cancel_request()
{
	if (!request_active)
		return;

	std::lock_guard<std::mutex> lock(thread_mutex);
	should_cancel = true;
	g_should_cancel = true;
	request_active = false;
	pending_request = false;
	std::cout << "Request canceled\n";
}
