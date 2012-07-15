#ifndef WFSConfig_H
#define WFSConfig_H

#include <QWidget>
#include "ui_WFSConfig.h"
#include "wfsportaudio.h"
#include "wfsmodel.h"

class WFSConfig : public QWidget
{
	Q_OBJECT

public:
	WFSConfig(WFSPortAudio* pa);
	~WFSConfig();
	WFSPortAudio* portAudio;
	PhysicalModel* physicalModel;

	int previousDevice;
	void show();

signals:
	void configurationChanged(QVector<bool>* newChannelList);

public slots:;
	void on_devicesComboBox_currentIndexChanged(); //auto-connected slot
	void on_toggleSelectionButton_clicked();
	void on_testAllButton_clicked();
	void on_showAsioButton_clicked();
	void on_okButton_clicked();
	void on_cancelButton_clicked();

private:
	Ui::WFSConfig ui;
};

#endif // WFSConfig_H
