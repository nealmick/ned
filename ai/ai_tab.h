#pragma once
#include <atomic>
#include <string>
#include <thread>

class AITab
{
  public:
	AITab();
	~AITab();
	bool load_key();
	void tab_complete();
	void update(); // Call this in main loop
	void accept_completion(); // New function to accept the completion
	void dismiss_completion(); // New function to dismiss the completion

	bool request_done = false;
	bool has_ghost_text = false; // New flag to track if we have ghost text
	std::string ghost_text; // The ghost text to show
	int ghost_text_start = 0; // Start position of ghost text
	int ghost_text_end = 0; // End position of ghost text

	std::string response;

	std::atomic<bool> request_active{false};

  private:
	void insert(const std::string &code);
	std::string collect_context() const;
	std::string get_file_extension() const;
	std::string api_key;
	bool key_loaded = false;
	std::thread worker;
};
extern AITab gAITab;