// Tree-sitter parse + query engine used only by EditorHighlight.
#pragma once
#include "../../buffer/text_buffer.h"
#include "../../editor_operations.h"
#include "capture_map.h"
#include "imgui.h"
#include <cstdint>
#include <functional>
#include <mutex>
#include <string>
#include <string_view>
#include <tree_sitter/api.h>
#include <vector>

class Settings;

// Syntax palette. Index is ThemeSlot; JSON keys are kThemeKeys.
struct ThemeColors
{
	ImVec4 slots[static_cast<size_t>(ThemeSlot::Count)]{};

	const ImVec4 &operator[](ThemeSlot slot) const
	{
		const auto i = static_cast<uint8_t>(slot);
		return slots[i < static_cast<uint8_t>(ThemeSlot::Count) ? i : 0];
	}
	ImVec4 &operator[](ThemeSlot slot)
	{
		const auto i = static_cast<uint8_t>(slot);
		return slots[i < static_cast<uint8_t>(ThemeSlot::Count) ? i : 0];
	}
};

// Half-open [start, end) byte range on a single line.
struct ColorSpan
{
	int start = 0;
	int end = 0;
	ThemeSlot slot = ThemeSlot::Text;
};

using LineColorSpans = std::vector<ColorSpan>;
using ColorRangeMap = std::vector<LineColorSpans>;

enum class ParseKind : uint8_t {
	Failed,
	Full,	  // replace the span map with fullColors
	Partial,  // splice dirtyRows / dirtySpans
	TreeOnly, // tree committed; caller must query windows
};

struct ParseResult
{
	ParseKind kind = ParseKind::Failed;
	size_t lineCount = 0;
	ColorRangeMap fullColors;
	std::vector<int> dirtyRows;
	std::vector<LineColorSpans> dirtySpans;
};

class TreeSitter
{
  public:
	// Immutable input for a parse.
	// Text is a CoW snapshot (cheap to take on the UI thread).
	// lineStarts may be empty when built — parse() fills them (on the worker
	// for large files so the UI thread only pays for morph + CoW snap).
	struct ParseSnapshot
	{
		TextBuffer::Snapshot text;
		std::string lineEnding;
		// Byte offset of each line start. Size == line count once ensured.
		std::vector<uint32_t> lineStarts;
		std::string path;
		std::string languageId;
		std::vector<PendingEdit> pendingEdits;
		// If non-zero, treat the document as this many bytes (prefix parse).
		uint32_t byteLimit = 0;
	};

	TreeSitter();
	~TreeSitter();

	void bind(Settings &appSettings) { settings = &appSettings; }

	// Parse then emit color windows under one lock, so a later parse cannot
	// swap `tree` mid-query. Incremental dirties emit one Partial; a full
	// rebuild emits Partial windows of `chunkLines`.
	void colorDocument(ParseSnapshot &snapshot,
					   uint64_t gen,
					   int chunkLines,
					   const std::function<bool()> &canceled,
					   const std::function<void(ParseResult &&)> &emit);

	// First maxLines only — throwaway parser, no full rope walk, tree untouched.
	ParseResult queryPrefix(const ParseSnapshot &snapshot, int maxLines);

	// One-shot highlight of a small snippet (hover code fences). `languageId`
	// is a file extension or fence tag (cpp, python, rust, …).
	static ColorRangeMap highlightSnippet(const std::string &languageId,
										  const std::string &text);

	void updateThemeColors();
	// True if this language's .scm is already compiled (safe to query on UI).
	bool queryReady(const std::string &languageId) const;
	// Compile every shipped query on a background thread. Once per process.
	static void startBackgroundPrewarm();
	ThemeColors cachedColors;

  private:
	// If more than this fraction of lines is dirty, rebuild the full color map.
	static constexpr float kPartialDirtyFraction = 0.45f;

	Settings *settings = nullptr;

	TSParser *parser = nullptr;
	TSTree *tree = nullptr;
	std::string treeFilePath;
	std::string treeLanguageId;
	size_t treeDocBytes = 0;
	uint64_t lastCommittedGen = 0;
	std::mutex parserMutex;
	std::string inputScratch;

	static std::pair<TSLanguage *, std::string>
	detectLanguageAndQuery(const std::string &languageId);
	static TSQuery *loadQueryFromCacheOrFile(TSLanguage *lang,
											 const std::string &query_path);

	ParseResult parse(ParseSnapshot &snapshot, uint64_t gen);
	ParseResult queryWindow(const ParseSnapshot &snapshot, int lineLo, int lineHi);
	void queryWindowInto(TSQuery *query,
						 TSTree *tree,
						 const ParseSnapshot &snap,
						 int lineLo,
						 int lineHi,
						 std::vector<int> &outRows,
						 std::vector<LineColorSpans> &outSpans);

	void emitDirtyWindows(TSQuery *query,
						  TSTree *tree,
						  const ParseSnapshot &snap,
						  const std::vector<uint8_t> &dirtyLine,
						  std::vector<int> &outRows,
						  std::vector<LineColorSpans> &outSpans);

	void runQuery(TSQuery *query,
				  TSTree *tree,
				  const ParseSnapshot &snap,
				  uint32_t byteStart,
				  uint32_t byteEnd,
				  ColorRangeMap &colors,
				  int rowBase);

	bool applyPendingEdits(const std::vector<PendingEdit> &edits);

	static int lineLength(const ParseSnapshot &snap, size_t row);
	static void
	rowColFromOffset(const ParseSnapshot &snap, size_t offset, int &row, int &column);
	static void setRange(LineColorSpans &spans, int start, int end, ThemeSlot slot);
	static void mergeAdjacent(LineColorSpans &spans);
	static TSPoint advancePoint(TSPoint point, std::string_view s);
	static void markDirtyLines(const ParseSnapshot &snap,
							   uint32_t byteStart,
							   uint32_t byteEnd,
							   std::vector<uint8_t> &dirty);
};
