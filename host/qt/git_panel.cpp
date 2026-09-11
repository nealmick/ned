/*
	File: host/qt/git_panel.cpp
	Description: see git_panel.h. The graph rows are painted by a delegate
	over QTreeWidget rows — lane assignment is the simple first-parent-
	stays-in-lane algorithm with forks taking free lanes.
*/

#include "git_panel.h"

#include "git_graph_view.h"

#include "qt_icons.h"
#include "theme.h"
#include "workbench.h"

#include <QBuffer>
#include <QFileInfo>
#include <QGuiApplication>
#include <QHeaderView>
#include <QHelpEvent>
#include <QKeyEvent>
#include <QLabel>
#include <QPainter>
#include <QPainterPath>
#include <QStyledItemDelegate>
#include <QTimer>
#include <QTreeWidget>
#include <QVBoxLayout>

#include <algorithm>

#if !NED_ENABLE_GIT
GitPanel::GitPanel(const Settings &, QWidget *parent) : QWidget(parent)
{
	setObjectName(QStringLiteral("GitPanel"));
	auto *lay = new QVBoxLayout(this);
	auto *off = new QLabel(QObject::tr("Git support is disabled in this build."), this);
	off->setAlignment(Qt::AlignCenter);
	lay->addWidget(off);
}

GitPanel::~GitPanel() {}
void GitPanel::setWorkspaceRoot(const QString &) {}
void GitPanel::paintEvent(QPaintEvent *event) { QWidget::paintEvent(event); }
#else

namespace {
constexpr int kHistoryLimit = 200; // commits in the graph
}

GitPanel::GitPanel(const Settings &settings, QWidget *parent)
	: QWidget(parent), m_settings(&settings)
{
	setObjectName(QStringLiteral("GitPanel"));

	m_splitter = new NedSplitter(Qt::Vertical, this);
	m_splitter->setContentsMargins(0, 0, 0, 0);
	m_splitter->setChildrenCollapsible(false);
	m_splitter->setHandleWidth(1); // hairline like the editor splits

	auto sectionHeader = [&](const QString &text) {
		auto *l = new QLabel(text, this);
		l->setObjectName(QStringLiteral("GitSectionHeader"));
		return l;
	};
	// Clickable change lists: the full repo-relative path rides in
	// UserRole (staged/unstaged know their side by which list they're in).
	auto fileList = [&](bool staged) {
		auto *list = new QTreeWidget(this);
		// Hover highlight via the app stylesheet (see theme.cpp) — same
		// feel as the graph rows.
		list->setObjectName(QStringLiteral("GitChangeList"));
		list->setRootIsDecorated(false);
		list->setUniformRowHeights(true);
		list->setSelectionMode(QAbstractItemView::NoSelection);
		list->setFocusPolicy(Qt::NoFocus);
		list->header()->hide();
		list->header()->setSectionResizeMode(0, QHeaderView::Stretch);
		list->setCursor(Qt::PointingHandCursor);
		connect(
			list, &QTreeWidget::itemClicked, this, [this, staged](QTreeWidgetItem *item) {
				const QString path = item->data(0, Qt::UserRole).toString();
				if (!path.isEmpty())
					Q_EMIT diffRequested(path, staged);
			});
		return list;
	};

	auto section = [&](const QString &title, QWidget *body) {
		auto *group = new QWidget(this);
		auto *lay = new QVBoxLayout(group);
		lay->setContentsMargins(0, 4, 0, 0);
		lay->setSpacing(0);
		lay->addWidget(sectionHeader(title));
		lay->addWidget(body, 1);
		return group;
	};

	// ---- Changes: Staged / Unstaged in their own resizable split --------
	m_stagedList = fileList(true);
	m_unstagedList = fileList(false);
	m_stagedHeader = sectionHeader(QObject::tr("Staged"));
	m_unstagedHeader = sectionHeader(QObject::tr("Unstaged"));
	auto listGroup = [&](QLabel *header, QTreeWidget *list) {
		auto *group = new QWidget(this);
		auto *lay = new QVBoxLayout(group);
		lay->setContentsMargins(0, 0, 0, 0);
		lay->setSpacing(0);
		lay->addWidget(header);
		lay->addWidget(list, 1);
		return group;
	};
	auto *changesSplit = new NedSplitter(Qt::Vertical, this);
	changesSplit->setContentsMargins(0, 0, 0, 0);
	changesSplit->setChildrenCollapsible(false);
	changesSplit->setHandleWidth(1); // resize seam between Staged and Unstaged
	changesSplit->addWidget(listGroup(m_stagedHeader, m_stagedList));
	changesSplit->addWidget(listGroup(m_unstagedHeader, m_unstagedList));
	changesSplit->setStretchFactor(0, 1);
	changesSplit->setStretchFactor(1, 1);

	// ---- Graph ------------------------------------------------------------
	// Graph list is NOT one of the change lists: clicks open the commit
	// modal (never openDiffView — an early bug routed them to diff tabs
	// with the commit sha as the "path").
	m_graph = new QTreeWidget(this);
	m_graph->setRootIsDecorated(false);
	m_graph->setUniformRowHeights(true);
	m_graph->setSelectionMode(QAbstractItemView::NoSelection);
	m_graph->setFocusPolicy(Qt::NoFocus);
	m_graph->header()->hide();
	m_graph->header()->setSectionResizeMode(0, QHeaderView::Stretch);
	auto *delegate = new GitGraph::Delegate(settings, m_graph);
	m_graph->setItemDelegate(delegate);
	delegate->setView(m_graph);
	m_graph->setMouseTracking(true); // `entered` drives the row highlight
	connect(m_graph, &QTreeWidget::itemClicked, this, [this](QTreeWidgetItem *item) {
		const QString id = item->data(0, Qt::UserRole + 1).toString();
		if (!id.isEmpty())
			onCommitActivated(id);
	});
	connect(
		m_graph, &QAbstractItemView::entered, this, [delegate](const QModelIndex &idx) {
			delegate->setHoverRow(idx.row());
		});
	// Clear the row highlight when the cursor leaves / the list scrolls.
	m_graph->viewport()->installEventFilter(this);
#if NED_ENABLE_GIT
	m_setHoverRow = [delegate](int row) { delegate->setHoverRow(row); };
#endif

	// ---- Outer split: collapsible CHANGES / GRAPH sections ---------------
	m_splitter->addWidget(section(QObject::tr("CHANGES"), changesSplit));
	m_splitter->addWidget(section(QObject::tr("GRAPH"), m_graph));
	m_splitter->setStretchFactor(0, 1);
	m_splitter->setStretchFactor(1, 2);

	auto *lay = new QVBoxLayout(this);
	lay->setContentsMargins(0, 0, 0, 0);
	lay->setSpacing(0);
	lay->addWidget(m_splitter);

	// Poll: kicks the worker when idle, adopts finished payloads. 3s keeps
	// external staging/committing reflected without hammering the repo.
	m_tick = new QTimer(this);
	m_tick->setInterval(3000);
	connect(m_tick, &QTimer::timeout, m_tick, [this] {
		kickScan();
		applyResults();
	});
	m_tick->start();
}

GitPanel::~GitPanel()
{
#if NED_ENABLE_GIT
	m_shuttingDown = true;
	if (m_worker.joinable())
		m_worker.join();
	if (m_detailWorker.joinable())
		m_detailWorker.join();
#endif
}

void GitPanel::setWorkspaceRoot(const QString &root)
{
#if NED_ENABLE_GIT
	if (m_root == root)
		return;
	m_root = root;
	kickScan();
#else
	Q_UNUSED(root);
#endif
}

void GitPanel::kickScan()
{
#if NED_ENABLE_GIT
	if (m_inFlight.exchange(true))
		return; // worker already running
	if (m_worker.joinable())
		m_worker.join();
	const QString root = m_root;
	m_worker = std::thread([this, root] {
		if (!m_shuttingDown)
		{
			Payload p;
			if (root.isEmpty())
			{
				m_repo.close();
			} else
			{
				m_repo.open(
					root.toStdString()); // reopens per scan: cheap, picks up HEAD moves
				if (m_repo.isOpen())
				{
					m_repo.statusGroups(p.staged, p.unstaged);
					p.commits = m_repo.history(kHistoryLimit);
				}
			}
			std::lock_guard<std::mutex> lock(m_mu);
			m_pending = std::move(p);
			m_pending.fresh = true;
		}
		m_inFlight = false;
	});
#endif
}

void GitPanel::applyResults()
{
#if NED_ENABLE_GIT
	Payload p;
	{
		std::lock_guard<std::mutex> lock(m_mu);
		if (!m_pending.fresh)
			return;
		p = std::move(m_pending);
		m_pending.fresh = false;
	}

	// Unchanged state (the common 3s-tick case): keep everything as-is —
	// repopulating identical lists just flickers the view and resets
	// scroll positions.
	if (p.staged == m_lastStaged && p.unstaged == m_lastUnstaged &&
		p.commits.size() == m_lastCommitCount)
		return;
	m_lastStaged = p.staged;
	m_lastUnstaged = p.unstaged;
	m_lastCommitCount = p.commits.size();

	// State DID change: rebuild the lists (details cache stays — commits
	// are immutable).
	if (m_setHoverRow)
		m_setHoverRow(-1);

	auto fill = [](QTreeWidget *list, const std::vector<std::string> &paths) {
		list->clear();
		for (const std::string &path : paths)
		{
			const QString qpath = QString::fromStdString(path);
			auto *item = new QTreeWidgetItem(list);
			item->setText(0, QFileInfo(qpath).fileName());
			item->setToolTip(0, qpath);
			item->setData(0, Qt::UserRole, qpath);
			const QIcon icon = QtIconSet::forFile(qpath, 16);
			if (!icon.isNull())
				item->setIcon(0, icon);
		}
	};
	fill(m_stagedList, p.staged);
	fill(m_unstagedList, p.unstaged);
	m_stagedHeader->setText(QObject::tr("Staged (%1)").arg(p.staged.size()));
	m_unstagedHeader->setText(QObject::tr("Unstaged (%1)").arg(p.unstaged.size()));

	m_graph->clear();
	auto *delegate = static_cast<GitGraph::Delegate *>(m_graph->itemDelegate());
	std::vector<GitGraph::Commit> graphCommits;
	graphCommits.reserve(p.commits.size());
	for (const GitRepo::Commit &c : p.commits)
		graphCommits.push_back({c.id, c.parentIds});
	delegate->setRows(GitGraph::computeLanes(graphCommits));
	for (const GitRepo::Commit &c : p.commits)
	{
		auto *item = new QTreeWidgetItem(m_graph);
		item->setText(0, QString::fromStdString(c.summary));
		item->setData(0, Qt::UserRole, QString::fromStdString(c.shortId));
		item->setData(0, Qt::UserRole + 1, QString::fromStdString(c.id));
	}
#endif
}

void GitPanel::onCommitActivated(const QString &fullId)
{
#if NED_ENABLE_GIT
	m_modalRequestId = fullId;
	if (m_details.find(fullId) != m_details.end())
	{
		openCommitModal(fullId);
		return;
	}
	if (m_detailInFlight.exchange(true))
		return; // a fetch is already out; it will open the modal
	if (m_detailWorker.joinable())
		m_detailWorker.join();
	const QString root = m_root;
	m_detailWorker = std::thread([this, fullId, root] {
		// OWN repo handle — never the scan worker's m_repo.
		if (!m_detailRepo.isOpen() && !root.isEmpty())
			m_detailRepo.open(root.toStdString());
		std::string message;
		std::vector<GitRepo::FileStat> files;
		const bool ok = m_detailRepo.isOpen() &&
						m_detailRepo.commitDetails(fullId.toStdString(), message, files);
		if (m_remoteUrl.isEmpty())
		{
			std::string url;
			if (m_detailRepo.isOpen() && m_detailRepo.remoteUrl(url))
				m_remoteUrl =
					QString::fromStdString(url); // read on UI after the queued hop
		}
		m_detailInFlight = false;
		if (ok && !m_shuttingDown)
			QMetaObject::invokeMethod(
				this,
				[this, fullId, message, files] {
					m_details[fullId] = Details{message, files};
					// Open only if this is still the clicked commit.
					if (m_modalRequestId == fullId)
						openCommitModal(fullId);
				},
				Qt::QueuedConnection);
	});
#endif
}

// Click-modal, the settings-popup pattern exactly: an in-WINDOW card
// child of the main window plus a scrim that dims everything and swallows
// the click that dismisses it (no OS popup window, no positioning games).
void GitPanel::openCommitModal(const QString &fullId)
{
#if NED_ENABLE_GIT
	auto it = m_details.find(fullId);
	if (it == m_details.end())
		return;

	QWidget *host = window();
	if (!host)
		host = this;

	if (!m_commitScrim)
	{
		m_commitScrim = new QWidget(host);
		m_commitScrim->setObjectName(QStringLiteral("GitCommitScrim"));
		m_commitScrim->setAttribute(Qt::WA_StyledBackground, true);
		m_commitScrim->setStyleSheet(
			"#GitCommitScrim { background: rgba(0, 0, 0, 110); }");
		m_commitScrim->installEventFilter(this); // click outside = dismiss
		m_commitCard = new QWidget(host);
		m_commitCard->setObjectName(QStringLiteral("GitCommitCard"));
		m_commitCard->setAttribute(Qt::WA_StyledBackground, true);
		m_commitCard->setFocusPolicy(Qt::StrongFocus); // Esc below
		auto *cardLay = new QVBoxLayout(m_commitCard);
		cardLay->setContentsMargins(14, 14, 14, 14);
		m_commitLabel = new QLabel(m_commitCard);
		m_commitLabel->setTextFormat(Qt::RichText);
		m_commitLabel->setWordWrap(true);
		m_commitLabel->setMaximumWidth(520);
		m_commitLabel->setOpenExternalLinks(true);
		cardLay->addWidget(m_commitLabel);
		host->installEventFilter(this); // window resize → reposition
	}

	// Card restyled on every open: the theme may have changed since the
	// one-time widget creation.
	m_commitCard->setStyleSheet(
		QStringLiteral("#GitCommitCard { background: %1; border: 1px solid rgba(128, "
					   "128, 128, 110); "
					   "border-radius: 6px; } QLabel { background: transparent; }")
			.arg(NedQtTheme::raised(NedQtTheme::background(*m_settings)).name()));
	m_commitLabel->setFont(QApplication::font()); // app font, not the panel's
	m_commitLabel->setText(buildCommitHtml(it->second, fullId));
	m_commitCard->adjustSize();
	repositionCommitModal();
	m_commitScrim->setGeometry(host->rect());
	m_commitScrim->show();
	m_commitScrim->raise();
	m_commitCard->show();
	m_commitCard->raise();
	m_commitCard->setFocus();
#endif
}

void GitPanel::repositionCommitModal()
{
	QWidget *host = window();
	if (!m_commitCard || !host)
		return;
	m_commitCard->move(host->rect().center() -
					   QPoint(m_commitCard->width() / 2, m_commitCard->height() / 2));
}

void GitPanel::closeCommitModal()
{
	if (m_commitScrim)
		m_commitScrim->hide();
	if (m_commitCard)
		m_commitCard->hide();
}

QString GitPanel::buildCommitHtml(const Details &d, const QString &fullId)
{
#if NED_ENABLE_GIT
	if (d.message.empty())
		return {};

	const QColor ink = NedQtTheme::text(*m_settings);
	const QString inkHex = ink.name();
	QColor dim = ink;
	dim.setAlpha(150);
	const QString dimHex = dim.name();

	// Message: first line bold, the rest dimmer below it; the whole body
	// truncates at 300 chars + "…" — long messages would tower over the
	// file list even with the width cap.
	constexpr int kMaxMessageChars = 300;
	QString raw = QString::fromStdString(d.message).trimmed();
	if (raw.size() > kMaxMessageChars)
		raw = raw.left(kMaxMessageChars) + QStringLiteral("…");
	const int cut = raw.indexOf('\n');
	const QString first = (cut < 0 ? raw : raw.left(cut)).toHtmlEscaped();
	QString rest;
	if (cut >= 0)
		rest = raw.mid(cut + 1).trimmed().toHtmlEscaped();

	// File rows: icon (QIcon → base64 PNG; HTML labels can't take QIcon)
	// + name, +N green / -M red right-aligned.
	// 2x raster + explicit width/height attrs = crisp on retina. (A 1x
	// pixmap stretched by the rich-text label is the blurry-file-tree bug
	// from before — same trap.)
	auto fileIconSrc = [](const QString &path) {
		constexpr int px = 11;
		const qreal dpr = qGuiApp->devicePixelRatio();
		QPixmap pm = QtIconSet::forFile(path, px).pixmap(QSize(px, px) * dpr);
		QByteArray bytes;
		QBuffer buf(&bytes);
		if (!pm.save(&buf, "PNG"))
			return QString();
		const QString data = QString::fromLatin1(bytes.toBase64());
		return QStringLiteral(
				   "<img src='data:image/png;base64,%1' width='%2' height='%3'/>")
			.arg(data)
			.arg(px)
			.arg(px);
	};

	QString rows;
	int shown = 0;
	constexpr int kMaxFiles = 10;
	for (const GitRepo::FileStat &f : d.files)
	{
		if (shown++ == kMaxFiles)
		{
			rows += QStringLiteral("<tr><td></td><td colspan=\"2\" style='color:%1'>… %2 "
								   "more files</td></tr>")
						.arg(dimHex)
						.arg(d.files.size() - kMaxFiles);
			break;
		}
		const QString path = QString::fromStdString(f.path);
		const QString name = QFileInfo(path).fileName();
		const QString icon = fileIconSrc(path);
		rows += QStringLiteral(
					"<tr><td width=\"18\" style='vertical-align:middle'>%1</td>"
					"<td style='color:%2; white-space:pre; vertical-align:middle'>%3</td>"
					"<td align=\"right\" style='color:#3fb950; "
					"vertical-align:middle'>&nbsp;+%4</td>"
					"<td align=\"right\" style='color:#f85149; "
					"vertical-align:middle'>&nbsp;−%5</td></tr>")
					.arg(icon, inkHex, name.toHtmlEscaped())
					.arg(f.added)
					.arg(f.deleted);
	}

	QString html = QStringLiteral("<b style='color:%1'>%2</b>").arg(inkHex, first);
	if (!rest.isEmpty())
		html += QStringLiteral("<br/><span style='color:%1; font-size:smaller'>%2</span>")
					.arg(dimHex, rest.simplified());
	// Layout: title / divider / sha / divider / file list / divider / link.
	html += QStringLiteral(
				"<hr style='border:none; border-top:1px solid rgba(255,255,255,0.12)'/>"
				"<span style='color:%1; font-size:smaller'>%2</span>")
				.arg(dimHex, fullId.toHtmlEscaped());
	if (!rows.isEmpty())
		html +=
			QStringLiteral(
				"<hr style='border:none; border-top:1px solid rgba(255,255,255,0.12)'/>"
				"<table cellspacing='0' cellpadding='0' width='100%'>%1</table>")
				.arg(rows);

	// GitHub commit link (bottom row): link-external codicon + anchor.
	if (!m_remoteUrl.isEmpty())
	{
		QString base = m_remoteUrl;
		if (base.startsWith(QStringLiteral("git@github.com:")))
			base = QStringLiteral("https://github.com/") +
				   base.mid(QStringLiteral("git@github.com:").size());
		if (base.endsWith(QStringLiteral(".git")))
			base.chop(4);
		if (base.startsWith(QStringLiteral("https://github.com/")))
			html += QStringLiteral("<hr style='border:none; border-top:1px solid "
								   "rgba(255,255,255,0.12)'/>"
								   "<a href='%1/commit/%2' style='color:#58a6ff; "
								   "text-decoration:none'>%3</a>")
						.arg(base,
							 fullId.toHtmlEscaped(),
							 QObject::tr("View commit on GitHub"));
	}
	return html;
#endif
	return {};
}

bool GitPanel::eventFilter(QObject *watched, QEvent *event)
{
	// Commit modal (settings-popup pattern): scrim click dismisses, Esc
	// dismisses, a window resize re-centers scrim + card.
	if (event->type() == QEvent::MouseButtonPress && watched == m_commitScrim)
		closeCommitModal();
	else if (event->type() == QEvent::KeyPress && watched == m_commitCard)
	{
		if (static_cast<QKeyEvent *>(event)->key() == Qt::Key_Escape)
		{
			closeCommitModal();
			return true;
		}
	} else if (event->type() == QEvent::Resize && watched == window())
	{
		if (m_commitScrim && m_commitScrim->isVisible())
		{
			m_commitScrim->setGeometry(window()->rect());
			repositionCommitModal();
		}
	}
	// Graph row highlight: clear when the cursor leaves or scrolls.
	if (watched == m_graph->viewport() && m_setHoverRow &&
		(event->type() == QEvent::Leave || event->type() == QEvent::Scroll ||
		 event->type() == QEvent::Wheel))
		m_setHoverRow(-1);
	return QWidget::eventFilter(watched, event);
}

void GitPanel::hideEvent(QHideEvent *event)
{
	QWidget::hideEvent(event);
	closeCommitModal();
}

void GitPanel::paintEvent(QPaintEvent *event)
{
	QWidget::paintEvent(event);
	QPainter p(this);
	// Right-edge hairline — the same separator the file tree carries
	// (#FileSidebar's QSS border doesn't render on plain QWidgets).
	p.setPen(QPen(QColor(255, 255, 255, 23), 1.0));
	p.drawLine(QPointF(width() - 0.5, 0), QPointF(width() - 0.5, height()));
}

#endif // NED_ENABLE_GIT
