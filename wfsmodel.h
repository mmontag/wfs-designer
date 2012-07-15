#ifndef WFSMODEL_H
#define WFSMODEL_H

#include <QGraphicsPixmapItem>
#include <QGraphicsItem>
#include <QDebug>
#include <QStringListModel>
#include <QList>
#include <QVector2D>
#include "sndfile.h"

#define RING_BUFFER_SIZE (1048576) //RING_BUFFER_SIZE must be a power of 2.  32767, 65536, 131072, 262144, 524288, 1048576
#define TEST_FILE "Debug\\stereo-44k-16bit-test.wav"

class PaUtilRingBuffer; // forward declaration

class Listener:public QObject, public QGraphicsPixmapItem
{
	Q_OBJECT
	Q_PROPERTY(float _z READ z WRITE setZ)
public:
	Listener(void);
	~Listener(void);
	float _z;
	float z() { return _z; };
	void setZ(float z) { _z = z; };
	 void mouseReleaseEvent(QGraphicsSceneMouseEvent *event) {
		/*
		Trap it and bubble it: 
		"If you want to keep the base implementation when reimplementing this function, 
		call QGraphicsItem::mouseMoveEvent() in your reimplementation."
		*/
		//qDebug("Listener received mouseReleaseEvent.");
		QGraphicsItem::mouseReleaseEvent(event);
		//SIGNAL time!
		//emit listenerMoved(this);
	 };
signals:
//	 void listenerMoved(Listener* me);
};

class Room
{
public:
	Room(void);
	~Room(void);
};

class Loudspeaker:public QObject, public QGraphicsItem
{
	Q_OBJECT
	Q_PROPERTY(float _z READ z WRITE setZ)
public:
	int id;
	int channel;
	//bool enabled; //unnecessary?
	float srcWFSGain;
	int	  srcWFSDelay;
	float srcVBAPGain;
	QVector2D arrayNormal;
	float taperGain;
	float _z;

	Loudspeaker(void);
	~Loudspeaker(void);
	
	float z() { return _z; };
	void setZ(float z) { _z = z; };
	QRectF boundingRect() const;
	void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);
	void mouseMoveEvent(QGraphicsSceneMouseEvent *event) {
				QGraphicsItem::mouseMoveEvent(event);
				emit loudspeakerMoved(this);
			};
	void mouseReleaseEvent(QGraphicsSceneMouseEvent *event) {
				QGraphicsItem::mouseReleaseEvent(event); 
				emit loudspeakerMovedFinal(this);
			};
	void prepareGeometryChange() { QGraphicsItem::prepareGeometryChange(); };
signals:
	void loudspeakerMoved(Loudspeaker* me);
	void loudspeakerMovedFinal(Loudspeaker* me);

};

class PhysicalModel:public QObject
{
	Q_OBJECT
public:
	PhysicalModel(void);
	~PhysicalModel(void);
	QList<Loudspeaker *> loudspeakerList;
	QList<QGraphicsItemGroup *> loudspeakerGroupList;
	Listener* listener;
	#ifndef ARRAY_GEOM_ENUM
	#define ARRAY_GEOM_ENUM
	#endif
	int arrayGeometry;
	float arrayListenerDistance;
	int arrayNumRows;
	int arrayNumCols;
	int arrayRow1Z;
	int arrayRow2Z;

public slots:;
	void updateLoudspeakersFromChannelList(QVector<bool>* channelEnabled);
};


/* 
	Class: VirtualSource
	The VirtualSource class is responsible for maintaining its own WAV read buffer. 
*/
class VirtualSource:public QObject, public QGraphicsPixmapItem
{
	Q_OBJECT
public:
	PaUtilRingBuffer	*ringbuffer;
	float				*ringbufferdata[RING_BUFFER_SIZE];
	/*
	a scratch buffer is needed because libsndfile's buffer-filling
	utility functions operate on pointers and won't do the proper 
	wrapping at the end of the ring buffer. 
	*/
	float				*scratchbuffer;//[RING_BUFFER_SIZE/2];
	float				gain;
	bool				readyForPlay;
	int					id;
	QString				myName;
	QString				fileName;
	SNDFILE				*sfFile;
	SF_INFO				sfInfo;
	int					sfPosition; //maintained for the ring buffer, not the output buffer
	int					fileInitialized;
	int					sfChannel; //which channel to use from the audio file?
	float _z;
	QPointF				lastPanPoint;

	VirtualSource(QWidget *parent, QString fileName, float xpos, float ypos, float zpos);
	~VirtualSource(void);

	/*
	Function: maybeFillRingBuffer
	This function runs on the separate non-portaudio thread, 
	probably called by some timer object that gets hit ever 100 ms or so.
	Buffer is a FIFO buffer maintained by the PaUtilRingBuffer class,
	so we only need to worry about making timely reads/writes and keeping the
	buffer full for the reads.
	The idea is that this gets called about as often as the portaudio
	callback, keeping the buffer full and allowing time for some slippage.
	*/
	void maybeFillRingBuffer();

	/*
	Function: copyFromRingBuffer
	Copies audio data from the ring buffer to the client buffer for 
	further processing.  This gets called by the controller (WFSDesigner)
	in the portaudio callback thread. This might happen very frequently,
	at the portaudio buffer rate, maybe 10 to 100 times per second.
	*/
	void copyFromRingBuffer(float* buffer, int size);
	float z() { return _z; };
	void setZ(float z) { _z = z; };
	void resetRingBuffer();
	void mousePressEvent(QGraphicsSceneMouseEvent *event);
	void mouseMoveEvent(QGraphicsSceneMouseEvent *event);
	void mouseReleaseEvent(QGraphicsSceneMouseEvent *event);

signals:;
	void sourceClicked(VirtualSource* me);
	void sourceMoved(VirtualSource* me);
	void sourceReleased(VirtualSource* me);
};

class VirtualSourceModel:public QStringListModel
{
	Q_OBJECT
public:
	int lastID;
	VirtualSourceModel(void){
		// start the ring buffer maintenance thread
		startTimer(200); //milliseconds. 
		lastID = 0;
	};
	~VirtualSourceModel(void);

	// stringlistmodel virtual methods
	int						rowCount(const QModelIndex &parent = QModelIndex()) const;
	QVariant				data(const QModelIndex &index, int role = Qt::DisplayRole) const;
	bool					removeRows(int row, int count, const QModelIndex &parent = QModelIndex());

	// my methods
	VirtualSource*			addSource();
	VirtualSource*			addSource(QString fileName, float xpos, float ypos, float zpos);
	QList<VirtualSource *>	virtualSourceList;
	/* 
	Function: timerEvent
	Relaying function for Qt timer event.
	*/
	void timerEvent(QTimerEvent	*event) {
		foreach(VirtualSource* v, virtualSourceList) {
			v->maybeFillRingBuffer();
		}
	}
};


#endif //WFSMODEL_H