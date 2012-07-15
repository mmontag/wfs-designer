#include "wfsdesigner.h"
#include <QtGui/QApplication>
#include <QSplashScreen>

int main(int argc, char *argv[])
{
	QApplication a(argc, argv);
	QPixmap pixmap("splashscreen_opaque.png");
	QSplashScreen splash(pixmap);
	splash.show();

	WFSDesigner w;
	w.show();
	splash.finish(&w);
	return a.exec();
}