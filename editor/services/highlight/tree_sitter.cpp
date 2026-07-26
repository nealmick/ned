#include "tree_sitter.h"
#include "../../../util/settings.h"
#include "../../editor_operations.h"
#include "capture_map.h"

#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <regex>
#include <string_view>
#include <vector>

extern "C" TSLanguage *tree_sitter_cpp();
extern "C" TSLanguage *tree_sitter_javascript();
extern "C" TSLanguage *tree_sitter_python();
extern "C" TSLanguage *tree_sitter_c_sharp();
extern "C" TSLanguage *tree_sitter_html();
extern "C" TSLanguage *tree_sitter_tsx();
extern "C" TSLanguage *tree_sitter_css();
extern "C" TSLanguage *tree_sitter_java();
extern "C" TSLanguage *tree_sitter_go();
extern "C" TSLanguage *tree_sitter_hcl();
extern "C" TSLanguage *tree_sitter_json();
extern "C" TSLanguage *tree_sitter_kotlin();
extern "C" TSLanguage *tree_sitter_bash();
extern "C" TSLanguage *tree_sitter_c();
extern "C" TSLanguage *tree_sitter_rust();
extern "C" TSLanguage *tree_sitter_toml();
extern "C" TSLanguage *tree_sitter_ruby();

namespace {
struct InputState
{
	const TextBuffer::Snapshot *text = nullptr;
	std::string *scratch = nullptr;
};

const char *tsRead(void *payload, uint32_t byte_index, TSPoint, uint32_t *bytes_read)
{
	auto *st = static_cast<InputState *>(payload);
	if (!st || !st->text || !st->scratch || !bytes_read)
	{
		if (bytes_read)
			*bytes_read = 0;
		return "";
	}
	const size_t sz = st->text->size();
	if (byte_index >= sz)
	{
		*bytes_read = 0;
		return "";
	}
	constexpr size_t kChunk = 4096;
	const size_t want = std::min(kChunk, sz - static_cast<size_t>(byte_index));
	st->scratch->resize(want);
	st->text->copyBytes(byte_index, want, st->scratch->data());
	*bytes_read = static_cast<uint32_t>(want);
	return st->scratch->data();
}
} // namespace

TreeSitter::TreeSitter() : parser(ts_parser_new()) {}

TreeSitter::~TreeSitter()
{
	for (auto &[key, query] : queryCache)
		ts_query_delete(query);
	ts_tree_delete(tree);
	ts_parser_delete(parser);
}

// ---------------------------------------------------------------------------
// Span helpers
// ---------------------------------------------------------------------------

void TreeSitter::mergeAdjacent(LineColorSpans &spans)
{
	if (spans.size() < 2)
		return;
	LineColorSpans out;
	out.reserve(spans.size());
	out.push_back(spans[0]);
	for (size_t i = 1; i < spans.size(); ++i)
	{
		ColorSpan &prev = out.back();
		const ColorSpan &cur = spans[i];
		if (prev.end == cur.start && prev.color.x == cur.color.x &&
			prev.color.y == cur.color.y && prev.color.z == cur.color.z &&
			prev.color.w == cur.color.w)
			prev.end = cur.end;
		else
			out.push_back(cur);
	}
	spans = std::move(out);
}

void TreeSitter::setRange(LineColorSpans &spans, int start, int end, const ImVec4 &color)
{
	if (start >= end)
		return;

	LineColorSpans out;
	out.reserve(spans.size() + 2);
	bool inserted = false;

	for (const ColorSpan &s : spans)
	{
		if (s.end <= start)
		{
			out.push_back(s);
			continue;
		}
		if (s.start >= end)
		{
			if (!inserted)
			{
				out.push_back({start, end, color});
				inserted = true;
			}
			out.push_back(s);
			continue;
		}
		if (s.start < start)
			out.push_back({s.start, start, s.color});
		if (!inserted)
		{
			out.push_back({start, end, color});
			inserted = true;
		}
		if (s.end > end)
			out.push_back({end, s.end, s.color});
	}
	if (!inserted)
		out.push_back({start, end, color});

	mergeAdjacent(out);
	spans = std::move(out);
}

int TreeSitter::lineLength(const ParseSnapshot &snap, size_t row)
{
	const size_t n = snap.lineStarts.size();
	if (row >= n)
		return 0;
	const size_t begin = snap.lineStarts[row];
	if (row + 1 < n)
		return static_cast<int>(snap.lineStarts[row + 1] - begin - snap.lineEnding.size());
	return static_cast<int>(snap.text.size() - begin);
}

void TreeSitter::rowColFromOffset(const ParseSnapshot &snap,
								  size_t offset,
								  int &row,
								  int &column)
{
	if (snap.lineStarts.empty())
	{
		row = 0;
		column = 0;
		return;
	}

	const size_t sep = snap.lineEnding.size();
	const size_t n = snap.lineStarts.size();
	const size_t docSize = snap.text.size();
	offset = std::min(offset, docSize);

	size_t lo = 0, hi = n - 1;
	while (lo < hi)
	{
		const size_t mid = lo + (hi - lo + 1) / 2;
		if (snap.lineStarts[mid] <= offset)
			lo = mid;
		else
			hi = mid - 1;
	}
	row = static_cast<int>(lo);
	const size_t lineBegin = snap.lineStarts[lo];
	const size_t lineEnd = (lo + 1 < n) ? snap.lineStarts[lo + 1] - sep : docSize;

	if (offset <= lineEnd)
	{
		column = static_cast<int>(offset - lineBegin);
		return;
	}
	if (lo + 1 < n)
	{
		row = static_cast<int>(lo + 1);
		column = 0;
		return;
	}
	column = lineLength(snap, lo);
}

void TreeSitter::markDirtyLines(const ParseSnapshot &snap,
								uint32_t byteStart,
								uint32_t byteEnd,
								std::vector<uint8_t> &dirty)
{
	if (byteStart >= byteEnd || dirty.empty())
		return;
	int sr = 0, sc = 0, er = 0, ec = 0;
	rowColFromOffset(snap, byteStart, sr, sc);
	// end is exclusive; step back one byte for last included line when possible.
	const size_t endPos =
		byteEnd > 0 ? static_cast<size_t>(byteEnd) - 1 : static_cast<size_t>(byteStart);
	rowColFromOffset(snap, endPos, er, ec);
	if (sr > er)
		std::swap(sr, er);
	sr = std::clamp(sr, 0, static_cast<int>(dirty.size()) - 1);
	er = std::clamp(er, 0, static_cast<int>(dirty.size()) - 1);
	for (int r = sr; r <= er; ++r)
		dirty[static_cast<size_t>(r)] = 1;
}

TSPoint TreeSitter::advancePoint(TSPoint point, std::string_view s)
{
	for (unsigned char c : s)
	{
		if (c == '\n')
		{
			++point.row;
			point.column = 0;
		} else
		{
			++point.column;
		}
	}
	return point;
}

// ---------------------------------------------------------------------------
// Query predicates (#match?, #eq?, …)
//
// The C API returns ALL pattern matches; hosts must evaluate predicates.
// Without this, (#match? @constant "^[A-Z]…") still paints every identifier
// as @constant — which made themes look nothing like Neovim.
// ---------------------------------------------------------------------------

namespace {

std::string nodeText(const TreeSitter::ParseSnapshot &snap, TSNode node)
{
	const uint32_t a = ts_node_start_byte(node);
	const uint32_t b = ts_node_end_byte(node);
	if (b <= a || a >= snap.text.size())
		return {};
	const size_t len = std::min(static_cast<size_t>(b - a), snap.text.size() - a);
	std::string out(len, '\0');
	snap.text.copyBytes(a, len, out.data());
	return out;
}

// Text of the first capture in `match` whose capture index equals `captureIndex`.
std::string captureTextByIndex(const TreeSitter::ParseSnapshot &snap,
							   const TSQueryMatch &match,
							   uint32_t captureIndex)
{
	for (uint16_t i = 0; i < match.capture_count; ++i)
	{
		if (match.captures[i].index == captureIndex)
			return nodeText(snap, match.captures[i].node);
	}
	return {};
}

// Capture index from a Capture-step value_id (same id as capture names).
bool regexFullMatch(const std::string &text, const std::string &pattern)
{
	try
	{
		// Tree-sitter / nvim use Rust-style regex; ECMAScript covers the
		// common ^…$ identifier conventions in grammar highlights.
		// tree-sitter #match? uses "find" semantics (Rust Regex::is_match),
		// not full-string only — so ^[A-Z] matches ClassName, not only "A".
		const std::regex re(pattern, std::regex::ECMAScript);
		return std::regex_search(text, re);
	} catch (const std::regex_error &)
	{
		return false;
	}
}

// Returns false if any predicate fails (match should be discarded).
bool predicatesPass(TSQuery *query,
					const TSQueryMatch &match,
					const TreeSitter::ParseSnapshot &snap)
{
	uint32_t stepCount = 0;
	const TSQueryPredicateStep *steps =
		ts_query_predicates_for_pattern(query, match.pattern_index, &stepCount);
	if (!steps || stepCount == 0)
		return true;

	// Walk steps; each predicate is [String op] [args…] [Done].
	uint32_t i = 0;
	while (i < stepCount)
	{
		if (steps[i].type == TSQueryPredicateStepTypeDone)
		{
			++i;
			continue;
		}
		if (steps[i].type != TSQueryPredicateStepTypeString)
		{
			// Unexpected; skip to next Done.
			while (i < stepCount && steps[i].type != TSQueryPredicateStepTypeDone)
				++i;
			if (i < stepCount)
				++i;
			continue;
		}

		uint32_t opLen = 0;
		const char *opPtr =
			ts_query_string_value_for_id(query, steps[i].value_id, &opLen);
		const std::string_view op(opPtr, opLen);
		++i;

		// Collect args until Done.
		struct Arg
		{
			bool isCapture = false;
			uint32_t id = 0; // capture index or string id
		};
		std::vector<Arg> args;
		while (i < stepCount && steps[i].type != TSQueryPredicateStepTypeDone)
		{
			Arg a;
			if (steps[i].type == TSQueryPredicateStepTypeCapture)
			{
				a.isCapture = true;
				a.id = steps[i].value_id;
			} else if (steps[i].type == TSQueryPredicateStepTypeString)
			{
				a.isCapture = false;
				a.id = steps[i].value_id;
			}
			args.push_back(a);
			++i;
		}
		if (i < stepCount && steps[i].type == TSQueryPredicateStepTypeDone)
			++i;

		auto argText = [&](const Arg &a) -> std::string {
			if (a.isCapture)
				return captureTextByIndex(snap, match, a.id);
			uint32_t n = 0;
			const char *s = ts_query_string_value_for_id(query, a.id, &n);
			return std::string(s, n);
		};

		// Unsupported predicates (nvim #lua-match?, #set!, etc.): ignore so
		// the structural match still applies (best-effort).
		if (op == "match?" || op == "not-match?")
		{
			if (args.size() < 2 || !args[0].isCapture || args[1].isCapture)
				continue;
			const std::string text = argText(args[0]);
			const std::string pat = argText(args[1]);
			const bool matched = regexFullMatch(text, pat);
			if (op == "match?" && !matched)
				return false;
			if (op == "not-match?" && matched)
				return false;
		} else if (op == "eq?" || op == "not-eq?")
		{
			if (args.size() < 2)
				continue;
			const std::string a = argText(args[0]);
			const std::string b = argText(args[1]);
			const bool eq = (a == b);
			if (op == "eq?" && !eq)
				return false;
			if (op == "not-eq?" && eq)
				return false;
		} else if (op == "any-of?")
		{
			if (args.size() < 2 || !args[0].isCapture)
				continue;
			const std::string text = argText(args[0]);
			bool any = false;
			for (size_t k = 1; k < args.size(); ++k)
			{
				if (text == argText(args[k]))
				{
					any = true;
					break;
				}
			}
			if (!any)
				return false;
		}
		// else: #set!, #is?, #lua-match?, … — ignore
	}
	return true;
}

} // namespace

// ---------------------------------------------------------------------------
// Query → ranges
// ---------------------------------------------------------------------------

void TreeSitter::runQuery(TSQuery *query,
						  TSTree *tree,
						  const ParseSnapshot &snap,
						  uint32_t byteStart,
						  uint32_t byteEnd,
						  ColorRangeMap &colors)
{
	auto colorForCapture = [this](std::string_view name) -> const ImVec4 & {
		return cachedColors.colorForKey(themeKeyForCapture(name));
	};

	auto fillRange = [&](uint32_t start, uint32_t end, const ImVec4 &color) {
		if (start >= end)
			return;
		int sr, sc, er, ec;
		rowColFromOffset(snap, start, sr, sc);
		rowColFromOffset(snap, end, er, ec);
		for (int r = sr; r <= er; ++r)
		{
			if (r < 0 || r >= static_cast<int>(colors.size()))
				continue;
			const int lineLen = lineLength(snap, static_cast<size_t>(r));
			const int a = std::clamp(r == sr ? sc : 0, 0, lineLen);
			const int b = std::clamp(r == er ? ec : lineLen, 0, lineLen);
			if (a < b)
				setRange(colors[static_cast<size_t>(r)], a, b, color);
		}
	};

	// Collect captures then apply weak → strong so function/type beat @variable.
	// @none (e.g. f-string interpolation) punches holes in @string spans.
	struct Hit
	{
		uint32_t start = 0;
		uint32_t end = 0;
		int priority = 0;
		std::string_view capture;
	};
	std::vector<Hit> hits;
	std::vector<std::pair<uint32_t, uint32_t>> noneHoles;
	hits.reserve(256);

	TSQueryCursor *cursor = ts_query_cursor_new();
	if (byteEnd > byteStart)
		ts_query_cursor_set_byte_range(cursor, byteStart, byteEnd);
	ts_query_cursor_exec(cursor, query, ts_tree_root_node(tree));

	TSQueryMatch match;
	while (ts_query_cursor_next_match(cursor, &match))
	{
		if (!predicatesPass(query, match, snap))
			continue;

		for (uint32_t i = 0; i < match.capture_count; ++i)
		{
			TSNode node = match.captures[i].node;
			uint32_t name_length = 0;
			const char *name_ptr = ts_query_capture_name_for_id(
				query, match.captures[i].index, &name_length);
			const std::string_view cap{name_ptr, name_length};
			// nvim helper captures like @_parent are not highlight roles.
			if (!cap.empty() && cap.front() == '_')
				continue;

			const uint32_t a = ts_node_start_byte(node);
			const uint32_t b = ts_node_end_byte(node);
			if (isNoneCapture(cap))
			{
				if (a < b)
					noneHoles.push_back({a, b});
				continue;
			}

			const int prio = capturePriority(cap);
			// Skip injection / non-color captures.
			if (prio <= 10)
				continue;

			hits.push_back(Hit{a, b, prio, cap});
		}
	}
	ts_query_cursor_delete(cursor);

	std::sort(noneHoles.begin(), noneHoles.end());
	// Merge overlapping holes for subtractRanges.
	if (!noneHoles.empty())
	{
		std::vector<std::pair<uint32_t, uint32_t>> merged;
		merged.push_back(noneHoles[0]);
		for (size_t i = 1; i < noneHoles.size(); ++i)
		{
			auto &last = merged.back();
			if (noneHoles[i].first <= last.second)
				last.second = std::max(last.second, noneHoles[i].second);
			else
				merged.push_back(noneHoles[i]);
		}
		noneHoles = std::move(merged);
	}

	// Expand string hits: clip out @none ranges (nvim interpolation reset).
	std::vector<Hit> expanded;
	expanded.reserve(hits.size() + noneHoles.size());
	for (const Hit &h : hits)
	{
		if (isStringCapture(h.capture) && !noneHoles.empty())
		{
			for (const auto &[a, b] : subtractRanges(h.start, h.end, noneHoles))
				expanded.push_back(Hit{a, b, h.priority, h.capture});
		} else
			expanded.push_back(h);
	}

	std::stable_sort(expanded.begin(), expanded.end(), [](const Hit &a, const Hit &b) {
		if (a.priority != b.priority)
			return a.priority < b.priority; // low first, high last (wins)
		return a.start < b.start;
	});

	for (const Hit &h : expanded)
		fillRange(h.start, h.end, colorForCapture(h.capture));
}

ColorRangeMap
TreeSitter::buildColorRangesFull(TSQuery *query, TSTree *tree, const ParseSnapshot &snap)
{
	ColorRangeMap colors(snap.lineStarts.size());
	runQuery(query, tree, snap, 0, static_cast<uint32_t>(snap.text.size()), colors);
	return colors;
}

void TreeSitter::buildColorRangesPartial(TSQuery *query,
										 TSTree *tree,
										 const ParseSnapshot &snap,
										 const std::vector<uint8_t> &dirtyLine,
										 std::vector<int> &outRows,
										 std::vector<LineColorSpans> &outSpans)
{
	const int n = static_cast<int>(dirtyLine.size());
	if (n == 0)
		return;

	// Scratch map is document-sized; only dirty runs are cleared and filled.
	ColorRangeMap scratch(static_cast<size_t>(n));

	int i = 0;
	while (i < n)
	{
		if (!dirtyLine[static_cast<size_t>(i)])
		{
			++i;
			continue;
		}
		const int lo = i;
		while (i < n && dirtyLine[static_cast<size_t>(i)])
			++i;
		const int hi = i - 1; // inclusive

		for (int r = lo; r <= hi; ++r)
			scratch[static_cast<size_t>(r)].clear();

		const uint32_t byteStart = snap.lineStarts[static_cast<size_t>(lo)];
		const uint32_t byteEnd = (hi + 1 < n)
									 ? snap.lineStarts[static_cast<size_t>(hi + 1)]
									 : static_cast<uint32_t>(snap.text.size());

		runQuery(query, tree, snap, byteStart, byteEnd, scratch);

		for (int r = lo; r <= hi; ++r)
		{
			outRows.push_back(r);
			outSpans.push_back(std::move(scratch[static_cast<size_t>(r)]));
			scratch[static_cast<size_t>(r)].clear();
		}
	}
}

// ---------------------------------------------------------------------------
// Incremental tree + parse
// ---------------------------------------------------------------------------

bool TreeSitter::applyPendingEdits(const std::vector<PendingTreeEdit> &edits)
{
	if (!tree || edits.empty())
		return false;

	for (const PendingTreeEdit &pe : edits)
	{
		if (pe.oldEndByte < pe.startByte || pe.newEndByte < pe.startByte)
			return false;

		const TSPoint startPoint{static_cast<uint32_t>(std::max(0, pe.op.row)),
								 static_cast<uint32_t>(std::max(0, pe.op.column))};

		TSPoint oldEndPoint = startPoint;
		TSPoint newEndPoint = startPoint;
		if (pe.op.kind == OpKind::Insert)
		{
			newEndPoint = advancePoint(startPoint, pe.op.text);
		} else
		{
			if (!pe.removedBytes.empty())
				oldEndPoint = advancePoint(startPoint, pe.removedBytes);
			else if (pe.oldEndByte > pe.startByte)
				return false;
		}

		TSInputEdit edit{pe.startByte,
						 pe.oldEndByte,
						 pe.newEndByte,
						 startPoint,
						 oldEndPoint,
						 newEndPoint};
		ts_tree_edit(tree, &edit);
	}
	return true;
}

ParseResult TreeSitter::parse(ParseSnapshot snapshot, uint64_t gen)
{
	std::lock_guard<std::mutex> lock(parserMutex);

	// Build line index here (not on the UI thread for large async recolors).
	if (snapshot.lineStarts.empty())
	{
		snapshot.text.lineStarts(snapshot.lineStarts);
		if (snapshot.lineStarts.empty())
			snapshot.lineStarts.push_back(0);
	}

	ParseResult result;
	result.lineCount = snapshot.lineStarts.size();

	if (gen < lastCommittedGen)
		return result;

	auto [lang, query_path] = detectLanguageAndQuery(snapshot.languageId);
	if (!lang)
	{
		// Unknown language: empty spans (paint uses default text color).
		result.ok = true;
		result.full = true;
		result.fullColors.assign(result.lineCount, {});
		return result;
	}

	ts_parser_set_language(parser, lang);

	std::vector<PendingTreeEdit> pendingEdits = std::move(snapshot.pendingEdits);
	const bool hadPending = !pendingEdits.empty();

	if (treeFilePath != snapshot.path)
	{
		ts_tree_delete(tree);
		tree = nullptr;
		treeDocBytes = 0;
		pendingEdits.clear();
	}

	if (tree)
	{
		bool ok = true;
		if (!pendingEdits.empty())
			ok = applyPendingEdits(pendingEdits);
		else if (treeDocBytes != snapshot.text.size())
			ok = false;

		if (!ok)
		{
			ts_tree_delete(tree);
			tree = nullptr;
			treeDocBytes = 0;
		}
	}

	TSTree *oldTree = tree; // may be null; parse consumes as old_tree

	InputState inputState{&snapshot.text, &inputScratch};
	TSInput input{};
	input.payload = &inputState;
	input.read = tsRead;
	input.encoding = TSInputEncodingUTF8;
	input.decode = nullptr;

	TSTree *newTree = ts_parser_parse(parser, oldTree, input);

	if (gen < lastCommittedGen)
	{
		ts_tree_delete(newTree);
		// oldTree still owned as tree if non-null
		return result;
	}

	// Changed ranges require old and new trees both alive.
	std::vector<uint8_t> dirty;
	bool tryPartial = false;
	if (oldTree && newTree && hadPending)
	{
		uint32_t rangeCount = 0;
		TSRange *ranges = ts_tree_get_changed_ranges(oldTree, newTree, &rangeCount);
		dirty.assign(result.lineCount, 0);
		if (ranges && rangeCount > 0)
		{
			for (uint32_t i = 0; i < rangeCount; ++i)
				markDirtyLines(snapshot, ranges[i].start_byte, ranges[i].end_byte, dirty);
		}
		if (ranges)
			std::free(ranges);

		// Also mark lines touched by the edits (structure may not change).
		for (const PendingTreeEdit &pe : pendingEdits)
		{
			const uint32_t a = pe.startByte;
			const uint32_t b = std::max(pe.oldEndByte, pe.newEndByte);
			// Insert: [start, newEnd). Delete: point at start still dirties the line.
			const uint32_t end = (b > a) ? b : (a + 1);
			markDirtyLines(snapshot, a, end, dirty);
		}

		size_t dirtyCount = 0;
		for (uint8_t d : dirty)
			dirtyCount += d ? 1 : 0;

		if (dirtyCount > 0 &&
			dirtyCount <=
				static_cast<size_t>(kPartialDirtyFraction * result.lineCount + 1))
			tryPartial = true;
	}

	// Commit new tree; free the previous one if distinct.
	if (oldTree && oldTree != newTree)
		ts_tree_delete(oldTree);
	tree = newTree;
	treeFilePath = snapshot.path;
	treeDocBytes = snapshot.text.size();
	lastCommittedGen = gen;

	TSQuery *query = loadQueryFromCacheOrFile(lang, query_path);
	if (!query)
	{
		result.ok = true;
		result.full = true;
		result.fullColors.assign(result.lineCount, {});
		return result;
	}

	if (tryPartial)
	{
		buildColorRangesPartial(
			query, tree, snapshot, dirty, result.dirtyRows, result.dirtySpans);
		result.ok = true;
		result.full = false;
		return result;
	}

	result.fullColors = buildColorRangesFull(query, tree, snapshot);
	result.ok = true;
	result.full = true;
	return result;
}

void TreeSitter::updateThemeColors()
{
	const ImVec4 fallbackText(0.85f, 0.85f, 0.85f, 1.0f);
	const ImVec4 fallbackComment(0.5f, 0.5f, 0.5f, 1.0f);
	cachedColors = {};
	cachedColors.text = fallbackText;
	cachedColors.comment = fallbackComment;
	// remaining fields default to {} then we fill from theme; use text as fallback
	auto setAll = [&](const ImVec4 &v) {
		cachedColors.keyword = cachedColors.string = cachedColors.number = v;
		cachedColors.function = cachedColors.type = cachedColors.variable = v;
		cachedColors.parameter = cachedColors.property = cachedColors.constant = v;
		cachedColors.operatorColor = cachedColors.punctuation = cachedColors.special = v;
	};
	setAll(fallbackText);
	cachedColors.comment = fallbackComment;

	if (!settings || !settings->settings.is_object())
		return;

	const std::string themeName =
		settings->settings.value("theme", std::string("default"));
	if (!settings->settings.contains("themes") ||
		!settings->settings["themes"].is_object() ||
		!settings->settings["themes"].contains(themeName))
		return;

	auto &theme = settings->settings["themes"][themeName];
	auto load = [&theme](const char *key, const ImVec4 &fb) -> ImVec4 {
		if (!theme.contains(key) || !theme[key].is_array() || theme[key].size() < 4)
			return fb;
		auto &a = theme[key];
		return ImVec4(
			a[0].get<float>(), a[1].get<float>(), a[2].get<float>(), a[3].get<float>());
	};

	cachedColors.text = load("text", fallbackText);
	cachedColors.comment = load("comment", fallbackComment);
	cachedColors.keyword = load("keyword", cachedColors.text);
	cachedColors.string = load("string", cachedColors.text);
	cachedColors.number = load("number", cachedColors.text);
	cachedColors.function = load("function", cachedColors.text);
	cachedColors.type = load("type", cachedColors.text);
	cachedColors.variable = load("variable", cachedColors.text);
	cachedColors.parameter = load("parameter", cachedColors.variable);
	cachedColors.property = load("property", cachedColors.variable);
	cachedColors.constant = load("constant", cachedColors.number);
	cachedColors.operatorColor = load("operator", cachedColors.text);
	cachedColors.punctuation = load("punctuation", cachedColors.text);
	cachedColors.special = load("special", cachedColors.keyword);
}

// ---------------------------------------------------------------------------
// Language / query loading
// ---------------------------------------------------------------------------

std::pair<TSLanguage *, std::string>
TreeSitter::detectLanguageAndQuery(const std::string &languageId)
{
	std::string id = languageId;
	if (!id.empty() && id[0] == '.')
		id.erase(0, 1);

	// query file base name only; loadQueryFromCacheOrFile resolves the directory.
	auto q = [](const char *name) { return std::string(name); };

	if (id == "c")
		return {tree_sitter_c(), q("c.scm")};
	if (id == "cpp" || id == "h" || id == "hpp" || id == "mm" || id == "cc" || id == "cxx")
		return {tree_sitter_cpp(), q("cpp.scm")};
	if (id == "js" || id == "jsx")
		return {tree_sitter_javascript(), q("jsx.scm")};
	if (id == "py")
		return {tree_sitter_python(), q("python.scm")};
	if (id == "cs")
		return {tree_sitter_c_sharp(), q("csharp.scm")};
	if (id == "html" || id == "cshtml")
		return {tree_sitter_html(), q("html.scm")};
	if (id == "tsx" || id == "ts")
		return {tree_sitter_tsx(), q("tsx.scm")};
	if (id == "css")
		return {tree_sitter_css(), q("css.scm")};
	if (id == "java")
		return {tree_sitter_java(), q("java.scm")};
	if (id == "go")
		return {tree_sitter_go(), q("go.scm")};
	if (id == "tf" || id == "hcl")
		return {tree_sitter_hcl(), q("hcl.scm")};
	if (id == "json")
		return {tree_sitter_json(), q("json.scm")};
	if (id == "sh" || id == "bash")
		return {tree_sitter_bash(), q("sh.scm")};
	if (id == "kt" || id == "kts")
		return {tree_sitter_kotlin(), q("kotlin.scm")};
	if (id == "rs")
		return {tree_sitter_rust(), q("rs.scm")};
	if (id == "toml")
		return {tree_sitter_toml(), q("toml.scm")};
	if (id == "rb")
		return {tree_sitter_ruby(), q("rb.scm")};
	return {};
}

namespace {

// Packaged apps put queries next to resources (Windows portable, macOS Resources,
// or /usr/lib/Ned on Debian). Dev trees keep them under editor/services/highlight.
std::filesystem::path resolveQueryFile(const std::string &queryFile)
{
	namespace fs = std::filesystem;
	const fs::path res = Settings::getAppResourcesPath();
	const fs::path candidates[] = {
		res / "queries" / queryFile,
		res / "editor" / "services" / "highlight" / "queries" / queryFile,
		fs::path("queries") / queryFile,
		fs::path("editor") / "services" / "highlight" / "queries" / queryFile,
#ifdef __linux__
		// Deb: binary in /usr/lib/Ned (queries), assets often in /usr/share/Ned
		fs::path("/usr/lib/Ned/queries") / queryFile,
#endif
	};
	for (const fs::path &p : candidates)
	{
		if (fs::exists(p))
			return p;
	}
	return candidates[0];
}

} // namespace

TSQuery *TreeSitter::loadQueryFromCacheOrFile(TSLanguage *lang,
											  const std::string &query_path)
{
	// query_path is a bare filename (e.g. "cpp.scm") after detectLanguageAndQuery.
	const std::string full_path = resolveQueryFile(query_path).string();

	if (auto it = queryCache.find(full_path); it != queryCache.end())
		return it->second;

	std::ifstream file(full_path);
	if (!file.is_open())
		return nullptr;
	std::string query_src((std::istreambuf_iterator<char>(file)),
						  std::istreambuf_iterator<char>());

	uint32_t error_offset = 0;
	TSQueryError error_type{};
	TSQuery *query = ts_query_new(
		lang, query_src.c_str(), query_src.size(), &error_offset, &error_type);
	if (!query)
	{
		std::cerr << "Query error (" << error_type << ") at offset " << error_offset
				  << "\n";
		return nullptr;
	}
	queryCache[full_path] = query;
	return query;
}
