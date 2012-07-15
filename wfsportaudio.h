#pragma once
#include "portaudio.h"
#include "pa_asio.h"
//#include "sndfile.h"
#include <QDebug>
#include <QList>
#include <QStringList>
#include <QAbstractTableModel>
#include <QWidget>

//class PortAudioDeviceModel:public QAbstractTableModel {
//	Q_OBJECT
//public: 
//	PortAudioDeviceModel(QObject *parent);
//	~PortAudioDeviceModel(void);
//private:
//	QVariant m_devices[
//class PortAudioConfigModel {
//public:
//	PortAudioConfig(void);
//	~PortAudioConfig(void);
//	int activeDeviceID;
//	
//};

class ChannelModel:public QAbstractTableModel {
	Q_OBJECT
public:
	PaDeviceIndex paDeviceIndex;
	QString paHostName;
	QVector<bool> m_channelEnabled;

	ChannelModel(QObject *parent);
	int rowCount(const QModelIndex &parent = QModelIndex()) const;
	int columnCount(const QModelIndex &parent = QModelIndex()) const { return 2; };
	QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
	bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole);
	Qt::ItemFlags flags(const QModelIndex &index) const;
	void toggle(QModelIndex &index);
signals:;
	void editCompleted(const bool);
	void configurationChanged(QVector<bool>* channelList);
};


class WFSPortAudio:public QObject
{
	Q_OBJECT
public:
	WFSPortAudio(QWidget *parent);
	~WFSPortAudio(void);

	QWidget* controller;
	PaStream *stream;
	PaError error;

	enum EngineState {
		ENGINE_IDLE,
		ENGINE_WFS,
		ENGINE_CHANNEL_TEST
	};
	EngineState engineState;

	struct ChannelTestData {
		float toneDuration; //seconds
		float toneFrequency;
		float gapDuration; //seconds
		float envRate;
		float env; //envelope
		int currentChannel;
		int counter;
	};
	ChannelTestData channelTestData;

	QVector<PaStreamParameters *> outputParameters; //we cache outputparams for all devices so we don't have to check again.
	QVector<ChannelModel *> channelList;

	const PaDeviceInfo *deviceInfo;
	int framesPerBuffer;
	int sampleRate;
	int numDevices;
	PaDeviceIndex activeDeviceIndex;
	QList<int> outputDeviceIndexList;
	QStringList outputDeviceStringList;

	/*
	Channel Buffer - essentially for deinterleaving
	*/
	float* channelBuffer;

	QVector<bool>* getActiveChannelList() {
		return(&(channelList.at(activeDeviceIndex)->m_channelEnabled));
	}

	void start();
	void stop();
	void softStart();
	void softStop();

	int myMemberCallback(const void *input,
				 void *output,
				 unsigned long frameCount,
				 const PaStreamCallbackTimeInfo* paTimeInfo,
				 PaStreamCallbackFlags statusFlags);

	//this is a wrapper function for the member callback, see http://www.portaudio.com/trac/wiki/tips/c%2B%2B
	static int myPaCallback(
				const void *input, void *output,
				unsigned long frameCount,
				const PaStreamCallbackTimeInfo* timeInfo,
				PaStreamCallbackFlags statusFlags,
				void *userData )
	{
	  return ((WFSPortAudio*)userData)->myMemberCallback(input, output, frameCount, timeInfo, statusFlags);
	}

	void maybeSwitchDevice(PaDeviceIndex i);
	void testAllChannels();
	void testChannel(int ch);
	void showAsio(PaDeviceIndex i, void* h);
signals:
	void configurationChanged(QVector<bool>* newChannelList);
	//void lockMutex();
	//void unlockMutex();

};
