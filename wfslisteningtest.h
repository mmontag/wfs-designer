#ifndef WFSLISTENINGTEST_H
#define WFSLISTENINGTEST_H

#include <QDialog>
#include "ui_wfslisteningtest.h"

// Forward declaration
class WFSDesigner;

class WFSListeningTest : public QDialog
{
	Q_OBJECT

public:
	WFSListeningTest(WFSDesigner *parent = 0);
	~WFSListeningTest();

	WFSDesigner* designer;

public slots:;
	void on_testButton1_clicked();
	void on_testButton2_clicked();
	void on_testButton3_clicked();
	void on_testButton4_clicked();
	void on_testButton5_clicked();
	void on_testButton6_clicked();
	void on_testButton7_clicked();
	void on_testButton8_clicked();
	void on_testButton9_clicked();
	void on_testButton10_clicked();
	void close();

private:
	Ui::WFSListeningTest ui;
};

#endif // WFSListeningTest_H
