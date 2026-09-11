/*
	File: host/qt/git_graph_view.h
	Description: Commit-graph rendering component for the git panel — the
	classic git flow-chart (colored lanes, nodes, fork curves) painted by an
	item delegate over QTreeWidget rows, plus the lane-assignment algorithm
	(first parent stays in-lane, forks take free lanes). Panel-owned data
	flows in through setRows(); row clicks report back through a callback.
*/

#pragma once

#include "../../../util/settings.h"

#include <QAbstractItemView>
#include <QStyledItemDelegate>
#include <QVector>

#include <functional>
#include <string>
#include <vector>

class QHelpEvent;

namespace GitGraph {

// Minimal commit shape the lane algorithm needs (the panel maps its
// GitRepo::Commit list onto this — keeps the view decoupled from libgit2).
struct Commit
{
	std::string id;
	std::vector<std::string> parentIds;
};

// Per-row graph geometry computed next to the commit list.
struct Row
{
	int nodeLane = 0;
	QVector<int> parentLanes; // lane each parent continues on
	QVector<int> activeLanes; // lanes crossing this row (verticals)
};

QVector<Row> computeLanes(const std::vector<Commit> &commits);

// Paints the flow-chart gutter + commit summary for each history row.
// Hover highlight is built in (the view's `entered` signal drives it via
// setHoverRow); dwell tooltips are suppressed — clicks report through
// setOnActivate (the panel opens the commit modal).
class Delegate : public QStyledItemDelegate
{
  public:
	Delegate(const Settings &settings, QObject *parent);

	void setRows(QVector<Row> rows);
	void setView(QAbstractItemView *view) { m_view = view; } // hover repaints
	void setHoverRow(int row);
	void setOnActivate(std::function<void(const QString &)> fn);

  protected:
	bool helpEvent(QHelpEvent *event,
				   QAbstractItemView *view,
				   const QStyleOptionViewItem &option,
				   const QModelIndex &index) override;
	void paint(QPainter *p,
			   const QStyleOptionViewItem &option,
			   const QModelIndex &index) const override;

  private:
	const Settings *m_settings;
	QVector<Row> m_rows;
	int m_hoverRow = -1;
	QAbstractItemView *m_view = nullptr;
	std::function<void(const QString &)> m_onActivate;
};

} // namespace GitGraph
