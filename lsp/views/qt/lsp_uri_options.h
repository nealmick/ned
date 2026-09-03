/*
	File: views/qt/lsp_uri_options.h
	Description: Goto-definition/references results picker for the Qt
	backend (parallel of views/imgui/lsp_uri_options). Popup list over the
	host window; Up/Down navigate, Enter/double-click jumps (signal),
	Escape/outside-click closes. `present()` is safe to call repeatedly —
	results arrive asynchronously and refresh the open list in place.
*/

#pragma once

#include "../../../lsp/lsp_locations.h"

#include <QDialog>
#include <string>
#include <vector>

class QLabel;
class QListWidget;
class QShowEvent;

class LSPUriOptions : public QDialog
{
	Q_OBJECT

  public:
	explicit LSPUriOptions(QWidget *parent);

	// Show (or refresh, while a request is still in flight) the result set.
	// `showFlag` is the caller's liveness flag (LSPGoto::show); every close
	// path clears it so the request stops rendering. `pending` distinguishes
	// "Searching…" (request in flight) from "No results" (server answered).
	void present(const std::string &title,
				 const std::vector<LSPLocation> &options,
				 bool *showFlag,
				 bool pending);

  Q_SIGNALS:
	void locationSelected(const LSPLocation &location);

  protected:
	void keyPressEvent(QKeyEvent *event) override;
	// Every close path (Escape, outside click, accept/reject) hides the
	// popup — one hook clears the request's show flag.
	void hideEvent(QHideEvent *event) override;
	// This dialog is long-lived (built once with LspView), so its card
	// stylesheet can't snapshot the popover color at construction — the
	// theme may change many times before the next open. Re-derive it from
	// the live palette here (finder/line-jump are rebuilt per use and
	// don't need this).
	void showEvent(QShowEvent *event) override;
	// Keyboard focus sits on the list; intercept Up/Down/Enter there.
	bool eventFilter(QObject *watched, QEvent *event) override;

  private:
	void commit();

	std::vector<LSPLocation> options;
	bool *showFlag = nullptr;
	std::string titleText;
	std::string shownSignature; // unchanged results skip the rebuild

	QWidget *card = nullptr;
	QLabel *title = nullptr;
	QListWidget *list = nullptr;
};
