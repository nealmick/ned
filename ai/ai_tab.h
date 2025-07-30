#pragma once
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <string>
#include <thread>

class AITab
{
  public:
	AITab();
	~AITab();
	bool load_key();
	void tab_complete();
	void update();					// Call this in main loop
	void accept_completion();		// New function to accept the completion
	void dismiss_completion();		// New function to dismiss the completion
	void handle_editor_operation(); // New function to handle editor operations
	void cancel_request();			// New function to cancel ongoing requests

	bool request_done = false;
	bool has_ghost_text = false; // New flag to track if we have ghost text
	std::string ghost_text;		 // The ghost text to show
	int ghost_text_start = 0;	 // Start position of ghost text
	int ghost_text_end = 0;		 // End position of ghost text

	std::string response;

	std::atomic<bool> request_active{false};

	std::thread debounce_thread;
	std::atomic<bool> should_cancel;
	std::chrono::steady_clock::time_point last_request_time;

	std::atomic<bool> pending_request;

  private:
	void insert(const std::string &code);
	std::string collect_context() const;
	std::string get_file_extension() const;
	std::string api_key;
	bool key_loaded = false;
	std::thread worker;

	static constexpr int MAX_CONCURRENT_THREADS = 3;
	std::atomic<int> active_thread_count{0};
	std::mutex thread_mutex;
	std::condition_variable thread_cv;

	void cleanup_old_threads();
	bool can_start_new_thread();
	void increment_thread_count();
	void decrement_thread_count();
};
extern AITab gAITab;