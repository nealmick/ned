#pragma once
#include <string>
#include <atomic>
#include <functional>

extern std::atomic<bool> g_should_cancel;

class OpenRouter
{
  public:
	static std::string request(const std::string &prompt, const std::string &api_key);
	static std::string promptRequest(const std::string &prompt, const std::string &api_key);
	static bool promptRequestStream(const std::string &prompt, const std::string &api_key, 
								   std::function<void(const std::string&)> tokenCallback,
								   std::atomic<bool>* cancelFlag = nullptr);
	
	// CURL global initialization and cleanup
	static bool initializeCURL();
	static void cleanupCURL();
	
	// Write data function for CURL callbacks
	static size_t WriteData(void *ptr, size_t size, size_t nmemb, std::string *data);

  private:
	static std::string sanitize_completion(const std::string &completion);
	static size_t WriteDataStream(void *ptr, size_t size, size_t nmemb, std::string *data);
	
	// Track CURL initialization state
	static std::atomic<bool> curl_initialized;
};
