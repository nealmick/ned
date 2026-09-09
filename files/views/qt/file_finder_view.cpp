#include "file_finder_view.h"

#include "host/qt/theme.h"

#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QTimer>
#include <QVBoxLayout>

#include <memory>

// Scan + matching: the shared backend-neutral core (files/file_finder.cpp
// scanWorkspaceFiles + files/file_finder_match.h) — same behavior as the
// ImGui finder.

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

	// Scan runs in the CORE (scanWorkspaceFiles in files/file_finder.cpp):
	// skip list, file cap and entry format are shared with the ImGui
	// finder. This view only owns the thread handle + result handoff.
	// shared_ptr: the worker fills it, the UI-thread poller consumes it —
	// if the dialog is destroyed before the scan finishes, whichever side
	// unwinds last still frees it (a raw new here leaked on early close).
	auto collected = std::make_shared<std::vector<FileEntry>>();
	std::string rootDir = workspace.toStdString();
	std::thread worker([this, collected, rootDir] {
		*collected = scanWorkspaceFiles(rootDir, stopScan);
		scanDone = true;
	});
	scanThread = std::move(worker);

	// Fold finished scan results in when control returns to the event loop.
	auto *poller = new QObject(this);
	QTimer *timer = new QTimer(poller);
	connect(timer, &QTimer::timeout, this, [this, collected, poller, timer] {
		if (!scanDone.load())
			return;
		allFiles = collected;
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

	if (!allFiles)
		return;

	// Rank through the shared matcher (files/file_finder_match.h) so the
	// Qt and ImGui finders return the same results for the same query.
	for (const FileEntry &file : FileFinderMatch::filterFiles(*allFiles, query, 50))
	{
		const QString rel = QString::fromStdString(file.relativePath);
		auto *item = new QListWidgetItem(rel.section('/', -1) + "  —  " + rel);
		// fullPath from the core scan — workspace may carry a trailing
		// slash, so the raw concat would miss openPath's existing-tab dedup
		// (exact string compare).
		item->setData(Qt::UserRole, QString::fromStdString(file.fullPath));
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
			if (results->count() == 0)
				return true;
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
