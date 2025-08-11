#pragma once
#include "../lib/json.hpp"
#include <atomic>
#include <functional>
#include <mutex>
#include <string>

using json = nlohmann::json;

extern std::atomic<bool> g_should_cancel;

class OpenRouter
{
  public:
	static std::string request(const std::string &prompt, const std::string &api_key);
	static std::string promptRequest(const std::string &prompt,
									 const std::string &api_key);
	static bool promptRequest(const std::string &prompt,
							  const std::string &api_key,
							  std::string &response);
	static bool promptRequestStream(const std::string &prompt,
									const std::string &api_key,
									std::function<void(const std::string &)> tokenCallback,
									std::atomic<bool> *cancelFlag = nullptr);

	// New function for modern JSON payload streaming
	static bool jsonPayloadStream(const std::string &jsonPayload,
								  const std::string &api_key,
								  std::function<void(const std::string &)> tokenCallback,
								  std::atomic<bool> *cancelFlag = nullptr);

	// New function for modern JSON payload streaming with full response callback
	static std::pair<bool, std::string>
	jsonPayloadStreamWithResponse(const std::string &jsonPayload,
								  const std::string &api_key,
								  std::function<void(const std::string &)> tokenCallback,
								  std::function<void(const json &)> responseCallback,
								  std::atomic<bool> *cancelFlag = nullptr);

	// CURL global initialization and cleanup
	static bool initializeCURL();
	static void cleanupCURL();

	// Write data function for CURL callbacks
	static size_t WriteData(void *ptr, size_t size, size_t nmemb, std::string *data);
	static size_t WriteDataStream(void *ptr, size_t size, size_t nmemb, std::string *data);
	static size_t
	WriteDataStreamWithResponse(void *ptr, size_t size, size_t nmemb, std::string *data);

  private:
	static std::string sanitize_completion(const std::string &completion);

	// Track CURL initialization state
	static std::atomic<bool> curl_initialized;
	static std::mutex g_curlInitMutex;
};
