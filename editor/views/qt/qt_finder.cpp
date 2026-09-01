#include "qt_finder.h"

#include <QKeyEvent>
#include <QLineEdit>
#include <QTimer>
#include <QListWidget>
#include <QVBoxLayout>

#include <filesystem>
#include <algorithm>

namespace fs = std::filesystem;

int fuzzyMatchScore(const QString &candidate, const QString &query)
{
	if (query.isEmpty())
		return 0;
	int score = 0;
	int ci = 0;
	int prevHit = -2;
	for (int qi = 0; qi < query.size(); ++qi)
	{
		const QChar qc = query[qi];
		int hit = -1;
		for (; ci < candidate.size(); ++ci)
		{
			if (candidate[ci].toLower() == qc)
			{
				hit = ci;
				break;
			}
		}
		if (hit < 0)
			return -1;
		// Consecutive matches and matches after a separator score better.
		score += (hit == prevHit + 1) ? 3 : 1;
		if (hit == 0 || candidate[hit - 1] == '/' || candidate[hit - 1] == '_' ||
			candidate[hit - 1] == '-')
			score += 2;
		prevHit = hit;
		ci = hit + 1;
	}
	return score;
}

namespace {
bool shouldSkipDir(const QString &name)
{
	return name == ".git" || name == ".build" || name == ".build-qt" ||
		   name == ".build-min" || name == "build" || name == "node_modules" ||
		   name == "dist" || name == ".cache";
}
} // namespace

QtFileFinder::QtFileFinder(const QString &root, QWidget *parent)
	: QDialog(parent, Qt::Popup), workspace(root)
{
	setModal(true);
	auto *layout = new QVBoxLayout(this);
	layout->setContentsMargins(6, 6, 6, 6);

	input = new QLineEdit(this);
	input->setPlaceholderText("Find File");
	input->setFocus();
	layout->addWidget(input);

	results = new QListWidget(this);
	results->setMinimumSize(520, 260);
	layout->addWidget(results);

	connect(input, &QLineEdit::textChanged, this, &QtFileFinder::refilter);
	connect(results, &QListWidget::itemDoubleClicked, this, [this](QListWidgetItem *item) {
		Q_EMIT fileSelected(item->data(Qt::UserRole).toString());
		accept();
	});

	restartScan();
}

QtFileFinder::~QtFileFinder()
{
	stopScan = true;
	if (scanThread.joinable())
		scanThread.join();
}

void QtFileFinder::restartScan()
{
	if (scanThread.joinable())
	{
		stopScan = true;
		scanThread.join();
	}
	stopScan = false;
	scanDone = false;

	QStringList *collected = new QStringList();
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
				if (shouldSkipDir(QString::fromStdString(p.filename().string())))
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
		delete collected;
		timer->stop();
		poller->deleteLater();
		refilter();
	});
	timer->start(50);
}

void QtFileFinder::refilter()
{
	results->clear();
	const QString query = input->text();

	std::vector<std::pair<int, QString>> ranked;
	for (const QString &rel : allFiles)
	{
		const int score = fuzzyMatchScore(rel, query);
		if (score >= 0)
			ranked.emplace_back(score, rel);
	}
	std::sort(ranked.begin(), ranked.end(),
			  [](const auto &a, const auto &b) { return a.first > b.first; });

	int shown = 0;
	for (const auto &[score, rel] : ranked)
	{
		if (shown++ >= 50)
			break;
		auto *item = new QListWidgetItem(
			QString::fromStdString(fs::path(rel.toStdString()).filename().string()) +
			"  —  " + rel);
		item->setData(Qt::UserRole,
					  workspace + "/" + rel);
		item->setToolTip(rel);
		results->addItem(item);
		if (results->count() == 1)
			results->setCurrentItem(item);
	}
}

void QtFileFinder::keyPressEvent(QKeyEvent *event)
{
	switch (event->key())
	{
	case Qt::Key_Escape: reject(); return;
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
	default: break;
	}
	QDialog::keyPressEvent(event);
}
