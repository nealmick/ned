/*
	File: host/qt/git_graph_view.cpp
	Description: see git_graph_view.h.
*/

#include "git_graph_view.h"

#include "theme.h"

#include <QHelpEvent>
#include <QPainter>
#include <QPainterPath>

#include <algorithm>

namespace {

constexpr int kLaneSpacing = 14; // px between graph lanes
constexpr int kGraphGutter = 8;	 // left inset before lane 0
constexpr qreal kNodeRadius = 3.2;

// Lane palette (VSCode-ish rotation).
const QColor kLaneColors[] = {
	QColor("#3fb950"),
	QColor("#58a6ff"),
	QColor("#d29922"),
	QColor("#f778ba"),
	QColor("#a371f7"),
	QColor("#39c5cf"),
	QColor("#7ee787"),
	QColor("#ff9bce"),
};

QColor laneColor(int lane)
{
	return kLaneColors[lane % (sizeof(kLaneColors) / sizeof(kLaneColors[0]))];
}

} // namespace

namespace GitGraph {

QVector<Row> computeLanes(const std::vector<Commit> &commits)
{
	// lanes[i] = commit id expected next on lane i ("" = free).
	std::vector<std::string> lanes;
	QVector<Row> rows;
	rows.reserve(commits.size());

	auto findLane = [&](const std::string &id) {
		for (int i = 0; i < (int)lanes.size(); ++i)
			if (lanes[i] == id)
				return i;
		return -1;
	};
	auto freeLane = [&]() {
		for (int i = 0; i < (int)lanes.size(); ++i)
			if (lanes[i].empty())
				return i;
		lanes.push_back({});
		return (int)lanes.size() - 1;
	};

	for (const Commit &c : commits)
	{
		Row row;
		row.nodeLane = findLane(c.id);
		if (row.nodeLane < 0)
			row.nodeLane = freeLane(); // branch head appears on a fresh lane

		// First parent continues on the node's lane; extra parents fork
		// onto free lanes (the classic merge split).
		if (!c.parentIds.empty())
		{
			int p0 = findLane(c.parentIds[0]);
			if (p0 < 0)
				p0 = row.nodeLane;
			row.parentLanes.push_back(p0);
			for (size_t i = 1; i < c.parentIds.size(); ++i)
			{
				int pl = findLane(c.parentIds[i]);
				if (pl < 0)
					pl = freeLane();
				row.parentLanes.push_back(pl);
			}
		}

		// Active lanes at this row: everything currently placed, plus the
		// node's own lane — it must run DOWN from the node to its parent,
		// and for the head row nothing was "placed" yet, which left the
		// first node floating with no line (the disconnected-head bug).
		for (int i = 0; i < (int)lanes.size(); ++i)
			if (!lanes[i].empty() || i == row.nodeLane)
				row.activeLanes.push_back(i);

		// Advance: the node's lane now expects its first parent (or frees).
		if (!c.parentIds.empty())
		{
			lanes[row.nodeLane] = c.parentIds[0];
			for (size_t i = 1; i < c.parentIds.size(); ++i)
				lanes[row.parentLanes[i]] = c.parentIds[i];
		} else
			lanes[row.nodeLane].clear();

		rows.push_back(row);
	}
	return rows;
}

Delegate::Delegate(const Settings &settings, QObject *parent)
	: QStyledItemDelegate(parent), m_settings(&settings)
{
}

void Delegate::setRows(QVector<Row> rows) { m_rows = std::move(rows); }

void Delegate::setHoverRow(int row)
{
	if (row == m_hoverRow)
		return;
	m_hoverRow = row;
	if (m_view)
		m_view->viewport()->update();
}

void Delegate::setOnActivate(std::function<void(const QString &)> fn)
{
	m_onActivate = std::move(fn);
}

bool Delegate::helpEvent(QHelpEvent *event,
						 QAbstractItemView *view,
						 const QStyleOptionViewItem &option,
						 const QModelIndex &index)
{
	// No hover popup (click opens the modal) — swallow the dwell events
	// so no native QToolTip appears over the rows.
	Q_UNUSED(view);
	Q_UNUSED(option);
	Q_UNUSED(index);
	return event->type() == QEvent::ToolTip
			   ? true
			   : QStyledItemDelegate::helpEvent(event, view, option, index);
}

void Delegate::paint(QPainter *p,
					 const QStyleOptionViewItem &option,
					 const QModelIndex &index) const
{
	const int row = index.row();
	if (row < 0 || row >= m_rows.size())
		return QStyledItemDelegate::paint(p, option, index);

	const Row &g = m_rows[row];
	const QRect r = option.rect;
	// Hover highlight under everything else (VSCode list-hover feel).
	if (row == m_hoverRow)
	{
		QColor fill = NedQtTheme::text(*m_settings);
		fill.setAlpha(18);
		p->fillRect(r, fill);
	}
	p->save();
	p->setRenderHint(QPainter::Antialiasing);
	p->setClipRect(r);

	const auto laneX = [&](int lane) {
		return kGraphGutter + lane * kLaneSpacing + kLaneSpacing / 2.0;
	};
	const qreal cy = r.center().y();

	// Verticals: every lane crossing this row.
	for (int lane : g.activeLanes)
	{
		p->setPen(QPen(laneColor(lane), 1.6, Qt::SolidLine, Qt::RoundCap));
		p->drawLine(QPointF(laneX(lane), r.top()), QPointF(laneX(lane), r.bottom()));
	}
	// Fork/merge curves: node lane bottom → each parent lane.
	for (int parentLane : g.parentLanes)
	{
		if (parentLane == g.nodeLane)
			continue; // straight continuation, the vertical covers it
		p->setPen(QPen(laneColor(parentLane), 1.6, Qt::SolidLine, Qt::RoundCap));
		QPainterPath path;
		path.moveTo(laneX(g.nodeLane), cy);
		path.quadTo(QPointF(laneX(g.nodeLane), r.bottom()),
					QPointF(laneX(parentLane), r.bottom()));
		p->drawPath(path);
	}
	// Node.
	p->setPen(Qt::NoPen);
	p->setBrush(laneColor(g.nodeLane));
	p->drawEllipse(QPointF(laneX(g.nodeLane), cy), kNodeRadius, kNodeRadius);
	p->setPen(QPen(NedQtTheme::background(*m_settings), 1.4));
	p->drawEllipse(QPointF(laneX(g.nodeLane), cy), kNodeRadius, kNodeRadius);

	// Summary + dim short sha.
	const QString text = index.data(Qt::DisplayRole).toString();
	const QString sha = index.data(Qt::UserRole).toString();
	QFont f = option.font;
	f.setPointSizeF(f.pointSizeF() * 0.92);
	p->setFont(f);
	// Per-row text column: deep rows carry more lanes so sit further
	// right (the original proportions).
	const int rowMaxLane =
		g.activeLanes.isEmpty()
			? 0
			: *std::max_element(g.activeLanes.begin(), g.activeLanes.end());
	const int textX = kGraphGutter + (rowMaxLane + 1) * kLaneSpacing + 6;
	const QRect textRect(textX, r.top(), r.right() - textX, r.height());
	const QFontMetrics fm(f);
	const QString shaText = fm.elidedText(sha, Qt::ElideRight, 52);
	p->setPen(NedQtTheme::text(*m_settings));
	p->drawText(textRect,
				Qt::AlignVCenter | Qt::AlignLeft,
				fm.elidedText(text, Qt::ElideRight, textRect.width() - 58));
	QColor dim = NedQtTheme::text(*m_settings);
	dim.setAlpha(120);
	p->setPen(dim);
	p->drawText(textRect, Qt::AlignVCenter | Qt::AlignRight, shaText);
	p->restore();
}

} // namespace GitGraph
