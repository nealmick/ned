#pragma once

/*
	Document path normalization shared by anything that keys state per open
	file (LSP document sync, diagnostics store). Produces an absolute /
	canonical path so URIs match across open/edit/close even when the same
	file is addressed via different relative spellings.
*/

#include <filesystem>
#include <string>

namespace DocPath {

inline std::string normalize(const std::string &path)
{
	if (path.empty())
		return {};

	namespace fs = std::filesystem;
	std::error_code ec;
	fs::path p = fs::weakly_canonical(path, ec);
	if (ec)
		p = fs::absolute(path, ec);
	if (ec)
		return path;
	return p.string();
}

} // namespace DocPath
