#pragma once

/*
	File: lsp/lsp_writer.h
	Description: Non-blocking outbound LSP stream.

	The framework's Connection::writeMessage writes straight into the server
	process's stdin with a blocking ::write() loop. A server that stalls
	(starting up, indexing hard, or wedged) stops draining the pipe, the
	64KB OS buffer fills, and whatever thread sent the message blocks
	forever. When that thread is the UI thread (didOpen/didChange of a
	document), the whole app freezes — no recovery, no crash, exactly the
	"opened a file and the UI never came back" failure.

	This decorator sits between the Connection and the real process stream:
	write() copies into a FIFO queue and returns immediately; a dedicated
	writer thread drains it in order. Reads pass straight through (only the
	reader thread reads). A dead server surfaces as a one-line error on the
	writer thread and subsequent sends are dropped — the reader thread
	notices the closed pipe independently.
*/

#include <chrono>
#include <condition_variable>
#include <deque>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include "lsp/io/stream.h"

class QueuedStreamWriter : public lsp::io::Stream
{
  public:
	// inner must outlive this object (the process's stdIO stream).
	explicit QueuedStreamWriter(lsp::io::Stream &inner);
	~QueuedStreamWriter() override;

	QueuedStreamWriter(const QueuedStreamWriter &) = delete;
	QueuedStreamWriter &operator=(const QueuedStreamWriter &) = delete;

	// Reader thread only — passthrough to the real stream.
	void read(char *buffer, std::size_t size) override;

	// Any thread. Enqueues a copy; NEVER blocks the caller. After a write
	// failure the connection is marked dead and later writes are dropped.
	void write(const char *buffer, std::size_t size) override;

	// Bounded drain before a deliberate server shutdown: waits until the
	// queue is empty (or the stream failed, or the timeout passes) so the
	// shutdown/exit notifications actually reach the process.
	void flushFor(std::chrono::milliseconds timeout);

	// True once a write has failed — callers may skip further sends.
	bool failed() const;

  private:
	void run();

	lsp::io::Stream &inner;

	mutable std::mutex mu;
	std::condition_variable cv;
	std::deque<std::vector<char>> queue;
	bool stopped = false;
	bool dead = false;

	std::thread writer;
};
