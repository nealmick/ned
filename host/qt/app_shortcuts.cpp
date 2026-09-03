#include "app_shortcuts.h"

#include "host.h"

#include "editor/views/qt/editor_frame.h"
#include "files/views/qt/file_finder_view.h"
#include "workbench.h"

#include <QFileDialog>
#include <QKeySequence>
#include <QList>
#include <QShortcut>
#include <QString>

void installAppShortcuts(AppHost &host)
{
	// Global shortcuts (keybinds.json parity comes with the NedKey host layer).
	// Cmd/Ctrl+1..9 — switch tab N; Cmd/Ctrl+W — close active tab (ImGui parity).
	for (int i = 1; i <= 9; ++i)
	{
		auto *tabShortcut = new QShortcut(QKeySequence(QString("Ctrl+%1").arg(i)), &host);
		QObject::connect(tabShortcut, &QShortcut::activated, &host, [&host, i] {
			host.workbench->activateTabIndex(i - 1);
		});
	}
	auto *closeShortcut = new QShortcut(QKeySequence("Ctrl+W"), &host);
	QObject::connect(closeShortcut, &QShortcut::activated, &host, [&host] {
		host.workbench->closeActiveTab();
	});
	// VS Code-style splits (additive — the ImGui host has no split key).
	auto *splitRightShortcut = new QShortcut(QKeySequence("Ctrl+\\"), &host);
	QObject::connect(splitRightShortcut, &QShortcut::activated, &host, [&host] {
		host.workbench->splitActive(Qt::Horizontal);
	});
	auto *splitDownShortcut = new QShortcut(QKeySequence("Ctrl+Shift+\\"), &host);
	QObject::connect(splitDownShortcut, &QShortcut::activated, &host, [&host] {
		host.workbench->splitActive(Qt::Vertical);
	});

#if NED_QT_TERMINAL
	// Cmd/Ctrl+T — toggle the bottom terminal panel (ImGui parity).
	auto *terminalShortcut = new QShortcut(QKeySequence("Ctrl+T"), &host);
	QObject::connect(
		terminalShortcut, &QShortcut::activated, &host, &AppHost::toggleTerminalPanel);
#endif
	// In-document find (Cmd/Ctrl+F). ONE shortcut at the host: a per-view
	// QShortcut goes ambiguous the moment a second view exists (background
	// tab or split) and Qt then fires nothing at all.
	const auto findTarget = [&host]() -> EditorFrame * {
		if (EditorFrame *view = host.workbench->activeView())
			return view;
		// No group focused yet (e.g. fresh window): first view.
		const QList<EditorFrame *> all = host.workbench->views();
		return all.isEmpty() ? nullptr : all.first();
	};
	auto *findShortcut = new QShortcut(QKeySequence("Ctrl+F"), &host);
	QObject::connect(findShortcut, &QShortcut::activated, &host, [&host, findTarget] {
		if (EditorFrame *view = findTarget())
			view->toggleFindBar();
	});
	// Same ambiguity trap for the per-view line-jump card (Ctrl+;).
	auto *lineJumpShortcut = new QShortcut(QKeySequence("Ctrl+;"), &host);
	QObject::connect(lineJumpShortcut, &QShortcut::activated, &host, [&host, findTarget] {
		if (EditorFrame *view = findTarget())
			view->goToLineDialog();
	});
	auto *finderShortcut = new QShortcut(QKeySequence("Ctrl+P"), &host);
	QObject::connect(finderShortcut, &QShortcut::activated, &host, [&host] {
		if (host.workspaceRoot.isEmpty())
			return;
		auto *finder = new FileFinderView(host.workspaceRoot, &host);
		QObject::connect(
			finder, &FileFinderView::fileSelected, &host, [&host](const QString &path) {
				host.openPath(path, true);
			});
		finder->show();
	});
	auto *settingsShortcut = new QShortcut(QKeySequence("Ctrl+,"), &host);
	QObject::connect(settingsShortcut, &QShortcut::activated, &host, [&host] {
		Q_EMIT host.settingsRequested(); // one path: the in-window settings popup
	});
	auto *openShortcut = new QShortcut(QKeySequence("Ctrl+O"), &host);
	QObject::connect(openShortcut, &QShortcut::activated, &host, [&host] {
		// ImGui parity (app_shortcuts.cpp): Cmd/Ctrl+O opens a workspace
		// folder — the welcome screen's "CMD+O Open Folder".
		const QString root = QFileDialog::getExistingDirectory(&host, "Open Folder");
		if (!root.isEmpty())
			host.openWorkspace(root);
	});
}
