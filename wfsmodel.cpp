#include "wfsmodel.h"
#include "pa_ringbuffer.h"
#include <QFileInfo>
#include <QPainter>
#include <QFileDialog>
#include <QApplication>
#include <QGraphicsSceneMouseEvent>


/*
=========================================================================================================================================
																																	  
																																	  
																																	 
	Class: Listener																											
																																 	  
																																  
																																	
=========================================================================================================================================
*/

Listener::Listener(void) {
	//set up QGraphicsPixmapItem properties
	setPixmap(QPixmap("listener.png"));
	setOffset( -0.5 * QPointF( pixmap().width(), pixmap().height() ) ); //center on position
	//setFlag(QGraphicsItem::ItemIsMovable);
	setOpacity(0.5);
	setFlag(QGraphicsItem::ItemIgnoresTransformations);
	setToolTip(QString("Nominal Listener Position"));
	_z = 100; // default Z to one meter...
}

Listener::~Listener(void){}

/*
=========================================================================================================================================
																																	  
																																	  
																																	 
	Class: Loudspeaker																											
																																 	  
																																  
																																	
=========================================================================================================================================
*/

Loudspeaker::Loudspeaker(void) {
	srcWFSGain = 0.0;
	srcWFSDelay = 0;
	srcVBAPGain = 0.0;
	taperGain = 1.0;
	arrayNormal = QVector2D(0,-1); // default points down (toward the listener)
	_z = 100; // default Z to one meter...

	setFlag(QGraphicsItem::ItemIsMovable);
	setFlag(QGraphicsItem::ItemIsSelectable);
	setFlag(QGraphicsItem::ItemIgnoresTransformations);
}

Loudspeaker::~Loudspeaker()
{
	// TODO
}

QRectF Loudspeaker::boundingRect() const
{
	qreal penWidth = 6;
	qreal w = 20;
	qreal h = 20;
	if(srcWFSDelay != 0) { w = 40; h = 40; }
	return QRectF(-w/2 - penWidth/2, -h/2 - penWidth/2, w + penWidth, h + penWidth);
}

void Loudspeaker::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
	painter->setRenderHint(QPainter::Antialiasing);


	painter->setBrush(QBrush(QColor(128,128,128)));
	if(isSelected()) {
		painter->setBrush(QBrush(QColor(200,100,0)));
	}
	painter->setPen(Qt::NoPen);
	painter->drawRoundedRect(-10, -10, 20, 20, 5, 5);

	if(srcWFSGain > 0) {
		painter->setBrush(QBrush(QColor(40,240,0,srcWFSGain*255)));
		painter->drawRoundedRect(-10, -10, 20, 20, 5, 5);
		if(srcWFSDelay != 0) {
			painter->setPen(QColor(0,0,0,128));
			painter->setFont(QFont("Tahoma",8,-1,false));
			painter->drawText(QRect(-20,10,40,20),QString::number(srcWFSDelay),QTextOption(Qt::AlignCenter));
		}
	}

	if(srcVBAPGain > 0) {
		painter->setBrush(Qt::NoBrush);
		painter->setPen(QPen(QBrush(QColor(255,128,0,255)), srcVBAPGain*6));
		painter->drawRoundedRect(-10, -10, 20, 20, 5, 5);
	}

	painter->setPen(QColor(255,255,255));
	painter->setFont(QFont("Tahoma",-1,QFont::Bold,false));
	painter->drawText(QRect(-10,-10,20,20),QString::number(channel),QTextOption(Qt::AlignCenter));
}

/*
=========================================================================================================================================
																																	  
																																	 
																																	 
	Class: VirtualSource																											
	The VirtualSource class is responsible for maintaining its own WAV read buffer.													
																																	 
																																	  
																																	   
=========================================================================================================================================
*/

/*
Prepare the sound file handle for reading and if successful,
set the initialized flag and start the buffer monitoring timer thread.
*/
VirtualSource::VirtualSource(QWidget *parent=0, QString fileName=QString::null, float xpos=0, float ypos=-150, float zpos=100) {
	fileInitialized = 0;
	readyForPlay = false;
	gain = 1;
	id = 0;
	sfChannel = 0; //left channel only (or mono) by default
	setX(xpos);
	setY(ypos);
	setZ(zpos);

	// Set up QGraphicsPixmapItem properties
	setPixmap(QPixmap("source.png"));
	setOffset( -0.5 * QPointF( pixmap().width(), pixmap().height() ) ); //center on position
	setFlag(QGraphicsItem::ItemIsMovable);
	setFlag(QGraphicsItem::ItemIgnoresTransformations);
	
	// Initialize our data structure 
	ringbuffer = new PaUtilRingBuffer();
	PaUtil_InitializeRingBuffer(ringbuffer, sizeof(float), RING_BUFFER_SIZE, ringbufferdata);
	sfPosition = 0;
	sfInfo.format = 0;

	// Maybe prompt user to select a wav file 
	if(fileName == QString::null) {
		fileName = QFileDialog::getOpenFileName((QWidget *)this->parent(), tr("Choose Audio File"), "", tr("Audio File (*.wav *.aiff)"));
		if(fileName == QString::null) return;
	}
	myName = QFileInfo(fileName).baseName();
	setToolTip(QString("Virtual Source\n%1 [%2]").arg(myName).arg(id));
	// Try opening the file, and send info to the sfInfo struct
	sfFile = sf_open(fileName.toLocal8Bit().data(), SFM_READ, &sfInfo); // convert QString to char*
	qDebug("Read sound file.");
	qDebug() << "Frames: " << sfInfo.frames << "SampleRate: " << sfInfo.samplerate
				<< "Channels: " << sfInfo.channels; 
	// Is the file valid?
	if (!sfFile)
	{
		// TODO: issue an error message
		qDebug("Error opening sound file.");
		return;
	}

	// File is ready
	scratchbuffer = new float[sfInfo.channels * RING_BUFFER_SIZE/2];
	readyForPlay = true;
	fileInitialized = 1;
}

void VirtualSource::resetRingBuffer() {
	//reset for playback, "prepareForPlay"
	PaUtil_FlushRingBuffer(ringbuffer);
	sfPosition = 0;
	maybeFillRingBuffer();
}

/*
Function: maybeFillRingBuffer
This function runs on the separate non-portaudio thread, 
probably called by some timer object that gets hit every 100 ms or so.
Buffer is a FIFO buffer maintained by the PaUtilRingBuffer class,
so we only need to worry about making timely reads/writes and keeping the
buffer full for the reads.
The idea is that this gets called about as often as the portaudio
callback, keeping the buffer full and allowing time for some slippage.
*/
void VirtualSource::maybeFillRingBuffer(){ 
	if(!fileInitialized) return;
	//if bytes readable is less than half RING_BUFFER_SIZE, fetch and write RING_BUFFER_SIZE/2 bytes
	int readAvailable = PaUtil_GetRingBufferReadAvailable(ringbuffer);
	//qDebug() << readAvailable << " bytes left in ring buffer for reading.";
	if(readAvailable < RING_BUFFER_SIZE/2) {

		float *cursor; // pointer into scratchbuffer
		cursor = (float *)scratchbuffer; // reset to the beginning (requires cast to undimensioned pointer)
		int thisSize = sfInfo.channels * RING_BUFFER_SIZE/2; //  <------- same size of scratchbuffer. see initialization of scratchbuffer in init() or whatever
		int thisRead;

		while (thisSize > 0) // loop until scratchbuffer is full
		{
			sf_seek(sfFile, sfPosition / sfInfo.channels, SEEK_SET); // seek to our current file sfPosition 
			// are we going to read past the end of the file?
			if (thisSize > (sfInfo.frames * sfInfo.channels - sfPosition)/*remaining frames*/)
			{
				thisRead = sfInfo.frames * sfInfo.channels - sfPosition; // if we are, only read to the end of the file
				sfPosition = 0; // and then loop to the beginning of the file
			} else {				
				thisRead = thisSize; // otherwise, we'll just fill up the rest of the output buffer 
				sfPosition += thisRead; // and increment the file sfPosition 
			}
			// since our output format and channel interleaving is the same as sf_readf_int's requirements 
			// we'll just read straight into the output buffer 
			/*
			
			sf_read vs. sf_readf

			  F R A M E  1   F R A M E  2   F R A M E  3 
		      item1  item2   item3  item4   item5  item6 
			[ [ch1]  [ch2]   [ch1]  [ch2]   [ch1]  [ch2]  ...  ]

			What's the difference? sf_read takes a number of "items" which means you have to 
			multiply samples by channels yourself and then make the request.

			sf_read takes ITEMS as an argument, sf_readf takes FRAMES as an argument.
			ITEMS = FRAMES * CHANNELS.

			*/
			
			//sf_readf_int(sfFile, cursor, thisRead);

			int sfDidRead = sf_read_float(sfFile, cursor, thisRead);  // copy thisRead samples to the memory at cursor
			qDebug() << QString("[timer] libsndfile read %1 items for source %2.").arg(sfDidRead).arg(id);
			// increment the output cursor 
			cursor += thisRead;
			// decrement the number of samples left to process 
			thisSize -= thisRead;
		}
		
		//copy the scratch buffer into the ring buffer
		PaUtil_WriteRingBuffer(ringbuffer, scratchbuffer, RING_BUFFER_SIZE/2, sfInfo.channels, sfChannel);
		qDebug() << QString("[timer] ringbuffer writeIndex is now %1 for source %2.").arg(ringbuffer->writeIndex).arg(id);
	}
}

/*
Function: copyFromRingBuffer
Copies audio data from the ring buffer to the client buffer for 
further processing.  This gets called by the controller (WFSDesigner)
in the portaudio callback thread. This might happen very frequently,
at the portaudio buffer rate, maybe 10 to 100 times per second.
*/
void VirtualSource::copyFromRingBuffer(float* buffer, int size){
	if(fileInitialized == 0) {  //wait until file is ready
		return;
	}

	//copy bytes from ring buffer to client buffer
	PaUtil_ReadRingBuffer(ringbuffer, buffer, size); // size = channels * samples; 
	qDebug() << "[portaudio callback] ringbuffer readIndex is now" << ringbuffer->readIndex << "source" << id;
}

void VirtualSource::mousePressEvent(QGraphicsSceneMouseEvent *event) {
	emit sourceClicked(this);
	QGraphicsItem::mousePressEvent(event);
}

void VirtualSource::mouseMoveEvent(QGraphicsSceneMouseEvent *event) {
	// If shift is held, move the source on the Z axis.
	
	if(qApp->keyboardModifiers() == Qt::ShiftModifier) {
		QPointF delta = mapToScene(lastPanPoint) - mapToScene(event->pos());
		lastPanPoint = event->pos();
		float newZ = _z + delta.y()/10;
		setZ(newZ);
	} else {
		//...otherwise, propagate the mouseMoveEvent
		QGraphicsItem::mouseMoveEvent(event); 
	}
	emit sourceMoved(this);
}

void VirtualSource::mouseReleaseEvent(QGraphicsSceneMouseEvent *event) {
	lastPanPoint = QPointF(0,0);
	emit sourceReleased(this);
	QGraphicsItem::mouseReleaseEvent(event); 
}

VirtualSource::~VirtualSource()
{
	qDebug() << "Destroyed virtual source.";
	// TODO: Delete pointers created by virtual source
}

/*
=========================================================================================================================================
																																	  
																																	  
																																	 
	Class: PhysicalModel																											
																																 	  
																																  
																																	
=========================================================================================================================================
*/
PhysicalModel::PhysicalModel(void){
	listener = new Listener();
	listener->setPos(0,0);

	/*
	Comments on PhysicalModel 3/11/2011:
	====================================
	Don't create the loudspeakers, listener, and room objects here.
	For now, create them in the controller because they need to be added to the graphics scene
	and the physical model has no knowledge of the graphics scene. This is sort of stupid
	because the controller:
		1. Creates the graphics scene
		2. Creates the virtual source model
		3. Creates the physical model
		4. Iterates through loudspeakers/sources in each model and adds them to the scene

	The loudspeakers will also be manipulated whenever the audio configuration changes.
	The way this works now, the loudspeaker list, room, and listener objects may just
	as well be owned by the WFSDesigner controller.  The PhysicalModel is purely for 
	organizational purposes.  It should be more than that.

	Later, it could be fixed to preserve encapsulation of the physical model, and send all
	sorts of messages through signals and slots with the controller. 

	The PhysicalModel could subclass GraphicsScene, or the Physical Model and Virtual Source Model
	could be unified into WFS Model, which could subclass GraphicsScene.

	There are three ways to do this:
		1. The current way, of having the controller instantiate the models and graphics scene
			and add all the objects to the graphics scene itself
		2. Pass a pointer to the graphics scene to the model, so that the model can add its 
			own items to the scene. 
		3. Send messages thru signals. Sending messages doesn't help when there is still a tight 
			coupled relationship, like when the controller still accesses into the model 
			whenever the scene changes.
	*/

	//for(int i = 0; i < maxLoudspeakers; i++) {
	//	Loudspeaker speaker = Loudspeaker();
	//	speaker.setPos(qrand()%100 - 50,	qrand()%100 - 50);
	//	speaker.id = i;
	//	speaker.enabled = false;
	//	speaker.hide();
	//	loudspeakerList.append(speaker);
	//}
}

PhysicalModel::~PhysicalModel()
{
	// TODO: Delete pointers created by physical model
}

void PhysicalModel::updateLoudspeakersFromChannelList(QVector<bool>* channelEnabled){
	/*
	Instead of trying to preserve loudspeaker information after
	audio configuration, for now we reset the whole thing
	*/
	//for(int j = 0; j < loudspeakerList.count(); j++) {
	//	loudspeakerList.at(j)->enabled = false;
	//	loudspeakerList.at(j)->hide();
	//}
	//for(int i = 0; i < channelEnabled.count() && i < loudspeakerList.count(); i++) {
	//	if(channelEnabled.at(i) == true) {
	//		loudspeakerList.at(i)->enabled = true;
	//		loudspeakerList.at(i)->show();
	//	}
	//}
	
	//destroy all existing loudspeaker objects. Or just move them?
	foreach(Loudspeaker* lspk, loudspeakerList) {
		delete lspk;
	}
	loudspeakerList.clear();
	int id = 0;
	for(int i = 0; i < channelEnabled->count(); i++) {
		if(channelEnabled->at(i) == true) {
			Loudspeaker* lspk = new Loudspeaker();
			lspk->channel = i+1;
			lspk->id = id;
			lspk->setToolTip(QString("Channel %1").arg(lspk->channel));
			loudspeakerList.append(lspk);
			id++;
		}
	}
}

/*
=========================================================================================================================================
																																	  
																																	  
																																	 
	Class: VirtualSourceModel																											
																																 	  
																																  
																																	
=========================================================================================================================================
*/
VirtualSourceModel::~VirtualSourceModel()
{
	// TODO: Delete pointers created by virtual source model
}

int VirtualSourceModel::rowCount( const QModelIndex &/*parent*/ ) const
{
	return virtualSourceList.count();
}

QVariant VirtualSourceModel::data( const QModelIndex &index, int role ) const
{
	int row = index.row();
	int col = index.column();
	if (role == Qt::DisplayRole && col == 0) {
		return QString("%1 [%2]").arg(virtualSourceList[row]->myName).arg(virtualSourceList[row]->id);
	}
	/* TODO: implement check if virtual source is enabled
	if (role == Qt::CheckStateRole) {
		if(m_channelEnabled[row] == true)
			return Qt::Checked;
		else 
			return Qt::Unchecked;
	}
	*/
	return QVariant();
}

VirtualSource* VirtualSourceModel::addSource() {
	VirtualSource *newSource = new VirtualSource(0);
	if(newSource->readyForPlay == true) {
		newSource->id = lastID;
		lastID++;
		virtualSourceList.append(newSource);
		emit dataChanged(index(virtualSourceList.count(),0),index(virtualSourceList.count(),0)); //for QListModel
		return newSource;
	} else {
		delete newSource;
	}
	return 0;
}

VirtualSource* VirtualSourceModel::addSource(QString fileName=QString::null, float xpos=0, float ypos=-150, float zpos=100) {
	VirtualSource *newSource = new VirtualSource(0, fileName, xpos, ypos, zpos);
	if(newSource->readyForPlay == true) {
		newSource->id = lastID;
		lastID++;
		virtualSourceList.append(newSource);
		emit dataChanged(index(virtualSourceList.count(),0),index(virtualSourceList.count(),0)); //for QListModel
		return newSource;
	} else {
		delete newSource;
	}
	return 0;
}

bool VirtualSourceModel::removeRows(int row, int count, const QModelIndex &parent) {
	beginRemoveRows(QModelIndex(), row, row+count-1);
	for(int i = count-1; i >= 0; i--) {
		delete virtualSourceList[row+i];
		virtualSourceList.removeAt(row+i);
	}
	endRemoveRows();
	return true;
}

/*
=========================================================================================================================================
																																	  
																																	  
																																	 
	Class: Room
																																 	  
																																  
																																	
=========================================================================================================================================
*/
Room::Room()
{
	// TODO
}

Room::~Room()
{
	// TODO
}