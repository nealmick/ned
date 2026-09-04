#include "lsp_writer.h"

#include <iostream>
#include <thread>

QueuedStreamWriter::QueuedStreamWriter(lsp::io::Stream &inner) : inner(inner)
{
	writer = std::thread(&QueuedStreamWriter::run, this);
}

QueuedStreamWriter::~QueuedStreamWriter()
{
	{
		const std::lock_guard<std::mutex> lock(mu);
		stopped = true;
	}
	cv.notify_all();
	if (writer.joinable())
		writer.join();
}

void QueuedStreamWriter::read(char *buffer, std::size_t size)
{
	inner.read(buffer, size);
}

void QueuedStreamWriter::write(const char *buffer, std::size_t size)
{
	{
		const std::lock_guard<std::mutex> lock(mu);
		if (dead || stopped)
			return;
		queue.emplace_back(buffer, buffer + size);
	}
	cv.notify_one();
}

void QueuedStreamWriter::flushFor(std::chrono::milliseconds timeout)
{
	std::unique_lock<std::mutex> lock(mu);
	cv.wait_for(lock, timeout, [this] { return dead || queue.empty(); });
}

bool QueuedStreamWriter::failed() const
{
	const std::lock_guard<std::mutex> lock(mu);
	return dead;
}

void QueuedStreamWriter::run()
{
	std::vector<char> chunk;
	for (;;)
	{
		{
			std::unique_lock<std::mutex> lock(mu);
			cv.wait(lock, [this] { return stopped || dead || !queue.empty(); });
			if (dead)
				return;
			if (queue.empty())
			{
				if (stopped)
					return;
				continue;
			}
			chunk = std::move(queue.front());
			queue.pop_front();
		}

		try
		{
			// Blocking is FINE here — this is the one thread allowed to wait
			// on the server. A full pipe just parks the queue, and callers
			// keep running.
			inner.write(chunk.data(), chunk.size());
		} catch (const std::exception &e)
		{
			std::cerr << "LSP: server write failed (" << e.what()
					  << ") — dropping outbound queue" << std::endl;
			const std::lock_guard<std::mutex> lock(mu);
			dead = true;
			queue.clear();
			cv.notify_all();
			return;
		}
	}
}
