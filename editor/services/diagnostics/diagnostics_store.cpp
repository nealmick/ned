#include "diagnostics_store.h"
#include "../../util/doc_path.h"

#include <algorithm>

std::string LSPDiagnostics::keyFor(const std::string &path) const
{
	if (path.empty())
		return {};
	const std::lock_guard<std::mutex> lock(mutex_);
	auto it = keyCache_.find(path);
	if (it != keyCache_.end())
		return it->second;
	std::string key = DocPath::normalize(path);
	keyCache_.emplace(path, key);
	return key;
}

void LSPDiagnostics::replace(const std::string &path,
							 std::vector<DiagnosticItem> items,
							 int version)
{
	const std::string key = keyFor(path);
	if (key.empty())
		return;
	std::lock_guard<std::mutex> lock(mutex_);
	const auto it = versions_.find(key);
	if (version >= 0 && it != versions_.end() && it->second >= 0 && version < it->second)
		return; // stale publish computed against an older document version
	versions_[key] = version;
	byPath_[key] = std::move(items);
}

void LSPDiagnostics::clear(const std::string &path)
{
	const std::string key = keyFor(path);
	if (key.empty())
		return;
	std::lock_guard<std::mutex> lock(mutex_);
	byPath_.erase(key);
	versions_.erase(key);
}

void LSPDiagnostics::clearAll()
{
	std::lock_guard<std::mutex> lock(mutex_);
	byPath_.clear();
	versions_.clear();
	keyCache_.clear();
}

std::vector<DiagnosticItem> LSPDiagnostics::forDocument(const std::string &path) const
{
	const std::string key = keyFor(path);
	std::lock_guard<std::mutex> lock(mutex_);
	auto it = byPath_.find(key);
	if (it == byPath_.end())
		return {};
	return it->second;
}

std::vector<DiagnosticItem> LSPDiagnostics::forLine(const std::string &path,
													int line) const
{
	std::vector<DiagnosticItem> out;
	if (line < 0)
		return out;
	const auto all = forDocument(path);
	for (const auto &d : all)
	{
		if (d.startLine <= line && line <= d.endLine)
			out.push_back(d);
	}
	return out;
}

std::vector<int> LSPDiagnostics::maxSeverityByLine(const std::string &path,
												   int lineCount) const
{
	std::vector<int> out;
	if (lineCount <= 0)
		return out;
	out.assign(static_cast<size_t>(lineCount), 0);

	const std::string key = keyFor(path);
	std::lock_guard<std::mutex> lock(mutex_);
	const auto it = byPath_.find(key);
	if (it == byPath_.end())
		return out;
	for (const auto &d : it->second)
	{
		const int sev = d.severity <= 0 ? 1 : d.severity;
		for (int row = d.startLine; row <= d.endLine && row < lineCount; ++row)
		{
			if (row < 0)
				continue;
			int &slot = out[static_cast<size_t>(row)];
			if (slot == 0 || sev < slot)
				slot = sev;
		}
	}
	return out;
}

bool LSPDiagnostics::contains(const std::string &path, int line, int utf16Column) const
{
	for (const auto &d : forLine(path, line))
	{
		if (DiagnosticContains(d, line, utf16Column))
			return true;
	}
	return false;
}
