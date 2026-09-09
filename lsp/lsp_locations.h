#pragma once

#include "lsp/types.h"

#include <string>
#include <vector>

// One jump target from a goto-definition / goto-reference response.
struct LSPLocation
{
	std::string file;
	int line = 0;	   // 0-based
	int character = 0; // 0-based, UTF-16 (LSP)
};

namespace lsp_locations {

// "basename:line+1:character+1" — the picker row label both backends show
// (wire values are 0-based).
inline std::string locationLabel(const LSPLocation &loc)
{
	std::string filename = loc.file;
	const size_t lastSlash = filename.find_last_of("/\\");
	if (lastSlash != std::string::npos)
		filename = filename.substr(lastSlash + 1);
	return filename + ":" + std::to_string(loc.line + 1) + ":" +
		   std::to_string(loc.character + 1);
}

// Content signature — lets a picker no-op while the (async) result set is
// unchanged instead of rebuilding the visible list on every poll tick.
inline std::string locationSignature(const std::string &title,
									 const std::vector<LSPLocation> &options)
{
	std::string sig = title + "/" + std::to_string(options.size());
	for (const LSPLocation &loc : options)
	{
		sig += ";";
		sig += loc.file;
		sig += ":" + std::to_string(loc.line);
		sig += ":" + std::to_string(loc.character);
	}
	return sig;
}

inline LSPLocation fromLocation(const lsp::Location &loc)
{
	LSPLocation entry;
	entry.file = std::string(loc.uri.fsPath());
	entry.line = static_cast<int>(loc.range.start.line);
	entry.character = static_cast<int>(loc.range.start.character);
	return entry;
}

inline LSPLocation fromLink(const lsp::LocationLink &link)
{
	LSPLocation entry;
	entry.file = std::string(link.targetUri.path());
	entry.line = static_cast<int>(link.targetSelectionRange.start.line);
	entry.character = static_cast<int>(link.targetSelectionRange.start.character);
	return entry;
}

inline std::vector<LSPLocation>
fromDefinitionResult(const lsp::TextDocument_DefinitionResult &result)
{
	std::vector<LSPLocation> out;
	if (result.isNull())
		return out;

	const auto &variant = result.value();
	if (std::holds_alternative<lsp::Definition>(variant))
	{
		const lsp::Definition &def = std::get<lsp::Definition>(variant);
		if (std::holds_alternative<lsp::Location>(def))
		{
			out.push_back(fromLocation(std::get<lsp::Location>(def)));
		} else if (std::holds_alternative<lsp::Array<lsp::Location>>(def))
		{
			for (const auto &loc : std::get<lsp::Array<lsp::Location>>(def))
				out.push_back(fromLocation(loc));
		}
	} else if (std::holds_alternative<lsp::Array<lsp::DefinitionLink>>(variant))
	{
		for (const auto &link : std::get<lsp::Array<lsp::DefinitionLink>>(variant))
			out.push_back(fromLink(link));
	}
	return out;
}

inline std::vector<LSPLocation>
fromReferencesResult(const lsp::TextDocument_ReferencesResult &result)
{
	std::vector<LSPLocation> out;
	if (result.isNull())
		return out;
	for (const auto &loc : result.value())
		out.push_back(fromLocation(loc));
	return out;
}

} // namespace lsp_locations
