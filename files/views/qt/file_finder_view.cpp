#include "file_finder_view.h"

#include "host/qt/theme.h"

#include <QDir>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QTimer>
#include <QVBoxLayout>

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <memory>

namespace fs = std::filesystem;

// Scan rules + fuzzy matching: the shared backend-neutral model
// (files/file_finder_match.h) — same behavior as the ImGui finder.

FileFinderView::FileFinderView(const QString &root, QWidget *parent)
	: QDialog(parent, Qt::Popup), workspace(root)
{
	setObjectName("nedFileFinder");
	// Translucent window so the stylesheet radius rounds the popup corners.
	// QSS backgrounds never render on the translucent top-level itself, so
	// the opaque card is a CHILD widget (HoverTooltip's pattern):
	// WA_StyledBackground + local rule with the opaque raised tone —
	// palette(window) would carry the window's opacity alpha.
	setAttribute(Qt::WA_TranslucentBackground);
	auto *outer = new QVBoxLayout(this);
	outer->setContentsMargins(0, 0, 0, 0);
	auto *card = new QWidget(this);
	card->setAttribute(Qt::WA_StyledBackground, true);
	// Translucent popup + opaque card child (see NedQtTheme::popoverCardSheet).
	card->setObjectName("nedPopoverCard");
	card->setStyleSheet(NedQtTheme::popoverCardSheet());
	outer->addWidget(card);

	auto *layout = new QVBoxLayout(card);
	layout->setContentsMargins(10, 10, 10, 10);
	layout->setSpacing(6);

	// Palette title (VS Code quick-open style): centered header strip above
	// the search box.
	auto *title = new QLabel("File Finder", card);
	title->setObjectName("finderTitle");
	title->setAlignment(Qt::AlignHCenter);
	title->setStyleSheet("#finderTitle { font-weight: 600; }");
	layout->addWidget(title);

	input = new QLineEdit(card);
	input->setPlaceholderText("Find File");
	input->setFocus();
	layout->addWidget(input);

	results = new QListWidget(card);
	results->setMinimumSize(520, 260);
	layout->addWidget(results);

	input->installEventFilter(this);
	connect(input, &QLineEdit::textChanged, this, &FileFinderView::refilter);
	connect(results, &QListWidget::itemDoubleClicked, this, [this](QListWidgetItem *item) {
		Q_EMIT fileSelected(item->data(Qt::UserRole).toString());
		accept();
	});

	restartScan();
}

FileFinderView::~FileFinderView()
{
	stopScan = true;
	if (scanThread.joinable())
		scanThread.join();
}

void FileFinderView::restartScan()
{
	if (scanThread.joinable())
	{
		stopScan = true;
		scanThread.join();
	}
	stopScan = false;
	scanDone = false;

	// shared_ptr: the worker fills it, the UI-thread poller consumes it —
	// if the dialog is destroyed before the scan finishes, whichever side
	// unwinds last still frees it (a raw new here leaked on early close).
	auto collected = std::make_shared<QStringList>();
	QString rootDir = workspace;
	std::thread worker([this, collected, rootDir] {
		QStringList local;
		std::error_code ec;
		fs::recursive_directory_iterator it(
			rootDir.toStdString(), fs::directory_options::skip_permission_denied, ec);
		fs::recursive_directory_iterator end;
		while (!stopScan && it != end)
		{
			const fs::path p = it->path();
			if (it->is_directory(ec))
			{
				if (FileFinderMatch::shouldSkipDir(p.filename().string()))
				{
					it.disable_recursion_pending();
				}
			} else if (it->is_regular_file(ec) && local.size() < 20000)
			{
				const QString rel = QString::fromStdString(
					fs::relative(p, rootDir.toStdString(), ec).string());
				if (!rel.isEmpty())
					local.append(rel);
			}
			it.increment(ec);
			if (ec)
				break;
		}
		*collected = std::move(local);
		scanDone = true;
	});
	scanThread = std::move(worker);

	// Fold finished scan results in when control returns to the event loop.
	auto *poller = new QObject(this);
	QTimer *timer = new QTimer(poller);
	connect(timer, &QTimer::timeout, this, [this, collected, poller, timer] {
		if (!scanDone.load())
			return;
		allFiles = *collected;
		timer->stop();
		poller->deleteLater();
		refilter();
	});
	timer->start(50);
}

void FileFinderView::refilter()
{
	results->clear();
	const std::string query = input->text().toLower().toStdString();

	// Rank through the shared matcher (files/file_finder_match.h) so the
	// Qt and ImGui finders return the same results for the same query.
	std::vector<FileEntry> entries;
	entries.reserve(static_cast<size_t>(allFiles.size()));
	const auto lowerOf = [](std::string s) {
		std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) {
			return static_cast<char>(std::tolower(c));
		});
		return s;
	};
	for (const QString &rel : allFiles)
	{
		std::string path = rel.toStdString();
		// filenameLower drives the dotfile-hide rule — the FILE name, not
		// the whole path ("src/.env" is a dotfile too).
		entries.push_back(FileEntry{
			path, path, lowerOf(path), lowerOf(rel.section('/', -1).toStdString())});
	}

	for (const FileEntry &file : FileFinderMatch::filterFiles(entries, query, 50))
	{
		const QString rel = QString::fromStdString(file.relativePath);
		auto *item = new QListWidgetItem(rel.section('/', -1) + "  —  " + rel);
		// cleanPath: workspace may carry a trailing slash — the raw concat
		// would miss openPath's existing-tab dedup (exact string compare).
		item->setData(Qt::UserRole, QDir::cleanPath(workspace + "/" + rel));
		item->setToolTip(rel);
		results->addItem(item);
		if (results->count() == 1)
			results->setCurrentItem(item);
	}
}

bool FileFinderView::eventFilter(QObject *watched, QEvent *event)
{
	// Up/Down in the input move the list selection (no Tab needed).
	if (watched == input && event->type() == QEvent::KeyPress)
	{
		auto *key = static_cast<QKeyEvent *>(event);
		if (key->key() == Qt::Key_Down || key->key() == Qt::Key_Up)
		{
			const int dir = key->key() == Qt::Key_Down ? 1 : -1;
			results->setCurrentRow(
				std::clamp(results->currentRow() + dir, 0, results->count() - 1));
			return true;
		}
	}
	return QDialog::eventFilter(watched, event);
}

void FileFinderView::keyPressEvent(QKeyEvent *event)
{
	switch (event->key())
	{
	case Qt::Key_Escape:
		reject();
		return;
	case Qt::Key_Return:
	case Qt::Key_Enter:
		if (QListWidgetItem *item = results->currentItem())
		{
			Q_EMIT fileSelected(item->data(Qt::UserRole).toString());
			accept();
		}
		return;
	case Qt::Key_Up:
		results->setCurrentRow(std::max(0, results->currentRow() - 1));
		return;
	case Qt::Key_Down:
		results->setCurrentRow(std::min(results->count() - 1, results->currentRow() + 1));
		return;
	default:
		break;
	}
	QDialog::keyPressEvent(event);
}
