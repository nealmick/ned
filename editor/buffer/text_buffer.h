/*
	File: text_buffer.h
	Description: Copy-on-write rope of joined document bytes (chunks + line metrics).

	Public API is read-only (metrics, lines, snapshot, copyBytes).
	Mutation is private — only EditorState may call assign/insert/erase.
	Session edits go through EditorOperations → EditorState.
*/
#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

class EditorState;

class TextBuffer
{
  private:
	friend class EditorState;

	struct Summary
	{
		size_t bytes = 0;
		uint32_t breaks = 0;
		uint32_t lastLineBytes = 0;
		// For CRLF that straddles a leaf boundary: lone \r at end of left +
		// lone \n at start of right is one break, not two.
		bool endsWithCR = false;
		bool startsWithLF = false;
	};

	struct Node
	{
		virtual ~Node() = default;
		Summary sum;
	};

	struct Leaf : Node
	{
		std::string data;
	};

	struct Branch : Node
	{
		std::shared_ptr<const Node> left;
		std::shared_ptr<const Node> right;
	};

	std::shared_ptr<const Node> root;

	void assign(std::string_view joined);
	void insert(size_t off, std::string_view text);
	void erase(size_t off, size_t len);

	static Summary summarizeLeaf(std::string_view s);
	static Summary merge(const Summary &a, const Summary &b);
	static std::shared_ptr<const Node> emptyLeaf();
	static std::shared_ptr<const Node> makeLeaf(std::string data);
	static std::shared_ptr<const Node> makeBranch(std::shared_ptr<const Node> left,
												  std::shared_ptr<const Node> right);
	static std::shared_ptr<const Node> fromString(std::string_view s);
	static std::shared_ptr<const Node> concat(std::shared_ptr<const Node> a,
											  std::shared_ptr<const Node> b);
	static std::shared_ptr<const Node>
	insertNode(const std::shared_ptr<const Node> &n, size_t off, std::string_view text);
	static std::shared_ptr<const Node>
	eraseNode(const std::shared_ptr<const Node> &n, size_t off, size_t len);
	static std::shared_ptr<const Node>
	sliceNode(const std::shared_ptr<const Node> &n, size_t off, size_t len);
	static void collect(const Node *n, std::string &out);
	static void copyNode(const Node *n, size_t off, size_t len, char *out);
	static size_t lineStartOffset(const Node *n, int row);
	static int totalLines(const Summary &s);
	// pendingAfterCR: absolute byte offset after a trailing \r (0 = none).
	// Next byte \n completes CRLF; anything else commits a break at that offset.
	static void collectLineStarts(const Node *n,
								  size_t base,
								  std::vector<uint32_t> &out,
								  size_t &pendingAfterCR);
	static bool nodeContainsByte(const Node *n, char c);

  public:
	static constexpr size_t kMaxLeafBytes = 128;

	TextBuffer();
	TextBuffer(const TextBuffer &) = default;
	TextBuffer &operator=(const TextBuffer &) = default;
	TextBuffer(TextBuffer &&) noexcept = default;
	TextBuffer &operator=(TextBuffer &&) noexcept = default;

	size_t size() const;
	int lineCount() const;
	int lineLength(int row) const;
	size_t offsetFromRowCol(int row, int col) const;
	void rowColFromOffset(size_t off, int &row, int &col) const;

	std::string line(int row) const;
	void lineInto(int row, std::string &out) const;
	std::string str() const;
	void copyBytes(size_t off, size_t len, char *out) const;

	void lineStarts(std::vector<uint32_t> &out) const;
	bool containsByte(char c) const;

	class Snapshot
	{
	  public:
		Snapshot() = default;
		size_t size() const;
		int lineCount() const;
		void copyBytes(size_t off, size_t len, char *out) const;
		std::string str() const;
		// Full rope walk — prefer off the UI thread for large documents.
		void lineStarts(std::vector<uint32_t> &out) const;

	  private:
		friend class TextBuffer;
		std::shared_ptr<const Node> root;
		explicit Snapshot(std::shared_ptr<const Node> nodes) : root(std::move(nodes)) {}
	};

	Snapshot snapshot() const;
};
