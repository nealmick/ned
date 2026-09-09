#include "lsp_uri_options.h"

#include "host/qt/theme.h"

#include <QHideEvent>
#include <QKeyEvent>
#include <QLabel>
#include <QListWidget>
#include <QShowEvent>
#include <QVBoxLayout>

#include <algorithm>

LSPUriOptions::LSPUriOptions(QWidget *parent) : QDialog(parent, Qt::Popup)
{
	setModal(true);
	setObjectName("lspUriOptions");
	// Translucent window so the stylesheet radius rounds the popup corners.
	// QSS backgrounds never render on the translucent top-level itself, so
	// the opaque card is a CHILD widget (HoverTooltip's pattern):
	// WA_StyledBackground + local rule with the opaque raised tone —
	// palette(window) would carry the window's opacity alpha.
	setAttribute(Qt::WA_TranslucentBackground);
	auto *outer = new QVBoxLayout(this);
	outer->setContentsMargins(0, 0, 0, 0);
	card = new QWidget(this);
	card->setAttribute(Qt::WA_StyledBackground, true);
	// Scoped to the card: a selector-less declaration block would also
	// apply to every descendant, drawing the card's hairline border
	// around the title, list, and footer (the "extra borders" bug).
	card->setObjectName("nedPopoverCard");
	card->setStyleSheet(NedQtTheme::popoverCardSheet());
	outer->addWidget(card);

	auto *layout = new QVBoxLayout(card);
	layout->setContentsMargins(12, 10, 12, 10);
	layout->setSpacing(8);

	title = new QLabel(card);
	title->setStyleSheet(
		"font-weight: 600; border-bottom: 1px solid rgba(128, 128, 128, 70); "
		"padding-bottom: 6px;");
	layout->addWidget(title);

	list = new QListWidget(card);
	list->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	layout->addWidget(list, 1);

	auto *footer = new QLabel("Up/Down navigate · Enter jump · Esc close", card);
	footer->setStyleSheet("color: rgba(128, 128, 128, 200); font-size: 10px;");
	layout->addWidget(footer);

	// Keyboard focus lands on the list, so navigation keys arrive there —
	// intercept for wrap-around and commit.
	list->installEventFilter(this);
	connect(list, &QListWidget::itemDoubleClicked, this, [this](QListWidgetItem *) {
		commit();
	});
}

void LSPUriOptions::present(const std::string &titleTextIn,
							const std::vector<LSPLocation> &optionsIn,
							bool *showFlagIn,
							bool pending)
{
	showFlag = showFlagIn;

	const std::string signature =
		lsp_locations::locationSignature(titleTextIn, optionsIn) +
		(pending ? "/pending" : "");
	if (isVisible() && signature == shownSignature)
		return; // same request, same results — keep scroll + selection
	shownSignature = signature;

	options = optionsIn;
	titleText = titleTextIn;

	title->setText(QString::fromStdString(titleText) +
				   QString(" (%1)").arg(options.size()));

	const int previousRow = list->currentRow();
	list->clear();
	if (options.empty())
	{
		// "Searching" while the request is in flight; once the server
		// answered, an empty list is an answer (ImGui picker parity).
		auto *placeholder = new QListWidgetItem(pending ? QStringLiteral("Searching…")
														: QStringLiteral("No results"));
		placeholder->setFlags(placeholder->flags() & ~Qt::ItemIsSelectable &
							  ~Qt::ItemIsEnabled);
		list->addItem(placeholder);
	} else
	{
		for (const LSPLocation &loc : options)
			list->addItem(QString::fromStdString(lsp_locations::locationLabel(loc)));
		// Keep the selection stable across in-place refreshes; a fresh open
		// starts at the top.
		list->setCurrentRow(std::clamp(previousRow, 0, list->count() - 1));
	}

	// Size like the ImGui picker: ~30 monospace chars wide, height capped
	// at half the host window.
	QWidget *host = parentWidget();
	const QFontMetrics fm = list->fontMetrics();
	const int rowH = fm.height() + 8;
	const int shown = std::max<size_t>(options.size(), 1);
	const int maxW = host ? host->width() * 9 / 10 : 560;
	const int maxH = host ? host->height() / 2 : 360;
	setFixedSize(std::min(fm.horizontalAdvance(QString(60, 'M')) / 2, maxW),
				 std::min(rowH * static_cast<int>(shown) + 110, maxH));

	if (!isVisible() && host)
	{
		// Centered horizontally, at ~35% of the host height (ImGui parity).
		move(host->mapToGlobal(QPoint((host->width() - width()) / 2,
									  host->height() * 35 / 100 - height() / 2)));
	}
	show();
	list->setFocus();
	list->scrollTo(list->currentIndex(), QAbstractItemView::PositionAtCenter);
}

bool LSPUriOptions::eventFilter(QObject *watched, QEvent *event)
{
	if (watched == list && event->type() == QEvent::KeyPress)
	{
		auto *key = static_cast<QKeyEvent *>(event);
		if (key->key() == Qt::Key_Return || key->key() == Qt::Key_Enter)
		{
			commit();
			return true;
		}
		if ((key->key() == Qt::Key_Up || key->key() == Qt::Key_Down) && !options.empty())
		{
			const int count = static_cast<int>(options.size());
			const int dir = key->key() == Qt::Key_Up ? -1 : 1;
			const int row = list->currentRow();
			list->setCurrentRow(((row < 0 ? 0 : row + dir) + count) % count); // wrap
			return true;
		}
	}
	return QDialog::eventFilter(watched, event);
}

void LSPUriOptions::keyPressEvent(QKeyEvent *event)
{
	if (event->key() == Qt::Key_Escape)
	{
		reject();
		return;
	}
	QDialog::keyPressEvent(event);
}

void LSPUriOptions::showEvent(QShowEvent *event)
{
	QDialog::showEvent(event);
	// The card stylesheet is a local snapshot (translucent popup windows
	// don't resolve app-stylesheet rules) — rebuild it from the current
	// palette so the popover matches the theme active NOW.
	card->setStyleSheet(NedQtTheme::popoverCardSheet());
}

void LSPUriOptions::hideEvent(QHideEvent *event)
{
	QDialog::hideEvent(event);
	if (showFlag)
	{
		*showFlag = false;
		showFlag = nullptr;
	}
}

void LSPUriOptions::commit()
{
	const int row = list->currentRow();
	if (row < 0 || row >= static_cast<int>(options.size()))
		return;
	Q_EMIT locationSelected(options[static_cast<size_t>(row)]);
	accept(); // hides -> hideEvent clears the show flag
}
