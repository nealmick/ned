#pragma once
#include <string>

class OpenRouter
{
  public:
	static std::string request(const std::string &prompt, const std::string &api_key);

  private:
	static std::string sanitize_completion(const std::string &completion);
	static size_t WriteData(void *ptr, size_t size, size_t nmemb, std::string *data);
};
