#include "WFSConfig.h"
#include <QStringList>
#include <QStringListModel>

WFSConfig::WFSConfig(WFSPortAudio* pa)
{
	portAudio = pa;
	previousDevice = portAudio->activeDeviceIndex;

	ui.setupUi(this);
	QStringListModel *model = new QStringListModel();
	model->setStringList(portAudio->outputDeviceStringList);
	//TODO: instead of this, the model should be inside portAudio->... 

	ui.devicesComboBox->setModel(model);
	ui.devicesComboBox->setCurrentIndex(portAudio->outputDeviceIndexList.indexOf(portAudio->activeDeviceIndex));
	qDebug("Showing OutputDevices ComboBox item %d, which is portaudio device %d.",
		portAudio->outputDeviceIndexList.indexOf(portAudio->activeDeviceIndex),
		portAudio->activeDeviceIndex);
	ui.channelList->setModel(portAudio->channelList[portAudio->outputDeviceIndexList[ui.devicesComboBox->currentIndex()]]);
	//ui.channelTable->setModel(portAudio->channelList[ui.devicesComboBox->currentIndex()]);
	//QStringListModel *channels = new QStringListModel();
}

WFSConfig::~WFSConfig()
{

}

void WFSConfig::show() {
	ui.devicesComboBox->setCurrentIndex(portAudio->outputDeviceIndexList.indexOf(portAudio->activeDeviceIndex));
	QWidget::show();
}

void WFSConfig::on_devicesComboBox_currentIndexChanged()
{
	ui.channelList->setModel(portAudio->channelList[portAudio->outputDeviceIndexList[ui.devicesComboBox->currentIndex()]]);

	if(portAudio->channelList[portAudio->outputDeviceIndexList[ui.devicesComboBox->currentIndex()]]->paHostName == "ASIO") {
		ui.showAsioButton->setEnabled(true);
	} else {
		ui.showAsioButton->setDisabled(true);
	}
	//ui.channelTable->setModel(portAudio->channelList[ui.devicesComboBox->currentIndex()]);
}

void WFSConfig::on_toggleSelectionButton_clicked()
{
	//portAudio->channelList[ui.devicesComboBox->currentIndex()]->toggleSelection();
	//TODO: This should take place in the model since the model owns the selection.
	//then use the following outside of setData. 
	//portAudio->channelList[ui.devicesComboBox->currentIndex()]->
	//	dataChanged(sel->selection().first().topLeft(),sel->selection().last().bottomRight());

	QItemSelectionModel* sel = ui.channelList->selectionModel();
	foreach(QModelIndex i, sel->selectedIndexes()) {
		qDebug() << QString("Row %1 Column %2").arg(i.row()).arg(i.column());
		((ChannelModel*)i.model())->toggle(i);
	}
}

void WFSConfig::on_testAllButton_clicked()
{
	if(portAudio->engineState == portAudio->ENGINE_CHANNEL_TEST) {
		portAudio->stop();
	} else {
		portAudio->maybeSwitchDevice(portAudio->outputDeviceIndexList[ui.devicesComboBox->currentIndex()]);
		portAudio->testAllChannels();
	}
}

void WFSConfig::on_showAsioButton_clicked()
{
	if(portAudio->engineState == portAudio->ENGINE_CHANNEL_TEST) {
		portAudio->stop();
	}
	portAudio->showAsio(portAudio->outputDeviceIndexList[ui.devicesComboBox->currentIndex()], WFSConfig::effectiveWinId());
}

void WFSConfig::on_okButton_clicked()
{
	if(portAudio->engineState == portAudio->ENGINE_CHANNEL_TEST) {
		portAudio->stop();
	}
	portAudio->maybeSwitchDevice(portAudio->outputDeviceIndexList[ui.devicesComboBox->currentIndex()]);

	portAudio->start();

	//this will update the graphics scene to show active channels
	//physicalModel->updateLoudspeakersFromChannelList(portAudio->getActiveChannelList());
	emit configurationChanged(&(portAudio->channelList.at(portAudio->activeDeviceIndex)->m_channelEnabled));
	close();
}

void WFSConfig::on_cancelButton_clicked()
{
	if(portAudio->engineState == portAudio->ENGINE_CHANNEL_TEST) {
		portAudio->stop();
	}
	portAudio->maybeSwitchDevice(previousDevice);
	close();
}