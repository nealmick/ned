/*
	File: views/qt/qt_lsp_dashboard.h
	Description: Language-server dashboard for the Qt backend (parallel of
	views/imgui/lsp_dashboard). Lists every configured server from lsp.json
	with found/active status via the shared probe (lsp/lsp_server_status);
	buttons refresh, reload lsp.json, and open it in the editor.
*/

#pragma once

#include <QDialog>
#include <functional>
#include <string>
#include <vector>

class QLabel;
class QTableWidget;
class LSPClient;

class QtLspDashboard : public QDialog
{
	Q_OBJECT

  public:
	// openFile receives an absolute path (the host opens it in a tab).
	QtLspDashboard(LSPClient &client,
				   std::function<void(const std::string &path)> openFile,
				   QWidget *parent);

	// Re-probe the lsp.json configuration + live client state.
	void refresh();

  protected:
	void showEvent(QShowEvent *event) override;

  private:
	LSPClient &client;
	std::function<void(const std::string &path)> openFile;

	QTableWidget *table = nullptr;
	QLabel *countLabel = nullptr;
};
