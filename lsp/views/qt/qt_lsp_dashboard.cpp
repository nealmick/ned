#include "qt_lsp_dashboard.h"

#include "../../../lsp/lsp_client.h"
#include "../../../lsp/lsp_server_status.h"
#include "../../../util/settings.h"

#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QPushButton>
#include <QShowEvent>
#include <QTableWidget>
#include <QVBoxLayout>

#include <filesystem>
#include <iostream>
#include <utility>

QtLspDashboard::QtLspDashboard(LSPClient &clientIn,
							   std::function<void(const std::string &path)> openFileIn,
							   QWidget *parent)
	: QDialog(parent), client(clientIn), openFile(std::move(openFileIn))
{
	setWindowTitle("LSP Server Dashboard");
	setModal(false);
	setMinimumSize(680, 380);

	auto *layout = new QVBoxLayout(this);
	layout->setContentsMargins(12, 12, 12, 12);
	layout->setSpacing(8);

	auto *buttonRow = new QWidget(this);
	auto *buttons = new QHBoxLayout(buttonRow);
	buttons->setContentsMargins(0, 0, 0, 0);
	buttons->setSpacing(8);

	auto *refreshButton = new QPushButton("Refresh Server Status", buttonRow);
	connect(refreshButton, &QPushButton::clicked, this, [this] { refresh(); });
	buttons->addWidget(refreshButton);

	auto *reloadButton = new QPushButton("Reload LSP.json", buttonRow);
	connect(reloadButton, &QPushButton::clicked, this, [this] {
		client.initializeLanguageServers();
		refresh();
	});
	buttons->addWidget(reloadButton);

	auto *openButton = new QPushButton("Open LSP.json", buttonRow);
	connect(openButton, &QPushButton::clicked, this, [this] {
		const std::string lspJsonPath =
			(std::filesystem::path(Settings::getUserConfigDir()) / "lsp.json").string();
		if (std::filesystem::exists(lspJsonPath))
		{
			if (openFile)
				openFile(lspJsonPath);
			close();
		} else
		{
			std::cerr << "[LSP Dashboard] LSP.json file not found at: " << lspJsonPath
					  << std::endl;
		}
	});
	buttons->addWidget(openButton);
	buttons->addStretch(1);
	layout->addWidget(buttonRow);

	countLabel = new QLabel(this);
	layout->addWidget(countLabel);

	table = new QTableWidget(this);
	table->setColumnCount(4);
	table->setHorizontalHeaderLabels({"Language", "Server Path", "Found", "Status"});
	table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
	table->verticalHeader()->hide();
	table->setEditTriggers(QAbstractItemView::NoEditTriggers);
	table->setSelectionBehavior(QAbstractItemView::SelectRows);
	table->setAlternatingRowColors(true);
	layout->addWidget(table, 1);
}

void QtLspDashboard::showEvent(QShowEvent *event)
{
	QDialog::showEvent(event);
	refresh(); // status goes stale while hidden
}

void QtLspDashboard::refresh()
{
	const std::vector<LSPServerInfo> servers = probeLspServers(client);

	countLabel->setText(QString("%1 servers configured").arg(servers.size()));
	table->setRowCount(static_cast<int>(servers.size()));

	int row = 0;
	for (const LSPServerInfo &server : servers)
	{
		auto *languageItem =
			new QTableWidgetItem(QString::fromStdString(server.language));

		auto *pathItem = new QTableWidgetItem(QString::fromStdString(server.serverPath));
		pathItem->setToolTip(QString::fromStdString(server.serverPath));

		auto *foundItem = new QTableWidgetItem(
			server.isFound ? QStringLiteral("● Found") : QStringLiteral("● Missing"));
		foundItem->setForeground(server.isFound ? QColor(0x47, 0xcc, 0x47)
												: QColor(0xcc, 0x47, 0x47));

		QString statusText = QStringLiteral("N/A");
		QColor statusColor(0x99, 0x99, 0x99);
		if (server.isFound)
		{
			statusText = server.isActive ? QStringLiteral("● Active")
										 : QStringLiteral("● Inactive");
			if (server.isActive)
				statusColor = QColor(0x47, 0xcc, 0x47);
		}
		auto *statusItem = new QTableWidgetItem(statusText);
		statusItem->setForeground(statusColor);

		table->setItem(row, 0, languageItem);
		table->setItem(row, 1, pathItem);
		table->setItem(row, 2, foundItem);
		table->setItem(row, 3, statusItem);
		++row;
	}
}
