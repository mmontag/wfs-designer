#include "wfslisteningtest.h"
#include "wfsdesigner.h"

WFSListeningTest::WFSListeningTest(WFSDesigner *parent)
	: QDialog(parent)
{
	designer = parent;
	ui.setupUi(this);
}

WFSListeningTest::~WFSListeningTest()
{

}

void WFSListeningTest::on_testButton1_clicked() 
{
	designer->playTestTone(1);
}

void WFSListeningTest::on_testButton2_clicked()
{
	designer->playTestTone(2);
}

void WFSListeningTest::on_testButton3_clicked()
{
	designer->playTestTone(3);
}

void WFSListeningTest::on_testButton4_clicked()
{
	designer->playTestTone(4);
}

void WFSListeningTest::on_testButton5_clicked()
{
	designer->playTestTone(5);
}

void WFSListeningTest::on_testButton6_clicked()
{
	designer->playTestTone(6);
}

void WFSListeningTest::on_testButton7_clicked()
{
	designer->playTestTone(7);
}

void WFSListeningTest::on_testButton8_clicked()
{
	designer->playTestTone(8);
}

void WFSListeningTest::on_testButton9_clicked()
{
	designer->playTestTone(9);
}

void WFSListeningTest::on_testButton10_clicked()
{
	designer->playTestTone(10);
}

void WFSListeningTest::close() {
	designer->on_stopAudio_clicked();
	QDialog::close();
}