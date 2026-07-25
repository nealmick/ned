// Tree-sitter parse + query engine used only by EditorHighlight.
#pragma once
#include "../../buffer/text_buffer.h"
#include "../../editor_operations.h"
#include "imgui.h"
#include <cstdint>
#include <mutex>
#include <string>
#include <string_view>
#include <tree_sitter/api.h>
#include <unordered_map>
#include <vector>

class Settings;

struct ThemeColors
{
	ImVec4 keyword;
	ImVec4 string;
	ImVec4 number;
	ImVec4 comment;
	ImVec4 text;
	ImVec4 function;
	ImVec4 type;
	ImVec4 variable;
};

// Half-open [start, end) byte range on a single line.
struct ColorSpan
{
	int start = 0;
	int end = 0;
	ImVec4 color{};
};

using LineColorSpans = std::vector<ColorSpan>;
using ColorRangeMap = std::vector<LineColorSpans>;

// Result of a parse/recolor pass.
struct ParseResult
{
	bool ok = false;
	// true: fullColors has one entry per line (replace entire map).
	// false: dirtyRows/dirtySpans are parallel; splice into existing map.
	bool full = true;
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
		std::vector<PendingTreeEdit> pendingEdits;
	};

	TreeSitter();
	~TreeSitter();

	void bind(Settings &appSettings) { settings = &appSettings; }

	// Parse snapshot → colors (full or dirty-line partial). Stale gens return !ok.
	// Thread-safe vs other parse() calls; never touches live EditorState.
	ParseResult parse(ParseSnapshot snapshot, uint64_t gen);

	void updateThemeColors();
	ThemeColors cachedColors;

  private:
	// If more than this fraction of lines is dirty, rebuild the full color map.
	static constexpr float kPartialDirtyFraction = 0.45f;

	Settings *settings = nullptr;

	TSParser *parser = nullptr;
	TSTree *tree = nullptr;
	std::string treeFilePath;
	size_t treeDocBytes = 0;
	uint64_t lastCommittedGen = 0;
	std::mutex parserMutex;
	std::unordered_map<std::string, TSQuery *> queryCache;
	std::string inputScratch;

	std::pair<TSLanguage *, std::string>
	detectLanguageAndQuery(const std::string &languageId);
	TSQuery *loadQueryFromCacheOrFile(TSLanguage *lang, const std::string &query_path);

	ColorRangeMap
	buildColorRangesFull(TSQuery *query, TSTree *tree, const ParseSnapshot &snap);

	// Query only dirty line runs; out rows/spans parallel.
	void buildColorRangesPartial(TSQuery *query,
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
				  ColorRangeMap &colors);

	bool applyPendingEdits(const std::vector<PendingTreeEdit> &edits);

	static int lineLength(const ParseSnapshot &snap, size_t row);
	static void
	rowColFromOffset(const ParseSnapshot &snap, size_t offset, int &row, int &column);
	static void setRange(LineColorSpans &spans, int start, int end, const ImVec4 &color);
	static void mergeAdjacent(LineColorSpans &spans);
	static TSPoint advancePoint(TSPoint point, std::string_view s);
	static void markDirtyLines(const ParseSnapshot &snap,
							   uint32_t byteStart,
							   uint32_t byteEnd,
							   std::vector<uint8_t> &dirty);
};
