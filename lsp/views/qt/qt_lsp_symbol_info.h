/*
	File: views/qt/qt_lsp_symbol_info.h
	Description: Hover/symbol-info tooltip for the Qt backend (parallel of
	views/imgui/lsp_symbol_info). Requests run through the shared LSPHover
	core on LSPClient; this class only tracks the target editor, renders the
	delivered markdown, and places the popup (anchored below the caret for
	the keybind, at the mouse for hover).
*/

#pragma once

#include "../../../editor/util/hover_trigger.h"
#include "../../../editor/views/qt/qt_hover_tip.h"

#include <QObject>
#include <QPoint>
#include <QPointer>
#include <QString>

class QtEditorView;
class LSPClient;
class Settings;

class QtLspSymbolInfo : public QObject
{
	Q_OBJECT

  public:
	QtLspSymbolInfo(LSPClient &client, Settings &settings, QObject *parent);
	~QtLspSymbolInfo() override;

	// Focus moved to another editor (or none): retarget + drop state.
	void setEditor(QtEditorView *view);

	// Keybind (Cmd/Ctrl+I): caret-anchored hover, until dismissed.
	void triggerAtCaret();

	// Mouse-hover trigger fired on the editor under the mouse (Text zone).
	// The hovered editor is the request target — split layouts can hover a
	// different document than the focused one.
	void hoverTarget(QtEditorView *hovered, const HoverTrigger::Info &info);

	// Any dismissal signal (key/click/scroll/retarget/close).
	void dismiss();

	// Poll the shared request state (call from a short timer): shows,
	// refreshes, or hides the tooltip as results arrive.
	void poll();

	bool isVisible() const;

  private:
	void hideTooltip();

	LSPClient &client;
	Settings &settings;
	// QPointer: split views close while hovered; tooltips must never target
	// a deleted editor.
	QPointer<QtEditorView> view;
	QPointer<QtEditorView> hoverView;

	QtHoverTip *tip = nullptr;			 // shared themed card (rounded, shadowed)
	bool anchored = false;				 // keybind mode: below the caret
	std::string shownMarkdown;			 // last rendered hover text (skip re-highlight)
	QPointer<QtEditorView> requestedFor; // cell dedup is per document
	int hoverRow = -1;
	int hoverCol = -1;
};
