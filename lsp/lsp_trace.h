#pragma once

/*
	Opt-in LSP message tracing: NED_LSP_TRACE=1 logs document-sync messages and
	request results to stderr. Zero cost when the env var is unset. Timestamped
	(HH:MM:SS.mmm, wall clock) so traces line up against the mirrored server
	log lines when measuring request/response latency.
*/

#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <string>

namespace lsptrace {
inline bool enabled()
{
	static const bool on = std::getenv("NED_LSP_TRACE") != nullptr;
	return on;
}

inline const std::string &timestamp()
{
	using clock = std::chrono::system_clock;
	const auto now = clock::now();
	const std::time_t seconds = clock::to_time_t(now);
	const auto ms =
		std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch())
			.count() %
		1000;
	std::tm tm{};
#ifdef _WIN32
	localtime_s(&tm, &seconds);
#else
	localtime_r(&seconds, &tm);
#endif
	char buf[16];
	std::snprintf(buf,
				  sizeof(buf),
				  "%02d:%02d:%02d.%03d",
				  tm.tm_hour,
				  tm.tm_min,
				  tm.tm_sec,
				  static_cast<int>(ms));
	static thread_local std::string stamped;
	stamped = buf;
	return stamped;
}
} // namespace lsptrace

#define NED_LSP_TRACE(msg)                                                               \
	do                                                                                   \
	{                                                                                    \
		if (lsptrace::enabled())                                                         \
			std::cerr << "[LSPTrace " << lsptrace::timestamp() << "] " << msg            \
					  << std::endl;                                                      \
	} while (0)
