#pragma once
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

	bool request_done = false;

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