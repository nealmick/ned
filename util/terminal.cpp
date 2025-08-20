/*
	util/terminal.cpp
	This utility adds a terminal emulator, activate it using the keybind cmd t
	The emulator is based suckless st.c and support most xterm ansi sequences
*/
#include "terminal.h"
#include "../editor/editor_header.h"
#include "files.h"
#include "font.h"
#include "imgui.h"
#include "util/settings.h"
#include <iostream>

#ifndef PLATFORM_WINDOWS
#include <errno.h>
#include <fcntl.h>
#include <limits.h> // For PATH_MAX (on Linux)
#include <pwd.h>	// For getpwuid
#include <signal.h>
#include <stdlib.h> // For getenv, setenv, unsetenv, realpath
#include <string.h> // For strrchr, strcpy, strncpy, strerror
#include <sys/ioctl.h>
#include <sys/types.h> // For getpwuid, uid_t
#include <termios.h>
#include <unistd.h>
#endif

#ifndef PATH_MAX // Define a fallback if PATH_MAX is not found (e.g., on some
				 // systems)
#define PATH_MAX 1024
#endif

ImVec4 Terminal::defaultColorMap[16] = {
	// Standard colors
	ImVec4(0.0f, 0.0f, 0.0f, 1.0f), // Black
	ImVec4(0.8f, 0.2f, 0.2f, 1.0f), // Rich Red
	ImVec4(0.2f, 0.8f, 0.2f, 1.0f), // Vibrant Green
	ImVec4(0.9f, 0.9f, 0.3f, 1.0f), // Sunny Yellow
	ImVec4(0.2f, 0.5f, 1.0f, 1.0f), // Sky Blue (brighter blue)
	ImVec4(0.8f, 0.3f, 0.8f, 1.0f), // Electric Purple
	ImVec4(0.3f, 0.8f, 0.8f, 1.0f), // Aqua Cyan
	ImVec4(0.9f, 0.9f, 0.9f, 1.0f), // Off-White

	// Bright colors (pastel-like but still vibrant)
	ImVec4(0.5f, 0.5f, 0.5f, 1.0f), // Medium Gray
	ImVec4(1.0f, 0.4f, 0.4f, 1.0f), // Coral Red
	ImVec4(0.4f, 1.0f, 0.4f, 1.0f), // Lime Green
	ImVec4(1.0f, 1.0f, 0.6f, 1.0f), // Lemon Yellow
	ImVec4(0.4f, 0.6f, 1.0f, 1.0f), // Bright Sky Blue
	ImVec4(1.0f, 0.5f, 1.0f, 1.0f), // Pink Purple
	ImVec4(0.5f, 1.0f, 1.0f, 1.0f), // Ice Blue
	ImVec4(1.0f, 1.0f, 1.0f, 1.0f)	// Pure White
};

extern Terminal gTerminal;
extern class FileExplorer gFileExplorer;

Terminal::Terminal()
{
	// Initialize with safe default size
	state.row = 24;
	state.col = 80;
	state.bot = state.row - 1;
	sel.mode = SEL_IDLE;
	sel.type = SEL_REGULAR;
	sel.snap = 0;
	sel.ob.x = -1;
	sel.ob.y = -1;
	sel.oe.x = -1;
	sel.oe.y = -1;
	sel.nb.x = -1;
	sel.nb.y = -1;
	sel.ne.x = -1;
	sel.ne.y = -1;
	sel.alt = 0;
	// Initialize screen buffer
	state.lines.resize(state.row, std::vector<Glyph>(state.col));
	state.altLines.resize(state.row, std::vector<Glyph>(state.col));
	state.dirty.resize(state.row, true);

	// Initialize tab stops (every 8 columns)
	state.tabs.resize(state.col, false);
	for (int i = 8; i < state.col; i += 8)
	{
		state.tabs[i] = true;
	}
}
void Terminal::UpdateTerminalColors()
{
	// Get the current theme from settings.
	const auto &themeJson =
		gSettings.getSettings()["themes"][gSettings.getCurrentTheme()]; // Renamed to
																		// avoid conflict

	// A simple lambda to load a color array from the theme.
	auto loadThemeColor = [&](const char *key) -> ImVec4 {
		const auto &colorJson = themeJson[key]; // Use the renamed 'themeJson'
		return ImVec4(colorJson[0].get<float>(),
					  colorJson[1].get<float>(),
					  colorJson[2].get<float>(),
					  colorJson[3].get<float>());
	};

	// --- Map your 7 theme colors to ANSI colors 1-7 ---
	// defaultColorMap[0] is handled specially for default background transparency.
	defaultColorMap[1] = loadThemeColor("type");	 // ANSI Red
	defaultColorMap[2] = loadThemeColor("string");	 // ANSI Green
	defaultColorMap[3] = loadThemeColor("function"); // ANSI Yellow
	defaultColorMap[4] = loadThemeColor("keyword");	 // ANSI Blue
	defaultColorMap[5] = loadThemeColor("number");	 // ANSI Magenta
	defaultColorMap[6] = loadThemeColor("variable"); // ANSI Cyan
	defaultColorMap[7] = loadThemeColor("text");	 // ANSI White

	ImVec4 commentColor = loadThemeColor("comment");
	defaultColorMap[0] = ImVec4(0.0f, 0.0f, 0.0f, commentColor.w);

	float brightnessFactor = 1.25f;

	for (int i = 0; i < 8; ++i)
	{ // This will also process index 0
		ImVec4 baseColor = defaultColorMap[i];
		defaultColorMap[i + 8] = ImVec4(std::min(1.0f, baseColor.x * brightnessFactor),
										std::min(1.0f, baseColor.y * brightnessFactor),
										std::min(1.0f, baseColor.z * brightnessFactor),
										baseColor.w // Keep original alpha
		);
	}
}

Terminal::~Terminal()
{
	shouldTerminate = true;

	// Close PTY first to signal the child process
#ifndef PLATFORM_WINDOWS
	if (ptyFd >= 0)
	{
		close(ptyFd);
		ptyFd = -1;
	}

	// Terminate child process
	if (childPid > 0)
	{
		kill(childPid, SIGTERM);
		childPid = -1;
	}
#endif

	// Wait for read thread with timeout
	if (readThread.joinable())
	{
		readThread.join();
	}
}

#ifndef PLATFORM_WINDOWS
void Terminal::startShell()
{
	// Open PTY master
	ptyFd = posix_openpt(O_RDWR | O_NOCTTY);
	if (ptyFd < 0)
	{
		perror("posix_openpt failed");
		return;
	}

	if (grantpt(ptyFd) < 0)
	{
		perror("grantpt failed");
		close(ptyFd);
		ptyFd = -1;
		return;
	}

	if (unlockpt(ptyFd) < 0)
	{
		perror("unlockpt failed");
		close(ptyFd);
		ptyFd = -1;
		return;
	}

	char *slaveName = ptsname(ptyFd);
	if (!slaveName)
	{
		perror("ptsname failed");
		close(ptyFd);
		ptyFd = -1;
		return;
	}

	childPid = fork();

	if (childPid < 0)
	{ // Fork failed
		perror("fork failed");
		close(ptyFd);
		ptyFd = -1;
		return;
	}

	if (childPid == 0)
	{				  // Child process
		close(ptyFd); // Close master PTY in child

		if (setsid() < 0)
		{ // Create new session, detach from parent's controlling TTY
			perror("setsid failed");
			exit(EXIT_FAILURE);
		}

		// Open slave PTY
		int slaveFd = open(slaveName, O_RDWR);
		if (slaveFd < 0)
		{
			perror("open slave PTY failed");
			exit(EXIT_FAILURE);
		}

		// Make the slave PTY the controlling terminal for this new session
		if (ioctl(slaveFd, TIOCSCTTY, 0) < 0)
		{
			// This can fail if the process is not a session leader and already
			// has a controlling TTY. setsid() should make us a session leader.
			perror("ioctl TIOCSCTTY failed (can be non-fatal depending on "
				   "context)");
		}

		dup2(slaveFd, STDIN_FILENO);
		dup2(slaveFd, STDOUT_FILENO);
		dup2(slaveFd, STDERR_FILENO);

		if (slaveFd > STDERR_FILENO)
		{
			close(slaveFd);
		}

		// Configure terminal modes for the slave PTY
		struct termios tios;
		if (tcgetattr(STDIN_FILENO, &tios) < 0)
		{
			perror("tcgetattr failed on slave pty");
			exit(EXIT_FAILURE);
		}

		// Set reasonable default modes (from st/typical terminal settings)
		tios.c_iflag = ICRNL | IXON | IXANY | IMAXBEL | BRKINT;
#ifdef IUTF8 // Common on Linux, good to enable if available
		tios.c_iflag |= IUTF8;
#endif
		tios.c_oflag =
			OPOST | ONLCR; // OPOST: enable output processing, ONLCR: map NL to CR-NL

		tios.c_cflag &= ~(CSIZE | PARENB); // Clear size and parity bits
		tios.c_cflag |= CS8;			   // 8 bits per character
		tios.c_cflag |= CREAD;			   // Enable receiver
		tios.c_cflag |= HUPCL;			   // Hang up on last close (sends SIGHUP to
										   // foreground process group)

		// Standard local modes for interactive shells
		tios.c_lflag = ICANON | ISIG | IEXTEN | ECHO | ECHOE | ECHOK | ECHOCTL | ECHOKE;

		if (tcsetattr(STDIN_FILENO, TCSANOW, &tios) < 0)
		{
			perror("tcsetattr failed on slave pty");
			exit(EXIT_FAILURE);
		}

		// Set window size
		struct winsize ws = {};
		ws.ws_row = state.row; // Use initial rows/cols from Terminal state object
		ws.ws_col = state.col;
		if (ioctl(STDIN_FILENO, TIOCSWINSZ, &ws) < 0)
		{
			perror("ioctl TIOCSWINSZ failed on slave pty (non-fatal, shell "
				   "might misbehave)");
		}

		// Prepare environment for the shell
		setenv("TERM", "xterm-256color", 1);
		// Unsetting these is often good as the login shell will set them
		// appropriately
		unsetenv("COLUMNS");
		unsetenv("LINES");
		// Optionally, clear other inherited variables that might cause issues,
		// e.g., unsetenv("TERMCAP"); unsetenv("WINDOWID"); // Common in some
		// terminal emulators

#ifdef __APPLE__
		// Revised logic for macOS: Launch as a login shell
		const char *user_shell_from_passwd = nullptr;
		struct passwd *pw = getpwuid(getuid());
		if (pw && pw->pw_shell && pw->pw_shell[0] != '\0')
		{
			user_shell_from_passwd = pw->pw_shell;
		}

		const char *shell_to_launch_path_str = user_shell_from_passwd;

		// Fallback 1: getenv("SHELL")
		if (!shell_to_launch_path_str || shell_to_launch_path_str[0] == '\0')
		{
			shell_to_launch_path_str = getenv("SHELL");
		}

		// Fallback 2: Default to /bin/zsh (modern macOS default)
		// If on an older Mac where /bin/bash is default, this might still be better.
		if (!shell_to_launch_path_str || shell_to_launch_path_str[0] == '\0')
		{
			shell_to_launch_path_str = "/bin/zsh";
		}

		char shell_exec_path_buf[PATH_MAX]; // Full path to the executable
		strncpy(shell_exec_path_buf,
				shell_to_launch_path_str,
				sizeof(shell_exec_path_buf));
		shell_exec_path_buf[sizeof(shell_exec_path_buf) - 1] =
			'\0'; // Ensure null termination

		// Prepare argv[0] for the child shell: "-basename"
		char shell_argv0_login[PATH_MAX + 1]; // +1 for the leading hyphen
		shell_argv0_login[0] = '-'; // Signal to the shell to act as a login shell

		const char *shell_basename_ptr = strrchr(shell_exec_path_buf, '/');
		if (shell_basename_ptr)
		{
			// Found a '/', so use the part after it
			strncpy(shell_argv0_login + 1,
					shell_basename_ptr + 1,
					sizeof(shell_argv0_login) - 2);
		} else
		{
			// No '/', so use the whole path string (it's already a basename or
			// relative)
			strncpy(shell_argv0_login + 1,
					shell_exec_path_buf,
					sizeof(shell_argv0_login) - 2);
		}
		shell_argv0_login[sizeof(shell_argv0_login) - 1] =
			'\0'; // Ensure null termination

		// Arguments for a login shell.
		char *const args[] = {shell_argv0_login, NULL};

		// --- Debugging Output ---
		fprintf(stderr, "[TERMINAL DEBUG] macOS Shell Launch Information:\n");
		fprintf(stderr,
				"  User's pw_shell (from getpwuid): '%s'\n",
				(pw && pw->pw_shell) ? pw->pw_shell : "(not found or empty)");
		const char *env_shell_in_child = getenv("SHELL");
		fprintf(stderr,
				"  getenv(\"SHELL\") in child process: '%s'\n",
				env_shell_in_child ? env_shell_in_child : "(not set or empty)");
		fprintf(stderr,
				"  Path to be executed (shell_exec_path_buf): '%s'\n",
				shell_exec_path_buf);
		fprintf(stderr,
				"  argv[0] for child shell (shell_argv0_login): '%s'\n",
				args[0] ? args[0] : "(NULL)");
		// --- End Debugging Output ---

		execv(shell_exec_path_buf, args);

		// If execv returns, an error occurred.
		fprintf(stderr,
				"FATAL: Failed to execv shell '%s' (intended argv[0]='%s'): %s\n",
				shell_exec_path_buf,
				args[0] ? args[0] : "(null)",
				strerror(errno));
		exit(EXIT_FAILURE); // Or exit(127) for command not found / exec failure
							// Removed unreachable:
		// std::this_thread::sleep_for(std::chrono::milliseconds(100));
#else
		// Logic for Linux and other Unix-like systems (also launch as login shell)
		char shell_path_buf[PATH_MAX];
		const char *shell_env_val = getenv("SHELL"); // SHELL env var is primary on Linux

		if (shell_env_val && shell_env_val[0] != '\0')
		{
			strncpy(shell_path_buf, shell_env_val, sizeof(shell_path_buf));
			shell_path_buf[sizeof(shell_path_buf) - 1] = '\0';
		} else
		{
			struct passwd *pw_linux = getpwuid(getuid()); // Fallback to passwd entry
			if (pw_linux && pw_linux->pw_shell && pw_linux->pw_shell[0] != '\0')
			{
				strncpy(shell_path_buf, pw_linux->pw_shell, sizeof(shell_path_buf));
				shell_path_buf[sizeof(shell_path_buf) - 1] = '\0';
			} else
			{
				strncpy(shell_path_buf,
						"/bin/bash",
						sizeof(shell_path_buf)); // Absolute fallback for Linux
				shell_path_buf[sizeof(shell_path_buf) - 1] = '\0';
			}
		}

		char shell_argv0_login_linux[PATH_MAX + 1];
		shell_argv0_login_linux[0] = '-';
		const char *shell_basename_linux = strrchr(shell_path_buf, '/');
		if (shell_basename_linux)
		{
			strncpy(shell_argv0_login_linux + 1,
					shell_basename_linux + 1,
					sizeof(shell_argv0_login_linux) - 2);
		} else
		{
			strncpy(shell_argv0_login_linux + 1,
					shell_path_buf,
					sizeof(shell_argv0_login_linux) - 2);
		}
		shell_argv0_login_linux[sizeof(shell_argv0_login_linux) - 1] = '\0';

		char *const new_argv_linux[] = {shell_argv0_login_linux, NULL};

		// --- Debugging Output for Linux ---
		fprintf(stderr, "[TERMINAL DEBUG] Linux/Other Shell Launch Information:\n");
		struct passwd *pw_linux_debug = getpwuid(getuid());
		fprintf(stderr,
				"  User's pw_shell (from getpwuid): '%s'\n",
				(pw_linux_debug && pw_linux_debug->pw_shell) ? pw_linux_debug->pw_shell
															 : "(not found or empty)");
		const char *env_shell_child_linux = getenv("SHELL");
		fprintf(stderr,
				"  getenv(\"SHELL\") in child process: '%s'\n",
				env_shell_child_linux ? env_shell_child_linux : "(not set or empty)");
		fprintf(stderr, "  Path to be executed (shell_path_buf): '%s'\n", shell_path_buf);
		fprintf(stderr,
				"  argv[0] for child shell (new_argv_linux[0]): '%s'\n",
				new_argv_linux[0] ? new_argv_linux[0] : "(NULL)");
		// --- End Debugging Output for Linux ---

		execv(shell_path_buf, new_argv_linux);

		fprintf(stderr,
				"FATAL: Failed to execv shell '%s' (intended argv[0]='%s'): %s\n",
				shell_path_buf,
				new_argv_linux[0] ? new_argv_linux[0] : "(null)",
				strerror(errno));
		exit(127); // Standard exit code for command not found / exec failure
#endif
	}

	// Parent process
	// Make sure ptyFd is non-blocking for the read thread if select/poll isn't
	// used This can be optional if read() behaves well or if your readOutput
	// uses select/poll. int flags = fcntl(ptyFd, F_GETFL, 0); if (flags != -1)
	// fcntl(ptyFd, F_SETFL, flags | O_NONBLOCK);

#ifdef __APPLE__
	// Send initial commands (cd ~ and clear) to the shell on Apple systems.
	// A short delay helps ensure the shell has initialized before receiving
	// commands. This is a heuristic; 100ms is a common value.
	std::this_thread::sleep_for(std::chrono::milliseconds(100));

	std::string command_str =
		std::string("cd ") + gFileExplorer.selectedFolder + "\nclear\n";

	// 2. Now get the pointer. It's valid as long as command_str exists.
	const char *initial_commands = command_str.c_str();
	ssize_t bytes_written = write(ptyFd, initial_commands, strlen(initial_commands));
	if (bytes_written == -1)
	{
		perror("write to pty (initial_commands for Apple) failed in parent");
		// This is a non-critical enhancement, so we log the error and continue.
	} else if (static_cast<size_t>(bytes_written) < strlen(initial_commands))
	{
		// Handle partial write, though unlikely for small command strings with TTYs.
		fprintf(stderr, "Partial write to pty (initial_commands for Apple) in parent.\n");
	}
#endif

	readThread = std::thread(&Terminal::readOutput, this);
}
#else
void Terminal::startShell()
{
	// Terminal not supported on Windows
}
#endif

void Terminal::render()
{
	if (!isVisible)
		return;

#ifndef PLATFORM_WINDOWS
	if (ptyFd < 0)
	{
		startShell();
	}
#endif

	ImGuiIO &io = ImGui::GetIO();

	// **changed**: Push the global font to ensure terminal uses the correct font after
	// reloading
	extern Font gFont;
	ImGui::PushFont(gFont.currentFont);

	checkFontSizeChange();
	bool windowCreated = setupWindow();

	// Only render terminal content if window is open and not collapsed
	if (windowCreated && (!isEmbedded || !embeddedWindowCollapsed))
	{
		// Move cursor down 8px from top
		ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 8.0f);

		// Add horizontal padding for left and right margins
		ImGui::Indent(9.0f);

		// Render editor header above terminal content
		static EditorHeader editorHeader;
		editorHeader.render(gFont.currentFont, "Terminal", false);

		// Add small spacing after header
		ImGui::Spacing();

		handleTerminalResize();

		// Add left padding only - right padding handled in terminal rendering
		ImGui::Indent(10.0f);
		renderBuffer();
		ImGui::Unindent(10.0f);

		ImGui::Unindent(9.0f);
		handleScrollback(io, state.row);
		handleMouseInput(io);
		handleKeyboardInput(io);
	}

	// Only call End() if Begin() was actually called and succeeded
	if (windowCreated)
	{
		ImGui::End();
	}

	// **changed**: Pop the font we pushed
	ImGui::PopFont();
}

void Terminal::renderBuffer()
{
	std::lock_guard<std::mutex> lock(bufferMutex);

	ImDrawList *drawList;
	ImVec2 pos;
	float charWidth, lineHeight;
	setupRenderContext(drawList, pos, charWidth, lineHeight);

	if (state.mode & MODE_ALTSCREEN)
	{
		renderAltScreen(drawList, pos, charWidth, lineHeight);
	} else
	{
		renderMainScreen(drawList, pos, charWidth, lineHeight);
	}
}

void Terminal::checkFontSizeChange()
{
	float currentFontSize = ImGui::GetFont()->LegacySize;
	if (currentFontSize != lastFontSize)
	{
		lastFontSize = currentFontSize;
		handleTerminalResize(); // Calculate new dimensions based on new font size
	}
}

bool Terminal::setupWindow()
{
	if (isEmbedded)
	{
		ImGui::SetNextWindowPos(embeddedWindowPos, ImGuiCond_FirstUseEver);
		ImGui::SetNextWindowSize(embeddedWindowSize, ImGuiCond_FirstUseEver);

		bool windowOpen = true;
		bool windowCreated =
			ImGui::Begin("Terminal", &windowOpen, ImGuiWindowFlags_NoCollapse);
		if (windowCreated)
		{
			embeddedWindowPos = ImGui::GetWindowPos();
			embeddedWindowSize = ImGui::GetWindowSize();
			embeddedWindowCollapsed = ImGui::IsWindowCollapsed();
			if (!windowOpen)
			{
				isVisible = false;
			}
		} else
		{
			embeddedWindowCollapsed = true;
		}
		return windowCreated;
	} else
	{
		ImGui::SetNextWindowPos(ImVec2(0, 0));
		ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);
		bool windowCreated =
			ImGui::Begin("Terminal",
						 nullptr,
						 ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
							 ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);
		return windowCreated;
	}
}

void Terminal::handleTerminalResize()
{
	ImVec2 contentSize = ImGui::GetContentRegionAvail();
	// Reduce width by 27px to account for right padding (10px left indent + 17px right margin)
	contentSize.x -= 27.0f;
	float charWidth = ImGui::GetFontBaked()->GetCharAdvance('M');
	float lineHeight = ImGui::GetTextLineHeight();

	int new_cols = std::max(1, static_cast<int>(contentSize.x / charWidth));
	int new_rows = std::max(1, static_cast<int>(contentSize.y / lineHeight));

	if (new_cols != state.col || new_rows != state.row)
	{
		std::cout << "resizing terminal" << std::endl;

		resize(new_cols, new_rows);
	}
}

void Terminal::handleScrollback(const ImGuiIO &io, int new_rows)
{
	if (ImGui::IsWindowFocused() && ImGui::IsWindowHovered() &&
		!(state.mode & MODE_ALTSCREEN))
	{
		if (io.MouseWheel != 0.0f)
		{
			int maxScroll =
				std::max(0, (int)(scrollbackBuffer.size() + state.row) - new_rows);
			// Reverse the scroll direction by changing subtraction to addition
			scrollOffset += static_cast<int>(io.MouseWheel * 3);
			scrollOffset = std::clamp(scrollOffset, 0, maxScroll);
		}
	}
}

void Terminal::handleMouseInput(const ImGuiIO &io)
{
	if (!ImGui::IsWindowFocused() || !ImGui::IsWindowHovered())
		return;

	ImVec2 mousePos = ImGui::GetMousePos();
	ImVec2 contentPos = ImGui::GetCursorScreenPos();
	float charWidth = ImGui::GetFontBaked()->GetCharAdvance('M');
	float lineHeight = ImGui::GetTextLineHeight();

	// Account for terminal padding: 9px (first indent) + 10px (second indent) = 19px total
	const float totalLeftPadding = 19.0f;

	int cellX =
		static_cast<int>((mousePos.x - contentPos.x - totalLeftPadding) / charWidth);
	int cellY =
		static_cast<int>((mousePos.y - contentPos.y + (lineHeight * 0.2)) / lineHeight);

	cellX = std::clamp(cellX, 0, state.col - 1);

	// Account for scrollback offset when not in alt screen
	if (!(state.mode & MODE_ALTSCREEN))
	{
		ImVec2 contentSize = ImGui::GetContentRegionAvail();
		int visibleRows = std::max(1, static_cast<int>(contentSize.y / lineHeight));
		int totalLines = scrollbackBuffer.size() + state.row;
		int maxScroll = std::max(0, totalLines - visibleRows);
		scrollOffset = std::clamp(scrollOffset, 0, maxScroll);
		int startLine = std::max(0, totalLines - visibleRows - scrollOffset);

		// Convert visible Y coordinate to actual buffer coordinate
		int actualY = startLine + cellY;

		// Convert to selection coordinate system (relative to scrollback buffer)
		cellY = actualY - scrollbackBuffer.size();

	} else
	{
		// In alt screen, clamp to current screen
		cellY = std::clamp(cellY, 0, state.row - 1);
	}

	static ImVec2 clickStartPos{0, 0};

	if (ImGui::IsMouseClicked(ImGuiMouseButton_Left))
	{
		clickStartPos = mousePos;
		selectionStart(cellX, cellY);
	} else if (ImGui::IsMouseDragging(ImGuiMouseButton_Left))
	{
		ImVec2 dragDelta = ImGui::GetMouseDragDelta(ImGuiMouseButton_Left);
		float dragDistance = sqrt(dragDelta.x * dragDelta.x + dragDelta.y * dragDelta.y);

		if (dragDistance > DRAG_THRESHOLD)
		{
			selectionExtend(cellX, cellY);
		}
	} else if (ImGui::IsMouseReleased(ImGuiMouseButton_Left))
	{
		ImVec2 dragDelta = ImGui::GetMouseDragDelta(ImGuiMouseButton_Left);
		float dragDistance = sqrt(dragDelta.x * dragDelta.x + dragDelta.y * dragDelta.y);

		if (dragDistance <= DRAG_THRESHOLD)
		{
			selectionClear();
		}
	}

	if (ImGui::IsMouseClicked(ImGuiMouseButton_Right))
	{
		pasteFromClipboard();
	}

	// Handle clipboard shortcuts
	if (io.KeyCtrl)
	{
		if (ImGui::IsKeyPressed(ImGuiKey_Y, false) ||
			ImGui::IsKeyPressed(ImGuiKey_C, false))
		{
			copySelection();
		}
		if (ImGui::IsKeyPressed(ImGuiKey_V, false))
		{
			pasteFromClipboard();
		}
	}
}

void Terminal::handleKeyboardInput(const ImGuiIO &io)
{
	if (!ImGui::IsWindowFocused())
		return;

	handleSpecialKeys(io);
	handleControlCombos(io);
	handleRegularTextInput(io);
}

void Terminal::handleSpecialKeys(const ImGuiIO &io)
{
	if (ImGui::IsKeyPressed(ImGuiKey_Enter))
	{
		processInput("\r");
	} else if (ImGui::IsKeyPressed(ImGuiKey_Backspace))
	{
		processInput("\x7f");
	} else if (ImGui::IsKeyPressed(ImGuiKey_UpArrow))
	{
		processInput(state.mode & MODE_APPCURSOR ? "\033OA" : "\033[A");
	} else if (ImGui::IsKeyPressed(ImGuiKey_DownArrow))
	{
		processInput(state.mode & MODE_APPCURSOR ? "\033OB" : "\033[B");
	} else if (ImGui::IsKeyPressed(ImGuiKey_RightArrow))
	{
		processInput(state.mode & MODE_APPCURSOR ? "\033OC" : "\033[C");
	} else if (ImGui::IsKeyPressed(ImGuiKey_LeftArrow))
	{
		if (io.KeyCtrl)
		{
			processInput("\033[1;5D");
		} else if (io.KeyShift)
		{
			processInput("\033[1;2D");
		} else if (state.mode & MODE_APPCURSOR)
		{
			processInput("\033OD");
		} else
		{
			processInput("\033[D");
		}
	} else if (ImGui::IsKeyPressed(ImGuiKey_Home))
	{
		processInput("\033[H");
	} else if (ImGui::IsKeyPressed(ImGuiKey_End))
	{
		processInput("\033[F");
	} else if (ImGui::IsKeyPressed(ImGuiKey_Delete))
	{
		processInput("\033[3~");
	} else if (ImGui::IsKeyPressed(ImGuiKey_PageUp))
	{
		processInput("\033[5~");
	} else if (ImGui::IsKeyPressed(ImGuiKey_PageDown))
	{
		processInput("\033[6~");
	} else if (ImGui::IsKeyPressed(ImGuiKey_Tab))
	{
		processInput("\t");
	} else if (ImGui::IsKeyPressed(ImGuiKey_Escape))
	{
		processInput("\033");
	}
}

void Terminal::handleControlCombos(const ImGuiIO &io)
{
	if (!io.KeyCtrl && !io.KeySuper)
		return;

	static const std::pair<ImGuiKey, char> controlKeys[] = {
		{ImGuiKey_A, '\x01'}, {ImGuiKey_B, '\x02'}, {ImGuiKey_C, '\x03'},
		{ImGuiKey_D, '\x04'}, {ImGuiKey_E, '\x05'}, {ImGuiKey_F, '\x06'},
		{ImGuiKey_G, '\x07'}, {ImGuiKey_H, '\x08'}, {ImGuiKey_I, '\x09'},
		{ImGuiKey_J, '\x0A'}, {ImGuiKey_K, '\x0B'}, {ImGuiKey_L, '\x0C'},
		{ImGuiKey_M, '\x0D'}, {ImGuiKey_N, '\x0E'}, {ImGuiKey_O, '\x0F'},
		{ImGuiKey_P, '\x10'}, {ImGuiKey_Q, '\x11'}, {ImGuiKey_R, '\x12'},
		{ImGuiKey_S, '\x13'}, {ImGuiKey_T, '\x14'}, {ImGuiKey_U, '\x15'},
		{ImGuiKey_W, '\x17'}, {ImGuiKey_X, '\x18'}, {ImGuiKey_Y, '\x19'},
		{ImGuiKey_Z, '\x1A'}};

	for (const auto &[key, ctrl_char] : controlKeys)
	{
		if (ImGui::IsKeyPressed(key))
		{
			processInput(std::string(1, ctrl_char));
		}
	}
}

void Terminal::handleRegularTextInput(const ImGuiIO &io)
{
	if (io.KeySuper || io.KeyCtrl || io.KeyAlt)
		return;

	for (int i = 0; i < io.InputQueueCharacters.Size; i++)
	{
		char c = (char)io.InputQueueCharacters[i];
		if (c != 0)
		{
			processInput(std::string(1, c));
		}
	}
}

void Terminal::setupRenderContext(ImDrawList *&drawList,
								  ImVec2 &pos,
								  float &charWidth,
								  float &lineHeight)
{
	drawList = ImGui::GetWindowDrawList();
	pos = ImGui::GetCursorScreenPos();
	charWidth = ImGui::GetFontBaked()->GetCharAdvance('M');
	lineHeight = ImGui::GetTextLineHeight();
}

void Terminal::renderAltScreen(ImDrawList *drawList,
							   const ImVec2 &pos,
							   float charWidth,
							   float lineHeight)
{
	// Handle selection highlight
	if (sel.mode != SEL_IDLE && sel.ob.x != -1)
	{
		renderSelectionHighlight(drawList, pos, charWidth, lineHeight, 0, state.row);
	}

	// Draw alt screen characters
	for (int y = 0; y < state.row; y++)
	{
		if (!state.dirty[y])
			continue;

		for (int x = 0; x < state.col; x++)
		{
			const Glyph &glyph = state.lines[y][x];
			if (glyph.mode & ATTR_WDUMMY)
				continue;

			ImVec2 charPos(pos.x + x * charWidth, pos.y + y * lineHeight);
			renderGlyph(drawList, glyph, charPos, charWidth, lineHeight);

			if (glyph.mode & ATTR_WIDE)
				x++;
		}
	}

	// Draw cursor
	if (ImGui::IsWindowFocused())
	{
		ImVec2 cursorPos(pos.x + state.c.x * charWidth, pos.y + state.c.y * lineHeight);
		float alpha = (sin(ImGui::GetTime() * 3.14159f) * 0.3f) + 0.5f;
		renderCursor(drawList,
					 cursorPos,
					 state.lines[state.c.y][state.c.x],
					 charWidth,
					 lineHeight,
					 alpha);
	}
}

void Terminal::renderMainScreen(ImDrawList *drawList,
								const ImVec2 &pos,
								float charWidth,
								float lineHeight)
{
	ImVec2 contentSize = ImGui::GetContentRegionAvail();
	int visibleRows = std::max(1, static_cast<int>(contentSize.y / lineHeight));
	int totalLines = scrollbackBuffer.size() + state.row;

	// Handle scrollback clamping
	int maxScroll = std::max(0, totalLines - visibleRows);
	scrollOffset = std::clamp(scrollOffset, 0, maxScroll);
	int startLine = std::max(0, totalLines - visibleRows - scrollOffset);

	// Handle selection highlighting
	if (sel.mode != SEL_IDLE && sel.ob.x != -1)
	{
		renderSelectionHighlight(drawList,
								 pos,
								 charWidth,
								 lineHeight,
								 startLine,
								 startLine + visibleRows,
								 scrollbackBuffer.size());
	}

	// Draw content
	for (int visY = 0; visY < visibleRows; visY++)
	{
		int currentLine = startLine + visY;
		const std::vector<Glyph> *line = nullptr;

		if (currentLine < scrollbackBuffer.size())
		{
			line = &scrollbackBuffer[currentLine];
		} else
		{
			int screenY = currentLine - scrollbackBuffer.size();
			if (screenY >= 0 && screenY < state.lines.size())
			{
				line = &state.lines[screenY];
			}
		}

		if (!line)
			continue;

		for (int x = 0; x < state.col && x < line->size(); x++)
		{
			const Glyph &glyph = (*line)[x];
			if (glyph.mode & ATTR_WDUMMY)
				continue;

			ImVec2 charPos(pos.x + x * charWidth, pos.y + visY * lineHeight);
			renderGlyph(drawList, glyph, charPos, charWidth, lineHeight);

			if (glyph.mode & ATTR_WIDE)
				x++;
		}
	}

	// Draw cursor when not scrolled
	if (ImGui::IsWindowFocused() && scrollOffset == 0)
	{
		ImVec2 cursorPos(pos.x + state.c.x * charWidth,
						 pos.y + (visibleRows - (totalLines - scrollbackBuffer.size()) +
								  state.c.y) *
									 lineHeight);
		float alpha = (sin(ImGui::GetTime() * 3.14159f) * 0.3f) + 0.5f;
		renderCursor(drawList,
					 cursorPos,
					 state.lines[state.c.y][state.c.x],
					 charWidth,
					 lineHeight,
					 alpha);
	}
}
void Terminal::renderGlyph(ImDrawList *drawList,
						   const Glyph &glyph,
						   const ImVec2 &charPos,
						   float charWidth,
						   float lineHeight)
{
	ImVec4 fg = glyph.fg;
	ImVec4 bg = glyph.bg;

	handleGlyphColors(glyph, fg, bg);

	// Draw background
	if (bg.x != 0 || bg.y != 0 || bg.z != 0 || (glyph.mode & ATTR_REVERSE))
	{
		drawList->AddRectFilled(charPos,
								ImVec2(charPos.x + charWidth, charPos.y + lineHeight),
								ImGui::ColorConvertFloat4ToU32(bg));
	}

	// Draw character
	if (glyph.u != ' ' && glyph.u != 0)
	{
		char text[UTF_SIZ] = {0};
		utf8Encode(glyph.u, text);
		drawList->AddText(charPos, ImGui::ColorConvertFloat4ToU32(fg), text);
	}

	// Draw underline
	if (glyph.mode & ATTR_UNDERLINE)
	{
		drawList->AddLine(ImVec2(charPos.x, charPos.y + lineHeight - 1),
						  ImVec2(charPos.x + charWidth, charPos.y + lineHeight - 1),
						  ImGui::ColorConvertFloat4ToU32(fg));
	}
}

void Terminal::handleGlyphColors(const Glyph &glyph, ImVec4 &fg, ImVec4 &bg)
{
	// Handle true color
	if (glyph.colorMode == COLOR_TRUE)
	{
		uint32_t tc = glyph.trueColorFg;
		fg = ImVec4(((tc >> 16) & 0xFF) / 255.0f,
					((tc >> 8) & 0xFF) / 255.0f,
					(tc & 0xFF) / 255.0f,
					1.0f);
	}

	// Handle reverse video
	if (glyph.mode & ATTR_REVERSE)
	{
		std::swap(fg, bg);
	}

	// Handle bold
	if (glyph.mode & ATTR_BOLD && glyph.colorMode == COLOR_BASIC)
	{
		fg.x = std::min(1.0f, fg.x * 1.5f);
		fg.y = std::min(1.0f, fg.y * 1.5f);
		fg.z = std::min(1.0f, fg.z * 1.5f);
	}
}

void Terminal::renderCursor(ImDrawList *drawList,
							const ImVec2 &cursorPos,
							const Glyph &cursorCell,
							float charWidth,
							float lineHeight,
							float alpha)
{
	if (state.mode & MODE_INSERT)
	{
		drawList->AddRectFilled(cursorPos,
								ImVec2(cursorPos.x + 2, cursorPos.y + lineHeight),
								ImGui::ColorConvertFloat4ToU32(
									ImVec4(0.7f, 0.7f, 0.7f, alpha)));
	} else
	{
		if (cursorCell.u != 0)
		{
			char text[UTF_SIZ] = {0};
			utf8Encode(cursorCell.u, text);
			ImVec4 bg = cursorCell.fg;
			ImVec4 fg = cursorCell.bg;

			drawList->AddRectFilled(
				cursorPos,
				ImVec2(cursorPos.x + charWidth, cursorPos.y + lineHeight),
				ImGui::ColorConvertFloat4ToU32(ImVec4(bg.x, bg.y, bg.z, alpha)));
			drawList->AddText(cursorPos, ImGui::ColorConvertFloat4ToU32(fg), text);
		} else
		{
			drawList->AddRectFilled(
				cursorPos,
				ImVec2(cursorPos.x + charWidth, cursorPos.y + lineHeight),
				ImGui::ColorConvertFloat4ToU32(ImVec4(0.7f, 0.7f, 0.7f, alpha)));
		}
	}
}

void Terminal::renderSelectionHighlight(ImDrawList *drawList,
										const ImVec2 &pos,
										float charWidth,
										float lineHeight,
										int startY,
										int endY,
										int screenOffset)
{
	for (int y = startY; y < endY; y++)
	{
		int screenY = y - screenOffset;
		// Handle both current screen and scrollback lines
		if (screenY >= 0 && screenY < state.row)
		{
			// Current screen line
			for (int x = 0; x < state.col; x++)
			{
				// Convert visible coordinate to selection coordinate system
				int selectionY = screenOffset + screenY - scrollbackBuffer.size();
				if (selectedText(x, selectionY))
				{
					ImVec2 highlightPos(pos.x + x * charWidth,
										pos.y + (y - startY) * lineHeight);
					drawList->AddRectFilled(
						highlightPos,
						ImVec2(highlightPos.x + charWidth, highlightPos.y + lineHeight),
						ImGui::ColorConvertFloat4ToU32(ImVec4(1.0f, 0.1f, 0.7f, 0.3f)));
				}
			}
		} else if (screenY < 0)
		{
			// Scrollback line - render it if it's visible
			int scrollbackIndex = -screenY - 1;
			if (scrollbackIndex >= 0 && scrollbackIndex < scrollbackBuffer.size())
			{
				for (int x = 0; x < state.col; x++)
				{
					// Convert visible coordinate to selection coordinate system
					int selectionY = screenOffset + screenY - scrollbackBuffer.size();
					if (selectedText(x, selectionY))
					{
						ImVec2 highlightPos(pos.x + x * charWidth,
											pos.y + (y - startY) * lineHeight);
						drawList->AddRectFilled(highlightPos,
												ImVec2(highlightPos.x + charWidth,
													   highlightPos.y + lineHeight),
												ImGui::ColorConvertFloat4ToU32(
													ImVec4(1.0f, 0.1f, 0.7f, 0.3f)));
					}
				}
			}
		}
	}
}

void Terminal::toggleVisibility()
{
#ifndef PLATFORM_WINDOWS
	isVisible = !isVisible;
#endif
	// On Windows, do nothing - terminal toggle is disabled
}

void Terminal::writeToBuffer(const char *data, size_t length)
{
	static char utf8buf[UTF_SIZ];
	static size_t utf8len = 0;

	for (size_t i = 0; i < length; ++i)
	{
		unsigned char c = data[i];

		// Existing STR sequence handling
		if (state.esc & ESC_STR)
		{
			if (c == '\a' || c == 030 || c == 032 || c == 033 || ISCONTROLC1(c))
			{
				state.esc &= ~(ESC_START | ESC_STR);
				state.esc |= ESC_STR_END;
				strparse();
				handleStringSequence();
				state.esc = 0;
				continue;
			}

			if (strescseq.len < 256)
			{
				strescseq.buf += c;
				strescseq.len++;
			}
			continue;
		}

		// Escape sequence start
		if (c == '\033')
		{
			state.esc = ESC_START;
			csiescseq.len = 0;
			strescseq.buf.clear();
			strescseq.len = 0;
			utf8len = 0; // Reset UTF-8 buffer
			continue;
		}

		// Ongoing escape sequence processing
		if (state.esc & ESC_START)
		{
			if (state.esc & ESC_CSI)
			{
				if (csiescseq.len < sizeof(csiescseq.buf) - 1)
				{
					csiescseq.buf[csiescseq.len++] = c;
					if (BETWEEN(c, 0x40, 0x7E))
					{
						csiescseq.buf[csiescseq.len] = '\0';
						csiescseq.mode[0] = c;
						parseCSIParam(csiescseq);
						handleCSI(csiescseq);
						state.esc = 0;
						csiescseq.len = 0;
					}
				}
				continue;
			}

			if (eschandle(c))
			{
				state.esc = 0;
			}
			continue;
		}

		// Control character handling
		if (ISCONTROL(c))
		{
			utf8len = 0; // Reset UTF-8 buffer
			handleControlCode(c);
			continue;
		}

		if (state.mode & MODE_UTF8)
		{
			if (utf8len == 0)
			{
				if ((c & 0x80) == 0)
				{
					writeChar(c);
				} else if ((c & 0xE0) == 0xC0 || // 2-byte start
						   (c & 0xF0) == 0xE0 || // 3-byte start
						   (c & 0xF8) == 0xF0)
				{ // 4-byte start
					utf8buf[utf8len++] = c;

					// If it's a 3-byte sequence (box drawing characters),
					// we want to immediately look for the next two bytes
					if ((c & 0xF0) == 0xE0)
					{
						// Look ahead for the next two bytes
						if (i + 2 < length)
						{
							utf8buf[utf8len++] = data[i + 1];
							utf8buf[utf8len++] = data[i + 2];

							Rune u;
							size_t decoded = utf8Decode(utf8buf, &u, utf8len);
							if (decoded > 0)
							{
								writeChar(u);
							}

							// Skip the next two bytes since we've processed
							// them
							i += 2;
							utf8len = 0;
						}
					}
				} else
				{
					// Unexpected start byte
					utf8buf[utf8len++] = c;
					utf8buf[utf8len] = '\0';
					writeChar(0xFFFD);
				}
			} else
			{
				// This block is now less likely to be used due to the changes
				// above
				if ((c & 0xC0) == 0x80)
				{
					utf8buf[utf8len++] = c;

					size_t expected_len = ((utf8buf[0] & 0xE0) == 0xC0)	  ? 2
										  : ((utf8buf[0] & 0xF0) == 0xE0) ? 3
										  : ((utf8buf[0] & 0xF8) == 0xF0) ? 4
																		  : 0;

					if (utf8len == expected_len)
					{
						Rune u;
						size_t decoded = utf8Decode(utf8buf, &u, utf8len);
						if (decoded > 0)
						{
							writeChar(u);
						}
						utf8len = 0;
					}
				} else
				{
					std::cerr << "Invalid continuation byte: 0x" << std::hex << (int)c
							  << std::dec << std::endl;
					utf8len = 0;
				}
			}
		} else
		{
			writeChar(c);
		}
	}
}

void Terminal::writeChar(Rune u)
{
	// Your existing box drawing character mapping logic
	auto it = boxDrawingChars.find(u);
	if (it != boxDrawingChars.end())
	{
		u = it->second;
	} else
	{
		// Log unmapped characters
		if (u >= 0x2500 && u <= 0x257F)
		{
			// std::cerr << "Unmapped box drawing character: U+" << std::hex <<
			// u << std::dec << " (hex: 0x" << std::hex
			//           << u << std::dec << ")" << std::endl;
		}
	}

	if (state.c.x >= state.col)
	{
		// Set wrap flag on current line before moving to next
		if (state.c.y < state.row && state.c.x > 0)
		{
			state.lines[state.c.y][state.c.x - 1].mode |= ATTR_WRAP;
		}

		state.c.x = 0;
		if (state.c.y == state.bot)
		{
			scrollUp(state.top, 1);
		} else if (state.c.y < state.row - 1)
		{
			state.c.y++;
		}
	}

	Glyph g;
	g.u = u;
	g.mode = state.c.attrs;
	g.fg = state.c.fg;
	g.bg = state.c.bg;
	g.colorMode = state.c.colorMode;
	g.trueColorFg = state.c.trueColorFg;
	g.trueColorBg = state.c.trueColorBg;

	// Set wrap flag if at end of line
	if (state.c.x == state.col - 1)
	{
		g.mode |= ATTR_WRAP;
	}

	writeGlyph(g, state.c.x, state.c.y);
	state.c.x++;
}

int Terminal::eschandle(unsigned char ascii)
{
	switch (ascii)
	{
	case '[':
		state.esc |= ESC_CSI;
		return 0;

	// Handle O sequence directly for cursor moves
	case 'O':
		return 0; // Keep processing to handle next char directly

	case 'A': // Cursor Up
		if (state.esc == ESC_START)
		{ // Direct O-sequence
			tmoveto(state.c.x, state.c.y - 1);
			return 1;
		}
		break;

	case 'B': // Cursor Down
		if (state.esc == ESC_START)
		{
			tmoveto(state.c.x, state.c.y + 1);
			return 1;
		}
		break;
	case 'z':					  // DECID -- Identify Terminal
		processInput("\033[?6c"); // Respond as VT102
		break;
	case ']':
	case 'P':
	case '_':
	case '^':
	case 'k':
		tstrsequence(ascii);
		return 0;
	case 'n':
		state.charset = 2;
		break;
	case 'o':
		state.charset = 3;
		break;
	case '(':
	case ')':
	case '*':
	case '+':
		state.icharset = ascii - '(';
		state.esc |= ESC_ALTCHARSET;
		return 0;
	case 'D': // IND
		if (state.c.y == state.bot)
			scrollUp(state.top, 1);
		else
			state.c.y++;
		break;
	case 'E': // NEL
		tnewline(1);
		break;
	case 'H': // HTS
		state.tabs[state.c.x] = true;
		break;
	case 'M': // RI
		if (state.c.y == state.top)
			scrollDown(state.top, 1);
		else
			state.c.y--;
		break;
	case 'Z': // DECID
		processInput("\033[?6c");
		break;
	case 'c': // RIS
		reset();
		break;
	case '=': // DECKPAM
		setMode(true, MODE_APPCURSOR);
		break;
	case '>': // DECKPNM
		setMode(false, MODE_APPCURSOR);
		break;
	case '7': // DECSC
		cursorSave();
		break;
	case '8': // DECRC
		cursorLoad();
		break;
	case '\\':
		break;
	default:
		fprintf(stderr, "esc unhandled: ESC %c\n", ascii);
		break;
	}
	return 1;
}

void Terminal::tnewline(int first_col)
{
	int y = state.c.y;

	if (y == state.bot)
	{
		scrollUp(state.top, 1);
	} else
	{
		y++;
	}
	moveTo(first_col ? 0 : state.c.x, y);
}
void Terminal::tstrsequence(unsigned char c)
{
	// Handle different string sequences more comprehensively
	switch (c)
	{
	case 0x90: // DCS - Device Control String
	case 0x9d: // OSC - Operating System Command
	case 0x9e: // PM - Privacy Message
	case 0x9f: // APC - Application Program Command
		// TODO: Implement full handling of these sequences
		// This may involve buffering and processing multi-character sequences
		state.esc |= ESC_STR;
		break;
	}
}

void Terminal::tmoveto(int x, int y)
{
	moveTo(x, y); // Use the existing moveTo method
}
void Terminal::parseCSIParam(CSIEscape &csi)
{
	char *p = csi.buf;
	csi.args.clear();

	// Check for private mode
	if (*p == '?')
	{
		csi.priv = 1;
		p++;
	} else
	{
		csi.priv = 0;
	}

	// Parse arguments
	while (p < csi.buf + csi.len)
	{
		// Parse numeric parameter
		int param = 0;
		while (p < csi.buf + csi.len && BETWEEN(*p, '0', '9'))
		{
			param = param * 10 + (*p - '0');
			p++;
		}
		csi.args.push_back(param);

		// Move to next parameter or end
		if (*p == ';')
		{
			p++;
		} else
		{
			break;
		}
	}
}
void Terminal::handleCSI(const CSIEscape &csi)
{
	/*
	std::cout << "CSI sequence: " << csi.mode[0] << " args: ";
	for (auto arg : csi.args) {
		std::cout << arg << " ";
	}
	std::cout << std::endl;
	*/

	switch (csi.mode[0])
	{
	case '@': // ICH -- Insert <n> blank char
	{
		int n = csi.args.empty() ? 1 : csi.args[0];
		if (n < 1)
			n = 1;

		for (int i = state.col - 1; i >= state.c.x + n; i--)
			state.lines[state.c.y][i] = state.lines[state.c.y][i - n];

		clearRegion(state.c.x, state.c.y, state.c.x + n - 1, state.c.y);
	}
	break;

	case 'A': // CUU -- Cursor <n> Up
	{
		int n = csi.args.empty() ? 1 : csi.args[0];

		if (n < 1)
			n = 1;
		moveTo(state.c.x, state.c.y - n);
	}
	break;

	case 'B': // CUD -- Cursor <n> Down
	{
		int n = csi.args.empty() ? 1 : csi.args[0];
		if (n < 1)
			n = 1;
		moveTo(state.c.x, state.c.y + n);
	}
	break;
	case 'e': // VPR -- Cursor <n> Down
	{
		int n = csi.args.empty() ? 1 : csi.args[0];
		if (n < 1)
			n = 1;
		moveTo(state.c.x, state.c.y + n);
	}
	break;

	case 'c': // DA -- Device Attributes
		if (csi.args.empty() || csi.args[0] == 0)
		{
			// Respond with xterm-like capabilities including 2004 (bracketed
			// paste)
			processInput("\033[?2004;1;6c"); // Indicate xterm with bracketed
											 // paste support
		}
		break;

	case 'C': // CUF -- Cursor <n> Forward
	case 'a': // HPR -- Cursor <n> Forward
	{
		int n = csi.args.empty() ? 1 : csi.args[0];
		if (n < 1)
			n = 1;
		moveTo(state.c.x + n, state.c.y);
	}
	break;

	case 'D': // CUB -- Cursor <n> Backward
	{
		int n = csi.args.empty() ? 1 : csi.args[0];
		if (n < 1)
			n = 1;
		moveTo(state.c.x - n, state.c.y);
	}
	break;

	case 'E': // CNL -- Cursor <n> Down and first col
	{
		int n = csi.args.empty() ? 1 : csi.args[0];
		if (n < 1)
			n = 1;
		moveTo(0, state.c.y + n);
	}
	break;

	case 'F': // CPL -- Cursor <n> Up and first col
	{
		int n = csi.args.empty() ? 1 : csi.args[0];
		if (n < 1)
			n = 1;
		moveTo(0, state.c.y - n);
	}
	break;

	case 'g': // TBC -- Tabulation Clear
		switch (csi.args.empty() ? 0 : csi.args[0])
		{
		case 0: // Clear current tab stop
			state.tabs[state.c.x] = false;
			break;
		case 3: // Clear all tab stops
			std::fill(state.tabs.begin(), state.tabs.end(), false);
			break;
		}
		break;

	case 'G': // CHA -- Cursor Character Absolute
	case '`': // HPA -- Horizontal Position Absolute
		if (!csi.args.empty())
		{
			moveTo(csi.args[0] - 1, state.c.y);
		}
		break;

	case 'H': // CUP -- Move to <row> <col>
	case 'f': // HVP
	{
		int row = csi.args.size() > 0 ? csi.args[0] : 1;
		int col = csi.args.size() > 1 ? csi.args[1] : 1;
		tmoveato(col - 1, row - 1);
	}
	break;

	case 'I': // CHT -- Cursor Forward Tabulation <n>
	{
		int n = csi.args.empty() ? 1 : csi.args[0];
		if (n < 1)
			n = 1;
		tputtab(n);
	}
	break;

	case 'J': // ED -- Erase in Display
		switch (csi.args.empty() ? 0 : csi.args[0])
		{
		case 0: // Below
			clearRegion(state.c.x, state.c.y, state.col - 1, state.c.y);
			if (state.c.y < state.row - 1)
				clearRegion(0, state.c.y + 1, state.col - 1, state.row - 1);
			break;
		case 1: // Above
			clearRegion(0, 0, state.col - 1, state.c.y - 1);
			clearRegion(0, state.c.y, state.c.x, state.c.y);
			break;
		case 2: // All
			clearRegion(0, 0, state.col - 1, state.row - 1);
			break;
		}
		break;

	case 'K': // EL -- Erase in Line
		switch (csi.args.empty() ? 0 : csi.args[0])
		{
		case 0: // Right
			clearRegion(state.c.x, state.c.y, state.col - 1, state.c.y);
			break;
		case 1: // Left
			clearRegion(0, state.c.y, state.c.x, state.c.y);
			break;
		case 2: // All
			clearRegion(0, state.c.y, state.col - 1, state.c.y);
			break;
		}
		break;

	case 'L': // IL -- Insert <n> blank lines
	{
		int n = csi.args.empty() ? 1 : csi.args[0];
		if (n < 1)
			n = 1;
		if (BETWEEN(state.c.y, state.top, state.bot))
			scrollDown(state.c.y, n);
	}
	break;

	case 'M': // DL -- Delete <n> lines
	{
		int n = csi.args.empty() ? 1 : csi.args[0];
		if (n < 1)
			n = 1;
		if (BETWEEN(state.c.y, state.top, state.bot))
			scrollUp(state.c.y, n);
	}
	break;

	case 'P': // DCH -- Delete <n> char
	{
		int n = csi.args.empty() ? 1 : csi.args[0];
		if (n < 1)
			n = 1;

		for (int i = state.c.x; i + n < state.col && i < state.col; i++)
			state.lines[state.c.y][i] = state.lines[state.c.y][i + n];

		clearRegion(state.col - n, state.c.y, state.col - 1, state.c.y);
	}
	break;

	case 'S': // SU -- Scroll <n> lines up
	{
		int n = csi.args.empty() ? 1 : csi.args[0];
		if (n < 1)
			n = 1;
		scrollUp(state.top, n);
	}
	break;

	case 'T': // SD -- Scroll <n> lines down
	{
		int n = csi.args.empty() ? 1 : csi.args[0];
		if (n < 1)
			n = 1;
		scrollDown(state.top, n);
	}
	break;

	case 'X': // ECH -- Erase <n> char
	{
		int n = csi.args.empty() ? 1 : csi.args[0];
		if (n < 1)
			n = 1;
		clearRegion(state.c.x, state.c.y, state.c.x + n - 1, state.c.y);
	}
	break;
	case 'Z': // CBT -- Cursor Backward Tabulation <n>
	{
		int n = csi.args.empty() ? 1 : csi.args[0];
		if (n < 1)
			n = 1;
		tputtab(-n);
	}
	break;

	case 'd': // VPA -- Move to <row>
	{
		int n = csi.args.empty() ? 1 : csi.args[0];
		tmoveato(state.c.x, n - 1);
	}
	break;

	case 'h': // SM -- Set Mode
		tsetmode(csi.priv, 1, csi.args);
		break;

	case 'l': // RM -- Reset Mode
		tsetmode(csi.priv, 0, csi.args);
		break;

	case 'm': // SGR -- Select Graphic Rendition
		handleSGR(csi.args);
		break;

	case 'n': // DSR -- Device Status Report
		switch (csi.args.empty() ? 0 : csi.args[0])
		{
		case 5: // Operating status
			processInput("\033[0n");
			break;
		case 6: // Cursor position
		{
			char buf[40];
			snprintf(buf, sizeof(buf), "\033[%i;%iR", state.c.y + 1, state.c.x + 1);
			processInput(buf);
		}
		break;
		}
		break;

	case 'r': // DECSTBM -- Set Scrolling Region
		if (csi.args.size() >= 2)
		{
			int top = csi.args[0] - 1;
			int bot = csi.args[1] - 1;

			if (BETWEEN(top, 0, state.row - 1) && BETWEEN(bot, 0, state.row - 1) &&
				top < bot)
			{
				state.top = top;
				state.bot = bot;
				if (state.c.state & CURSOR_ORIGIN)
					moveTo(0, state.top);
			}
		} else
		{
			// Reset to full screen when no args
			state.top = 0;
			state.bot = state.row - 1;
			if (state.c.state & CURSOR_ORIGIN)
				moveTo(0, state.top);
		}
		break;

	case 's': // DECSC -- Save cursor position
		cursorSave();
		break;

	case 'u': // DECRC -- Restore cursor position
		cursorLoad();
		break;
	}
}

void Terminal::setMode(bool set, int mode)
{
	if (mode == MODE_APPCURSOR)
	{
		std::cout << "APPCURSOR mode " << (set ? "enabled" : "disabled") << std::endl;
	}
	if (set)
		state.mode |= mode;
	else
		state.mode &= ~mode;
	switch (mode)
	{
	case 6: // DECOM -- Origin Mode
		MODBIT(state.c.state, set, CURSOR_ORIGIN);
		if (set)
			moveTo(0, state.top);
		break;

	case MODE_WRAP:
		// Line wrapping mode
		break;
	case MODE_INSERT:
		// Toggle insert mode
		break;
	case MODE_ALTSCREEN:
		if (set)
		{
			state.altLines.swap(state.lines);
			state.mode |= MODE_ALTSCREEN;
			scrollOffset = 0; // Reset scroll on entering alt screen
		} else
		{
			state.altLines.swap(state.lines);
			state.mode &= ~MODE_ALTSCREEN;
			scrollOffset = 0; // Reset scroll on exiting alt screen
		}
		std::fill(state.dirty.begin(), state.dirty.end(), true);
		break;
	case MODE_CRLF:
		// Change line feed behavior
		break;
	case MODE_ECHO:
		// Local echo mode
		break;
	}
}
void Terminal::handleSGR(const std::vector<int> &args)
{
	size_t i;
	int32_t idx;

	if (args.empty())
	{
		// Reset all attributes if no parameters
		state.c.attrs = 0;
		state.c.fg = defaultColorMap[7]; // Default foreground
		state.c.bg = defaultColorMap[0]; // Default background
		state.c.colorMode = COLOR_BASIC;
		return;
	}

	for (i = 0; i < args.size(); i++)
	{
		int attr = args[i];
		switch (attr)
		{
		case 0: // Reset
			state.c.attrs = 0;
			state.c.fg = defaultColorMap[7]; // Default foreground
			state.c.bg = defaultColorMap[0]; // Default background
			state.c.colorMode = COLOR_BASIC;
			break;
		case 1:
			state.c.attrs |= ATTR_BOLD;
			break;
		case 2:
			state.c.attrs |= ATTR_FAINT;
			break;
		case 3:
			state.c.attrs |= ATTR_ITALIC;
			break;
		case 4:
			state.c.attrs |= ATTR_UNDERLINE;
			break;
		case 5:
			state.c.attrs |= ATTR_BLINK;
			break;
		case 7:
			state.c.attrs |= ATTR_REVERSE;
			break;
		case 8:
			state.c.attrs |= ATTR_INVISIBLE;
			break;
		case 9:
			state.c.attrs |= ATTR_STRUCK;
			break;

		case 22:
			state.c.attrs &= ~(ATTR_BOLD | ATTR_FAINT);
			break;
		case 23:
			state.c.attrs &= ~ATTR_ITALIC;
			break;
		case 24:
			state.c.attrs &= ~ATTR_UNDERLINE;
			break;
		case 25:
			state.c.attrs &= ~ATTR_BLINK;
			break;
		case 27:
			state.c.attrs &= ~ATTR_REVERSE;
			break;
		case 28:
			state.c.attrs &= ~ATTR_INVISIBLE;
			break;
		case 29:
			state.c.attrs &= ~ATTR_STRUCK;
			break;

		// Foreground color
		case 30:
		case 31:
		case 32:
		case 33:
		case 34:
		case 35:
		case 36:
		case 37:
			state.c.fg = defaultColorMap[attr - 30];
			break;
		case 38:
			if (i + 2 < args.size())
			{
				if (args[i + 1] == 5)
				{ // 256 colors
					i += 2;
					state.c.colorMode = COLOR_256;
					if (args[i] < 16)
					{
						state.c.fg = defaultColorMap[args[i]];
					} else
					{
						// Convert 256 color to RGB
						uint8_t r = 0, g = 0, b = 0;
						if (args[i] < 232)
						{ // 216 colors: 16-231
							uint8_t index = args[i] - 16;
							r = (index / 36) * 51;
							g = ((index / 6) % 6) * 51;
							b = (index % 6) * 51;
						} else
						{ // Grayscale: 232-255
							r = g = b = (args[i] - 232) * 11;
						}
						state.c.fg = ImVec4(r / 255.0f, g / 255.0f, b / 255.0f, 1.0f);
					}
				} else if (args[i + 1] == 2 && i + 4 < args.size())
				{ // RGB
					i += 4;
					state.c.colorMode = COLOR_TRUE;
					uint8_t r = args[i - 2];
					uint8_t g = args[i - 1];
					uint8_t b = args[i];
					state.c.fg = ImVec4(r / 255.0f, g / 255.0f, b / 255.0f, 1.0f);
					state.c.trueColorFg = (r << 16) | (g << 8) | b;
				}
			}
			break;
		case 39: // Default foreground
			state.c.fg = defaultColorMap[7];
			state.c.colorMode = COLOR_BASIC;
			break;

		// Background color
		case 40:
		case 41:
		case 42:
		case 43:
		case 44:
		case 45:
		case 46:
		case 47:
			state.c.bg = defaultColorMap[attr - 40];
			break;
		case 48:
			if (i + 2 < args.size())
			{
				if (args[i + 1] == 5)
				{ // 256 colors
					i += 2;
					if (args[i] < 16)
					{
						state.c.bg = defaultColorMap[args[i]];
					} else
					{
						// Convert 256 color to RGB
						uint8_t r = 0, g = 0, b = 0;
						if (args[i] < 232)
						{ // 216 colors: 16-231
							uint8_t index = args[i] - 16;
							r = (index / 36) * 51;
							g = ((index / 6) % 6) * 51;
							b = (index % 6) * 51;
						} else
						{ // Grayscale: 232-255
							r = g = b = (args[i] - 232) * 11;
						}
						state.c.bg = ImVec4(r / 255.0f, g / 255.0f, b / 255.0f, 1.0f);
					}
				} else if (args[i + 1] == 2 && i + 4 < args.size())
				{ // RGB
					i += 4;
					uint8_t r = args[i - 2];
					uint8_t g = args[i - 1];
					uint8_t b = args[i];
					state.c.bg = ImVec4(r / 255.0f, g / 255.0f, b / 255.0f, 1.0f);
					state.c.trueColorBg = (r << 16) | (g << 8) | b;
				}
			}
			break;
		case 49: // Default background
			state.c.bg = defaultColorMap[0];
			break;

		// Bright foreground colors
		case 90:
		case 91:
		case 92:
		case 93:
		case 94:
		case 95:
		case 96:
		case 97:
			state.c.fg = defaultColorMap[(attr - 90) + 8];
			break;

		// Bright background colors
		case 100:
		case 101:
		case 102:
		case 103:
		case 104:
		case 105:
		case 106:
		case 107:
			state.c.bg = defaultColorMap[(attr - 100) + 8];
			break;
		}
	}
}

void Terminal::writeGlyph(const Glyph &g, int x, int y)
{
	if (x >= state.col || y >= state.row || x < 0 || y < 0)
		return;

	Glyph &cell = state.lines[y][x];
	cell = g;

	// Ensure we properly handle attribute clearing
	if (!(g.mode & (ATTR_REVERSE | ATTR_BOLD | ATTR_ITALIC | ATTR_BLINK | ATTR_UNDERLINE)))
	{
		cell.mode &=
			~(ATTR_REVERSE | ATTR_BOLD | ATTR_ITALIC | ATTR_BLINK | ATTR_UNDERLINE);
	}

	cell.mode =
		(cell.mode &
		 ~(ATTR_REVERSE | ATTR_BOLD | ATTR_ITALIC | ATTR_BLINK | ATTR_UNDERLINE)) |
		(g.mode & (ATTR_REVERSE | ATTR_BOLD | ATTR_ITALIC | ATTR_BLINK | ATTR_UNDERLINE));

	cell.colorMode = g.colorMode;

	if (cell.mode & ATTR_REVERSE)
	{
		cell.fg = state.c.bg;
		cell.bg = state.c.fg;
		cell.trueColorFg = state.c.trueColorBg;
		cell.trueColorBg = state.c.trueColorFg;
	} else
	{
		cell.fg = state.c.fg;
		cell.bg = state.c.bg;
		cell.trueColorFg = state.c.trueColorFg;
		cell.trueColorBg = state.c.trueColorBg;
	}

	// Handle bold attribute affecting colors
	if (cell.mode & ATTR_BOLD && cell.colorMode == COLOR_BASIC)
	{
		// Make the color brighter for bold text
		if (cell.trueColorFg < 0x8)
		{													 // If using standard colors
			cell.fg = defaultColorMap[cell.trueColorFg + 8]; // Use bright version
		}
	}
	if (cell.mode & ATTR_WIDE && x + 1 < state.col)
	{
		Glyph &nextCell = state.lines[y][x + 1];
		nextCell.u = ' ';
		nextCell.mode = ATTR_WDUMMY;
		// Copy color/attrs from base cell
		nextCell.fg = cell.fg;
		nextCell.bg = cell.bg;
	}

	state.dirty[y] = true;
}

void Terminal::handleEscape(char c)
{
	switch (c)
	{
	case '7': // Save cursor position
		cursorSave();
		break;
	case '8': // Restore cursor position
		cursorLoad();
		break;
	case 'D': // IND - Index (move down and scroll if at bottom)
		if (state.c.y == state.bot)
		{
			scrollUp(state.top, 1);
		} else
		{
			state.c.y++;
		}
		break;
	case 'E': // NEL - Next Line
		state.c.x = 0;
		if (state.c.y == state.bot)
		{
			scrollUp(state.top, 1);
		} else
		{
			state.c.y++;
		}
		break;
	case 'H': // HTS - Horizontal Tab Set
		state.tabs[state.c.x] = true;
		break;
	case 'M': // RI - Reverse Index (move up and scroll if at top)
		if (state.c.y == state.top)
		{
			scrollDown(state.top, 1);
		} else
		{
			state.c.y--;
		}
		break;

	case '(': // G0 Character Set
	case ')': // G1 Character Set
	case '*': // G2 Character Set
	case '+': // G3 Character Set
		handleCharset(c);
		break;

	case 'n': // LS2 -- Locking shift 2
	case 'o': // LS3 -- Locking shift 3
		state.charset = 2 + (c - 'n');
		break;

	case 'A': // UK
		state.trantbl[state.charset] = CS_UK;
		break;
	case 'B': // US
		state.trantbl[state.charset] = CS_USA;
		break;
	case '0': // Special Graphics (Line Drawing)
		state.trantbl[state.charset] = CS_GRAPHIC0;
		break;
	case 'Z': // Device Control String (DCS)
		// Respond with terminal identification
		processInput("\033[?1;2c");
		break;

	case '~': // Keypad state
		// You can implement keypad mode handling here
		break;

	case 'c': // Full Reset
		reset();
		break;
	}
}

size_t Terminal::utf8Encode(Rune u, char *c)
{
	size_t len = 0;

	if (u < 0x80)
	{
		c[0] = u;
		len = 1;
	} else if (u < 0x800)
	{
		c[0] = 0xC0 | (u >> 6);
		c[1] = 0x80 | (u & 0x3F);
		len = 2;
	} else if (u < 0x10000)
	{
		c[0] = 0xE0 | (u >> 12);
		c[1] = 0x80 | ((u >> 6) & 0x3F);
		c[2] = 0x80 | (u & 0x3F);
		len = 3;
	} else
	{
		c[0] = 0xF0 | (u >> 18);
		c[1] = 0x80 | ((u >> 12) & 0x3F);
		c[2] = 0x80 | ((u >> 6) & 0x3F);
		c[3] = 0x80 | (u & 0x3F);
		len = 4;
	}

	return len;
}

void Terminal::reset()
{
	// Reset terminal to initial state
	state.mode = MODE_WRAP | MODE_UTF8;
	state.c = {}; // Reset cursor
	state.charset = 0;

	// Reset character set translation table
	for (int i = 0; i < 4; i++)
	{
		state.trantbl[i] = CS_USA;
	}

	// Clear screen and reset color/attributes
	clearRegion(0, 0, state.col - 1, state.row - 1);
	state.c.fg = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
	state.c.bg = ImVec4(0.0f, 0.0f, 0.0f, 1.0f);
}

void Terminal::clearRegion(int x1, int y1, int x2, int y2)
{
	int temp;
	if (x1 > x2)
	{
		temp = x1;
		x1 = x2;
		x2 = temp;
	}
	if (y1 > y2)
	{
		temp = y1;
		y1 = y2;
		y2 = temp;
	}

	// Constrain to terminal size
	x1 = std::max(0, std::min(x1, state.col - 1));
	x2 = std::max(0, std::min(x2, state.col - 1));
	y1 = std::max(0, std::min(y1, state.row - 1));
	y2 = std::max(0, std::min(y2, state.row - 1));

	// Clear the cells and properly reset attributes
	for (int y = y1; y <= y2; y++)
	{
		for (int x = x1; x <= x2; x++)
		{
			Glyph &g = state.lines[y][x];
			g.u = ' ';
			g.mode = state.c.attrs & ~(ATTR_REVERSE | ATTR_BOLD | ATTR_ITALIC |
									   ATTR_BLINK | ATTR_UNDERLINE);
			g.fg = state.c.fg;
			g.bg = state.c.bg;
			g.colorMode = state.c.colorMode;
			g.trueColorFg = state.c.trueColorFg;
			g.trueColorBg = state.c.trueColorBg;
		}
		state.dirty[y] = true;
	}
}

void Terminal::moveTo(int x, int y)
{
	int miny, maxy;

	// Get scroll region bounds
	if (state.c.state & CURSOR_ORIGIN)
	{
		miny = state.top;
		maxy = state.bot;
	} else
	{
		miny = 0;
		maxy = state.row - 1;
	}

	int oldx = state.c.x;
	int oldy = state.c.y;

	// Constrain cursor position
	state.c.x = LIMIT(x, 0, state.col - 1);
	state.c.y = LIMIT(y, miny, maxy);

	// Reset wrap flag if moved
	if (oldx != state.c.x || oldy != state.c.y)
		state.c.state &= ~CURSOR_WRAPNEXT;

	// Handle wrap-next state when moving to end of line
	if (state.c.x == state.col - 1)
	{
		state.c.state |= CURSOR_WRAPNEXT;
	}
}

void Terminal::scrollUp(int orig, int n)
{
	if (orig < 0 || orig >= state.row)
		return;
	if (n <= 0)
		return;

	n = std::min(n, state.bot - orig + 1);
	n = std::max(n, 0);

	// Add scrolled lines to scrollback when not in alt screen
	if (!(state.mode & MODE_ALTSCREEN))
	{
		for (int i = orig; i < orig + n; ++i)
		{
			if (i < state.lines.size())
			{
				addToScrollback(state.lines[i]);
			}
		}
	}

	// Existing scroll handling...
	for (int y = orig; y <= state.bot - n; ++y)
	{
		state.lines[y] = std::move(state.lines[y + n]);
	}

	// Clear revealed lines
	for (int i = state.bot - n + 1; i <= state.bot && i < state.row; i++)
	{
		state.lines[i].resize(state.col);
		for (int j = 0; j < state.col; j++)
		{
			Glyph &g = state.lines[i][j];
			g.u = ' ';
			g.mode = state.c.attrs;
			g.fg = state.c.fg;
			g.bg = state.c.bg;
		}
	}
}

void Terminal::scrollDown(int orig, int n)
{
	if (orig < 0 || orig >= state.row)
		return; // Safety check
	if (n <= 0)
		return;

	n = std::min(n, state.bot - orig + 1);
	n = std::max(n, 0);

	if (orig + n > state.row)
		return;

	// Mark lines as dirty
	for (int i = orig; i <= state.bot && i < state.row; i++)
	{
		state.dirty[i] = true;
	}

	// Move lines down with bounds checking
	for (int i = state.bot; i >= orig + n && i < state.row; i--)
	{
		state.lines[i] = std::move(state.lines[i - n]);
	}

	// Clear new lines
	for (int i = orig; i < orig + n && i < state.row; i++)
	{
		state.lines[i].resize(state.col);
		for (int j = 0; j < state.col; j++)
		{
			Glyph &g = state.lines[i][j];
			g.u = ' ';
			g.mode = state.c.attrs;
			g.fg = state.c.fg;
			g.bg = state.c.bg;
		}
	}
}

size_t Terminal::utf8Decode(const char *c, Rune *u, size_t clen)
{
	*u = UTF_INVALID;
	size_t len = 0;
	Rune udecoded = 0;

	// Determine sequence length and initial byte decoding
	if ((c[0] & 0x80) == 0)
	{
		// ASCII character
		*u = c[0];
		return 1;
	} else if ((c[0] & 0xE0) == 0xC0)
	{
		// 2-byte sequence
		len = 2;
		udecoded = c[0] & 0x1F;
	} else if ((c[0] & 0xF0) == 0xE0)
	{
		// 3-byte sequence (used by box drawing characters)
		len = 3;
		udecoded = c[0] & 0x0F;
	} else if ((c[0] & 0xF8) == 0xF0)
	{
		// 4-byte sequence
		len = 4;
		udecoded = c[0] & 0x07;
	} else
	{
		std::cerr << "Invalid UTF-8 start byte: 0x" << std::hex << static_cast<int>(c[0])
				  << std::dec << std::endl;
		return 0;
	}

	// Validate sequence length
	if (clen < len)
	{
		std::cerr << "Incomplete UTF-8 sequence. Expected " << len << " bytes, got "
				  << clen << std::endl;
		return 0;
	}

	// Process continuation bytes
	for (size_t i = 1; i < len; i++)
	{
		// Validate continuation byte
		if ((c[i] & 0xC0) != 0x80)
		{
			std::cerr << "Invalid continuation byte at position " << i << ": 0x"
					  << std::hex << static_cast<int>(c[i]) << std::dec << std::endl;
			return 0;
		}

		// Shift and add continuation byte
		udecoded = (udecoded << 6) | (c[i] & 0x3F);
	}

	// Additional validation for decoded Unicode point
	if (!BETWEEN(udecoded, utfmin[len], utfmax[len]) ||
		BETWEEN(udecoded, 0xD800, 0xDFFF) || udecoded > 0x10FFFF)
	{
		std::cerr << "Invalid Unicode code point: U+" << std::hex << udecoded << std::dec
				  << std::endl;
		*u = UTF_INVALID;
		return 0;
	}

	*u = udecoded;
	return len;
}

void Terminal::handleCharset(char c)
{
	static const struct
	{
		char code;
		Charset charset;
		const char *description;
	} charsetMap[] = {{'(', CS_USA, "US ASCII"},
					  {')', CS_UK, "UK"},
					  {'*', CS_MULTI, "Multilingual"},
					  {'+', CS_GER, "German"},
					  {'0', CS_GRAPHIC0, "Special Graphics"},
					  {'A', CS_GER, "German"},
					  {'B', CS_USA, "US ASCII"}};

	for (const auto &entry : charsetMap)
	{
		if (entry.code == c)
		{
			state.trantbl[state.charset] = entry.charset;
			break;
		}
	}
}

Terminal::Rune Terminal::utf8decodebyte(char c, size_t *i)
{
	for (*i = 0; *i < 4; ++(*i))
		if ((unsigned char)c >= utfmask[*i] && (unsigned char)c < utfmask[*i + 1])
			return (unsigned char)c & ~utfmask[*i];

	return 0;
}

size_t Terminal::utf8validate(Rune *u, size_t i)
{
	// Reject surrogate halves and out-of-range code points
	if (!BETWEEN(*u, utfmin[i], utfmax[i]) || BETWEEN(*u, 0xD800, 0xDFFF) || *u > 0x10FFFF)
	{
		*u = UTF_INVALID;
	}

	// Determine the correct UTF-8 sequence length
	for (i = 1; *u > utfmax[i]; ++i)
		;

	return i;
}

void Terminal::handleDeviceStatusReport(const CSIEscape &csi)
{
	switch (csi.args[0])
	{
	case 5:						 // Operation Status Report
		processInput("\033[0n"); // OK
		break;
	case 6: // Cursor Position Report
	{
		char report[32];
		snprintf(report, sizeof(report), "\033[%d;%dR", state.c.y + 1, state.c.x + 1);
		processInput(report);
	}
	break;
		// Add more report types
	}
}

void Terminal::ringBell()
{
	// Implement visual bell
	if (state.mode & MODE_VISUALBELL)
	{
		// Briefly invert screen colors
		for (auto &line : state.lines)
		{
			for (auto &glyph : line)
			{
				std::swap(glyph.fg, glyph.bg);
			}
		}
		// Schedule screen restoration
		std::thread([this]() {
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
			for (auto &line : state.lines)
			{
				for (auto &glyph : line)
				{
					std::swap(glyph.fg, glyph.bg);
				}
			}
		}).detach();
	} else
	{
		// System bell or audio bell
		// Implement platform-specific bell
	}
}

void Terminal::resize(int cols, int rows)
{
	std::lock_guard<std::mutex> lock(bufferMutex);
	selectionClear();
	// Get actual content area size
	ImVec2 contentSize = ImGui::GetContentRegionAvail();
	float charWidth = ImGui::GetFontBaked()->GetCharAdvance('M');
	float lineHeight = ImGui::GetTextLineHeight();

	// Calculate new dimensions based on actual font metrics
	int new_cols = std::max(1, static_cast<int>(contentSize.x / charWidth));
	int new_rows = std::max(1, static_cast<int>(contentSize.y / lineHeight));

	// Only resize if dimensions actually changed
	if (new_cols == state.col && new_rows == state.row)
		return;

	try
	{
		// Create new buffers
		std::vector<std::vector<Glyph>> newLines(rows);
		std::vector<std::vector<Glyph>> newAltLines(rows);
		std::vector<bool> newDirty(rows, true);
		std::vector<bool> newTabs(cols, false);

		// Initialize the new lines
		for (int i = 0; i < rows; i++)
		{
			newLines[i].resize(cols);
			newAltLines[i].resize(cols);
			// Initialize with default glyphs if needed
			for (int j = 0; j < cols; j++)
			{
				newLines[i][j].u = ' ';
				newLines[i][j].mode = state.c.attrs;
				newLines[i][j].fg = state.c.fg;
				newLines[i][j].bg = state.c.bg;
			}
		}

		// Copy existing content
		int minRows = std::min(rows, state.row);
		int minCols = std::min(cols, state.col);

		for (int y = 0; y < minRows; y++)
		{
			for (int x = 0; x < minCols; x++)
			{
				if (y < state.lines.size() && x < state.lines[y].size())
				{
					newLines[y][x] = state.lines[y][x];
				}
			}
		}

		// Set new tab stops
		for (int i = 8; i < cols; i += 8)
		{
			newTabs[i] = true;
		}

		// Update terminal state
		state.row = rows;
		state.col = cols;
		state.top = 0;
		state.bot = rows - 1;

		// Then clamp to ensure validity
		state.top = std::clamp(state.top, 0, rows - 1);
		state.bot = std::clamp(state.bot, state.top, rows - 1);
		if (state.bot < state.top)
			state.bot = state.top;

		// Swap in new buffers
		state.lines = std::move(newLines);
		state.altLines = std::move(newAltLines);
		state.dirty = std::move(newDirty);
		state.tabs = std::move(newTabs);

		// Ensure cursor stays within bounds
		state.c.x = std::min(state.c.x, cols - 1);
		state.c.y = std::min(state.c.y, rows - 1);

		// Update PTY size if valid
#ifndef PLATFORM_WINDOWS
		struct winsize ws = {};
		ws.ws_row = state.row;
		ws.ws_col = state.col;
		ioctl(ptyFd, TIOCSWINSZ, &ws); // Set master side size
#endif

	} catch (const std::exception &e)
	{
		std::cerr << "Error during resize: " << e.what() << std::endl;
	}
	std::cout << "Resized to " << cols << "x" << rows << std::endl;
}

void Terminal::enableBracketedPaste()
{
	// Send bracketed paste mode start sequence
	processInput("\033[?2004h");
	state.mode |= MODE_BRACKETPASTE;
}

void Terminal::disableBracketedPaste()
{
	// Send bracketed paste mode end sequence
	processInput("\033[?2004l");
	state.mode &= ~MODE_BRACKETPASTE;
}

void Terminal::handlePastedContent(const std::string &content)
{
	if (state.mode & MODE_BRACKETPASTE)
	{
		processInput("\033[200~"); // Start of paste
		processInput(content);
		processInput("\033[201~"); // End of paste
	} else
	{
		processInput(content);
	}
}
void Terminal::cursorSave() { savedCursor = state.c; }

void Terminal::cursorLoad()
{
	state.c = savedCursor;
	moveTo(state.c.x, state.c.y);
}

void Terminal::handleControlCode(unsigned char c)
{
	switch (c)
	{
	case '\t': // HT - Horizontal Tab
		tputtab(1);
		break;
	case '\b': // BS - Backspace
		if (state.c.x > 0)
		{
			state.c.x--;
			state.c.state &= ~CURSOR_WRAPNEXT;
		}
		break;
	case '\r': // CR - Carriage Return
		state.c.x = 0;
		state.c.state &= ~CURSOR_WRAPNEXT;
		break;
	case '\f': // FF - Form Feed
	case '\v': // VT - Vertical Tab
	case '\n': // LF - Line Feed
		if (state.c.y == state.bot)
			scrollUp(state.top, 1);
		else
			state.c.y++;
		if (state.mode & MODE_CRLF)
			state.c.x = 0;
		state.c.state &= ~CURSOR_WRAPNEXT;
		break;
	case '\a': // BEL - Bell
		ringBell();
		break;
	case 033: // ESC - Escape
		state.esc = ESC_START;
		break;
	}
}

#ifndef PLATFORM_WINDOWS
void Terminal::processInput(const std::string &input)
{
	if (ptyFd < 0)
		return;
	if (state.mode & MODE_BRACKETPASTE)
	{
		if (input.substr(0, 4) == "\033[200~")
		{
			write(ptyFd, input.c_str(), input.length());
			return;
		}
		if (input.substr(0, 4) == "\033[201~")
		{
			write(ptyFd, input.c_str(), input.length());
			return;
		}
	}
	if (state.mode & MODE_APPCURSOR)
	{
		if (input == "\033[A")
		{
			write(ptyFd, "\033OA", 3); // Up
			return;
		}
		if (input == "\033[B")
		{
			write(ptyFd, "\033OB", 3); // Down
			return;
		}
		if (input == "\033[C")
		{
			write(ptyFd, "\033OC", 3); // Right
			return;
		}
		if (input == "\033[D")
		{
			write(ptyFd, "\033OD", 3); // Left
			return;
		}
	}

	if (input == "\r\n" || input == "\n")
	{
		write(ptyFd, "\r", 1);
		return;
	}

	if (input == "\b")
	{
		write(ptyFd, "\b \b", 3);
		return;
	}

	write(ptyFd, input.c_str(), input.length());
}
#else
void Terminal::processInput(const std::string &input)
{
	// Terminal not supported on Windows
}
#endif

#ifndef PLATFORM_WINDOWS
void Terminal::readOutput()
{
	char buffer[4096];
	while (!shouldTerminate)
	{
		// Use select with timeout to make thread responsive to shouldTerminate
		fd_set readfds;
		struct timeval timeout;
		FD_ZERO(&readfds);
		FD_SET(ptyFd, &readfds);
		timeout.tv_sec = 0;
		timeout.tv_usec = 100000; // 100ms timeout

		int result = select(ptyFd + 1, &readfds, NULL, NULL, &timeout);
		if (result < 0)
			break;
		if (result == 0)
			continue; // timeout, check shouldTerminate

		ssize_t bytesRead = read(ptyFd, buffer, sizeof(buffer) - 1);
		if (bytesRead > 0)
		{
			std::lock_guard<std::mutex> lock(bufferMutex);
			writeToBuffer(buffer, bytesRead);
		} else if (bytesRead < 0 && errno != EINTR)
		{
			break;
		}
	}
}
#else
void Terminal::readOutput()
{
	// Terminal not supported on Windows
}
#endif
void Terminal::tputtab(int n)
{
	unsigned int x = state.c.x;

	if (n > 0)
	{
		while (x < state.col && n--)
		{
			// Find next tab stop
			do
			{
				x++;
			} while (x < state.col && !state.tabs[x]);
		}
	} else if (n < 0)
	{
		while (x > 0 && n++)
		{
			// Find previous tab stop
			do
			{
				x--;
			} while (x > 0 && !state.tabs[x]);
		}
	}

	state.c.x = LIMIT(x, 0, state.col - 1);
}

void Terminal::processStringSequence(const std::string &seq)
{
	// Handle different types of string sequences
	if (seq.empty())
		return;

	switch (seq[0])
	{
	case ']': // OSC - Operating System Command
		handleOSCSequence(seq);
		break;
	case 'P': // DCS - Device Control String
		handleDCSSequence(seq);
		break;
		// Add handling for other sequence types
	}
}

void Terminal::handleOSCSequence(const std::string &seq)
{
	// Example: Handle title setting
	if (seq.substr(0, 2) == "]0;" || seq.substr(0, 2) == "]2;")
	{
		// Extract title
		size_t titleEnd = seq.find('\007');
		if (titleEnd != std::string::npos)
		{
			std::string title = seq.substr(2, titleEnd - 2);
			// TODO: Set window title
			std::cout << "Window Title: " << title << std::endl;
		}
	}
}

void Terminal::handleDCSSequence(const std::string &seq)
{
	// Placeholder for Device Control String handling
	// This can include things like terminal reports, device-specific commands
}

void Terminal::selectionInit()
{
	sel.mode = SEL_IDLE;
	sel.snap = 0;
	sel.ob.x = -1;
}

void Terminal::selectionStart(int col, int row)
{
	selectionClear();
	sel.mode = SEL_EMPTY;
	sel.type = SEL_REGULAR;
	sel.alt = state.mode & MODE_ALTSCREEN;
	sel.snap = 0;
	sel.oe.x = sel.ob.x = col;
	sel.oe.y = sel.ob.y = row;
	selectionNormalize();

	if (sel.snap != 0)
		sel.mode = SEL_READY;
}

void Terminal::selectionExtend(int col, int row)
{
	if (sel.mode == SEL_IDLE)
		return;
	if (sel.mode == SEL_EMPTY)
	{
		sel.mode = SEL_SELECTING;
	}

	sel.oe.x = col;
	sel.oe.y = row;
	selectionNormalize();
}

void Terminal::selectionNormalize()
{
	// Existing normalization logic
	if (sel.type == SEL_REGULAR && sel.ob.y != sel.oe.y)
	{
		sel.nb.x = sel.ob.y < sel.oe.y ? sel.ob.x : sel.oe.x;
		sel.ne.x = sel.ob.y < sel.oe.y ? sel.oe.x : sel.ob.x;
	} else
	{
		sel.nb.x = std::min(sel.ob.x, sel.oe.x);
		sel.ne.x = std::max(sel.ob.x, sel.oe.x);
	}
	sel.nb.y = std::min(sel.ob.y, sel.oe.y);
	sel.ne.y = std::max(sel.ob.y, sel.oe.y);

	// Clamp X coordinates to terminal dimensions
	sel.nb.x = std::clamp(sel.nb.x, 0, state.col - 1);
	sel.ne.x = std::clamp(sel.ne.x, 0, state.col - 1);

	// Don't clamp Y coordinates to allow scrollback selection
	// Y coordinates can be negative for scrollback lines
}

void Terminal::selectionClear()
{
	if (sel.ob.x == -1)
		return;
	sel.mode = SEL_IDLE;
	sel.ob.x = -1;
}
std::string Terminal::getSelection()
{
	std::string str;
	if (sel.ob.x == -1)
		return str;

	// Convert selection coordinates to absolute buffer positions
	int selStartY = scrollbackBuffer.size() + sel.nb.y;
	int selEndY = scrollbackBuffer.size() + sel.ne.y;

	for (int absY = selStartY; absY <= selEndY; absY++)
	{
		const std::vector<Glyph> *line = nullptr;

		// Determine which buffer this line is in
		if (absY < scrollbackBuffer.size())
		{
			// Line is in scrollback buffer
			line = &scrollbackBuffer[absY];
		} else
		{
			// Line is in current screen buffer
			int screenY = absY - scrollbackBuffer.size();
			if (screenY >= 0 && screenY < state.lines.size())
			{
				line = &state.lines[screenY];
			}
		}

		if (!line)
			continue;

		int xstart = (absY == selStartY) ? sel.nb.x : 0;
		int xend = (absY == selEndY) ? sel.ne.x : state.col - 1;

		// Clamp xstart and xend to line size
		xstart = std::clamp(xstart, 0, static_cast<int>(line->size()) - 1);
		xend = std::clamp(xend, 0, static_cast<int>(line->size()) - 1);

		for (int x = xstart; x <= xend; x++)
		{
			if ((*line)[x].mode & ATTR_WDUMMY)
				continue;

			char buf[UTF_SIZ];
			size_t len = utf8Encode((*line)[x].u, buf);
			str.append(buf, len);
		}

		if (absY < selEndY)
			str += '\n';
	}
	return str;
}

void Terminal::copySelection()
{
	std::cout << "coppying selection dawggggg" << "\n";
	std::string selected = getSelection();
	if (!selected.empty())
	{
		// Use ImGui's clipboard functions
		ImGui::SetClipboardText(selected.c_str());
	}
}

void Terminal::pasteFromClipboard()
{
	const char *text = ImGui::GetClipboardText();
	std::cout << "state.mode: " << state.mode << "\n";
	std::cout << "MODE_BRACKETPASTE: " << MODE_BRACKETPASTE << "\n";
	std::cout << "Check result: " << (state.mode & MODE_BRACKETPASTE) << "\n";

#ifndef PLATFORM_WINDOWS
	if (state.mode & MODE_BRACKETPASTE)
	{
		std::cout << "Bracketed paste mode active\n";
		// Send paste start sequence
		write(ptyFd, "\033[200~", 6);
		// Send the actual text
		write(ptyFd, text, strlen(text));
		// Send paste end sequence
		write(ptyFd, "\033[201~", 6);
	} else
	{
		std::cout << "Normal paste mode\n";
		write(ptyFd, text, strlen(text));
	}
#endif
}
bool Terminal::selectedText(int x, int y)
{
	if (sel.mode == SEL_IDLE || sel.ob.x == -1 || sel.alt != (state.mode & MODE_ALTSCREEN))
		return false;

	// Convert coordinates to absolute buffer positions
	int actualY = scrollbackBuffer.size() + y;
	int selStartY = scrollbackBuffer.size() + sel.nb.y;
	int selEndY = scrollbackBuffer.size() + sel.ne.y;

	// Ensure start is less than or equal to end
	if (selStartY > selEndY)
	{
		std::swap(selStartY, selEndY);
	}

	if (sel.type == SEL_RECTANGULAR)
		return BETWEEN(actualY, selStartY, selEndY) && BETWEEN(x, sel.nb.x, sel.ne.x);

	return BETWEEN(actualY, selStartY, selEndY) &&
		   (actualY != selStartY || x >= sel.nb.x) &&
		   (actualY != selEndY || x <= sel.ne.x);
}

void Terminal::strparse()
{
	// Parse string sequences into arguments
	strescseq.args.clear();
	std::string current;

	for (size_t i = 0; i < strescseq.len; i++)
	{
		char c = strescseq.buf[i];
		if (c == ';')
		{
			strescseq.args.push_back(current);
			current.clear();
		} else
		{
			current += c;
		}
	}
	if (!current.empty())
	{
		strescseq.args.push_back(current);
	}
}

void Terminal::handleStringSequence()
{
	if (strescseq.len == 0)
		return;

	switch (strescseq.type)
	{
	case ']': // OSC - Operating System Command
		if (strescseq.args.size() >= 2)
		{
			int cmd = std::atoi(strescseq.args[0].c_str());
			switch (cmd)
			{
			case 0: // Set window title and icon name
			case 1: // Set icon name
			case 2: // Set window title
				// You would implement window title setting here
				// For now, we'll just print it
				std::cout << "Title: " << strescseq.args[1] << std::endl;
				break;

			case 4: // Set/get color
				handleOSCColor(strescseq.args);
				break;

			case 52: // Manipulate selection data
				handleOSCSelection(strescseq.args);
				break;
			}
		}
		break;

	case 'P': // DCS - Device Control String
		handleDCS();
		break;

	case '_': // APC - Application Program Command
		// Not commonly used, implement if needed
		break;

	case '^': // PM - Privacy Message
		// Not commonly used, implement if needed
		break;

	case 'k': // Old title set compatibility
		// Set window title using old xterm sequence
		std::cout << "Old Title: " << strescseq.buf << std::endl;
		break;
	}
}

void Terminal::handleOSCColor(const std::vector<std::string> &args)
{
	if (args.size() < 2)
		return;

	int index = std::atoi(args[1].c_str());
	if (args.size() > 2)
	{
		// Set color
		if (args[2][0] == '?')
		{
			// Color query - respond with current color
			char response[64];
			snprintf(response,
					 sizeof(response),
					 "\033]4;%d;rgb:%.2X/%.2X/%.2X\007",
					 index,
					 (int)(state.c.fg.x * 255),
					 (int)(state.c.fg.y * 255),
					 (int)(state.c.fg.z * 255));
			processInput(response);
		} else
		{
			// Set color - parse color value (typically in rgb:RR/GG/BB format)
			// Implementation would go here
		}
	}
}

void Terminal::handleOSCSelection(const std::vector<std::string> &args)
{
	if (args.size() < 3)
		return;

	// args[1] would contain the selection type (clipboard, primary, etc)
	// args[2] would contain the base64-encoded data

	// Example implementation:
	if (args[1] == "c")
	{						 // clipboard
		std::string decoded; // You would implement base64 decoding
		ImGui::SetClipboardText(decoded.c_str());
	}
}

// Test sequence handler - DECALN alignment pattern
void Terminal::handleTestSequence(char c)
{
	switch (c)
	{
	case '8': // DECALN - Screen Alignment Pattern
		// Fill screen with 'E'
		for (int y = 0; y < state.row; y++)
		{
			for (int x = 0; x < state.col; x++)
			{
				Glyph g;
				g.u = 'E';
				g.mode = state.c.attrs;
				g.fg = state.c.fg;
				g.bg = state.c.bg;
				writeGlyph(g, x, y);
			}
		}
		break;
	}
}
// Device Control String handler
void Terminal::handleDCS()
{
	// Basic DCS sequence handling
	// This function is called when DCS sequences are received
	// For now, we'll only implement some basic DCS handling

	// Extract DCS sequence from strescseq
	if (strescseq.buf.empty())
		return;

	// Example DCS sequence handling:
	// $q - DECRQSS (Request Status String)
	if (strescseq.buf.length() >= 2 && strescseq.buf.substr(0, 2) == "$q")
	{
		std::string param = strescseq.buf.substr(2);
		// Handle DECRQSS request
		if (param == "\"q")
		{										// DECSCA
			processInput("\033P1$r0\"q\033\\"); // Reply with default protection
		} else if (param == "r")
		{ // DECSTBM
			char response[40];
			snprintf(response,
					 sizeof(response),
					 "\033P1$r%d;%dr\033\\",
					 state.top + 1,
					 state.bot + 1);
			processInput(response);
		}
	}
}

void Terminal::tmoveato(int x, int y)
{
	// Origin mode moves relative to scroll region
	if (state.c.state & CURSOR_ORIGIN)
		moveTo(x, y + state.top);
	else
		moveTo(x, y);
}

void Terminal::tsetmode(int priv, int set, const std::vector<int> &args)
{
	// Mode setting per st.c
	int alt;

	for (int arg : args)
	{
		if (priv)
		{
			switch (arg)
			{
			case 1: // DECCKM -- Application cursor keys
				setMode(set, MODE_APPCURSOR);
				break;
			case 5: // DECSCNM -- Reverse video
				// TODO: Implement screen reversal
				break;
			case 6: // DECOM -- Origin
				MODBIT(state.c.state, set, CURSOR_ORIGIN);
				tmoveato(0, 0);
				break;
			case 7: // DECAWM -- Auto wrap
				if (set)
				{
					state.mode |= MODE_WRAP;
				} else
				{
					state.mode &= ~MODE_WRAP;
				}
				break;
			case 0: // Error (IGNORED)
			case 2: // DECANM -- ANSI/VT52 (IGNORED)
			case 3: // DECCOLM -- Column  (IGNORED)
			case 4: // DECSCLM -- Scroll (IGNORED)
			case 8: // DECARM -- Auto repeat (IGNORED)
				break;
			case 25: // DECTCEM -- Text Cursor Enable Mode
				// Optional: handle cursor visibility
				break;
			case 47:   // swap screen
			case 1047: // alternate screen
			case 1049:
				alt = (state.mode & MODE_ALTSCREEN) != 0;
				if (set ^ alt)
				{
					state.altLines.swap(state.lines);
					state.mode ^= MODE_ALTSCREEN;
				}
			case 1048:
				(set) ? cursorSave() : cursorLoad();
				break;
			case 2004: // Bracketed paste mode
				if (set)
				{
					state.mode |= MODE_BRACKETPASTE;
				} else
				{
					state.mode &= ~MODE_BRACKETPASTE;
				}
				break;
			}
		} else
		{
			switch (arg)
			{
			case 4: // IRM -- Insertion-replacement
				MODBIT(state.mode, set, MODE_INSERT);
				break;
			case 20: // LNM -- Linefeed/new line
				MODBIT(state.mode, set, MODE_CRLF);
				break;
			}
		}
	}
}

void Terminal::addToScrollback(const std::vector<Glyph> &line)
{
	scrollbackBuffer.push_back(line);
	if (scrollbackBuffer.size() > maxScrollbackLines)
	{
		scrollbackBuffer.erase(scrollbackBuffer.begin());
	}
}

void Terminal::resetFontSizeDetection()
{
	lastFontSize = 0; // Force terminal to detect the change on next render
}
