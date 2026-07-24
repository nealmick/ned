#include "text_model.h"

#include <algorithm>
#include <cctype>

namespace monaco {

// Local SnapToUtf8CharBoundary — avoids pulling ImGui
// into the headless model test binary via editor_utils.h.
static int snapUtf8(const std::string &str, int idx)
{
	if (idx <= 0 || idx >= static_cast<int>(str.size()))
		return idx;
	while (idx > 0 &&
		   (static_cast<unsigned char>(str[static_cast<size_t>(idx)]) & 0xC0) == 0x80)
		--idx;
	return idx;
}

TextModel::TextModel() : ops_(state_) {}

TextModel::TextModel(TextModel &&other) noexcept
	: state_(std::move(other.state_)), ops_(state_)
{
}

TextModel &TextModel::operator=(TextModel &&other) noexcept
{
	if (this != &other)
	{
		// Rebuild ops bound to this->state_ (state pointer is private on ops).
		this->~TextModel();
		new (this) TextModel(std::move(other));
	}
	return *this;
}

TextModel TextModel::create(const std::string &text)
{
	TextModel m;
	m.state_.setFromString(text);
	return m;
}

TextModel TextModel::createFromLines(const std::vector<std::string> &lines)
{
	std::string joined;
	for (size_t i = 0; i < lines.size(); ++i)
	{
		if (i)
			joined += '\n';
		joined += lines[i];
	}
	return create(joined);
}

void TextModel::setValue(const std::string &text)
{
	state_.setFromString(text);
	ops_.clearPending();
}

void TextModel::setEOL(const std::string &eol) { state_.setLineEnding(eol); }

std::string TextModel::getValue() const { return state_.join(); }

int TextModel::getLineCount() const { return std::max(1, state_.lineCount()); }

std::string TextModel::getLineContent(int lineNumber) const
{
	if (state_.lineCount() <= 0)
		return {};
	const int idx = std::clamp(lineNumber, 1, state_.lineCount()) - 1;
	return state_.line(idx);
}

std::vector<std::string> TextModel::getLinesContent() const { return state_.lines(); }

Range TextModel::order(Range r)
{
	const bool swapped =
		r.startLineNumber > r.endLineNumber ||
		(r.startLineNumber == r.endLineNumber && r.startColumn > r.endColumn);
	if (swapped)
	{
		std::swap(r.startLineNumber, r.endLineNumber);
		std::swap(r.startColumn, r.endColumn);
	}
	return r;
}

void TextModel::toNed(const Position &p, int &row, int &col) const
{
	row = std::max(0, p.lineNumber - 1);
	col = std::max(0, p.column - 1);
	if (state_.lineCount() <= 0)
	{
		row = 0;
		col = 0;
		return;
	}
	row = std::clamp(row, 0, state_.lineCount() - 1);
	col = std::clamp(col, 0, state_.lineLength(row));
}

Position TextModel::fromNed(int row, int col) const { return Position{row + 1, col + 1}; }

void TextModel::clampRange(Range &r) const
{
	r = order(r);
	Position s{r.startLineNumber, r.startColumn};
	Position e{r.endLineNumber, r.endColumn};
	s = validatePosition(s);
	e = validatePosition(e);
	r.startLineNumber = s.lineNumber;
	r.startColumn = s.column;
	r.endLineNumber = e.lineNumber;
	r.endColumn = e.column;
	r = order(r);
}

Position TextModel::validatePosition(Position p) const
{
	if (state_.lineCount() <= 0)
		return {1, 1};

	// Clamp invalid positions toward origin.
	if (p.lineNumber < 1)
		p.lineNumber = 1;
	if (p.column < 1)
		p.column = 1;

	if (p.lineNumber > state_.lineCount())
	{
		p.lineNumber = state_.lineCount();
		p.column = state_.lineLength(p.lineNumber - 1) + 1;
		return p;
	}

	const std::string line = state_.line(p.lineNumber - 1);
	const int maxCol = static_cast<int>(line.size()) + 1;
	if (p.column > maxCol)
		p.column = maxCol;

	// Snap off UTF-8 continuation bytes.
	int col0 = p.column - 1;
	col0 = snapUtf8(line, col0);
	p.column = col0 + 1;
	return p;
}

Range TextModel::validateRange(Range r) const
{
	Position s = validatePosition({r.startLineNumber, r.startColumn});
	Position e = validatePosition({r.endLineNumber, r.endColumn});
	// If a range starts mid-character, expand like Monaco does for surrogates:
	// start snaps to char start; end snaps to char start then if it was inside,
	// validatePosition already snapped — for partial char coverage expand end.
	r.startLineNumber = s.lineNumber;
	r.startColumn = s.column;
	r.endLineNumber = e.lineNumber;
	r.endColumn = e.column;
	return order(r);
}

std::string TextModel::getValueInRange(const Range &range, EndOfLinePreference eol) const
{
	Range r = range;
	clampRange(r);

	int sr, sc, er, ec;
	toNed({r.startLineNumber, r.startColumn}, sr, sc);
	toNed({r.endLineNumber, r.endColumn}, er, ec);

	std::string text = ops_.extractText(sr, sc, er, ec);
	if (eol == EndOfLinePreference::TextDefined)
		return text;

	const std::string want = (eol == EndOfLinePreference::LF) ? "\n" : "\r\n";
	if (want == state_.lineEnding)
		return text;

	// Rewrite EOLs in extracted span.
	std::string out;
	out.reserve(text.size());
	const std::string &le = state_.lineEnding;
	for (size_t i = 0; i < text.size();)
	{
		if (!le.empty() && i + le.size() <= text.size() &&
			text.compare(i, le.size(), le) == 0)
		{
			out += want;
			i += le.size();
		} else
		{
			out += text[i++];
		}
	}
	return out;
}

int TextModel::getValueLengthInRange(const Range &range, EndOfLinePreference eol) const
{
	return static_cast<int>(getValueInRange(range, eol).size());
}

Position TextModel::modifyPosition(Position p, int offset) const
{
	p = validatePosition(p);
	int row, col;
	toNed(p, row, col);
	size_t off = state_.offsetFromRowCol(row, col);
	if (offset >= 0)
		off = std::min(off + static_cast<size_t>(offset), state_.join().size());
	else
	{
		const size_t back = static_cast<size_t>(-offset);
		off = (back >= off) ? 0 : off - back;
	}
	int orow = 0, ocol = 0;
	state_.rowColFromOffset(off, orow, ocol);
	return fromNed(orow, ocol);
}

int TextModel::getLineFirstNonWhitespaceColumn(int lineNumber) const
{
	const std::string line = getLineContent(lineNumber);
	for (size_t i = 0; i < line.size(); ++i)
	{
		if (line[i] != ' ' && line[i] != '\t')
			return static_cast<int>(i) + 1;
	}
	return 0;
}

int TextModel::getLineLastNonWhitespaceColumn(int lineNumber) const
{
	const std::string line = getLineContent(lineNumber);
	for (int i = static_cast<int>(line.size()) - 1; i >= 0; --i)
	{
		if (line[static_cast<size_t>(i)] != ' ' && line[static_cast<size_t>(i)] != '\t')
			return i + 2; // Monaco: column after last non-ws char
	}
	return 0;
}

SingleEditOperation TextModel::applyOne(const SingleEditOperation &raw)
{
	SingleEditOperation op = raw;
	clampRange(op.range);

	int sr, sc, er, ec;
	toNed({op.range.startLineNumber, op.range.startColumn}, sr, sc);
	toNed({op.range.endLineNumber, op.range.endColumn}, er, ec);

	// Normalize foreign newlines in insert payload to document EOL.
	const std::string insertText = ops_.normalizeLineEndings(op.text);

	const int deleteLen = ops_.measureLength(sr, sc, er, ec);
	std::string deleted;

	if (deleteLen > 0)
	{
		TextOp del;
		del.kind = OpKind::Delete;
		del.row = sr;
		del.column = sc;
		del.length = deleteLen;
		const ApplyResult dr = ops_.apply(del);
		deleted = dr.deletedText;
	}

	int endRow = sr;
	int endCol = sc;
	if (!insertText.empty())
	{
		TextOp ins;
		ins.kind = OpKind::Insert;
		ins.row = sr;
		ins.column = sc;
		ins.text = insertText;
		const ApplyResult ir = ops_.apply(ins);
		endRow = ir.endRow;
		endCol = ir.endColumn;
	}

	// Inverse: replace newly written span with deleted text.
	SingleEditOperation inverse;
	inverse.range = Range(sr + 1, sc + 1, endRow + 1, endCol + 1);
	inverse.text = deleted;
	return inverse;
}

std::vector<SingleEditOperation>
TextModel::applyEdits(const std::vector<SingleEditOperation> &operations,
					  bool computeUndoEdits)
{
	if (operations.empty())
		return {};

	// Snapshot for a reliable multi-edit inverse (lower edits shift higher inverses).
	const std::string beforeValue = computeUndoEdits ? getValue() : std::string{};
	const std::string beforeEOL = state_.lineEnding;

	// Apply bottom-up so original coordinates stay valid (Monaco non-overlapping case).
	std::vector<size_t> order(operations.size());
	for (size_t i = 0; i < order.size(); ++i)
		order[i] = i;

	std::stable_sort(order.begin(), order.end(), [&](size_t a, size_t b) {
		const Range ra = TextModel::order(operations[a].range);
		const Range rb = TextModel::order(operations[b].range);
		if (ra.startLineNumber != rb.startLineNumber)
			return ra.startLineNumber > rb.startLineNumber;
		return ra.startColumn > rb.startColumn;
	});

	SingleEditOperation lastInverse;
	for (size_t idx : order)
		lastInverse = applyOne(operations[idx]);

	if (!computeUndoEdits)
		return {};

	// Single edit: precise inverse (Monaco modelEditOperation round-trip).
	if (operations.size() == 1)
		return {lastInverse};

	// Multi-edit: full-buffer inverse for undo when ops are batched.
	SingleEditOperation undoAll;
	undoAll.range = Range(
		1, 1, getLineCount(), static_cast<int>(getLineContent(getLineCount()).size()) + 1);
	// Preserve original EOL bytes in the payload.
	undoAll.text = beforeValue;
	// If EOL changed (shouldn't), still restore via setFromString semantics on apply.
	(void)beforeEOL;
	return {undoAll};
}

} // namespace monaco
