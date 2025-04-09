#pragma once
#include <string>

class OpenRouter
{
  public:
    static std::string request(const std::string &prompt, const std::string &api_key);
};