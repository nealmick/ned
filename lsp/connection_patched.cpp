#include <algorithm>
#include <cctype>
#include <charconv>
#include <cstring>
#include <ios>
#include <lsp/connection.h>
#include <lsp/io/stream.h>
#include <lsp/json/json.h>
#include <optional>
#include <string_view>
#include <system_error>

#ifndef LSP_MESSAGE_DEBUG_LOG
#ifdef NDEBUG
#define LSP_MESSAGE_DEBUG_LOG 0
#else
#define LSP_MESSAGE_DEBUG_LOG 1
#endif
#endif

#if LSP_MESSAGE_DEBUG_LOG
#ifdef __APPLE__
#include <os/log.h>
#elif defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#endif
#endif

namespace lsp {
namespace {

/*
 * Message logging
 */

#if LSP_MESSAGE_DEBUG_LOG
void debugLogMessageJson([[maybe_unused]] const std::string &messageType,
						 [[maybe_unused]] const lsp::json::Any &json)
{
#ifdef __APPLE__
	os_log_debug(OS_LOG_DEFAULT,
				 "%{public}s",
				 (messageType + ": " + lsp::json::stringify(json, true)).c_str());
#elif defined(_WIN32)
	OutputDebugStringA(
		(messageType + ": " + lsp::json::stringify(json, true) + '\n').c_str());
#endif
}
#endif

std::string_view trimWhitespace(std::string_view str)
{
	while (!str.empty() && std::isspace(static_cast<unsigned char>(str.front())))
		str.remove_prefix(1);

	while (!str.empty() && std::isspace(static_cast<unsigned char>(str.back())))
		str.remove_suffix(1);

	return str;
}

bool equalCaseInsensitive(std::string_view lhs, std::string_view rhs)
{
	return std::ranges::equal(lhs, rhs, [](char a, char b) {
		return std::tolower(static_cast<unsigned char>(a)) ==
			   std::tolower(static_cast<unsigned char>(b));
	});
}

void verifyContentType(std::string_view contentType)
{
	if (!contentType.starts_with("application/vscode-jsonrpc"))
		throw ConnectionError{"Protocol: Unsupported or invalid content type: " +
							  std::string(contentType)};

	constexpr std::string_view charsetKey{"charset="};
	if (const auto idx = contentType.find(charsetKey); idx != std::string_view::npos)
	{
		auto charset = contentType.substr(idx + charsetKey.size());
		charset = trimWhitespace(charset.substr(0, charset.find(';')));

		if (charset != "utf-8" && charset != "utf8")
			throw ConnectionError{
				"Protocol: Unsupported or invalid character encoding: " +
				std::string{charset}};
	}
}

} // namespace

/*
 * Connection::InputReader
 * Wrapper around io::Stream that allows for peeking and reading single chars
 */

class Connection::InputReader
{
  public:
	InputReader(io::Stream &stream) : m_stream{stream} {}

	char peek()
	{
		if (!m_peek.has_value())
			m_peek = get();

		return m_peek.value();
	}

	char get()
	{
		if (m_peek.has_value())
		{
			const char c = m_peek.value();
			m_peek.reset();
			return c;
		}

		char c = io::Stream::Eof;
		read(&c, 1);
		return c;
	}

	void read(char *buffer, std::size_t size)
	{
		if (size > 0)
		{
			if (m_peek.has_value())
			{
				*buffer = m_peek.value();
				m_peek.reset();
				++buffer;
				--size;
			}

			m_stream.read(buffer, size);
		}
	}

  private:
	io::Stream &m_stream;
	std::optional<char> m_peek;
};

/*
 * Connection
 */

struct Connection::MessageHeader
{
	std::size_t contentLength = 0;
	std::string contentType = "application/vscode-jsonrpc; charset=utf-8";
};

Connection::Connection(io::Stream &stream) : m_stream{stream} {}

json::Any Connection::readMessage()
{
	try
	{
		std::lock_guard lock{m_readMutex};
		InputReader reader{m_stream};

		if (reader.peek() == io::Stream::Eof)
			throw ConnectionError{"Connection lost"};

		const auto header = readMessageHeader(reader);

		std::string content;
		content.resize(header.contentLength);
		reader.read(&content[0], static_cast<std::streamsize>(header.contentLength));

		// Verify only after reading the entire message so no partially unread message is
		// left in the stream
		verifyContentType(header.contentType);

		auto json = json::parse(content);
#if LSP_MESSAGE_DEBUG_LOG
		debugLogMessageJson("incoming", json);
#endif

		return json;
	} catch (const ConnectionError &)
	{
		throw;
	} catch (const json::ParseError &)
	{
		throw;
	} catch (const std::exception &e)
	{
		throw ConnectionError{e.what()};
	} catch (...)
	{
		throw ConnectionError{"Unknown error"};
	}
}

void Connection::writeMessage(const json::Any &content)
{
	try
	{
#if LSP_MESSAGE_DEBUG_LOG
		debugLogMessageJson("outgoing", content);
#endif
		writeMessageData(json::stringify(content));
	} catch (const ConnectionError &)
	{
		throw;
	} catch (const std::exception &e)
	{
		throw ConnectionError{e.what()};
	} catch (...)
	{
		throw ConnectionError{"Unknown error"};
	}
}

Connection::MessageHeader Connection::readMessageHeader(InputReader &reader)
{
	MessageHeader header;

	while (reader.peek() != '\r')
		readNextMessageHeaderField(header, reader);

	if (reader.get() != '\r' || reader.get() != '\n')
		throw ConnectionError("Protocol: Expected header to be terminated by '\\r\\n'");

	return header;
}

void Connection::parseHeaderValue(MessageHeader &header, std::string_view line)
{
	const auto separatorIdx = line.find(':');

	if (separatorIdx != std::string_view::npos)
	{
		const auto key = trimWhitespace(line.substr(0, separatorIdx));
		const auto value = trimWhitespace(line.substr(separatorIdx + 1));

		if (equalCaseInsensitive(key, "Content-Length"))
		{
			const auto [ptr, ec] = std::from_chars(value.data(),
												   value.data() + value.size(),
												   header.contentLength);

			if (ec != std::error_code{} || ptr != value.data() + value.size())
				throw ConnectionError(
					"Protocol: Invalid value for Content-Length header field");
		} else if (equalCaseInsensitive(key, "Content-Type"))
		{
			header.contentType = std::string{value.data(), value.size()};
		}
	}
}

void Connection::readNextMessageHeaderField(MessageHeader &header, InputReader &reader)
{
	if (reader.peek() == std::char_traits<char>::eof())
		throw ConnectionError{"Connection lost"};

	std::string lineData;

	while (reader.peek() != '\r')
	{
		const auto c = reader.get();

		if (c == '\n')
			throw ConnectionError(
				"Protocol: Unexpected '\\n' in header field, expected '\\r\\n'");

		lineData.push_back(c);
	}

	parseHeaderValue(header, lineData);

	if (reader.get() != '\r' || reader.get() != '\n')
		throw ConnectionError(
			"Protocol: Expected header field to be terminated by '\\r\\n'");
}

void Connection::writeMessageData(const std::string &content)
{
	std::lock_guard lock{m_writeMutex};
	MessageHeader header{content.size()};
	const auto messageStr = messageHeaderString(header) + content;
	m_stream.write(messageStr.data(), static_cast<std::streamsize>(messageStr.size()));
}

std::string Connection::messageHeaderString(const MessageHeader &header)
{
	return "Content-Length: " + std::to_string(header.contentLength) + "\r\n" +
		   "Content-Type: " + header.contentType + "\r\n\r\n";
}

} // namespace lsp
