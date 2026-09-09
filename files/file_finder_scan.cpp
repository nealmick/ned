/*
	files/file_finder_scan.cpp
	Toolkit-neutral workspace scan for the Ctrl+P file finder — the ONE
	directory walk shared by the ImGui finder (FileFinder's background
	thread) and the Qt finder view. Lives in ned_core (no UI deps).
*/
#include "file_finder.h"
#include "file_finder_match.h"

#include <algorithm>
#include <cctype>
#include <exception>
#include <system_error>

namespace fs = std::filesystem;

namespace {

std::string toLower(std::string s)
{
	std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) {
		return static_cast<char>(std::tolower(c));
	});
	return s;
}

// Path → UTF-8 std::string (Windows-safe).
std::string pathToUtf8(const fs::path &p)
{
#ifdef PLATFORM_WINDOWS
	auto u8 = p.u8string();
	return std::string(u8.begin(), u8.end());
#else
	return p.string();
#endif
}

} // namespace

std::vector<FileEntry> scanWorkspaceFiles(const std::string &projectDir,
										  const std::atomic<bool> &stop,
										  size_t maxFiles)
{
	std::vector<FileEntry> files;
	if (maxFiles == 0)
		return files;
	try
	{
		// Skip list (FileFinderMatch::shouldSkipDir): .git / build dirs /
		// node_modules otherwise dominate every scan.
		std::error_code ec;
		for (fs::recursive_directory_iterator
				 it(projectDir, fs::directory_options::skip_permission_denied, ec),
			 end;
			 !ec && it != end && !stop.load() && files.size() < maxFiles;
			 it.increment(ec))
		{
			try
			{
				if (it->is_directory(ec))
				{
					if (FileFinderMatch::shouldSkipDir(it->path().filename().string()))
						it.disable_recursion_pending();
					continue;
				}
				if (ec || !it->is_regular_file(ec))
					continue;

				const fs::path fullPath = it->path();
				const fs::path relativePath = fs::relative(fullPath, projectDir);

				FileEntry fe;
				fe.fullPath = pathToUtf8(fullPath);
				fe.relativePath = pathToUtf8(relativePath);
				fe.relativePathLower = toLower(fe.relativePath);
				fe.filenameLower = toLower(pathToUtf8(relativePath.filename()));
				files.push_back(std::move(fe));
			} catch (const std::exception &)
			{
				// Skip entries that fail path conversion / access.
				continue;
			}
		}
	} catch (const std::exception &)
	{
		// Directory gone / permission — return whatever was collected.
	}
	return files;
}
