#include "lsp/lsp_writer.h"

#include "third_party/catch.hpp"

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

namespace {

// A stream whose write() blocks until the test releases it — the stand-in
// for a wedged server with a full stdin pipe (the freeze reproducer).
class GatedStream : public lsp::io::Stream
{
  public:
	void read(char *, std::size_t) override {}

	void write(const char *buffer, std::size_t size) override
	{
		std::unique_lock<std::mutex> lock(mu);
		++blocked;
		cvOpen.notify_all();
		// Park until the test lets this write finish.
		cvRelease.wait(lock, [this] { return released; });
		written.insert(written.end(), buffer, buffer + size);
		++passed;
	}

	// Test-side gate control.
	void waitForBlockedWrite()
	{
		std::unique_lock<std::mutex> lock(mu);
		cvOpen.wait(lock, [this] { return blocked > passed; });
	}

	void release()
	{
		std::lock_guard<std::mutex> lock(mu);
		released = true;
		cvRelease.notify_all();
	}

	std::string data()
	{
		std::lock_guard<std::mutex> lock(mu);
		return {written.begin(), written.end()};
	}

	std::mutex mu;
	std::condition_variable cvOpen;
	std::condition_variable cvRelease;
	bool released = false;
	int blocked = 0;
	int passed = 0;
	std::vector<char> written;
};

} // namespace

// Regression test for the whole-UI freeze: the old code path wrote LSP
// messages straight into the server's stdin from the calling thread. A
// wedged server (full pipe) blocked that thread forever — and when the
// caller was the UI thread sending didOpen/didChange, the app hung with no
// recovery. QueuedStreamWriter must return immediately no matter how stuck
// the underlying stream is, deliver everything in order once unblocked,
// and drop sends after a write failure instead of blocking.
TEST_CASE("QueuedStreamWriter never blocks the caller on a stuck stream", "[ned][lsp]")
{
	GatedStream gated;
	{
		QueuedStreamWriter writer(gated);

		const std::string first(80 * 1024, 'a'); // > one pipe buffer
		writer.write(first.data(), first.size());
		writer.write("second", 6);

		// The inner stream is parked mid-write; the enqueue must have
		// returned instantly regardless.
		REQUIRE(true);

		gated.waitForBlockedWrite();
		// Still parked: caller keeps going, queue keeps the rest.
		gated.release();
		writer.flushFor(std::chrono::milliseconds(2000));

		REQUIRE(gated.data() == first + "second");
	}
}

TEST_CASE("QueuedStreamWriter ordering is FIFO across writers", "[ned][lsp]")
{
	// Fast stream: writes complete immediately; order still preserved.
	class PassThrough : public lsp::io::Stream
	{
	  public:
		void read(char *, std::size_t) override {}
		void write(const char *buffer, std::size_t size) override
		{
			std::lock_guard<std::mutex> lock(mu);
			data.append(buffer, size);
		}
		std::mutex mu;
		std::string data;
	};

	PassThrough stream;
	{
		QueuedStreamWriter writer(stream);
		for (int i = 0; i < 100; ++i)
		{
			const std::string chunk = "[" + std::to_string(i) + "]";
			writer.write(chunk.data(), chunk.size());
		}
		writer.flushFor(std::chrono::milliseconds(1000));
	}

	std::string expected;
	for (int i = 0; i < 100; ++i)
		expected += "[" + std::to_string(i) + "]";
	REQUIRE(stream.data == expected);
}

namespace {

// write() that throws — the "server died" case.
class FailingStream : public lsp::io::Stream
{
  public:
	void read(char *, std::size_t) override {}
	void write(const char *, std::size_t) override { throw lsp::io::Error("pipe gone"); }
};

} // namespace

TEST_CASE("QueuedStreamWriter marks dead and drops after a failure", "[ned][lsp]")
{
	FailingStream dead;
	QueuedStreamWriter writer(dead);
	writer.write("payload", 7);
	writer.flushFor(std::chrono::milliseconds(1000));
	REQUIRE(writer.failed());
	// Post-failure sends are dropped, not enqueued behind a dead stream.
	writer.write("more", 4);
	writer.flushFor(std::chrono::milliseconds(100));
	REQUIRE(writer.failed());
}
