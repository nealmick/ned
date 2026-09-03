/*
	File: views/qt/hover_tooltip.h
	Description: Shared styled hover tooltip — themed rounded card with a
	soft shadow, used by the LSP symbol hover and the diagnostic hover
	(implements the ImGui backend's tooltip look for Qt). Plain rich-text
	content; sizes to content and only wraps when it would overflow the
	screen, then flips/clamps against the screen edges.
*/

#pragma once

#include <QLabel>
#include <QWidget>

class HoverTooltip : public QWidget
{
  public:
	explicit HoverTooltip(QWidget *parent = nullptr);

	// Render `html` in `font` on the themed card (bg/ink) and show it at
	// the anchor: below-right, flipped above near the screen bottom.
	void present(const QString &html,
				 const QFont &font,
				 const QColor &bg,
				 const QColor &ink,
				 const QPoint &globalAnchor);

	// Same content and size, new anchor (follows the caret/mouse).
	void replaceAt(const QPoint &globalAnchor);

  private:
	void placeAt(const QPoint &globalAnchor);

	QWidget *card = nullptr;
	QLabel *label = nullptr;
};
