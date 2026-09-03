#include "qt_welcome.h"

#include "../../../util/settings.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QPixmap>
#include <QPushButton>
#include <QVBoxLayout>

namespace {

QPixmap loadPixmap(const QString &relPath)
{
	QPixmap pix(QString::fromStdString(Settings::getAppResourcesPath()) + "/" + relPath);
	if (pix.isNull()) // dev fallback when cwd is the repo root
		pix.load(relPath);
	return pix;
}

QPixmap devicePixelPixmap(QWidget *host, QPixmap pix, int px)
{
	if (pix.isNull())
		return pix;
	const qreal dpr = host->devicePixelRatioF();
	pix = pix.scaled(
		qRound(px * dpr), qRound(px * dpr), Qt::KeepAspectRatio, Qt::SmoothTransformation);
	pix.setDevicePixelRatio(dpr);
	return pix;
}

} // namespace

QtWelcomePage::QtWelcomePage(QWidget *parent) : QWidget(parent)
{
	// Mirrors the ImGui welcome layout: logo and title side by side with
	// the Open Folder button in the right column, centered in the page.
	auto *layout = new QVBoxLayout(this);
	layout->setAlignment(Qt::AlignCenter);

	auto *hero = new QWidget(this);
	auto *heroRow = new QHBoxLayout(hero);
	heroRow->setAlignment(Qt::AlignCenter);
	heroRow->setSpacing(36);

	auto *logo = new QLabel(hero);
	logo->setPixmap(devicePixelPixmap(logo, loadPixmap("resources/icons/ned.png"), 150));

	auto *content = new QWidget(hero);
	auto *contentCol = new QVBoxLayout(content);
	contentCol->setSpacing(14);
	contentCol->setContentsMargins(0, 12, 0, 0);

	// Hero size must travel in the label's own stylesheet: the app
	// stylesheet's `* { font-size }` beats setFont(), but a widget's own
	// sheet wins over the application sheet.
	auto *title = new QLabel("Welcome to NED", content);
	title->setStyleSheet("font-size: 32pt; font-weight: bold;");
	contentCol->addWidget(title);

	auto *openFolder = new QPushButton("Open Folder", content);
	openFolder->setCursor(Qt::PointingHandCursor);
	openFolder->setMinimumWidth(180);
	contentCol->addWidget(openFolder);
	contentCol->addStretch();

	// Both items top-aligned so the title/button column shares the logo's
	// top edge instead of floating above it.
	heroRow->addWidget(logo, 0, Qt::AlignTop);
	heroRow->addWidget(content, 0, Qt::AlignTop);

	// Stretch bias above/below nudges the hero slightly above the
	// geometric center, which reads as visually centered.
	layout->addStretch(3);
	layout->addWidget(hero);
	layout->addStretch(4);

	connect(openFolder, &QPushButton::clicked, this, &QtWelcomePage::openFolderRequested);
}
