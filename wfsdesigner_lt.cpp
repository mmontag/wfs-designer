#include "wfsdesigner.h"
#include "wfs3dcanvas.h"
#include <QGenericMatrix>
#include <QMessageBox>
#include <QTime>
#include <QSettings>
#include <QCloseEvent>
#include <QVector2D>
#include <QVector3D>
#include <QTimer>

/*

// Windows Timer Functions
double PCFreq = 0.0;
__int64 CounterStart = 0;

void StartCounter()
{
    LARGE_INTEGER li;
    if(!QueryPerformanceFrequency(&li))
        qDebug() << "QueryPerformanceFrequency failed!\n";

    PCFreq = double(li.QuadPart)/1000.0;

    QueryPerformanceCounter(&li);
    CounterStart = li.QuadPart;
}
double GetCounter()
{
    LARGE_INTEGER li;
    QueryPerformanceCounter(&li);
    return double(li.QuadPart-CounterStart)/PCFreq;
}
*/

WFSDesigner::WFSDesigner(QWidget *parent, Qt::WFlags flags)
	: QMainWindow(parent, flags)
{

	QCoreApplication::setOrganizationName("University of Miami");
	QCoreApplication::setOrganizationDomain("mue.music.miami.edu");
	QCoreApplication::setApplicationName("WFS Designer");

	virtualSourceModel = new VirtualSourceModel();
	physicalModel = new PhysicalModel(); //max loudspeakers
	portAudio = new WFSPortAudio(this);
	wfsConfig = new WFSConfig(portAudio);
	wfsListeningTest = new WFSListeningTest(this);
	ui.setupUi(this);

	delayLineSize = 200000; // per virtual source. TODO: merge with libsndfile primary buffer
	speedOfSound = 345; // meters per second
	firsize = 256; // TODO: DEPRECATED
	m_dezipVel = 0.01; //per sample
	on_volumeSlider_valueChanged(ui.volumeSlider->value()); //initialize m_gain from GUI value

	// Set up graphics view/scene...
	graphicsScene = new QGraphicsScene(this);
	graphicsScene->setSceneRect(-500,-500,1000,1000); // x,y,w,h //TODO: synchronize scene area with WFS3DCanvas

	// Grid could possibly go back in the drawBackground method of WFSCanvas. Use stroke width=0.
	int gridInterval = 100;
	int sLeft = graphicsScene->sceneRect().left();
	int sTop = graphicsScene->sceneRect().top();
	int sBottom = graphicsScene->sceneRect().bottom();
	int sRight = graphicsScene->sceneRect().right();
	QPen dashed = QPen(QBrush(QColor(120,120,120)),0,Qt::DotLine);
	QPen solid = QPen(QBrush(QColor(160,160,160)),0);
	for (int i = sLeft; i <= sBottom; i+=gridInterval) {
		QGraphicsLineItem* line = 
			new QGraphicsLineItem(sLeft, i, sRight, i);
		line->setPen(dashed);
		if(i == 0) line->setPen(solid);
		graphicsScene->addItem(line);
	}
	for (int i = sTop; i <= sRight ; i+=gridInterval) {
		QGraphicsLineItem* line = new QGraphicsLineItem(i, sTop, i, sBottom);
		line->setPen(dashed);
		if(i == 0) line->setPen(solid);
		graphicsScene->addItem(line);
	}

	ui.virtualSourcesList->setModel(virtualSourceModel);
	sourceSelection = ui.virtualSourcesList->selectionModel();
	QObject::connect(sourceSelection, SIGNAL(selectionChanged(const QItemSelection, const QItemSelection)), this, SLOT(on_sourceSelection_changed(const QItemSelection)));

	ui.wfsCanvas->setScene(graphicsScene);
	ui.wfsCanvas->SetCenter(QPoint(0,0));
	ui.wfsCanvas->setZoom(1.5);

	ui.wfs3dCanvas->physicalModel = physicalModel; // pass the model pointers to the opengl view
	ui.wfs3dCanvas->virtualSourceModel = virtualSourceModel; 

	// Hide loudspeaker array 2 and 3 options until user selects a multi-array configuration
	ui.array2Label1->hide();
	ui.array2Label2->hide();
	ui.array2HeightSpinBox->hide();
	ui.array3Label1->hide();
	ui.array3Label2->hide();
	ui.array3HeightSpinBox->hide();

	// Set up statusbar
	statusBarSampleRate = new QLabel(QString("Sample Rate: %1").arg(portAudio->sampleRate));
	statusBarBufferSize = new QLabel(QString("Buffer Size: %1").arg(portAudio->framesPerBuffer));
	//statusBarFIRSize = new QLabel(QString("FIR Size: %1").arg(firsize));
	statusBar()->addWidget(statusBarSampleRate);
	statusBar()->addWidget(statusBarBufferSize);
	//statusBar()->addWidget(statusBarFIRSize);

	readSettings();

	// The controller has the authority to set up the loudspeaker model at startup
	physicalModel->updateLoudspeakersFromChannelList(portAudio->getActiveChannelList()); // SET UP MODEL
	on_configurationChanged(); // ADD ITEMS TO GRAPHICS SCENE
	graphicsScene->addItem(physicalModel->listener);

	// After initialization, things are taken care of through signals and slots
	QObject::connect(wfsConfig, SIGNAL(configurationChanged(QVector<bool>*)), physicalModel, SLOT(updateLoudspeakersFromChannelList(QVector<bool>*)));
	QObject::connect(wfsConfig, SIGNAL(configurationChanged(QVector<bool>*)), this, SLOT(on_configurationChanged()));

	portAudio->start();

	/* 

	BEGIN TEST SETTINGS STUFF

	*/
	loadTestSettings();
}

WFSDesigner::~WFSDesigner()
{

}

void WFSDesigner::on_actionRun_Listening_Test_triggered() {
	/*
	QMessageBox msgBox;
	int ret;
	QTimer timer;
	if(virtualSourceModel->rowCount() > 0) virtualSourceModel->removeRows(0, virtualSourceModel->rowCount());
	on_stopAudio_clicked();

	msgBox.setText("Welcome to the WFS Designer Listening Test.");
	msgBox.setInformativeText("Test will commence when you click OK. Ready?");
	msgBox.setStandardButtons(QMessageBox::Cancel | QMessageBox::Ok);
	msgBox.setIcon(QMessageBox::Information);
	msgBox.setDefaultButton(QMessageBox::Ok);
	ret = msgBox.exec();
	if(ret == QMessageBox::Cancel) return;

	msgBox.setText("Test Tone 1");
	msgBox.setInformativeText("Test tone will now begin playing. Click Ok to continue to the next test tone.");
	ret = msgBox.exec();

	VirtualSource* vs = virtualSourceModel->addSource("noise-bandlimited-6-pulse.wav",100,-300,200);
	initSource(vs);

	on_startAudio_clicked();

	msgBox.setInformativeText("You should now hear a test tone. The tone will play for a few seconds. Click ok to remove the source.");
	ret = msgBox.exec();

	virtualSourceModel->removeRows(0,1);
	VirtualSource* vs = virtualSourceModel->addSource("noise-bandlimited-6-pulse.wav",100,-300,200);
	initSource(vs);
	*/
	wfsListeningTest->show();

}

void WFSDesigner::playTestTone(int tone) {
	QVector<QString> fileNames;
	QVector<QVector3D> positions;
	fileNames.append("noise-bandlimited-6-pulse.wav");
	positions.append(QVector3D(0, -500, 197)); 

	fileNames.append("noise-bandlimited-6-pulse.wav");
	positions.append(QVector3D(-120, -1000, 138));

	fileNames.append("noise-bandlimited-6-pulse.wav");
	positions.append(QVector3D(0, -500, 50)); 

	fileNames.append("noise-6-pulse.wav");
	positions.append(QVector3D(115, -320, 216));

	fileNames.append("noise-bandlimited-6-pulse.wav");
	positions.append(QVector3D(0, -500, 270)); 



	fileNames.append("noise-bandlimited-6-pulse.wav");
	positions.append(QVector3D(-120, -1000, 277));

	fileNames.append("noise-bandlimited-6-pulse.wav");
	positions.append(QVector3D(115, -320, 173));

	fileNames.append("noise-bandlimited-6-pulse.wav");
	positions.append(QVector3D(0, -500, 123)); 

	fileNames.append("noise-6-pulse.wav");
	positions.append(QVector3D(-120, -1000, 0));

	fileNames.append("noise-bandlimited-6-pulse.wav");
	positions.append(QVector3D(115, -320, 130));

	on_stopAudio_clicked();
	if(virtualSourceModel->rowCount() > 0) virtualSourceModel->removeRows(0, virtualSourceModel->rowCount());
	VirtualSource* vs = virtualSourceModel->addSource(fileNames[tone-1],positions[tone-1].x(),positions[tone-1].y(),positions[tone-1].z());
	initSource(vs);
	on_startAudio_clicked();
	QTimer::singleShot(9000, this, SLOT(on_stopAudio_clicked()));

}

void WFSDesigner::loadTestSettings() {

	ui.loudspeakerSpacingSpinBox->setValue(12.85);
	ui.listenerDistanceSpinBox->setValue(290);
	ui.listenerHeightSpinBox->setValue(125);
	ui.array1HeightSpinBox->setValue(82);
	ui.array2HeightSpinBox->setValue(216);
	ui.volumeSlider->setValue(15);
	ui.arrayConfigurationComboBox->setCurrentIndex(GEOM_DOUBLE_LINE);
	on_applyConfigurationButton_clicked();

}

void WFSDesigner::readSettings()
{
	QSettings settings;
	resize(settings.value("mainwindow/size", QSize(700, 600)).toSize());
	move(settings.value("mainwindow/pos", QPoint(100, 100)).toPoint());
	int index = settings.value("portAudio/activeDeviceIndex", 0).toInt();
	portAudio->maybeSwitchDevice(index);
}

void WFSDesigner::writeSettings()
{
	QSettings settings;

	settings.setValue("mainwindow/size", size());
	settings.setValue("mainwindow/pos", pos());
}

void WFSDesigner::closeEvent(QCloseEvent* event) {
	writeSettings();
	event->accept();
}

void WFSDesigner::processAllChannels(float* buffer, int framesPerBuffer, int numChannels) {
	mutex.lock();

	// Zero the leftovers in the portaudio output buffer ...
	memset(buffer, 0, sizeof(float)*framesPerBuffer*numChannels);
	int mode = ui.modeComboBox->currentIndex();
	int numSources = virtualSourceModel->virtualSourceList.count();

	// Copy samples from ring buffer to delayLine, careful to wrap around
	for(int src = 0; src < numSources; src++) {
		if(delayLineWriteIndexes[src] + framesPerBuffer >= delayLineSize) {
			// Split buffer copy
			int splitIndex = delayLineSize - delayLineWriteIndexes[src];
			if(mode == MODE_WFSVBAP_SUBBAND) {
				virtualSourceModel->virtualSourceList.at(src)->copyFromRingBuffer(delayLinesWFS[src]+delayLineWriteIndexes[src], splitIndex);
				virtualSourceModel->virtualSourceList.at(src)->copyFromRingBuffer(delayLinesWFS[src], framesPerBuffer - splitIndex);
				memcpy(delayLinesVBAP[src]+delayLineWriteIndexes[src], delayLinesWFS[src]+delayLineWriteIndexes[src],sizeof(float)*splitIndex);
				memcpy(delayLinesVBAP[src],delayLinesWFS[src], sizeof(float)*(framesPerBuffer - splitIndex));
			} else if(mode == MODE_WFS_BANDLIMITED || mode == MODE_WFS) {
				virtualSourceModel->virtualSourceList.at(src)->copyFromRingBuffer(delayLinesWFS[src]+delayLineWriteIndexes[src], splitIndex);
				virtualSourceModel->virtualSourceList.at(src)->copyFromRingBuffer(delayLinesWFS[src], framesPerBuffer - splitIndex);
			} else if(mode == MODE_VBAP) {
				virtualSourceModel->virtualSourceList.at(src)->copyFromRingBuffer(delayLinesVBAP[src]+delayLineWriteIndexes[src], splitIndex);
				virtualSourceModel->virtualSourceList.at(src)->copyFromRingBuffer(delayLinesVBAP[src], framesPerBuffer - splitIndex);
			}

			// Mode-dependent split filtering
			if(mode == MODE_WFSVBAP_SUBBAND || mode == MODE_WFS_BANDLIMITED) {
				lpFilters[src]->processReplacing(delayLinesWFS[src]+delayLineWriteIndexes[src], splitIndex);
				lpFilters[src]->processReplacing(delayLinesWFS[src],framesPerBuffer - splitIndex);
			}
			if(mode == MODE_WFSVBAP_SUBBAND) {
				hpFilters[src]->processReplacing(delayLinesVBAP[src]+delayLineWriteIndexes[src], splitIndex);
				hpFilters[src]->processReplacing(delayLinesVBAP[src],framesPerBuffer - splitIndex);
			}
		} else {
			// Continuous buffer copy
			if(mode == MODE_WFSVBAP_SUBBAND) {
				virtualSourceModel->virtualSourceList.at(src)->copyFromRingBuffer(delayLinesWFS[src]+delayLineWriteIndexes[src], framesPerBuffer);
				memcpy(delayLinesVBAP[src]+delayLineWriteIndexes[src], delayLinesWFS[src]+delayLineWriteIndexes[src], sizeof(float)*framesPerBuffer);
			} else if(mode == MODE_WFS_BANDLIMITED || mode == MODE_WFS) {
				virtualSourceModel->virtualSourceList.at(src)->copyFromRingBuffer(delayLinesWFS[src]+delayLineWriteIndexes[src], framesPerBuffer);
			} else if(mode == MODE_VBAP) {
				virtualSourceModel->virtualSourceList.at(src)->copyFromRingBuffer(delayLinesVBAP[src]+delayLineWriteIndexes[src], framesPerBuffer);
			}
			
			// Mode-dependent continuous filtering
			if(mode == MODE_WFSVBAP_SUBBAND || mode == MODE_WFS_BANDLIMITED) {
				lpFilters[src]->processReplacing(delayLinesWFS[src]+delayLineWriteIndexes[src], framesPerBuffer);
			}
			if(mode == MODE_WFSVBAP_SUBBAND) {
				hpFilters[src]->processReplacing(delayLinesVBAP[src]+delayLineWriteIndexes[src], framesPerBuffer);
			}
		}

		for(int channel = 0; channel < numChannels; channel++) {
			//VirtualSource* source = virtualSourceModel->virtualSourceList.at(src);
			//Loudspeaker* lspk = physicalModel->loudspeakerList.at(channel);
			//int delay = calculateDelay(source, lspk);
			//float gain = calculateGain(source, lspk) * source->gain;
			//qDebug() << QString("Source %1 to Loudspeaker %2 delay: %3 samples/gain: %4").arg(src).arg(channel).arg(delay).arg(gain);

			// Interpolation
			int delay = delaySamples[src][channel][0];
			int oldDelay = delaySamples[src][channel][1]; 
			float delayStep = 0.0;
			if(delay != oldDelay) delayStep = float(delay-oldDelay)/framesPerBuffer;

			VirtualSource* source = virtualSourceModel->virtualSourceList.at(src);
			if(mode == MODE_WFSVBAP_SUBBAND || mode == MODE_WFS_BANDLIMITED || mode == MODE_WFS) {
				
				float gain = gainsWFS[src][channel] * source->gain;
				if(gain > 0) {
					for(int i = 0; i < framesPerBuffer; i++) {
						int delayIndex = (delayLineWriteIndexes[src] - int(oldDelay+delayStep*i)); // Interpolation magic
						if(delayIndex < 0) delayIndex += delayLineSize;
						buffer[i*numChannels+channel] += delayLinesWFS[src][(delayIndex+i)%delayLineSize] * gain * m_gainTo;
					}
				}
			}
			if(mode == MODE_WFSVBAP_SUBBAND || mode == MODE_VBAP) {
				float gain = vbapBank[src][channel] * source->gain; //TODO: Make sure VBAP gain incorporates (B/A+B)^.5 law
				if(gain > 0) {
					for(int i = 0; i < framesPerBuffer; i++) {
						int delayIndex = (delayLineWriteIndexes[src] - int(oldDelay+delayStep*i));
						if(delayIndex < 0) delayIndex += delayLineSize;
						buffer[i*numChannels+channel] += delayLinesVBAP[src][(delayIndex+i)%delayLineSize] * gain * m_gainTo;
					}
				}
			}
			delaySamples[src][channel][1] = delay; // Update oldDelay slot
		}

		//Advance write pointer
		delayLineWriteIndexes[src] = (delayLineWriteIndexes[src] + framesPerBuffer) % delayLineSize;
	}


	/*
	// An alternative approach using the pa_ringbuffer in place...maybe later
	foreach(VirtualSource src, virtualSourceModel->virtualSourceList) {
		PaUtil_AdvanceRingBufferReadIndex(src->ringbuffer,framesPerBuffer);
		int thisReadIndex = src->ringbuffer->readIndex; // * src->ringbuffer->elementSizeBytes;
	}
	*/
	mutex.unlock();
}

//TODO: DEPRECATED
#ifdef WFS_FIR
void WFSDesigner::processConvolvingAllChannels(float* buffer, int framesPerBuffer, int numChannels) {
	mutex.lock();
	/*
	Process all sources through all filters:

										M Channels

					    0    1    2    3    4    5    6    7    8        M-1
					0 [   ][   ][   ][   ][   ][   ][   ][   ][   ]     [   ]
					1 [   ][   ][   ][   ][   ][   ][   ][   ][   ]     [   ]
		N Sources	2 [   ][   ][   ][   ][   ][   ][   ][   ][   ] ... [   ]
					3 [   ][   ][   ][   ][   ][   ][   ][   ][   ]     [   ]
										   ...
				  N-1 [   ][   ][   ][   ][   ][   ][   ][   ][   ]     [   ]
					  -------------------------------------------------------
				  Sum [   ][   ][   ][   ][   ][   ][   ][   ][   ] ... [   ]

	Reproduce this logic here...

				for(int j = 0; j < numChannels; j++) {
						
					// Mix result of all virtual sources for this channel...
					// Pass the processChannel method a pointer to the channelBuffer
					((WFSDesigner*)controller)->processChannel(channelBuffer, framesPerBuffer, j);

					for(int i = 0; i < framesPerBuffer; i++) {

						// Interleave (distribute) the result of this channel operation into the output buffer
						output[i*numChannels] = channelBuffer[i];

					}
				}

	*/

	if(framesPerBuffer != buffersize) {
		buffersize = framesPerBuffer;
		statusBarBufferSize->setText(QString("Buffer Size: %1").arg(framesPerBuffer));
		wavVBAP = new float[framesPerBuffer];
		wavWFS = new float[framesPerBuffer];
		result = new float[framesPerBuffer];
	}


	// Zero the leftovers in the portaudio output buffer ...
	memset(buffer, 0, sizeof(float)*framesPerBuffer*numChannels);										// PORTAUDIO BUFFER SIZE

	int numSources = virtualSourceModel->virtualSourceList.count();
	int mode = ui.modeComboBox->currentIndex();
	for(int source = 0; source < numSources; source++) {
		// Synthesis method:
		// 0 = WFS + VBAP
		// 1 = WFS (Bandlimited)
		// 2 = WFS 
		// 3 = VBAP

		// Prepare input buffer from WAV on disk ...
		if(mode == 0) {
			virtualSourceModel->virtualSourceList.at(source)->copyFromRingBuffer(wavWFS, framesPerBuffer);
			memcpy(wavVBAP, wavWFS, sizeof(float)*framesPerBuffer);
		} else if (mode == 1 || mode == 2) {
			virtualSourceModel->virtualSourceList.at(source)->copyFromRingBuffer(wavWFS, framesPerBuffer);
		} else if(mode == 3) {
			virtualSourceModel->virtualSourceList.at(source)->copyFromRingBuffer(wavVBAP, framesPerBuffer);
		}

		// Scale signal according to source level (gain)
		// TODO: Move gain scaling to updateFilterBank? (and updateVbapBank?...)
		for(int i = 0; i < framesPerBuffer; i++) {
			//Generic dezippering routine - TODO: implement as static "dezip(current,final)" method
			if(m_gain != m_gainTo) {
				if(fabsf(m_gain - m_gainTo) < m_dezipVel) { // fabsf() is floating point abs()
					m_gain = m_gainTo; // once value is within the threshold (dezipIncr) of its target, set to equal
				} else { 
					if(m_gain < m_gainTo)
						m_gain += m_dezipVel;
					else if (m_gain > m_gainTo)
						m_gain -= m_dezipVel;
				}
			}
			if(virtualSourceModel->virtualSourceList.at(source)->gain != 1) {
				wavVBAP[i] = wavVBAP[i] * virtualSourceModel->virtualSourceList.at(source)->gain;
				wavWFS[i] = wavWFS[i] * virtualSourceModel->virtualSourceList.at(source)->gain;
			}
		}

		if(mode == 0 || mode == 1) {
			// Lowpass the input buffer into the wavWFS buffer
			lpFilters[source]->processReplacing(wavWFS, framesPerBuffer);
		}

		if(mode == 0) {
			// Highpass filter the input buffer in-place
			hpFilters[source]->processReplacing(wavVBAP, framesPerBuffer);
		}

		for(int channel = 0; channel < numChannels; channel++) {
			/*
			NOTICE
			We want to minimize memory copying and copy constructors around here;
			we also want to avoid creating objects and allocating new memory,
			so we are passing around references and working on the stack.
			*/

//TODO: STUFF DOESNT WORK WHEN SELECTIVE LOUDSPEAKERS ARE DISABLED IN CONFIGURATION

			if(mode == 0 || mode == 1 || mode == 2) {
				// Convolve wav with FIR for this virtualsource/channel pair
					
				//QTime timer;
				//timer.start();
				//StartCounter();

				filterBank.at(source).at(channel)->process(wavWFS, result, framesPerBuffer); //TODO: WFSFilters sometimes not initialized? 

				//qDebug() << QString("FFTW convolved %1 samples with %2  fir in %3 ms").arg(framesPerBuffer).arg(firsize).arg(GetCounter());

				// WFS: Interleave and mix low frequency content ...
				float wfsGain = (1/sqrt((float)numSources)) * m_gain;
				for(int j = 0; j < framesPerBuffer; j++) {
					buffer[j*numChannels+channel] += wfsGain * result[j];
				}
			}

			if(mode == 0 || mode == 3) {
				// VBAP: Interleave and mix high pass content ...
				float vbapGain = (1/sqrt((float)numSources)) * m_gain;
				if(vbapBank[source][channel] > 0) {
					for(int j = 0; j < framesPerBuffer; j++) {
						buffer[j*numChannels+channel] += vbapGain * vbapBank[source][channel] * wavVBAP[j];
					}
				}
			}
		}
	}

	mutex.unlock();
}
#endif

void WFSDesigner::on_actionAbout_WFS_Designer_triggered() {
	QMessageBox::about(this, "About WFS Designer", "WFS Designer\n2011 Matt Montag\n\n"
							"WFS Designer uses Qt, PortAudio, LibSndFile, FFTW, and OpenGL.\n"
							"University of Miami - Music Engineering Technology"); //<a href=\"http://mue.music.miami.edu\"></a>
}

void WFSDesigner::on_actionAudio_Configuration_triggered() {
	portAudio->stop();
	wfsConfig->show();
}

void WFSDesigner::on_action2D_Scene_View_triggered() {
	ui.wfs3dCanvas->hide();
	ui.wfs3dCanvas->setDisabled(true);
	ui.wfsCanvas->show();
	ui.wfsCanvas->setEnabled(true);
}

void WFSDesigner::on_action3D_Scene_View_triggered() {
	ui.wfs3dCanvas->show();
	ui.wfs3dCanvas->setEnabled(true);
	ui.wfsCanvas->hide();
	ui.wfsCanvas->setDisabled(true);
}

void WFSDesigner::on_action2D_3D_Split_Scene_View_triggered() {
	ui.wfsCanvas->show();
	ui.wfs3dCanvas->show();
	ui.wfsCanvas->setEnabled(true);
	ui.wfs3dCanvas->setEnabled(true);
}

void WFSDesigner::on_stopAudio_clicked()
{
	// TODO: foreach virtualSource->stop();
	portAudio->softStop();

	foreach(VirtualSource* vs, virtualSourceModel->virtualSourceList) {
		vs->resetRingBuffer();
		//TODO: reset delayLinesVBAP and delayLinesWFS when user clicks stop
	}
}

void WFSDesigner::on_startAudio_clicked()
{
	foreach(Biquad* lp, lpFilters) {
		lp->prepareForPlay();
	}
	foreach(Biquad* hp, hpFilters) {
		hp->prepareForPlay();
	}
	portAudio->softStart();
}

void WFSDesigner::on_addSource_clicked() 
{
	/*
	Add a source to the graphics scene
	*/
	VirtualSource* vs = virtualSourceModel->addSource(); // No fileName argument will show file prompt
	initSource(vs);

}

void WFSDesigner::initSource(VirtualSource* vs) {
	mutex.lock();

	if(vs->readyForPlay == false) return;

	graphicsScene->addItem(vs);
	int numChannels = physicalModel->loudspeakerList.count();

#ifdef WFS_FIR
	/*
	Add source to WFS FilterBank
	*/
	//create a filter for every channel linked to this source:
	QVector<WFSFilter*> channels;
	for(int j = 0; j < numChannels; j++) {
		WFSFilter* filter = new WFSFilter(portAudio->framesPerBuffer, firsize);  //WARNING: make sure filter array is filled with zeros. 
		channels.append(filter); 
		qDebug() << QString("Appended channel %1 filter to a new source.").arg(j);
	}
	filterBank.append(channels);
	updateFilterBank(virtualSourceModel->virtualSourceList.last()); //update the FIRs for the channels that were just added
#endif

	/*
	Add source to VBAP FilterBank
	*/
	QVector<float> channelsVBAP;
	for(int j = 0; j < numChannels; j++) {
		channelsVBAP.append(0.0);
	}
	vbapBank.append(channelsVBAP);
	updateVbapBank(virtualSourceModel->virtualSourceList.last()); //update the VBAP amplitudes

	// April 5 changes: Delay line implementation

	/*
	Add source to Delays and Gains matrix
	*/
	QVector<float> gains;
	QVector<QVector<int>> delays;
	for(int j = 0; j < numChannels; j++) {
		gains.append(0.0);
		QVector<int> d;
		d.append(0); 
		d.append(0);
		delays.append(d);
	}
	gainsWFS.append(gains);
	delaySamples.append(delays);

	/*
	Create lowpass and highpass filters
	These could all get moved into the VirtualSource
	*/
	lpFilters.append(new Biquad(portAudio->sampleRate, Biquad::LinkwitzRileyLowPass));
	lpFilters.last()->updateFilterCoeffs(ui.crossoverSpinBox->value());
	lpFilters.last()->prepareForPlay();
	hpFilters.append(new Biquad(portAudio->sampleRate, Biquad::LinkwitzRileyHighPass));
	hpFilters.last()->updateFilterCoeffs(ui.crossoverSpinBox->value());
	hpFilters.last()->prepareForPlay();
	delayLinesVBAP.append(new float[delayLineSize]);
	memset(delayLinesVBAP.last(),0,sizeof(float)*delayLineSize);
	delayLinesWFS.append(new float[delayLineSize]);
	memset(delayLinesWFS.last(),0,sizeof(float)*delayLineSize);
	delayLineWriteIndexes.append(0);
	updateGainsAndDelays(virtualSourceModel->virtualSourceList.last());

	/*
	Connect source signals
	*/
	QObject::connect(vs, SIGNAL(sourceClicked(VirtualSource*)), this, SLOT(on_sourceClicked(VirtualSource*)));
	QObject::connect(vs, SIGNAL(sourceMoved(VirtualSource*)), this, SLOT(on_sourceMoved(VirtualSource*)));
	QObject::connect(vs, SIGNAL(sourceReleased(VirtualSource*)), this, SLOT(on_sourceReleased(VirtualSource*)));
	qDebug() << QString("Connecting source %1 signals...").arg(static_cast<VirtualSource*>(vs)->id);

	mutex.unlock();
	ui.wfs3dCanvas->update();
}

void WFSDesigner::on_removeSource_clicked()
{
	mutex.lock();

	QItemSelectionModel* sel = ui.virtualSourcesList->selectionModel();
	foreach(QModelIndex i, sel->selectedIndexes()) {
		//Remove sources from filterbank
		//is filterbank indexing guaranteed to match source indexing?
		qDebug() << QString("Deleted source %1. Removed from filterBank.").arg(i.row());
		virtualSourceModel->removeRow(i.row());
		//filterBank.remove(i.row());

		//TODO: Fix old and current filterbank stuff - remove the filterbank row here
		//This line was commented, and was a cause of memory access errors on source removal
	}

	mutex.unlock();
	ui.wfs3dCanvas->update();
}

void WFSDesigner::on_volumeSlider_valueChanged( int value )
{
	m_gainTo = (float)value/ui.volumeSlider->maximum(); //cook slider value to a gain from 0 to 1
	qDebug() << "Master volume changed: value " << value;
}

void WFSDesigner::on_arrayConfigurationComboBox_currentIndexChanged(const QString &text){
	if(!text.contains("Double") && !text.contains("Single")) {
		ui.array2Label1->hide();
		ui.array2Label2->hide();
		ui.array2HeightSpinBox->hide();
		ui.array3Label1->hide();
		ui.array3Label2->hide();
		ui.array3HeightSpinBox->hide();
	}
	if(text.contains("Double") || text.contains("Triple")) {
		ui.array2Label1->show();
		ui.array2Label2->show();
		ui.array2HeightSpinBox->show();
	}
	if(text.contains("Triple")) {
		ui.array3Label1->show();
		ui.array3Label2->show();
		ui.array3HeightSpinBox->show();
	}
	if(text.contains("Circle")) {
		on_loudspeakerSpacingSpinBox_valueChanged(ui.loudspeakerSpacingSpinBox->value());
	}
}

void WFSDesigner::on_applyConfigurationButton_clicked() {
	/*
	Rearrange the physical model according to the array configuration parameters.
	*/

	// Ungroup all loudspeakers - TODO: move into class method of physicalModel
	/*
	foreach(QGraphicsItemGroup* group, physicalModel->loudspeakerGroupList) {
		graphicsScene->destroyItemGroup(group);
	}
	*/

	physicalModel->arrayListenerDistance = ui.listenerDistanceSpinBox->value();
	physicalModel->arrayGeometry = ui.arrayConfigurationComboBox->currentIndex();
	physicalModel->arrayNumRows = 1;
	physicalModel->listener->setZ(ui.listenerHeightSpinBox->value());

	float spacing = ui.loudspeakerSpacingSpinBox->value();
	float num = physicalModel->loudspeakerList.count();
	float totalwidth = (num-1) * spacing;
	float radius = totalwidth / 2 / M_PI;
	float origin = 0 - totalwidth/2;
	switch(ui.arrayConfigurationComboBox->currentIndex()) {
		case GEOM_LINE: //Line
			qDebug() << QString("Arranging loudspeakers in line.");
			for (int col = 0; col < num; col++) {
				Loudspeaker* lspk = physicalModel->loudspeakerList.at(col);
				lspk->setPos(origin + spacing * col, -physicalModel->arrayListenerDistance); // place X centimeters away from listener
				lspk->setZ(ui.array1HeightSpinBox->value());
				lspk->arrayNormal = QVector2D(0, -1);
				lspk->show();
				if(num > 4) {
					if(col == 0 || col == num-1) {
						lspk->taperGain = .2;
					} else if(col == 1 || col == num-2) {
						lspk->taperGain = .75;
					} else {
						lspk->taperGain = 1;
					}
				} else {
					lspk->taperGain = 1;
				}
			}
			updateCrossoverFromSpacing();
			break;
		case GEOM_CIRCLE: //Circle
			qDebug() << QString("Arranging loudspeakers in circle.");
			for(int i = 0; i < num; i++) {
				Loudspeaker* lspk = physicalModel->loudspeakerList.at(i);
				lspk->setPos(radius*sin(M_PI+i*(2*M_PI/num)), radius*cos(M_PI+i*(2*M_PI/num)));
				lspk->setZ(ui.array1HeightSpinBox->value());
				lspk->arrayNormal = QVector2D(lspk->pos()).normalized();
				lspk->taperGain = 1;
				lspk->show();
			}
			updateCrossoverFromSpacing();
			break;
		case GEOM_DOUBLE_LINE: {	//Double Line
			qDebug() << QString("Arranging loudspeakers in double line.");
			int arrayRows = 2;
			physicalModel->arrayNumRows = arrayRows;
			int spkPerRow = floor(float(num) / arrayRows);
			physicalModel->arrayNumCols = spkPerRow;

			float totalwidth = (spkPerRow-1) * spacing;
			float origin = 0 - totalwidth/2;
			float rowZ = ui.array1HeightSpinBox->value();
			if(spkPerRow < 2) {
				QMessageBox::information(this, "WFS Designer Error", "The current settings would result in an invalid array configuration (not enough loudspeakers).");
				return;
			}
			for(int row = 0; row < arrayRows; row++) {
				for (int col = 0; col < spkPerRow; col++) {
					int lspkIdx = row*spkPerRow + col;
					Loudspeaker* lspk = physicalModel->loudspeakerList.at(lspkIdx);
					if(row == 0) {
						lspk->show();
					} else {
						//lspk->hide();		
					}
					lspk->setX(origin + spacing * col);  
					lspk->setY(-physicalModel->arrayListenerDistance-row*33); //TODO: 33 cm offset for TEST SETUP ONLY
					lspk->setZ(rowZ);
					lspk->arrayNormal = QVector2D(0, -1);
					if(num > 4) {
						if(col == 0 || col == spkPerRow-1) {
							lspk->taperGain = .2;
						} else if(col == 1 || col == spkPerRow-2) {
							lspk->taperGain = .75;
						} else {
							lspk->taperGain = 1;
						}
					} else {
						lspk->taperGain = 1;
					}
				}
				rowZ = ui.array2HeightSpinBox->value();
			}
			updateCrossoverFromSpacing();

			/*
			// Group loudspeaker columns
			for(int col = 0; col < spkPerRow; col++) {
				QGraphicsItemGroup *group = new QGraphicsItemGroup();
				graphicsScene->addItem(group);
				for(int row = 0; row < arrayRows; row++) {
					int lspkIdx = row*spkPerRow + col;
					Loudspeaker* lspk = physicalModel->loudspeakerList.at(lspkIdx);
					group->addToGroup(lspk);
				}
			}
			*/

			// Delete unused loudspeakers
			for(int i = arrayRows*spkPerRow; i < num; i++) {
				Loudspeaker *lspk = physicalModel->loudspeakerList.at(i);
				physicalModel->loudspeakerList.removeAt(i);
				delete lspk;
			}
		}
		break;
		case GEOM_DOUBLE_CIRCLE: { //Double Circle
			qDebug() << QString("Arranging loudspeakers in double circle.");
			int arrayRows = 2;
			physicalModel->arrayNumRows = arrayRows;
			int spkPerRow = floor(float(num) / arrayRows);
			physicalModel->arrayNumCols = spkPerRow;
		}
		break;
		case GEOM_BOX: //Box
			qDebug() << QString("Arranging loudspeakers in box.");
			/* TODO: finish implementing box arrangement
			int sideNum = floor(num/4);
			for(int i = 0; i < sideNum; i++) {
				float offset = ui.loudspeakerSpacingSpinBox->value() * (sideNum - 1 + i) / 2;
				Loudspeaker* lspk = physicalModel->loudspeakerList.at(i);
				lspk->setPos(-offset, offset);
				lspk->arrayNormal = QVector2D(
				Loudspeaker* lspk = physicalModel->loudspeakerList.at(i*2);
				Loudspeaker* lspk = physicalModel->loudspeakerList.at(i*3);
				Loudspeaker* lspk = physicalModel->loudspeakerList.at(i*4);
			}
			*/
			break;
		case GEOM_U_SHAPE: //U-shape
			qDebug() << QString("Arranging loudspeakers in U-shape.");
			break;
	}
	ui.wfs3dCanvas->update();
}

void WFSDesigner::on_crossoverSpinBox_valueChanged(double value) {
	foreach(Biquad* lp, lpFilters) {
		lp->prepareForPlay();
		lp->updateFilterCoeffs(float(value));
	}
	foreach(Biquad* hp, hpFilters) {
		hp->prepareForPlay();
		hp->updateFilterCoeffs(float(value));
	}
}

void WFSDesigner::on_automaticCheckBox_toggled(bool checked) {
	if(checked == true) {
		ui.crossoverSpinBox->setReadOnly(true);
		updateCrossoverFromSpacing();
	} else {
		ui.crossoverSpinBox->setReadOnly(false);
	}

}

void WFSDesigner::updateCrossoverFromSpacing() {
	if(ui.automaticCheckBox->checkState() == Qt::Checked) {
		float frequency = speedOfSound/(ui.loudspeakerSpacingSpinBox->value()/100);
		ui.crossoverSpinBox->setValue(frequency);
	}
}

void WFSDesigner::on_configurationChanged() {
	qDebug("received configuration changed");
	//Here is where we add every loudspeaker item back to the graphics scene.
	//This is done here because the loudspeakers are allocated dynamically.
	ui.numChannelsLabel->setText(QString::number(physicalModel->loudspeakerList.count()));
	foreach(Loudspeaker* lspk, physicalModel->loudspeakerList) {
		graphicsScene->addItem(lspk);
	}

	//chain the Apply Configuration button...
	on_applyConfigurationButton_clicked();
	//chain the filterBank computation...
	resizeFilterBank();
	//dynamic SIGNALS and SLOTS - do this last so the signals don't get called during setup.
	foreach(Loudspeaker* lspk, physicalModel->loudspeakerList) {
		QObject::connect(lspk, SIGNAL(loudspeakerMoved(Loudspeaker*)), this, SLOT(on_loudspeakerMoved(Loudspeaker*)));
		QObject::connect(lspk, SIGNAL(loudspeakerMovedFinal(Loudspeaker*)), this, SLOT(resizeFilterBank())); 
		qDebug() << QString("Connecting source %1 to updateFilterBank()...").arg(lspk->id);
	}

	QSettings settings;
	settings.setValue("portAudio/activeDeviceIndex", portAudio->activeDeviceIndex);
	ui.wfs3dCanvas->update();
}

void WFSDesigner::on_loudspeakerMoved(Loudspeaker* lspk) {
	/*
	if(physicalModel->arrayNumRows > 1) {
		for(int row = 1; row < physicalModel->arrayNumRows; row++) {
			Loudspeaker* l = physicalModel->loudspeakerList.at(row*physicalModel->arrayNumCols+lspk->id);
			l->setPos(lspk->pos());
		}
	}
	*/
	ui.wfs3dCanvas->update();
}

void WFSDesigner::resizeFilterBank() {

	/*
	Reallocate filter bank QVectors...this destroys the filters
	Imagine a virtual source was just added...

		[0,0]           [1,0]           [2,0]    ...
	{ [ ( ) ( ) ( ) ] [ ( ) ( ) ( ) ] [ ( ) ( ) ( ) ] }

	There is certainly a more efficient way to do this, but for now 
	we expect the user will rarely change the loudspeaker configuration.
	*/

	int numSources = virtualSourceModel->virtualSourceList.count();
	int numChannels = physicalModel->loudspeakerList.count();

	mutex.lock();

#ifdef WFS_FIR
	/*
	WFS FilterBank
	*/
	qDebug() << QString("Destroying and reallocating filterBank.");
	filterBank.clear();
	//create the filter rack for each source:
	for(int i = 0; i < numSources; i++) {
		//create a filter for every channel linked to this source:
		QVector<WFSFilter*> channels;
		for(int j = 0; j < numChannels; j++) {
			WFSFilter* filter = new WFSFilter(portAudio->framesPerBuffer, firsize);  //WARNING: make sure filter array is filled with zeros. 
			channels.append(filter); 
			qDebug() << QString("Appended channel %1 filter to source %2.").arg(i).arg(j);
		}
		filterBank.append(channels);
		updateFilterBank(virtualSourceModel->virtualSourceList.at(i)); //update the FIRs for the channels that were just added
		// remember that we are now updating all sources when the number of loudspeakers changes
	}
#endif

	/*
	VBAP FilterBank
	*/
	qDebug() << QString("Destroying and reallocating filterBank.");
	vbapBank.clear();
	for(int i = 0; i < numSources; i++) {
		//create a filter for every channel linked to this source:
		QVector<float> channels;
		for(int j = 0; j < numChannels; j++) {
			channels.append(0.0); 
		}
		vbapBank.append(channels);
		updateVbapBank(virtualSourceModel->virtualSourceList.at(i));
		// remember that we are now updating all sources when the number of loudspeakers changes
	}

	// April 5 changes

	/*
	Sample Delay and Gains Matrix
	*/
	delaySamples.clear();
	gainsWFS.clear();
	for(int i = 0; i < numSources; i++) {
		QVector<QVector<int>> delays;
		QVector<float> gains;
		for(int j = 0; j < numChannels; j++) {
			QVector<int> d;
			d.append(0);
			d.append(0);
			delays.append(d);
			gains.append(0.0);
		}
		delaySamples.append(delays);
		gainsWFS.append(gains);
		updateGainsAndDelays(virtualSourceModel->virtualSourceList.at(i));
		// remember that we are now updating all sources when the number of loudspeakers changes
	}
	
	mutex.unlock();
	
}

void WFSDesigner::on_sourceClicked(VirtualSource* src) {
	int sourceIdx = virtualSourceModel->virtualSourceList.indexOf(src);
	// Setting the selection will update the volume slider and the spinboxes
	sourceSelection->select(virtualSourceModel->index(sourceIdx,0),QItemSelectionModel::ClearAndSelect); 
}

void WFSDesigner::on_sourceMoved(VirtualSource* src) {
	updateVbapBank(src);
	//updateFilterBank(src);
	updateGainsAndDelays(src);

	/*
	Update loudspeaker visual display to show gains for this source
	*/
	int sourceID = virtualSourceModel->virtualSourceList.indexOf(src);
	foreach(Loudspeaker* lspk, physicalModel->loudspeakerList) {
		//TODO: major design issue. Need to separate WFS gain from WFS delay.
		lspk->setSelected(false);
		lspk->srcVBAPGain = vbapBank[sourceID][lspk->id];
		//lspk->srcWFSGain = filterBank[sourceID][lspk->id]->gain;
		lspk->srcWFSGain = gainsWFS[sourceID][lspk->id];
		lspk->srcWFSDelay = delaySamples[sourceID][lspk->id][0];
		lspk->update();
	}
	ui.sourceXSpinBox->setValue(src->x());
	ui.sourceYSpinBox->setValue(src->y());
	ui.sourceZSpinBox->setValue(src->z());
	ui.wfs3dCanvas->update();
}

void WFSDesigner::on_sourceReleased(VirtualSource* src) {
	/*
	Turn off loudspeaker gain visualisation for all Loudspeaker items
	*/
	foreach(Loudspeaker* lspk, physicalModel->loudspeakerList) {
		lspk->srcVBAPGain = 0.0;
		lspk->srcWFSGain = 0.0;
		lspk->srcWFSDelay = 0;
		lspk->update();
	}
	ui.wfs3dCanvas->update();
}

void WFSDesigner::on_sourceSelection_changed(const QItemSelection &selected){	
	if(selected.count() < 1) {
		ui.sourceSettingsGroupBox->setDisabled(true);
	} else {
		ui.sourceSettingsGroupBox->setEnabled(true);
		VirtualSource* src = virtualSourceModel->virtualSourceList.at(selected.at(0).top());
		ui.sourceVolumeSlider->setValue(100 * src->gain);
		ui.sourceXSpinBox->setValue(src->x());
		ui.sourceYSpinBox->setValue(src->y());
		ui.sourceZSpinBox->setValue(src->z());
	}
}

void WFSDesigner::on_sourceVolumeSlider_valueChanged(int value) {
	if(sourceSelection->selectedRows().count() > 0) {
		VirtualSource* src = virtualSourceModel->virtualSourceList.at(sourceSelection->selectedRows().first().row());
		src->gain = (float)value / 100;
	}
}

void WFSDesigner::on_sourceXSpinBox_valueChanged(double value) {
	if(sourceSelection->selectedRows().count() > 0) {
		VirtualSource* src = virtualSourceModel->virtualSourceList.at(sourceSelection->selectedRows().first().row());
		src->setX(value);
		ui.wfs3dCanvas->update();
	}
}

void WFSDesigner::on_sourceYSpinBox_valueChanged(double value) {
	if(sourceSelection->selectedRows().count() > 0) {
		VirtualSource* src = virtualSourceModel->virtualSourceList.at(sourceSelection->selectedRows().first().row());
		src->setY(value);
		ui.wfs3dCanvas->update();
	}
}

void WFSDesigner::on_sourceZSpinBox_valueChanged(double value) {
	if(sourceSelection->selectedRows().count() > 0) {
		VirtualSource* src = virtualSourceModel->virtualSourceList.at(sourceSelection->selectedRows().first().row());
		src->setZ(value);
		ui.wfs3dCanvas->update();
	}
}

void WFSDesigner::on_listenerDistanceSpinBox_valueChanged(double value) {
	if(ui.arrayConfigurationComboBox->currentText().contains("Circle")) {
		int num = physicalModel->loudspeakerList.count();
		ui.loudspeakerSpacingSpinBox->setValue(2 * M_PI * value / num);
	}
}

void WFSDesigner::on_loudspeakerSpacingSpinBox_valueChanged(double value) {
	if(ui.arrayConfigurationComboBox->currentText().contains("Circle")) {
		int num = physicalModel->loudspeakerList.count();
		ui.listenerDistanceSpinBox->setValue(num * ui.loudspeakerSpacingSpinBox->value()/(2 * M_PI));
	}
}

void WFSDesigner::updateGainsAndDelays(VirtualSource *src) {
	int sourceID = virtualSourceModel->virtualSourceList.indexOf(src);
	if(physicalModel->arrayNumRows == 1) {
		// Calculate 2D delay and gain for all loudspeakers
		foreach(Loudspeaker* lspk, physicalModel->loudspeakerList) {
			delaySamples[sourceID][lspk->id][0] = calculateDelay(src, lspk);
			gainsWFS[sourceID][lspk->id] = calculateGain(src, lspk);
		}
	} else {
		// Calculate gain for the first row of loudspeakers
		for(int col = 0; col < physicalModel->arrayNumCols; col++) {  // First row only
			Loudspeaker* lspk = physicalModel->loudspeakerList.at(col);
			gainsWFS[sourceID][col] = calculateGain(src, lspk);
		}
		// Calculate 3D delay for all loudspeakers
		foreach(Loudspeaker* lspk, physicalModel->loudspeakerList) {
			delaySamples[sourceID][lspk->id][0] = calculateDelay3D(src, lspk);
		}

		// Prepare vectors for VBAP in YZ plane (vertical plane)
		Listener* lstnr = physicalModel->listener;
		QVector2D srcYZ = QVector2D(
			src->z() - lstnr->z(),
			src->y() - lstnr->y()).normalized();
		QVector2D row1YZ = QVector2D(
			physicalModel->loudspeakerList.at(0)->z() - lstnr->z(),
			physicalModel->loudspeakerList.at(0)->y()).normalized();
		QVector2D row2YZ = QVector2D(
			physicalModel->loudspeakerList.at(physicalModel->arrayNumCols)->z() - lstnr->z(),
			physicalModel->loudspeakerList.at(physicalModel->arrayNumCols)->y() - lstnr->y()).normalized();
		float row1gain;
		float row2gain;
		amplitudePan2D(row1YZ, row2YZ, srcYZ, row1gain, row2gain);

		// Apply vertical plane VBAP result to the horizontal plane WFS gain result
		for(int col = 0; col < physicalModel->arrayNumCols; col++) {
			// Second row
			gainsWFS[sourceID][col+physicalModel->arrayNumCols] = gainsWFS[sourceID][col] * row2gain;
			// First row 
			gainsWFS[sourceID][col] *= row1gain;
		}
	}
}

/*
Function: amplitudePan2D
Calculates vector-base/tangent-law/amplitude-power panning 
solution and plugs it into lspk1gain and lspk2gain. 
lspk1 and lspk2 are vectors from listener to loudspeakers.
src is a vector from listener to phantom source location.
*/
void WFSDesigner::amplitudePan2D(QVector2D lspk1, QVector2D lspk2, QVector2D src, float &lspk1gain, float &lspk2gain) {
	/*
	sourceVec = gainVec * loudspeakerBaseMatrix
	gainVec = sourceVec * loudspeakerBaseMatrixInverse
	*/
	QMatrix lspkBaseMatrix(lspk1.x(), lspk1.y(), lspk2.x(), lspk2.y(), 0, 0);
	lspkBaseMatrix = lspkBaseMatrix.inverted();
	QVector2D gainVec(	src.x() * lspkBaseMatrix.m11() + src.y() * lspkBaseMatrix.m21(),
						src.x() * lspkBaseMatrix.m12() + src.y() * lspkBaseMatrix.m22());
	if(gainVec.x() > 1) gainVec.setX(1);
	if(gainVec.y() > 1) gainVec.setY(1);
	lspk1gain = gainVec.x();
	lspk2gain = gainVec.y();
}

void WFSDesigner::updateVbapBank(VirtualSource* src) {
	/*
	VBAP FilterBank
	Update the row in the filter bank for this source. 
	Standard tangent-law amplitude panning currently implemented - 
	assumes single-row linear loudspeaker arrangement

	TODO: update for multi-row array geometry
	*/

	int sourceID = virtualSourceModel->virtualSourceList.indexOf(src);
	
	//Get the location of the listener
	QPointF listenerPos = physicalModel->listener->pos();

	//Get the location of this source
	QPointF sourcePos = src->pos();

	//Get a normal vector from listener to source
	QVector2D l2s = QVector2D(sourcePos - listenerPos);
	l2s.normalize();

	float srcAngle = atan2(l2s.x(), l2s.y());

	//Create a list to store dot products
	QHash<int, float> dps;
	QHash<int, float> angles;
	QHash<int, QVector2D> vecs;

	//Get the location of each loudspeaker
	int numChannels = physicalModel->loudspeakerList.count()/physicalModel->arrayNumRows; //TODO: make sure loudspeakerList.count() matches vbapBank[sourceID].count();
	for(int j = 0; j < numChannels; j++) {
		//Reset the gain (for all loudspeakers)
		vbapBank[sourceID][j] = 0.0;
		//Get a normal vector from listener to each loudspeaker
		QVector2D l2l = QVector2D(physicalModel->loudspeakerList[j]->pos() - listenerPos);
		l2l.normalize();
		vecs.insert(j, l2l);

		//Get the angle to this loudspeaker
		float angle = atan2(l2l.x(), l2l.y());
		angles.insert(j, angle);

		//Get the dot product of loudspeaker vector with the source vector
		float dp = QVector2D::dotProduct(l2s, l2l);
		dps.insert(j, dp);
	}

	//Sort the angles
	angles.insert(999, srcAngle);
	QList<float> vals = angles.values();
	qSort(vals);
	int idx = vals.indexOf(srcAngle);
	float s1, s2;
	if(idx == 0) {
		s1 = vals.last();
		s2 = vals.at(idx + 1);
	} else if(idx == vals.count() - 1) {
		s1 = vals.at(idx - 1);
		s2 = vals.at(0);
	} else {
		s1 = vals.at(idx - 1);
		s2 = vals.at(idx + 1);
	}
	Q_ASSERT(s1 != 999 && s2 != 999);
	
	int idx1 = angles.key(s1);
	int idx2 = angles.key(s2);
	/*
	sourceVec = gainVec * loudspeakerBaseMatrix
	gainVec = sourceVec * loudspeakerBaseMatrixInverse
	*/
	
	QMatrix lspkBaseMatrix(vecs.value(idx1).x(), vecs.value(idx1).y(), vecs.value(idx2).x(), vecs.value(idx2).y(), 0, 0);
	lspkBaseMatrix = lspkBaseMatrix.inverted();
	QVector2D gainVec(	l2s.x() * lspkBaseMatrix.m11() + l2s.y() * lspkBaseMatrix.m21(),
						l2s.x() * lspkBaseMatrix.m12() + l2s.y() * lspkBaseMatrix.m22());
	if(gainVec.x() > 1) gainVec.setX(1);
	if(gainVec.y() > 1) gainVec.setY(1);

	vbapBank[sourceID][idx1] = gainVec.x();
	vbapBank[sourceID][idx2] = gainVec.y(); // TODO: Sometimes crashes here, vbapBank index out of range

	// Multiply by distance attenuation
	QPointF lspkMeanPos = (physicalModel->loudspeakerList.at(idx1)->pos() + physicalModel->loudspeakerList.at(idx2)->pos())/2;
	QVector2D A = QVector2D(src->pos() - lspkMeanPos);
	QVector2D B = QVector2D(lspkMeanPos - physicalModel->listener->pos());
	float distanceGain = sqrt(B.length() / (A.length() + B.length()));
	vbapBank[sourceID][idx1] *= distanceGain;
	vbapBank[sourceID][idx2] *= distanceGain;
}  

void WFSDesigner::updateFilterBank(VirtualSource* src) {
	
	/*
	WFS FilterBank
	Update the row in the filter bank for this source. 
	For example, updating row index 1:

										M Channels

					    0    1    2    3    4    5    6    7    8        M-1
					0 [   ][   ][   ][   ][   ][   ][   ][   ][   ]     [   ]
					1 [ # ][ # ][ # ][ # ][ # ][ # ][ # ][ # ][ # ]     [ # ]
		N Sources	2 [   ][   ][   ][   ][   ][   ][   ][   ][   ] ... [   ]
					3 [   ][   ][   ][   ][   ][   ][   ][   ][   ]     [   ]
										   ...
				  N-1 [   ][   ][   ][   ][   ][   ][   ][   ][   ]     [   ]
					  -------------------------------------------------------
				  Sum [   ][   ][   ][   ][   ][   ][   ][   ][   ] ... [   ]

	*/

	// let's start by calculating the distances from each source to each loudspeaker
	// and convert that to samples. Assume 44.1k.

	int numSources = virtualSourceModel->virtualSourceList.count();
	int numChannels = physicalModel->loudspeakerList.count();
	int sourceID = virtualSourceModel->virtualSourceList.indexOf(src);

	//qDebug() << QString("Received a sourceMoved event from source %1").arg(src->id);
	//qDebug() << QString("Num sources: %1 / Num loudspeakrs: %2").arg(numSources).arg(numChannels);

	QList<float> spkrdelays;
	QList<float> sortabledelays;
	//VirtualSource* src = virtualSourceModel->virtualSourceList.at(sourceId); //FAIL 

	// A first pass through all loudspeakers...
	// Here, we find the distance of the current source to each loudspeaker and store this in a "spkrdelays" list.
	// We also determine the gain for this source-speaker pair.
	for(int j = 0; j < numChannels; j++) {
		Loudspeaker* lspk = physicalModel->loudspeakerList.at(j);
		QVector2D l2s = QVector2D(src->pos() - lspk->pos());
		float smp = portAudio->sampleRate * (float)l2s.length()/100/345; // calculate source to loudspeaker delay (in samples)
		//qDebug() << QString("Distance from src %1 to spkr %2: %3 (%4 samples)").arg(src->id).arg(j).arg(spkr2src.length()).arg(int(smp));
		spkrdelays.append(smp);
		sortabledelays.append(smp);
	}
	//Normalize delays to zero. Find the minimum delay and subtract it from all delays.
	qSort(sortabledelays.begin(), sortabledelays.end());
	float mindelay = sortabledelays.at(0);

	// Second pass through all loudspeakers...
	for(int k = 0; k < spkrdelays.count(); k++) {
		spkrdelays[k] -= mindelay;
		// Put an impulse of 1 at this sample index in the corresponding filterbank FIR
		//Q_ASSERT(int(spkrdelays[k])<firsize);
		if (int(spkrdelays[k]) > firsize) {
			spkrdelays[k] = firsize;
			qDebug("Warning: exceeded maximum delay time possible with FIR size %d",firsize);
		}
		memset(filterBank[sourceID][k]->m_FIR,0,sizeof(float)*firsize);

		Loudspeaker* lspk = physicalModel->loudspeakerList.at(k);
		QVector2D A = QVector2D(src->pos() - lspk->pos());
		QVector2D B = QVector2D(lspk->pos() - physicalModel->listener->pos());

		// Dot product loudspeaker selection
		if(QVector2D::dotProduct(A,B) > 0) {
			filterBank[sourceID][k]->gain = sqrt(B.length() / (A.length() + B.length())); 
			filterBank[sourceID][k]->m_FIR[int(spkrdelays[k])] = filterBank[sourceID][k]->gain; //  wave field synthesis
		} else {
			filterBank[sourceID][k]->gain = 0;
		}

	}

}

int WFSDesigner::calculateDelay(VirtualSource *src, Loudspeaker *lspk) {
	// 2D
	QVector2D l2s = QVector2D(src->pos() - lspk->pos());

	float smp = portAudio->sampleRate * (float)l2s.length()/100/speedOfSound;
	return int(smp);
}

int WFSDesigner::calculateDelay3D(VirtualSource *src, Loudspeaker *lspk) {
	// 3D
	QVector3D srcpos3d = QVector3D(src->x(), src->y(), src->z());
	QVector3D lspkpos3d = QVector3D(lspk->x(), lspk->y(), lspk->z());
	QVector3D l2s = QVector3D(srcpos3d - lspkpos3d);

	float smp = portAudio->sampleRate * (float)l2s.length()/100/speedOfSound;
	return int(smp);
}

float WFSDesigner::calculateGain(VirtualSource *src, Loudspeaker *lspk) {
	float gain = 0.0;
	QVector2D A = QVector2D(src->pos() - lspk->pos());
	QVector2D B = QVector2D(lspk->pos() - physicalModel->listener->pos());
	if(QVector2D::dotProduct(A,lspk->arrayNormal) > 0) {
		gain = QVector2D::dotProduct(A.normalized(),lspk->arrayNormal.normalized()) * sqrt(B.length() / (A.length() + B.length())); 
		gain *= lspk->taperGain;
	}
	return gain;
}