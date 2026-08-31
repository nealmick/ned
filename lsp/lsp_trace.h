#pragma once

/*
	Opt-in LSP message tracing: NED_LSP_TRACE=1 logs document-sync messages and
	request results to stderr. Zero cost when the env var is unset.
*/

#include <cstdlib>
#include <iostream>

namespace lsptrace {
inline bool enabled()
{
	static const bool on = std::getenv("NED_LSP_TRACE") != nullptr;
	return on;
}
} // namespace lsptrace

#define NED_LSP_TRACE(msg)                                                               \
	do                                                                                   \
	{                                                                                    \
		if (lsptrace::enabled())                                                         \
			std::cerr << "LSPTRACE: " << msg << std::endl;                               \
	} while (0)
