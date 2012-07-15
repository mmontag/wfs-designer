#include "wfsportaudio.h"
#include "wfsdesigner.h"
#include "math.h"

#define printf qDebug
#define PI  (3.14159265)
#define SAMPLE_RATE (44100)
#define 	PA_SWAP32_(x)   ((x>>24) | ((x>>8)&0xFF00) | ((x<<8)&0xFF0000) | (x<<24));
#define DEFAULT_DEVICE_ID (1)

WFSPortAudio::WFSPortAudio(QWidget *parent)
{
	controller = parent;

	// start portaudio 
	Pa_Initialize();
	numDevices = Pa_GetDeviceCount();
	sampleRate = SAMPLE_RATE;
	stream = NULL;
	engineState = ENGINE_IDLE;
	channelTestData.env = 0;
	channelTestData.toneDuration=1; //seconds
	channelTestData.toneFrequency=500;
	channelTestData.gapDuration=0.500; //seconds
	channelTestData.envRate=0.001;

	//generate device list and channel model
	outputDeviceStringList.clear();
	for(int i = 0; i < numDevices; i++) {

		// set the output parameters 
		deviceInfo = Pa_GetDeviceInfo(i);
		outputParameters.append(new PaStreamParameters());
		outputParameters.last()->device = i;
		outputParameters.last()->suggestedLatency = deviceInfo->defaultHighOutputLatency;
		outputParameters.last()->channelCount = deviceInfo->maxOutputChannels;
		//outputParameters.last()->channelCount = sfInfo.channels; // use the same number of channels as our sound file 
		outputParameters.last()->sampleFormat = paFloat32; // 32bit int format, non-interleaved
		//outputParameters.last()->suggestedLatency = 0.2; // 200 ms ought to satisfy even the worst sound card 
		outputParameters.last()->hostApiSpecificStreamInfo = 0; // no api specific data 
		const PaHostApiInfo* hostInfo = Pa_GetHostApiInfo(deviceInfo->hostApi);

		channelList.append(new ChannelModel(0));
		channelList.last()->paHostName = hostInfo->name;
		channelList.last()->paDeviceIndex = i;
		channelList.last()->m_channelEnabled.resize(deviceInfo->maxOutputChannels); 
		channelList.last()->m_channelEnabled.fill(true);

		if(deviceInfo->maxOutputChannels > 0) {
			outputDeviceIndexList << i;
			outputDeviceStringList << QString("[%1] %2 (%3 - %4 channels)").arg(outputDeviceStringList.count()+1).arg(deviceInfo->name)
				.arg(hostInfo->name).arg(deviceInfo->maxOutputChannels);
		}
	}

	// TODO: PortAudio device should come from a config file.
	maybeSwitchDevice(Pa_GetDefaultOutputDevice());
	emit configurationChanged(&(channelList.at(activeDeviceIndex)->m_channelEnabled));

	// if we can't open it, then bail out 
	if (error)
	{
		qDebug("[portaudio] Error opening output, error code = %i", error);
		Pa_Terminate();
		return;
	}
}

WFSPortAudio::~WFSPortAudio(void)
{
	Pa_Terminate(); // and shut down portaudio
}

/*

CALLBACK FUNCTION

*/
int WFSPortAudio::myMemberCallback(const void*            inputBuffer,
                          void*                           outputBuffer,
                          unsigned long                   frPerBuffer,
                          const PaStreamCallbackTimeInfo* timeInfo,
                          PaStreamCallbackFlags           statusFlags)
{
	//emit lockMutex();
	/* 
	Provide some feedback to the log about buffer size. 
	Dangerous to put a console debug in the callback, but this might be 
	the only way, since the buffer size can change dynamically.
	*/
	if(frPerBuffer != framesPerBuffer) {
		framesPerBuffer = frPerBuffer;
		//allocate channelBuffer
		channelBuffer = new float[framesPerBuffer];
		qDebug() << QString("[portaudio] Changed framesPerBuffer to %1").arg(frPerBuffer);
	}
	
	int numChannels = outputParameters[activeDeviceIndex]->channelCount;

	if(engineState == ENGINE_CHANNEL_TEST) {
	/*

	CHANNEL TEST

	*/
		int frameIndex, channelIndex, pulseFrames, totalTestFrames, toneDur;
		/* NON-INTERLEAVED STREAM FORMAT */
		////float** outputs = (float**)outputBuffer;

		/* INTERLEAVED STREAM FORMAT */
		float* output = (float*)outputBuffer;
		pulseFrames = SAMPLE_RATE * (channelTestData.toneDuration + channelTestData.gapDuration);
		totalTestFrames = pulseFrames * numChannels;
		toneDur = channelTestData.toneDuration * SAMPLE_RATE;

		for(unsigned long frameIndex=0; frameIndex<framesPerBuffer; frameIndex++ ) {
			channelIndex = (int)floor(numChannels * (float)channelTestData.counter/totalTestFrames);
			if(channelIndex >= numChannels)  channelIndex = numChannels - 1;

			if(channelTestData.counter % pulseFrames < toneDur) {
				//sounding...
				channelTestData.env += channelTestData.envRate; 
				if(channelTestData.env > 1) channelTestData.env = 1;
			}else {
				//silence...
				channelTestData.env -= channelTestData.envRate; 
				if(channelTestData.env < 0) channelTestData.env = 0; 
			}

			/* NON-INTERLEAVED STREAM FORMAT */
			//output a sine wave, or a sine wave with some neat distortion or whatever
			////outputs[channelIndex][frameIndex] = 
			////	0.5 * channelTestData.env * 
			////	(float)sin(PI*channelTestData.toneFrequency*channelTestData.counter/SAMPLE_RATE);
			////channelTestData.counter++;


			/* INTERLEAVED STREAM FORMAT */
			for(int ch = 0; ch < numChannels; ch++) {
				if(ch == channelIndex) {
					*output++ =
						0.05 * channelTestData.env * 
						(float)sin(PI*channelTestData.toneFrequency*channelTestData.counter/SAMPLE_RATE);
				} else {
					*output++ = 0;
				}
			}

			channelTestData.counter++;

			if(channelTestData.counter > totalTestFrames) {
				channelTestData.counter = 0;
				engineState = ENGINE_IDLE;
				return paComplete;
			}
		}
	} else if (engineState == ENGINE_IDLE) {
	/*

	IDLE

	*/
		//do nothing.
		memset((float*)outputBuffer, 0, sizeof(float)*framesPerBuffer*numChannels);
	} else {
	/*

	WFS ENGINE

	*/

		/*
		Ask the controller to fill an output buffer for each channel.
		The controller is aware of what loudspeaker each channel maps to,
		so it contains logic for bypassing disabled channels.
		Interleave each channel into the outputBuffer.

		On second thought, there's no way deinterleaving here was going to work: 
		source iteration needs to be the outer loop, 
		so that the WAV input libsndfile buffer I/O only gets done once per source.
		This logic is moved out of the portaudio realm completely.
		*/

		((WFSDesigner*)controller)->processAllChannels((float*)outputBuffer, framesPerBuffer, numChannels);

	}

	//emit unlockMutex();
	return paContinue;
}

void WFSPortAudio::start()
{
	Pa_StartStream(stream);
}

void WFSPortAudio::stop()
{
	Pa_StopStream(stream);
	softStop();
}

void WFSPortAudio::softStop()
{
	engineState = ENGINE_IDLE;
}

void WFSPortAudio::softStart()
{
	engineState = ENGINE_WFS;
}

void WFSPortAudio::maybeSwitchDevice( PaDeviceIndex i )
{
	if(activeDeviceIndex != i) {
		int oldDevice = activeDeviceIndex;

//switch output parameters here

		if(stream != 0) { //if stream object has been created
			//if (Pa_IsStreamActive(stream)) {
				Pa_CloseStream(stream);
			//}
		}

		/*
		PaStreamParameters:

		PaDeviceIndex 	device
		int 			channelCount
		PaSampleFormat 	sampleFormat
		PaTime 			suggestedLatency
		void *			hostApiSpecificStreamInfo

		Pa_OpenStream:
 
		Note: With some host APIs, the use of non-zero framesPerBuffer for a 
		callback stream may introduce an additional layer of buffering which 
		could introduce additional latency. PortAudio guarantees that the 
		additional latency will be kept to the theoretical minimum however, 
		it is strongly recommended that a non-zero framesPerBuffer value only
		be used when your algorithm requires a fixed number of frames per 
		stream callback.
		*/
		framesPerBuffer = 4096;
		error = Pa_OpenStream(&stream,	// stream is a 'token' that we need to save for future portaudio calls 
								0,	// no input 
								outputParameters[i], // pass the pointer
								SAMPLE_RATE, //sfInfo.samplerate,	// use the same sample rate as the sound file 
								framesPerBuffer, //paFramesPerBufferUnspecified,	// let portaudio choose the buffersize 
								paNoFlag,	// no special modes (clip off, dither off) 
								&WFSPortAudio::myPaCallback,	// callback function defined above 
								this ); // pass in our data structure so the callback knows what's up 
		qDebug() << QString("[portaudio] SampleRate: %1 Channels: %2.").arg(SAMPLE_RATE).arg(outputParameters[i]->channelCount);
		if (error)
		{
			qDebug() << QString("[portaudio] Can't open Device %1; error code %2.").arg(i).arg(error);
			Pa_Terminate();
			return;
		} else {
			qDebug() << QString("[portaudio] Switched from Device %1 to Device %2.").arg(oldDevice).arg(i);
			activeDeviceIndex = i;
		}
	}
}

void WFSPortAudio::testAllChannels()
{
	//if(Pa_IsStreamActive(stream)) {
		Pa_StopStream(stream);
	//}

	engineState = ENGINE_CHANNEL_TEST;
	channelTestData.counter = 0;

	Pa_StartStream(stream);
}

void WFSPortAudio::testChannel( int ch )
{
	// TODO
	if(Pa_IsStreamActive(stream)) {
		Pa_StopStream(stream);
	}
}

void WFSPortAudio::showAsio(PaDeviceIndex i, void* h)
{
	PaAsio_ShowControlPanel(i, h);
}

/*

CHANNEL MODEL IMPLEMENTATION

*/

int ChannelModel::rowCount( const QModelIndex &/*parent*/ ) const
{
	//return the number of channels for the selected device
	return m_channelEnabled.size();
}

QVariant ChannelModel::data( const QModelIndex &index, int role ) const
{
	int row = index.row();
	int col = index.column();
	if (role == Qt::DisplayRole && col == 0) {
		return QString("Channel %1").arg(row + 1);
	}
	if (role == Qt::CheckStateRole) {
		if(m_channelEnabled[row] == true)
			return Qt::Checked;
		else 
			return Qt::Unchecked;
	}
	return QVariant();
}

ChannelModel::ChannelModel(QObject *parent):QAbstractTableModel(parent)
{
	// TODO 
}

bool ChannelModel::setData( const QModelIndex& index, const QVariant& value, int role )
{
	if(role == Qt::CheckStateRole) {
		m_channelEnabled[index.row()] = value.toBool();
		//emit editCompleted(value.toBool());
		emit dataChanged(index,index); //index, index == top left, bottom right
	}
	return true;
}

Qt::ItemFlags ChannelModel::flags( const QModelIndex& /*index*/ ) const
{
	return(Qt::ItemIsSelectable | Qt::ItemIsUserCheckable | Qt::ItemIsEnabled);
}

void ChannelModel::toggle( QModelIndex& index )
{
	//m_channelEnabled[index.row()] = !m_channelEnabled[index.row()];
	setData(index, !m_channelEnabled[index.row()], Qt::CheckStateRole);
	qDebug() << QString("Toggled %1").arg(index.row());
	//emit editCompleted();
}
