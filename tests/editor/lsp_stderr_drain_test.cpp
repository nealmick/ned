#include "lsp/process.h"

#include "third_party/catch.hpp"

#include <chrono>
#include <string>
#include <thread>

// Regression test for the "LSP silently stopped" freeze. The framework
// pipes a server's stderr into a 64KB OS pipe that only the client's drain
// loop reads. A server that logs more than 64KB without a drain blocks in
// write(2) forever — both processes stay alive, but no JSON-RPC message
// ever flows again (clangd's background-index logging hits this over a long
// session). Polling readAvailableStdErr must keep the pipe empty, so a
// chatty child runs to completion instead of wedging.
TEST_CASE("draining stderr keeps a chatty server from wedging", "[ned][lsp]")
{
	// ~350KB of stderr — several pipe buffers' worth.
	lsp::Process chatter("/bin/sh",
						 {"-c",
						  "i=0; while [ $i -lt 7000 ]; do "
						  "echo \"log line $i with a realistic amount of padding\" >&2; "
						  "i=$((i+1)); done"});

	std::string drained;
	const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(20);
	while (std::chrono::steady_clock::now() < deadline)
	{
		drained += chatter.readAvailableStdErr();
		// isRunning() must be checked LAST: reaping the child also closes
		// the std handles, which would discard any unread pipe contents.
		if (!chatter.isRunning())
			break;
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}

	REQUIRE(!chatter.isRunning()); // wedged write would never let it exit
	REQUIRE(drained.size() > 64 * 1024);
}
