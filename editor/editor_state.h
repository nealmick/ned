/*
	File: editor_state.h
	Description: Document model — text, path, dirty, version, language.
	Not cursor, UI, or services.

	Text lives in a private TextBuffer. Session edits go through
	EditorOperations (friend). Everyone else uses the read API below.
*/
#pragma once

#include "buffer/text_buffer.h"

#include <cstddef>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

class EditorOperations;

// class (not struct) so forward decls as `class EditorState` match MSVC C4099.
class EditorState
{
  public:
	// --- metadata (not rope mutation) ---
	std::string path; // empty = none / untitled
	std::string lineEnding;
	std::string languageId; // "" = plain
	int version = 0;		// ++ on content change (incl. undo apply)
	bool dirty = false;		// true until successful save
	// True if the last setFromString saw a UTF-8 BOM (EF BB BF); stripped from content.
	// EditorSave rewrites the BOM when this is set.
	bool utf8Bom = false;

	EditorState();

	static std::string platformLineEnding();
	static std::pair<std::vector<std::string>, std::string>
	splitLines(const std::string &raw);
	static std::string languageIdFromPath(const std::string &filePath);

	// Load raw bytes into buffer + lineEnding; version=0; dirty=false.
	void setFromString(const std::string &raw);
	// Rebuild rope with a new line ending; preserves logical lines.
	void setLineEnding(const std::string &eol);

	// --- text reads ---
	std::string join() const;
	size_t byteSize() const { return text.size(); }
	int lineCount() const { return text.lineCount(); }
	int lineLength(int row) const { return text.lineLength(row); }
	std::string line(int row) const { return text.line(row); }
	void lineInto(int row, std::string &out) const { text.lineInto(row, out); }
	void linesInto(std::vector<std::string> &out) const;
	std::vector<std::string> lines() const;

	size_t offsetFromRowCol(int row, int column) const;
	void rowColFromOffset(size_t offset, int &row, int &column) const;

	void copyBytes(size_t off, size_t len, char *out) const
	{
		text.copyBytes(off, len, out);
	}
	void lineStarts(std::vector<uint32_t> &out) const { text.lineStarts(out); }
	bool containsByte(char c) const { return text.containsByte(c); }
	TextBuffer::Snapshot snapshot() const { return text.snapshot(); }

	void markEdited();
	void markSaved();

  private:
	friend class EditorOperations;

	TextBuffer text;

	// Session mutation — only EditorOperations.
	void bufferInsert(size_t off, std::string_view bytes) { text.insert(off, bytes); }
	void bufferErase(size_t off, size_t len) { text.erase(off, len); }
};
