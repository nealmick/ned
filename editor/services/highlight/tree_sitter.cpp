#include "tree_sitter.h"
#include "../../../util/settings.h"
#include "../../editor_operations.h"
#include "../../editor_state.h"
#include "capture_map.h"

#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <functional>
#include <future>
#include <iostream>
#include <map>
#include <mutex>
#include <optional>
#include <regex>
#include <string_view>
#include <thread>
#include <unordered_map>
#include <unordered_set>
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
// Compiled once per process — not per editor tab. ts_query_new on jsx/cpp
// is the expensive part of opening a 10-line file.
std::mutex gQueryMu;
std::unordered_map<std::string, TSQuery *> gQueryCache;
std::unordered_map<std::string, std::shared_future<TSQuery *>> gQueryInflight;

struct LangEntry
{
	const char *id;
	TSLanguage *(*lang)();
	const char *scm;
};

const LangEntry kLangs[] = {
	{"c", tree_sitter_c, "c.scm"},
	{"cpp", tree_sitter_cpp, "cpp.scm"},
	{"c++", tree_sitter_cpp, "cpp.scm"},
	{"h", tree_sitter_cpp, "cpp.scm"},
	{"hpp", tree_sitter_cpp, "cpp.scm"},
	{"mm", tree_sitter_cpp, "cpp.scm"},
	{"cc", tree_sitter_cpp, "cpp.scm"},
	{"cxx", tree_sitter_cpp, "cpp.scm"},
	{"js", tree_sitter_javascript, "jsx.scm"},
	{"jsx", tree_sitter_javascript, "jsx.scm"},
	{"javascript", tree_sitter_javascript, "jsx.scm"},
	{"py", tree_sitter_python, "python.scm"},
	{"python", tree_sitter_python, "python.scm"},
	{"cs", tree_sitter_c_sharp, "csharp.scm"},
	{"csharp", tree_sitter_c_sharp, "csharp.scm"},
	{"html", tree_sitter_html, "html.scm"},
	{"cshtml", tree_sitter_html, "html.scm"},
	{"tsx", tree_sitter_tsx, "tsx.scm"},
	{"ts", tree_sitter_tsx, "tsx.scm"},
	{"typescript", tree_sitter_tsx, "tsx.scm"},
	{"css", tree_sitter_css, "css.scm"},
	{"java", tree_sitter_java, "java.scm"},
	{"go", tree_sitter_go, "go.scm"},
	{"golang", tree_sitter_go, "go.scm"},
	{"tf", tree_sitter_hcl, "hcl.scm"},
	{"hcl", tree_sitter_hcl, "hcl.scm"},
	{"json", tree_sitter_json, "json.scm"},
	{"sh", tree_sitter_bash, "sh.scm"},
	{"bash", tree_sitter_bash, "sh.scm"},
	{"kt", tree_sitter_kotlin, "kotlin.scm"},
	{"kts", tree_sitter_kotlin, "kotlin.scm"},
	{"rs", tree_sitter_rust, "rs.scm"},
	{"rust", tree_sitter_rust, "rs.scm"},
	{"toml", tree_sitter_toml, "toml.scm"},
	{"rb", tree_sitter_ruby, "rb.scm"},
};
} // namespace

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
		if (prev.end == cur.start && prev.slot == cur.slot)
			prev.end = cur.end;
		else
			out.push_back(cur);
	}
	spans = std::move(out);
}

void TreeSitter::setRange(LineColorSpans &spans, int start, int end, ThemeSlot slot)
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
				out.push_back({start, end, slot});
				inserted = true;
			}
			out.push_back(s);
			continue;
		}
		if (s.start < start)
			out.push_back({s.start, start, s.slot});
		if (!inserted)
		{
			out.push_back({start, end, slot});
			inserted = true;
		}
		if (s.end > end)
			out.push_back({end, s.end, s.slot});
	}
	if (!inserted)
		out.push_back({start, end, slot});

	mergeAdjacent(out);
	spans = std::move(out);
}

static size_t snapSize(const TreeSitter::ParseSnapshot &snap)
{
	return snap.byteLimit ? snap.byteLimit : snap.text.size();
}

int TreeSitter::lineLength(const ParseSnapshot &snap, size_t row)
{
	const size_t n = snap.lineStarts.size();
	if (row >= n)
		return 0;
	const size_t begin = snap.lineStarts[row];
	if (row + 1 < n)
		return static_cast<int>(snap.lineStarts[row + 1] - begin - snap.lineEnding.size());
	return static_cast<int>(snapSize(snap) - begin);
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
	const size_t docSize = snapSize(snap);
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
// Query files have a small, fixed set of #match? patterns. Compile once.
const std::regex *cachedRegex(const std::string &pattern)
{
	static std::mutex mu;
	static std::unordered_map<std::string, std::optional<std::regex>> cache;
	std::lock_guard<std::mutex> lock(mu);
	auto it = cache.find(pattern);
	if (it == cache.end())
	{
		try
		{
			it =
				cache.emplace(pattern, std::regex(pattern, std::regex::ECMAScript)).first;
		} catch (const std::regex_error &)
		{
			it = cache.emplace(pattern, std::nullopt).first;
		}
	}
	return it->second ? &*it->second : nullptr;
}

bool regexFullMatch(const std::string &text, const std::string &pattern)
{
	// Tree-sitter / nvim use Rust-style regex; ECMAScript covers the
	// common ^…$ identifier conventions in grammar highlights.
	// tree-sitter #match? uses "find" semantics (Rust Regex::is_match),
	// not full-string only — so ^[A-Z] matches ClassName, not only "A".
	const std::regex *re = cachedRegex(pattern);
	return re && std::regex_search(text, *re);
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
						  ColorRangeMap &colors,
						  int rowBase)
{
	auto fillRange = [&](uint32_t start, uint32_t end, ThemeSlot slot) {
		if (start >= end)
			return;
		int sr, sc, er, ec;
		rowColFromOffset(snap, start, sr, sc);
		rowColFromOffset(snap, end, er, ec);
		for (int r = sr; r <= er; ++r)
		{
			const int idx = r - rowBase;
			if (idx < 0 || idx >= static_cast<int>(colors.size()))
				continue;
			const int lineLen = lineLength(snap, static_cast<size_t>(r));
			const int a = std::clamp(r == sr ? sc : 0, 0, lineLen);
			const int b = std::clamp(r == er ? ec : lineLen, 0, lineLen);
			if (a < b)
				setRange(colors[static_cast<size_t>(idx)], a, b, slot);
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
		fillRange(h.start, h.end, themeSlotForCapture(h.capture));
}

void TreeSitter::queryWindowInto(TSQuery *query,
								 TSTree *tree,
								 const ParseSnapshot &snap,
								 int lineLo,
								 int lineHi,
								 std::vector<int> &outRows,
								 std::vector<LineColorSpans> &outSpans)
{
	if (!query || !tree || lineLo >= lineHi || snap.lineStarts.empty())
		return;
	const int nStarts = static_cast<int>(snap.lineStarts.size());
	lineLo = std::clamp(lineLo, 0, nStarts);
	lineHi = std::clamp(lineHi, lineLo, nStarts);
	if (lineLo >= lineHi)
		return;

	ColorRangeMap scratch(static_cast<size_t>(lineHi - lineLo));
	const uint32_t byteStart = snap.lineStarts[static_cast<size_t>(lineLo)];
	const uint32_t byteEnd = (lineHi < nStarts)
								 ? snap.lineStarts[static_cast<size_t>(lineHi)]
								 : static_cast<uint32_t>(snapSize(snap));
	runQuery(query, tree, snap, byteStart, byteEnd, scratch, lineLo);
	for (int r = lineLo; r < lineHi; ++r)
	{
		outRows.push_back(r);
		outSpans.push_back(std::move(scratch[static_cast<size_t>(r - lineLo)]));
	}
}

void TreeSitter::emitDirtyWindows(TSQuery *query,
								  TSTree *tree,
								  const ParseSnapshot &snap,
								  const std::vector<uint8_t> &dirtyLine,
								  std::vector<int> &outRows,
								  std::vector<LineColorSpans> &outSpans)
{
	const int n = static_cast<int>(dirtyLine.size());
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
		queryWindowInto(query, tree, snap, lo, i, outRows, outSpans);
	}
}

// ---------------------------------------------------------------------------
// Incremental tree + parse
// ---------------------------------------------------------------------------

bool TreeSitter::applyPendingEdits(const std::vector<PendingEdit> &edits)
{
	if (!tree || edits.empty())
		return false;

	for (const PendingEdit &pe : edits)
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

ParseResult TreeSitter::parse(ParseSnapshot &snapshot, uint64_t gen)
{
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
		result.kind = ParseKind::Full;
		result.fullColors.assign(result.lineCount, {});
		return result;
	}

	ts_parser_set_language(parser, lang);

	std::vector<PendingEdit> pendingEdits = std::move(snapshot.pendingEdits);
	const bool hadPending = !pendingEdits.empty();

	if (treeFilePath != snapshot.path || treeLanguageId != snapshot.languageId)
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
		for (const PendingEdit &pe : pendingEdits)
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
	treeLanguageId = snapshot.languageId;
	treeDocBytes = snapshot.text.size();
	lastCommittedGen = gen;

	if (tryPartial)
	{
		TSQuery *query = loadQueryFromCacheOrFile(lang, query_path);
		if (!query)
		{
			result.kind = ParseKind::Full;
			result.fullColors.assign(result.lineCount, {});
			return result;
		}
		emitDirtyWindows(
			query, tree, snapshot, dirty, result.dirtyRows, result.dirtySpans);
		result.kind = ParseKind::Partial;
		return result;
	}

	result.kind = ParseKind::TreeOnly;
	return result;
}

ParseResult TreeSitter::queryWindow(const ParseSnapshot &snapshot, int lineLo, int lineHi)
{
	ParseResult result;
	result.lineCount = snapshot.lineStarts.size();
	if (!tree || snapshot.lineStarts.empty())
		return result;

	auto [lang, query_path] = detectLanguageAndQuery(snapshot.languageId);
	TSQuery *query = lang ? loadQueryFromCacheOrFile(lang, query_path) : nullptr;
	if (!query)
	{
		result.kind = ParseKind::Full;
		result.fullColors.assign(result.lineCount, {});
		return result;
	}

	const int n = static_cast<int>(result.lineCount);
	lineLo = std::clamp(lineLo, 0, n);
	lineHi = std::clamp(lineHi, lineLo, n);
	queryWindowInto(
		query, tree, snapshot, lineLo, lineHi, result.dirtyRows, result.dirtySpans);
	result.kind = ParseKind::Partial;
	return result;
}

void TreeSitter::colorDocument(ParseSnapshot &snapshot,
							   uint64_t gen,
							   int chunkLines,
							   const std::function<bool()> &canceled,
							   const std::function<void(ParseResult &&)> &emit)
{
	std::lock_guard<std::mutex> lock(parserMutex);
	if (canceled && canceled())
		return;

	ParseResult built = parse(snapshot, gen);
	if ((canceled && canceled()) || built.kind == ParseKind::Failed)
		return;
	if (built.kind != ParseKind::TreeOnly)
	{
		emit(std::move(built));
		return;
	}

	const int n = static_cast<int>(snapshot.lineStarts.size());
	const int chunk = std::max(1, chunkLines);
	for (int lo = 0; lo < n;)
	{
		if (canceled && canceled())
			return;
		const int hi = std::min(n, lo + chunk);
		emit(queryWindow(snapshot, lo, hi));
		lo = hi;
	}
}

ParseResult TreeSitter::queryPrefix(const ParseSnapshot &src, int maxLines)
{
	ParseResult result;
	const int docLines = std::max(1, src.text.lineCount());
	result.lineCount = static_cast<size_t>(docLines);
	if (maxLines <= 0 || src.text.size() == 0)
	{
		result.kind = ParseKind::Partial;
		return result;
	}

	auto [lang, query_path] = detectLanguageAndQuery(src.languageId);
	if (!lang)
	{
		result.kind = ParseKind::Full;
		result.fullColors.assign(result.lineCount, {});
		return result;
	}

	// Bounded copy — do not walk the whole rope.
	constexpr size_t kMaxPrefixBytes = 64 * 1024;
	const size_t cap = std::min(src.text.size(), kMaxPrefixBytes);
	std::string buf(cap, '\0');
	if (cap)
		src.text.copyBytes(0, cap, buf.data());

	const std::string_view sep = src.lineEnding.empty()
									 ? std::string_view("\n")
									 : std::string_view(src.lineEnding);

	ParseSnapshot snap;
	snap.text = src.text;
	snap.lineEnding = src.lineEnding.empty() ? std::string("\n") : src.lineEnding;
	snap.languageId = src.languageId;
	snap.lineStarts.push_back(0);
	size_t pos = 0;
	while (static_cast<int>(snap.lineStarts.size()) < maxLines && pos < cap)
	{
		const size_t at = buf.find(sep, pos);
		if (at == std::string::npos)
			break;
		pos = at + sep.size();
		snap.lineStarts.push_back(static_cast<uint32_t>(pos));
	}

	uint32_t prefixEnd = static_cast<uint32_t>(cap);
	if (static_cast<int>(snap.lineStarts.size()) >= maxLines)
	{
		const size_t last = snap.lineStarts[static_cast<size_t>(maxLines - 1)];
		const size_t at = buf.find(sep, last);
		prefixEnd = at == std::string::npos ? static_cast<uint32_t>(cap)
											: static_cast<uint32_t>(at + sep.size());
	}
	snap.byteLimit = prefixEnd;

	const int found = static_cast<int>(snap.lineStarts.size());
	if (found <= 0)
	{
		result.kind = ParseKind::Partial;
		return result;
	}

	TSQuery *query = loadQueryFromCacheOrFile(lang, query_path);
	if (!query)
	{
		result.kind = ParseKind::Partial;
		return result;
	}

	TSParser *tmp = ts_parser_new();
	ts_parser_set_language(tmp, lang);
	TSTree *tmpTree = ts_parser_parse_string(
		tmp, nullptr, buf.data(), static_cast<uint32_t>(prefixEnd));

	if (tmpTree)
		queryWindowInto(
			query, tmpTree, snap, 0, found, result.dirtyRows, result.dirtySpans);

	ts_tree_delete(tmpTree);
	ts_parser_delete(tmp);

	result.kind = ParseKind::Partial;
	return result;
}

ColorRangeMap TreeSitter::highlightSnippet(const std::string &languageId,
										   const std::string &text)
{
	// Hover fences are tiny; 32 entries cover every block of a few tooltips.
	constexpr size_t kSnippetCacheLimit = 32;

	ColorRangeMap out;
	if (text.empty())
		return out;

	// Hover tooltips re-render every frame; parse once per (language, snippet)
	// instead of per frame. Small bounded cache — hover fences are tiny.
	static std::mutex cacheMutex;
	static std::map<std::pair<std::string, std::string>, ColorRangeMap> cache;
	const std::pair<std::string, std::string> key(languageId, text);
	{
		std::lock_guard<std::mutex> lock(cacheMutex);
		const auto it = cache.find(key);
		if (it != cache.end())
			return it->second;
	}

	EditorState tmp;
	tmp.setFromString(text);
	tmp.lineEnding = "\n";

	ParseSnapshot snap;
	snap.text = tmp.snapshot();
	snap.lineEnding = "\n";
	snap.languageId = languageId;
	tmp.lineStarts(snap.lineStarts);

	TreeSitter engine;
	const int lines = std::max(1, tmp.lineCount());
	ParseResult r = engine.queryPrefix(snap, lines);
	out.assign(static_cast<size_t>(lines), {});
	if (r.kind == ParseKind::Full && r.fullColors.size() == out.size())
		out = std::move(r.fullColors);
	else
		for (size_t i = 0; i < r.dirtyRows.size() && i < r.dirtySpans.size(); ++i)
		{
			const int row = r.dirtyRows[i];
			if (row >= 0 && row < lines)
				out[static_cast<size_t>(row)] = std::move(r.dirtySpans[i]);
		}

	{
		std::lock_guard<std::mutex> lock(cacheMutex);
		// Bounded; evict the oldest entry rather than flushing everything so a
		// long-lived tooltip does not re-parse its fences on cache churn.
		if (cache.size() >= kSnippetCacheLimit)
			cache.erase(cache.begin()); // std::map iterates in key order
		cache.emplace(key, out);
	}
	return out;
}

void TreeSitter::updateThemeColors()
{
	const ImVec4 fbText(0.85f, 0.85f, 0.85f, 1.0f);
	const ImVec4 fbComment(0.5f, 0.5f, 0.5f, 1.0f);
	for (auto &s : cachedColors.slots)
		s = fbText;
	cachedColors[ThemeSlot::Comment] = fbComment;

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

	cachedColors[ThemeSlot::Text] = load("text", fbText);
	cachedColors[ThemeSlot::Comment] = load("comment", fbComment);
	const ImVec4 text = cachedColors[ThemeSlot::Text];
	cachedColors[ThemeSlot::Keyword] = load("keyword", text);
	cachedColors[ThemeSlot::String] = load("string", text);
	cachedColors[ThemeSlot::Number] = load("number", text);
	cachedColors[ThemeSlot::Function] = load("function", text);
	cachedColors[ThemeSlot::Type] = load("type", text);
	cachedColors[ThemeSlot::Variable] = load("variable", text);
	cachedColors[ThemeSlot::Operator] = load("operator", text);
	cachedColors[ThemeSlot::Punctuation] = load("punctuation", text);
	cachedColors[ThemeSlot::Parameter] =
		load("parameter", cachedColors[ThemeSlot::Variable]);
	cachedColors[ThemeSlot::Property] =
		load("property", cachedColors[ThemeSlot::Variable]);
	cachedColors[ThemeSlot::Constant] = load("constant", cachedColors[ThemeSlot::Number]);
	cachedColors[ThemeSlot::Special] = load("special", cachedColors[ThemeSlot::Keyword]);
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
	for (const LangEntry &e : kLangs)
	{
		if (id == e.id)
			return {e.lang(), e.scm};
	}
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
	const std::string full_path = resolveQueryFile(query_path).string();

	std::promise<TSQuery *> compiled;
	std::shared_future<TSQuery *> waiter;
	bool owner = false;
	{
		std::lock_guard<std::mutex> lock(gQueryMu);
		if (auto it = gQueryCache.find(full_path); it != gQueryCache.end())
			return it->second;
		if (auto it = gQueryInflight.find(full_path); it != gQueryInflight.end())
			waiter = it->second;
		else
		{
			waiter = compiled.get_future().share();
			gQueryInflight[full_path] = waiter;
			owner = true;
		}
	}
	if (!owner)
		return waiter.get();

	// ts_query_new is seconds on cpp/jsx. queryReady() is called from the UI
	// thread and must not wait on this lock.
	TSQuery *query = nullptr;
	try
	{
		std::ifstream file(full_path);
		if (file.is_open())
		{
			std::string query_src((std::istreambuf_iterator<char>(file)),
								  std::istreambuf_iterator<char>());
			uint32_t error_offset = 0;
			TSQueryError error_type{};
			query = ts_query_new(
				lang, query_src.c_str(), query_src.size(), &error_offset, &error_type);
			if (!query)
			{
				std::cerr << "Query error (" << error_type << ") at offset "
						  << error_offset << "\n";
			}
		}
	} catch (const std::exception &)
	{
		query = nullptr;
	}

	{
		std::lock_guard<std::mutex> lock(gQueryMu);
		if (query)
			gQueryCache[full_path] = query;
		gQueryInflight.erase(full_path);
	}
	compiled.set_value(query);
	return query;
}

void TreeSitter::startBackgroundPrewarm()
{
	static std::once_flag once;
	std::call_once(once, [] {
		std::thread([] {
			std::unordered_set<std::string> seen;
			for (const LangEntry &e : kLangs)
			{
				if (!seen.insert(e.scm).second)
					continue;
				loadQueryFromCacheOrFile(e.lang(), e.scm);
			}
		}).detach();
	});
}

bool TreeSitter::queryReady(const std::string &languageId) const
{
	auto [lang, query_path] = detectLanguageAndQuery(languageId);
	if (!lang)
		return true;
	const std::string full_path = resolveQueryFile(query_path).string();
	std::lock_guard<std::mutex> lock(gQueryMu);
	return gQueryCache.count(full_path) != 0;
}
