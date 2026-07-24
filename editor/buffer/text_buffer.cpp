#include "text_buffer.h"

#include <algorithm>
#include <cassert>
#include <cstring>

// ---------------------------------------------------------------------------
// Summary helpers
// ---------------------------------------------------------------------------

TextBuffer::Summary TextBuffer::summarizeLeaf(std::string_view s)
{
	Summary sum;
	sum.bytes = s.size();
	size_t lineStart = 0;
	for (size_t i = 0; i < s.size();)
	{
		if (s[i] == '\r' && i + 1 < s.size() && s[i + 1] == '\n')
		{
			++sum.breaks;
			i += 2;
			lineStart = i;
		} else if (s[i] == '\n' || s[i] == '\r')
		{
			++sum.breaks;
			++i;
			lineStart = i;
		} else
		{
			++i;
		}
	}
	sum.lastLineBytes = static_cast<uint32_t>(s.size() - lineStart);
	if (!s.empty())
	{
		sum.startsWithLF = (s.front() == '\n');
		// Ends with CR only when not already a CRLF pair (last is \n then false).
		sum.endsWithCR = (s.back() == '\r');
	}
	return sum;
}

TextBuffer::Summary TextBuffer::merge(const Summary &a, const Summary &b)
{
	if (a.bytes == 0)
		return b;
	if (b.bytes == 0)
		return a;
	Summary s;
	s.bytes = a.bytes + b.bytes;
	// "\r" | "\n..." across the join is one CRLF break, not two.
	const bool crlfJoin = a.endsWithCR && b.startsWithLF;
	s.breaks = a.breaks + b.breaks - (crlfJoin ? 1u : 0u);
	if (b.breaks == 0 && !crlfJoin)
		s.lastLineBytes = a.lastLineBytes + static_cast<uint32_t>(b.bytes);
	else
		s.lastLineBytes = b.lastLineBytes;
	s.startsWithLF = a.startsWithLF;
	s.endsWithCR = b.endsWithCR;
	return s;
}

int TextBuffer::totalLines(const Summary &s)
{
	// Empty buffer still has one empty line.
	return static_cast<int>(s.breaks) + 1;
}

// ---------------------------------------------------------------------------
// Node constructors
// ---------------------------------------------------------------------------

std::shared_ptr<const TextBuffer::Node> TextBuffer::emptyLeaf() { return makeLeaf({}); }

std::shared_ptr<const TextBuffer::Node> TextBuffer::makeLeaf(std::string data)
{
	auto leaf = std::make_shared<Leaf>();
	leaf->data = std::move(data);
	leaf->sum = summarizeLeaf(leaf->data);
	return leaf;
}

std::shared_ptr<const TextBuffer::Node>
TextBuffer::makeBranch(std::shared_ptr<const Node> left, std::shared_ptr<const Node> right)
{
	if (!left || left->sum.bytes == 0)
		return right ? right : emptyLeaf();
	if (!right || right->sum.bytes == 0)
		return left;
	auto br = std::make_shared<Branch>();
	br->left = std::move(left);
	br->right = std::move(right);
	br->sum = merge(br->left->sum, br->right->sum);
	return br;
}

std::shared_ptr<const TextBuffer::Node> TextBuffer::fromString(std::string_view s)
{
	if (s.empty())
		return emptyLeaf();
	if (s.size() <= kMaxLeafBytes)
		return makeLeaf(std::string(s));

	// Split near the middle — never between \r and \n (would double-count breaks).
	size_t mid = s.size() / 2;
	if (mid > 0 && mid < s.size() && s[mid - 1] == '\r' && s[mid] == '\n')
		++mid; // keep CRLF in the right half
	if (mid == 0 || mid >= s.size())
		mid = s.size() / 2; // safety if edge case
	return makeBranch(fromString(s.substr(0, mid)), fromString(s.substr(mid)));
}

std::shared_ptr<const TextBuffer::Node> TextBuffer::concat(std::shared_ptr<const Node> a,
														   std::shared_ptr<const Node> b)
{
	if (!a || a->sum.bytes == 0)
		return b ? b : emptyLeaf();
	if (!b || b->sum.bytes == 0)
		return a;

	// Fuse small adjacent leaves.
	const auto *la = dynamic_cast<const Leaf *>(a.get());
	const auto *lb = dynamic_cast<const Leaf *>(b.get());
	if (la && lb && la->data.size() + lb->data.size() <= kMaxLeafBytes)
		return makeLeaf(la->data + lb->data);

	return makeBranch(std::move(a), std::move(b));
}

// ---------------------------------------------------------------------------
// Slice / insert / erase
// ---------------------------------------------------------------------------

std::shared_ptr<const TextBuffer::Node>
TextBuffer::sliceNode(const std::shared_ptr<const Node> &n, size_t off, size_t len)
{
	if (!n || len == 0 || off >= n->sum.bytes)
		return emptyLeaf();
	len = std::min(len, n->sum.bytes - off);

	if (const auto *leaf = dynamic_cast<const Leaf *>(n.get()))
		return fromString(std::string_view(leaf->data).substr(off, len));

	const auto *br = dynamic_cast<const Branch *>(n.get());
	assert(br);
	const size_t leftBytes = br->left->sum.bytes;
	if (off + len <= leftBytes)
		return sliceNode(br->left, off, len);
	if (off >= leftBytes)
		return sliceNode(br->right, off - leftBytes, len);

	const size_t leftTake = leftBytes - off;
	return concat(sliceNode(br->left, off, leftTake),
				  sliceNode(br->right, 0, len - leftTake));
}

std::shared_ptr<const TextBuffer::Node> TextBuffer::insertNode(
	const std::shared_ptr<const Node> &n, size_t off, std::string_view text)
{
	if (text.empty())
		return n ? n : emptyLeaf();
	if (!n || n->sum.bytes == 0)
		return fromString(text);

	off = std::min(off, n->sum.bytes);

	if (const auto *leaf = dynamic_cast<const Leaf *>(n.get()))
	{
		std::string merged;
		merged.reserve(leaf->data.size() + text.size());
		merged.append(leaf->data, 0, off);
		merged.append(text);
		merged.append(leaf->data, off, std::string::npos);
		return fromString(merged);
	}

	const auto *br = dynamic_cast<const Branch *>(n.get());
	assert(br);
	const size_t leftBytes = br->left->sum.bytes;
	if (off <= leftBytes)
		return makeBranch(insertNode(br->left, off, text), br->right);
	return makeBranch(br->left, insertNode(br->right, off - leftBytes, text));
}

std::shared_ptr<const TextBuffer::Node>
TextBuffer::eraseNode(const std::shared_ptr<const Node> &n, size_t off, size_t len)
{
	if (!n || len == 0 || off >= n->sum.bytes)
		return n ? n : emptyLeaf();
	len = std::min(len, n->sum.bytes - off);

	if (off == 0 && len == n->sum.bytes)
		return emptyLeaf();

	// General path: left slice + right slice.
	return concat(sliceNode(n, 0, off), sliceNode(n, off + len, n->sum.bytes - off - len));
}

// ---------------------------------------------------------------------------
// Collect / copy
// ---------------------------------------------------------------------------

void TextBuffer::collect(const Node *n, std::string &out)
{
	if (!n)
		return;
	if (const auto *leaf = dynamic_cast<const Leaf *>(n))
	{
		out.append(leaf->data);
		return;
	}
	const auto *br = dynamic_cast<const Branch *>(n);
	assert(br);
	collect(br->left.get(), out);
	collect(br->right.get(), out);
}

void TextBuffer::copyNode(const Node *n, size_t off, size_t len, char *out)
{
	if (!n || len == 0 || !out)
		return;
	if (off >= n->sum.bytes)
		return;
	len = std::min(len, n->sum.bytes - off);

	if (const auto *leaf = dynamic_cast<const Leaf *>(n))
	{
		std::memcpy(out, leaf->data.data() + off, len);
		return;
	}
	const auto *br = dynamic_cast<const Branch *>(n);
	assert(br);
	const size_t leftBytes = br->left->sum.bytes;
	if (off + len <= leftBytes)
	{
		copyNode(br->left.get(), off, len, out);
		return;
	}
	if (off >= leftBytes)
	{
		copyNode(br->right.get(), off - leftBytes, len, out);
		return;
	}
	const size_t leftTake = leftBytes - off;
	copyNode(br->left.get(), off, leftTake, out);
	copyNode(br->right.get(), 0, len - leftTake, out + leftTake);
}

// ---------------------------------------------------------------------------
// Line navigation
// ---------------------------------------------------------------------------

// Return byte offset of the start of `row` (0-based), or size if past end.
size_t TextBuffer::lineStartOffset(const Node *n, int row)
{
	if (!n || row <= 0)
		return 0;
	if (row >= totalLines(n->sum))
		return n->sum.bytes;

	// Need the offset just after `row` line breaks from the start.
	uint32_t need = static_cast<uint32_t>(row);

	if (const auto *leaf = dynamic_cast<const Leaf *>(n))
	{
		const auto &s = leaf->data;
		uint32_t seen = 0;
		for (size_t i = 0; i < s.size();)
		{
			if (s[i] == '\r' && i + 1 < s.size() && s[i + 1] == '\n')
			{
				++seen;
				i += 2;
				if (seen == need)
					return i;
			} else if (s[i] == '\n' || s[i] == '\r')
			{
				++seen;
				++i;
				if (seen == need)
					return i;
			} else
			{
				++i;
			}
		}
		return s.size();
	}

	const auto *br = dynamic_cast<const Branch *>(n);
	assert(br);
	const uint32_t leftBreaks = br->left->sum.breaks;
	const size_t leftBytes = br->left->sum.bytes;
	// Incomplete \r at end of left + \n at start of right = one break (not left's).
	const bool crlfJoin = br->left->sum.endsWithCR && br->right->sum.startsWithLF;
	const uint32_t leftCompleteBreaks = leftBreaks - (crlfJoin ? 1u : 0u);
	// Rows 0..leftCompleteBreaks start in the left subtree; later rows in the right.
	if (static_cast<uint32_t>(row) <= leftCompleteBreaks)
		return lineStartOffset(br->left.get(), row);
	return leftBytes +
		   lineStartOffset(br->right.get(), row - static_cast<int>(leftCompleteBreaks));
}

// ---------------------------------------------------------------------------
// TextBuffer public API
// ---------------------------------------------------------------------------

TextBuffer::TextBuffer() : root(emptyLeaf()) {}

void TextBuffer::assign(std::string_view joined) { root = fromString(joined); }

size_t TextBuffer::size() const { return root ? root->sum.bytes : 0; }

int TextBuffer::lineCount() const { return root ? totalLines(root->sum) : 1; }

int TextBuffer::lineLength(int row) const
{
	if (!root || row < 0 || row >= lineCount())
		return 0;
	const size_t start = lineStartOffset(root.get(), row);
	const size_t end =
		(row + 1 < lineCount()) ? lineStartOffset(root.get(), row + 1) : size();
	// end points at start of next line (after the terminator of this line).
	// Line length excludes the terminator between start and end.
	if (end < start)
		return 0;
	size_t len = end - start;
	// Strip trailing break belonging to this line (if not last empty after final break).
	if (row + 1 < lineCount() && len > 0)
	{
		// Terminator is at the end of [start, end).
		// Peek last bytes of the line range.
		char buf[2] = {};
		if (len >= 2)
		{
			copyNode(root.get(), end - 2, 2, buf);
			if (buf[0] == '\r' && buf[1] == '\n')
				return static_cast<int>(len - 2);
		}
		copyNode(root.get(), end - 1, 1, buf);
		if (buf[0] == '\n' || buf[0] == '\r')
			return static_cast<int>(len - 1);
	}
	return static_cast<int>(len);
}

size_t TextBuffer::offsetFromRowCol(int row, int col) const
{
	if (!root)
		return 0;
	const int lines = lineCount();
	if (lines <= 0)
		return 0;
	row = std::clamp(row, 0, lines - 1);
	const int ll = lineLength(row);
	col = std::clamp(col, 0, ll);
	return lineStartOffset(root.get(), row) + static_cast<size_t>(col);
}

void TextBuffer::rowColFromOffset(size_t off, int &row, int &col) const
{
	if (!root || size() == 0)
	{
		row = 0;
		col = 0;
		return;
	}
	off = std::min(off, size());

	const int lines = lineCount();
	// Find greatest row with lineStart(row) <= off.
	int lo = 0, hi = lines - 1;
	while (lo < hi)
	{
		const int mid = lo + (hi - lo + 1) / 2;
		if (lineStartOffset(root.get(), mid) <= off)
			lo = mid;
		else
			hi = mid - 1;
	}
	row = lo;
	const size_t start = lineStartOffset(root.get(), row);
	col = static_cast<int>(off - start);
	// Offset past line body (in terminator) maps to next line start.
	const int ll = lineLength(row);
	if (col > ll)
	{
		if (row + 1 < lines)
		{
			row += 1;
			col = 0;
		} else
		{
			col = ll;
		}
	}
}

std::string TextBuffer::line(int row) const
{
	std::string out;
	lineInto(row, out);
	return out;
}

void TextBuffer::lineInto(int row, std::string &out) const
{
	out.clear();
	if (!root || row < 0 || row >= lineCount())
		return;
	const size_t start = lineStartOffset(root.get(), row);
	const int ll = lineLength(row);
	if (ll <= 0)
		return;
	out.resize(static_cast<size_t>(ll));
	copyNode(root.get(), start, static_cast<size_t>(ll), out.data());
}

void TextBuffer::collectLineStarts(const Node *n,
								   size_t base,
								   std::vector<uint32_t> &out,
								   size_t &pendingAfterCR)
{
	if (!n)
		return;
	if (const auto *leaf = dynamic_cast<const Leaf *>(n))
	{
		const auto &s = leaf->data;
		for (size_t i = 0; i < s.size();)
		{
			if (pendingAfterCR != 0)
			{
				const size_t afterCr = pendingAfterCR;
				pendingAfterCR = 0;
				if (s[i] == '\n')
				{
					// Complete CRLF that started in a previous leaf.
					++i;
					out.push_back(static_cast<uint32_t>(base + i));
					continue;
				}
				// Lone \r break: next line starts at afterCr.
				out.push_back(static_cast<uint32_t>(afterCr));
			}
			if (s[i] == '\r' && i + 1 < s.size() && s[i + 1] == '\n')
			{
				i += 2;
				out.push_back(static_cast<uint32_t>(base + i));
			} else if (s[i] == '\r')
			{
				++i;
				if (i == s.size())
					pendingAfterCR = base + i; // maybe next leaf starts with \n
				else
					out.push_back(static_cast<uint32_t>(base + i));
			} else if (s[i] == '\n')
			{
				++i;
				out.push_back(static_cast<uint32_t>(base + i));
			} else
			{
				++i;
			}
		}
		return;
	}
	const auto *br = dynamic_cast<const Branch *>(n);
	assert(br);
	collectLineStarts(br->left.get(), base, out, pendingAfterCR);
	collectLineStarts(br->right.get(), base + br->left->sum.bytes, out, pendingAfterCR);
}

void TextBuffer::lineStarts(std::vector<uint32_t> &out) const
{
	out.clear();
	out.push_back(0);
	if (root && root->sum.bytes > 0)
	{
		size_t pendingAfterCR = 0;
		collectLineStarts(root.get(), 0, out, pendingAfterCR);
		if (pendingAfterCR != 0)
			out.push_back(static_cast<uint32_t>(pendingAfterCR));
	}
}

bool TextBuffer::nodeContainsByte(const Node *n, char c)
{
	if (!n)
		return false;
	if (const auto *leaf = dynamic_cast<const Leaf *>(n))
		return leaf->data.find(c) != std::string::npos;
	const auto *br = dynamic_cast<const Branch *>(n);
	assert(br);
	return nodeContainsByte(br->left.get(), c) || nodeContainsByte(br->right.get(), c);
}

bool TextBuffer::containsByte(char c) const
{
	return root && nodeContainsByte(root.get(), c);
}

std::string TextBuffer::str() const
{
	std::string out;
	if (root)
	{
		out.reserve(root->sum.bytes);
		collect(root.get(), out);
	}
	return out;
}

void TextBuffer::copyBytes(size_t off, size_t len, char *out) const
{
	if (!root || !out || len == 0)
		return;
	copyNode(root.get(), off, len, out);
}

void TextBuffer::insert(size_t off, std::string_view text)
{
	if (text.empty())
		return;
	root = insertNode(root, off, text);
}

void TextBuffer::erase(size_t off, size_t len)
{
	if (len == 0)
		return;
	root = eraseNode(root, off, len);
}

TextBuffer::Snapshot TextBuffer::snapshot() const { return Snapshot(root); }

// ---------------------------------------------------------------------------
// Snapshot
// ---------------------------------------------------------------------------

size_t TextBuffer::Snapshot::size() const { return root ? root->sum.bytes : 0; }

int TextBuffer::Snapshot::lineCount() const
{
	return root ? TextBuffer::totalLines(root->sum) : 1;
}

void TextBuffer::Snapshot::lineStarts(std::vector<uint32_t> &out) const
{
	out.clear();
	out.push_back(0);
	if (root && root->sum.bytes > 0)
	{
		size_t pendingAfterCR = 0;
		collectLineStarts(root.get(), 0, out, pendingAfterCR);
		if (pendingAfterCR != 0)
			out.push_back(static_cast<uint32_t>(pendingAfterCR));
	}
}

void TextBuffer::Snapshot::copyBytes(size_t off, size_t len, char *out) const
{
	if (!root || !out || len == 0)
		return;
	TextBuffer::copyNode(root.get(), off, len, out);
}

std::string TextBuffer::Snapshot::str() const
{
	std::string out;
	if (root)
	{
		out.reserve(root->sum.bytes);
		TextBuffer::collect(root.get(), out);
	}
	return out;
}
