// debug_console.h
#pragma once
#include "imgui.h"
#include <deque>
#include <ios>
#include <sstream>
#include <streambuf>
#include <string>
#include <vector>

class DebugConsole
{
  private:
	// Forward declare ConsoleBuf
	class ConsoleBuf;

  public:
	static DebugConsole &getInstance()
	{
		static DebugConsole instance;
		return instance;
	}
	void render();
	void addLog(const std::string &message);
	void clear() { logs.clear(); }
	void toggleVisibility() { isVisible = !isVisible; }
	bool isConsoleVisible() const { return isVisible; }

  private:
	DebugConsole();
	std::deque<std::string> logs;
	bool isVisible;

	class ConsoleBuf : public std::streambuf
	{
	  public:
		ConsoleBuf(DebugConsole &console, std::streambuf *orig)
			: console(console), originalBuffer(orig)
		{
		}

	  protected:
		virtual int_type overflow(int_type c = traits_type::eof()) override
		{
			if (c != traits_type::eof())
			{
				if (c == '\n')
				{
					buffer += '\n';
					console.addLog(buffer);
					// Write to original buffer
					if (originalBuffer)
					{
						originalBuffer->sputn(buffer.c_str(), buffer.length());
					}
					buffer.clear();
				} else
				{
					buffer += static_cast<char>(c);
				}
			}
			return c;
		}

		virtual std::streamsize xsputn(const char *s, std::streamsize n) override
		{
			std::string str(s, n);
			size_t pos;
			while ((pos = str.find('\n')) != std::string::npos)
			{
				std::string line = str.substr(0, pos + 1);
				console.addLog(line);
				// Write to original buffer
				if (originalBuffer)
				{
					originalBuffer->sputn(line.c_str(), line.length());
				}
				str.erase(0, pos + 1);
			}
			if (!str.empty())
			{
				buffer += str;
			}
			return n;
		}

	  private:
		std::string buffer;
		DebugConsole &console;
		std::streambuf *originalBuffer;
	};

	ConsoleBuf consoleBuf;
	std::streambuf *oldCoutBuf;
	friend class ConsoleBuf;
};

extern DebugConsole &gDebugConsole;