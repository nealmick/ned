#pragma once

/*
	Shared state between a UI-thread request initiator and an LSP callback
	thread (goto, hover). One ticket per request:

	  begin()    UI thread: invalidates older in-flight responses.
	  deliver()  LSP thread: stores the result, or drops a stale ticket.
	  cancel()   UI thread: dismiss — later deliveries for the old ticket drop.

	`deliver` always clears pending, so an empty answer is an answer (the old
	goto code stayed "loading" forever on a no-result response). All access is
	locked; the ticket is a single monotonic counter, so a stale response can
	never win over a newer one.
*/

#include <cstdint>
#include <mutex>
#include <optional>
#include <utility>

template <typename T> class LSPRequestState
{
  public:
	using Ticket = std::uint64_t;

	Ticket begin()
	{
		const std::lock_guard<std::mutex> lock(mu);
		++ticket;
		pending = true;
		result.reset();
		return ticket;
	}

	void cancel()
	{
		const std::lock_guard<std::mutex> lock(mu);
		++ticket;
		pending = false;
		result.reset();
	}

	void deliver(Ticket t, std::optional<T> value)
	{
		const std::lock_guard<std::mutex> lock(mu);
		if (t != ticket)
			return; // superseded by a newer request or a cancel
		pending = false;
		result = std::move(value);
	}

	bool isPending() const
	{
		const std::lock_guard<std::mutex> lock(mu);
		return pending;
	}

	// UI thread copy of the latest delivered result; nullopt until delivered.
	std::optional<T> snapshot() const
	{
		const std::lock_guard<std::mutex> lock(mu);
		return result;
	}

  private:
	mutable std::mutex mu;
	Ticket ticket = 0;
	bool pending = false;
	std::optional<T> result;
};
