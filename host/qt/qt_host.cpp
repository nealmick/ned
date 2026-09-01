#include "qt_host.h"

#include "util/macos_window.h"
#include "util/settings.h"

#include <QApplication>
#include <QTabWidget>

#ifdef __APPLE__
// Defined in qt_mac_chrome.mm (ObjC++).
extern void configureNedQtChrome(void *nsWindow, float opacity, bool blurEnabled);
#endif

NedQtHost::NedQtHost(QWidget *parent) : QMainWindow(parent)
{
	setWindowTitle("Ned Text Editor");
	resize(1200, 750);

	tabs = new QTabWidget(this);
	tabs->setTabsClosable(true);
	tabs->setDocumentMode(true);
	setCentralWidget(tabs);

	applyNativeChrome();
}

NedQtHost::~NedQtHost() = default;

void NedQtHost::applyNativeChrome()
{
#ifdef __APPLE__
	// Same treatment as the GLFW host: transparent title bar, vibrancy
	// blur behind, and the content extending under the title bar.
	const Settings defaults; // chrome uses profile defaults until views land
	configureNedQtChrome(reinterpret_cast<void *>(winId()),
						 defaults.settings.value("mac_background_opacity", 0.5f),
						 defaults.settings.value("mac_blur_enabled", true));
#endif
}
