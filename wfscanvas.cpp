#include "wfscanvas.h"
#include <QVarLengthArray.h>
#include <QPropertyAnimation>
#include <QWheelEvent>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QDebug>

WFSCanvas::WFSCanvas(QWidget *parent)
	: QGraphicsView(parent)
{
	m_zoom = 1.0;
	isPanning = false;
	clampView = false;
	//setSceneRect(0, 0, 1000, 1000);
    //SetCenter(QPointF(0, 0)); //A modified version of centerOn(), handles special cases
}

WFSCanvas::~WFSCanvas()
{

}

//void WFSCanvas::drawBackground(QPainter* painter, const QRectF& rect) {
//	/*
//	drawForeground reimplemented to draw a grid in the WFS Canvas.
//	*/
//	int gridInterval = 10;
//	painter->setWorldMatrixEnabled(true);
//	painter->setPen(QPen(QBrush(QColor(210,210,210)),1,Qt::DotLine));
//	qreal left = int(rect.left()) - (int(rect.left()) % gridInterval);
//	qreal top = int(rect.top()) - (int(rect.top()) % gridInterval);
//	
//	QVarLengthArray<QLineF, 100> linesX;
//	for(qreal x = left; x < rect.right(); x += gridInterval)
//		linesX.append(QLineF(x, rect.top(), x, rect.bottom()));
//
//	QVarLengthArray<QLineF, 100> linesY;
//	for(qreal y = top; y < rect.bottom(); y += gridInterval)
//		linesY.append(QLineF(rect.left(), y, rect.right(), y));
//
//	painter->drawLines(linesX.data(), linesX.size());
//	painter->drawLines(linesY.data(), linesY.size());
//}

/**
  * Sets the current centerpoint.  Also updates the scene's center point.
  * Unlike centerOn, which has no way of getting the floating point center
  * back, SetCenter() stores the center point.  It also handles the special
  * sidebar case.  This function will claim the centerPoint to sceneRec ie.
  * the centerPoint must be within the sceneRec.
  */
//Set the current centerpoint in the
void WFSCanvas::SetCenter(const QPointF& centerPoint) {
    //Get the rectangle of the visible area in scene coords
    QRectF visibleArea = mapToScene(rect()).boundingRect();
 
    //Get the scene area
    QRectF sceneBounds = sceneRect();
 
    double boundX = (visibleArea.width()/2.0 + sceneBounds.left());
    double boundY = (visibleArea.height()/2.0 + sceneBounds.top());
    double boundWidth = sceneBounds.width() - visibleArea.width();
    double boundHeight = sceneBounds.height() - visibleArea.height();
 
    //The max boundary that the centerPoint can be to
    QRectF bounds(boundX, boundY, boundWidth, boundHeight);
 
    if(bounds.contains(centerPoint)) {
        //We are within the bounds
        CurrentCenterPoint = centerPoint;
    } else {
        //We need to clamp or use the center of the screen
        if(visibleArea.contains(sceneBounds)) {
            //Use the center of scene ie. we can see the whole scene
            CurrentCenterPoint = sceneBounds.center();
        } else {
 
            CurrentCenterPoint = centerPoint;
 
            //We need to clamp the center. The centerPoint is too large
            if(centerPoint.x() > bounds.x() + bounds.width()) {
                CurrentCenterPoint.setX(bounds.x() + bounds.width());
            } else if(centerPoint.x() < bounds.x()) {
                CurrentCenterPoint.setX(bounds.x());
            }
 
            if(centerPoint.y() > bounds.y() + bounds.height()) {
                CurrentCenterPoint.setY(bounds.y() + bounds.height());
            } else if(centerPoint.y() < bounds.y()) {
                CurrentCenterPoint.setY(bounds.y());
            }
			
 
        }
    }
 
    //Update the scrollbars
    centerOn(CurrentCenterPoint);
}
 
/**
  * Handles when the mouse button is pressed
  */
void WFSCanvas::mousePressEvent(QMouseEvent* event) {
    //For panning the view
	if(isPanning) {
		LastPanPoint = event->pos();
		setCursor(Qt::ClosedHandCursor);
	} else {
		QGraphicsView::mousePressEvent(event);
	}
}
 
/**
  * Handles when the mouse button is released
  */
void WFSCanvas::mouseReleaseEvent(QMouseEvent* event) {
	if(isPanning) {
		setCursor(Qt::OpenHandCursor);
		LastPanPoint = QPoint();
	} else {
		QGraphicsView::mouseReleaseEvent(event);
	}
}
 
/**
*Handles the mouse move event
*/
void WFSCanvas::mouseMoveEvent(QMouseEvent* event) {
    if(!LastPanPoint.isNull() && isPanning) {
        //Get how much we panned
        QPointF delta = mapToScene(LastPanPoint) - mapToScene(event->pos());
        LastPanPoint = event->pos();
 
        //Update the center ie. do the pan
        SetCenter(GetCenter() + delta);
    } else {
		QGraphicsView::mouseMoveEvent(event);
	}
}
 
/**
  * Zoom the view in and out.
  */
void WFSCanvas::wheelEvent(QWheelEvent* event) {
 
 //   //Get the position of the mouse before scaling, in scene coords 
 //   QPointF pointBeforeScale(mapToScene(event->pos())); 

	////Always zoom on center. Don't care where mouse cursor is.

 //   //Get the original screen centerpoint
 //   QPointF screenCenter = GetCenter(); //CurrentCenterPoint; //(visRect.center());
 
    //Scale the view ie. do the zoom


    //double scaleFactor = 1.15; //How fast we zoom
    //if(event->delta() > 0) {
    //    //Zoom in
    //    scale(scaleFactor, scaleFactor);
    //} else {
    //    //Zooming out
    //    scale(1.0 / scaleFactor, 1.0 / scaleFactor);
    //}
   double scaleFactor = 1.4; //How fast we zoom
    if(event->delta() > 0) {
        ;
    } else {
        //Zooming out
        scaleFactor = 1/scaleFactor;
    }
	 QPropertyAnimation* animation = new QPropertyAnimation(this, "zoom");
	 animation->setDuration(100);
	 animation->setEndValue(this->zoom() * scaleFactor);
	 animation->start(QAbstractAnimation::DeleteWhenStopped);
 
    ////Get the position after scaling, in scene coords
    //QPointF pointAfterScale(mapToScene(event->pos()));
 
    ////Get the offset of how the screen moved
    //QPointF offset = pointBeforeScale - pointAfterScale;
 
    ////Adjust to the new center for correct zooming
    //QPointF newCenter = screenCenter + offset;
    //SetCenter(newCenter);
}
 
/**
  * Need to update the center so there is no jolt in the
  * interaction after resizing the widget.
  */
void WFSCanvas::resizeEvent(QResizeEvent* event) {
    //Get the rectangle of the visible area in scene coords
    QRectF visibleArea = mapToScene(rect()).boundingRect();
   // SetCenter(visibleArea.center());
 
    ////Call the subclass resize so the scrollbars are updated correctly
    QGraphicsView::resizeEvent(event);
}

void WFSCanvas::keyPressEvent(QKeyEvent* event) {
	if(event->key() == Qt::Key_Space) {
		isPanning = true;
		setCursor(Qt::OpenHandCursor);
	}
}

void WFSCanvas::keyReleaseEvent(QKeyEvent* event) {
	if(event->key() == Qt::Key_Space) {
		isPanning = false;
		setCursor(Qt::ArrowCursor);
	}
}

float WFSCanvas::zoom() {
	return m_zoom;
}

void WFSCanvas::setZoom(float z) {
	QTransform oldT = transform();
	QTransform newT(z, oldT.m12(), oldT.m13(),
					oldT.m21(), z, oldT.m23(),
					oldT.m31(), oldT.m32(), oldT.m33());
	setTransform(newT);
	m_zoom = z;
}