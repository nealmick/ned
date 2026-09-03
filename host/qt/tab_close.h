/*
	File: host/qt/tab_close.h
	Description: The ✕ tool button shown on the ACTIVE tab only — shared
	by the editor tab groups (Workbench) and the terminal panel so the
	size, sheet and hover stay identical. Header-only: the terminal panel
	compiles in the qtermwidget6 target (QT_NO_KEYWORDS boundary), so no
	plain .cpp may be shared across it and ned_qt.
*/

#pragma once

#include <QApplication>
#include <QTabBar>
#include <QToolButton>

#include <algorithm>
#include <functional>

// Chrome scale vs the 13pt base profile — the app font zooms, the tab
// chrome follows (13pt is where the px constants were tuned).
inline qreal nedChromeScale()
{
	const int pt = qApp->font().pointSize();
	return pt > 0 ? pt / 13.0 : 1.0;
}

inline QToolButton *makeTabCloseButton(QWidget *owner, std::function<void()> onActivate)
{
	// FIXED size + own sheet: until polish runs, the app sheet's
	// font/padding produce a bigger size hint that inflated the pill for
	// one layout pass (the "double height" flash on tab open). A fixed
	// hint can't vary between frames.
	const qreal z = nedChromeScale();
	auto *close = new QToolButton(owner);
	close->setText(QStringLiteral("✕"));
	close->setFixedSize(std::max(18, qRound(18 * z)), std::max(16, qRound(16 * z)));
	close->setStyleSheet(QString("padding: 0; border: none; background: transparent; "
								 "font-size: %1px;")
							 .arg(std::max(9, qRound(11 * z))));
	close->setFocusPolicy(Qt::NoFocus);
	close->setAutoRaise(true);
	close->setCursor(Qt::PointingHandCursor);
	QObject::connect(
		close, &QToolButton::clicked, owner, [onActivate]() { onActivate(); });
	return close;
}

// Refresh the per-active-tab close button on a tab bar: the current
// index gets a fresh button, every other tab's is cleared. QTabBar
// owns the widgets (replaced/removed ones are deleteLater'd by it).
inline void
updateTabCloseButtons(QTabBar *bar, int current, std::function<void(int)> onClose)
{
	for (int i = 0; i < bar->count(); ++i)
	{
		if (i != current)
		{
			bar->setTabButton(i, QTabBar::RightSide, nullptr);
			continue;
		}
		bar->setTabButton(
			i, QTabBar::RightSide, makeTabCloseButton(bar->parentWidget(), [onClose, i] {
				onClose(i);
			}));
	}
}
