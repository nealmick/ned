#include "lsp_writer.h"

#include <iostream>
#include <thread>

QueuedStreamWriter::QueuedStreamWriter(lsp::io::Stream &inner)
	: state(std::make_shared<State>(inner))
{
	writer = std::thread(&QueuedStreamWriter::run, this, state);
}

QueuedStreamWriter::~QueuedStreamWriter()
{
	{
		const std::lock_guard<std::mutex> lock(state->mu);
		state->stopped = true;
	}
	state->cv.notify_all();

	// Bounded join. The writer thread may be parked INSIDE inner.write() on
	// a wedged server's full pipe — the exact scenario this class exists
	// for — and blocking is fine there, but never at teardown: give it a
	// short grace window, then detach. The shared State (and nothing else)
	// outlives us with the thread: the caller kills the server process
	// right after (see stopServer), the pipe fails, the write errors out,
	// and the thread exits and frees the State.
	std::unique_lock<std::mutex> lock(state->mu);
	if (state->cv.wait_for(
			lock, std::chrono::milliseconds(250), [this] { return state->finished; }))
		writer.join(); // thread exited — a finished thread is still joinable
	else
		writer.detach(); // wedged mid-write; State dies with the thread
}

void QueuedStreamWriter::read(char *buffer, std::size_t size)
{
	state->inner.read(buffer, size);
}

void QueuedStreamWriter::write(const char *buffer, std::size_t size)
{
	{
		const std::lock_guard<std::mutex> lock(state->mu);
		if (state->dead || state->stopped)
			return;
		state->queue.emplace_back(buffer, buffer + size);
		++state->enqueued;
	}
	state->cv.notify_one();
}

void QueuedStreamWriter::flushFor(std::chrono::milliseconds timeout)
{
	std::unique_lock<std::mutex> lock(state->mu);
	state->cv.wait_for(lock, timeout, [this] {
		return state->dead || state->written == state->enqueued;
	});
}

bool QueuedStreamWriter::failed() const
{
	const std::lock_guard<std::mutex> lock(state->mu);
	return state->dead;
}

void QueuedStreamWriter::run(std::shared_ptr<State> s)
{
	for (;;)
	{
		std::vector<char> chunk;
		{
			std::unique_lock<std::mutex> lock(s->mu);
			s->cv.wait(lock, [&] { return s->stopped || s->dead || !s->queue.empty(); });
			if (s->dead)
				break;
			if (s->queue.empty())
			{
				if (s->stopped)
					break;
				continue;
			}
			chunk = std::move(s->queue.front());
			s->queue.pop_front();
		}

		try
		{
			// Blocking is FINE here — this is the one thread allowed to wait
			// on the server. A full pipe just parks the queue, and callers
			// keep running.
			s->inner.write(chunk.data(), chunk.size());
		} catch (const std::exception &e)
		{
			std::cerr << "[LSP] server write failed (" << e.what()
					  << ") — dropping outbound queue" << std::endl;
			const std::lock_guard<std::mutex> lock(s->mu);
			s->dead = true;
			s->queue.clear();
			break;
		}

		{
			const std::lock_guard<std::mutex> lock(s->mu);
			++s->written;
		}
		s->cv.notify_all();
	}

	{
		const std::lock_guard<std::mutex> lock(s->mu);
		s->finished = true;
	}
	s->cv.notify_all();
}
