#pragma once
#include <atomic>
#include <functional>
#include <mutex>
#include <string>
#include <thread>

class AgentRequest
{
  public:
	using StreamingCallback = std::function<void(const std::string &)>;
	using CompleteCallback = std::function<void(const std::string &, bool)>;

	AgentRequest();
	~AgentRequest();

	void sendMessage(const std::string &prompt,
					 const std::string &api_key,
					 StreamingCallback onStreamingToken,
					 CompleteCallback onComplete);

	void stopRequest();
	bool isProcessing() const { return isStreaming.load(); }

  private:
	std::atomic<bool> isStreaming{false};
	std::atomic<bool> shouldCancelStreaming{false};
	std::thread streamingThread;
	std::mutex requestMutex;
	std::string utf8_buffer;
};
