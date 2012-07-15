#ifndef WFSDESIGNER_H
#define WFSDESIGNER_H

#include <QGraphicsScene>
#include <QtGui/QMainWindow>
#include <QVector2D>
#include <QDebug>
#include <QMutex>
#include <QItemSelectionModel>

#include "ui_wfsdesigner.h"
#include "WFSConfig.h"
#include "WFSPortAudio.h"
#include "WFSListeningTest.h"
#include "WFSModel.h"
#include "WFSFilter.h"
#include "Biquad.h"
#include "pa_ringbuffer.h"
#define _USE_MATH_DEFINES
#include <math.h>

/*
filterBundle is a structure to package the filter process arguments for concurrent processing.
NOT YET IMPLEMENTED
*/
enum SynthesisMode {
	MODE_WFSVBAP_SUBBAND = 0,
	MODE_WFS_BANDLIMITED,
	MODE_WFS,
	MODE_VBAP
};

enum ArrayGeometry {
	GEOM_LINE = 0,
	GEOM_CIRCLE,
	GEOM_DOUBLE_LINE,
	GEOM_DOUBLE_CIRCLE,
	GEOM_BOX,
	GEOM_U_SHAPE
};

struct filterBundle {
	float* input;
	float* output;
	int framesPerBuffer;
};

class WFS3DCanvas; //forward declaration. TODO: use forward declaration for all classes? 
class QTimer;

class WFSDesigner : public QMainWindow
{
	Q_OBJECT

public:
	WFSDesigner(QWidget *parent = 0, Qt::WFlags flags = 0);
	~WFSDesigner();

	PhysicalModel* physicalModel;
	VirtualSourceModel* virtualSourceModel; 
	QGraphicsScene* graphicsScene;
	WFS3DCanvas* wfs3dcanvas;					//OpenGL 3D Scene View
	WFSPortAudio* portAudio;					//PortAudio wrapper TODO: move portaudio into WFSDesigner?
	WFSConfig* wfsConfig;							//Audio Device Configuration window
	WFSListeningTest* wfsListeningTest;


	// Convolution Filter
	QVector<QVector<WFSFilter*>> filterBank;	// 2D array of FILTER INSTANCES (this is being disabled)
	QVector<QVector<float>> vbapBank;			// 2D array of VBAP AMPLITUDES (CONVENTIONAL PANNING)
	float* wavVBAP;
	float* wavWFS;
	float* result;
	int buffersize;
	int firsize;

	// Delay Line
	int delayLineSize;
	int delayLineOffset;
	QVector<int> delayLineWriteIndexes;
	QVector<float*> delayLinesVBAP;
	QVector<float*> delayLinesWFS;
	QVector<QVector<QVector<int>>> delaySamples;// 2D array of DELAY TIMES (in samples)
	QVector<QVector<float>> gainsWFS;			// 2D array of gains for WFS

	// Source Filters - need one for each source
	QVector<Biquad*> hpFilters;
	QVector<Biquad*> lpFilters;

	// Master Volume
	float m_gain;
	float m_gainTo;
	float m_dezipVel;
	float speedOfSound;

	QMutex mutex;
	QItemSelectionModel* sourceSelection;
	QLabel* statusBarSampleRate;
	QLabel* statusBarBufferSize;
	QLabel* statusBarFIRSize;

	QTimer* testTimer;

	bool sourceIsFocused(VirtualSource* src, float arrayDistance, int arrayGeometry); 
	int calculateDelay(VirtualSource* src, Loudspeaker* lspk);
	int calculateDelay3D(VirtualSource* src, Loudspeaker* lspk);
	float calculateGain(VirtualSource* src, Loudspeaker* lspk);
	void amplitudePan2D(QVector2D lspk1, QVector2D lspk2, QVector2D src, float &lspk1gain, float &lspk2gain);
	void loadTestSettings();
	void playTestTone(int tone);
	void readSettings();
	void writeSettings();
	void initSource(VirtualSource* vs);
	void closeEvent(QCloseEvent* event);
	void updateGainsAndDelays(VirtualSource* src);
	void updateFilterBank(VirtualSource* src);
	void updateVbapBank(VirtualSource* src);
	void updateCrossoverFromSpacing(void);
	void processAllChannels(float* buffer, int framesPerBuffer, int numChannels);
#ifdef WFS_FIR
	void processConvolvingAllChannels(float* buffer, int framesPerBuffer, int numChannels); //deprecated
#endif

public slots:; // semicolon helps Visual Studio

	// UI Slots
	void on_actionAudio_Configuration_triggered(); //auto-connected slot
	void on_actionAdjust_Speed_of_Sound_triggered(); //auto-connected slot
	void on_actionAbout_WFS_Designer_triggered();
	void on_action2D_Scene_View_triggered();
	void on_action3D_Scene_View_triggered();
	void on_action2D_3D_Split_Scene_View_triggered();
	void on_actionRun_Listening_Test_triggered();
	void on_stopAudio_clicked();
	void on_startAudio_clicked();
	void on_addSource_clicked();
	void on_removeSource_clicked();
	void on_volumeSlider_valueChanged(int value);
	void on_sourceVolumeSlider_valueChanged(int value);
	void on_sourceXSpinBox_valueChanged(double value);
	void on_sourceYSpinBox_valueChanged(double value);
	void on_sourceZSpinBox_valueChanged(double value);
	void on_listenerDistanceSpinBox_valueChanged(double value);
	void on_loudspeakerSpacingSpinBox_valueChanged(double value);
	void on_arrayConfigurationComboBox_currentIndexChanged(const QString &text);
	void on_applyConfigurationButton_clicked();
	void on_crossoverSpinBox_valueChanged(double value);
	void on_automaticCheckBox_toggled(bool checked);

	// My Slots
	void on_configurationChanged();
	void on_sourceClicked(VirtualSource* src);
	void on_sourceMoved(VirtualSource* src);
	void on_sourceReleased(VirtualSource* src);
	void on_sourceSelection_changed(const QItemSelection &selected);
	void resizeFilterBank();
	void on_loudspeakerMoved(Loudspeaker* lspk);

private:
	Ui::WFSDesignerClass ui;
};

#endif // WFSDESIGNER_H