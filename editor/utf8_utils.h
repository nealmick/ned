#pragma once
#include <string>

inline int snapToUtf8CharBoundary(const std::string &str, int idx)
{
	if (idx <= 0 || idx >= (int)str.size())
		return idx;
	while (idx > 0 && (static_cast<unsigned char>(str[idx]) & 0xC0) == 0x80)
	{
		--idx;
	}
	return idx;
}