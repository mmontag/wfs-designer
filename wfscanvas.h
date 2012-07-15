#ifndef WFSCANVAS_H
#define WFSCANVAS_H

#include <QGraphicsView>

class WFSCanvas : public QGraphicsView
{
	Q_OBJECT
	Q_PROPERTY(float zoom READ zoom WRITE setZoom)
public:
	WFSCanvas(QWidget *parent);
	~WFSCanvas();

	//void WFSCanvas::drawBackground(QPainter* painter, const QRectF& rect);
    void SetCenter(const QPointF& centerPoint);
	void setZoom(float z);
	float zoom();

protected:
    //Holds the current centerpoint for the view, used for panning and zooming
    QPointF CurrentCenterPoint;
 
    //From panning the view
    QPoint LastPanPoint;

	bool isPanning;
	bool clampView;
 
    //Set the current centerpoint in the
    QPointF GetCenter() { return CurrentCenterPoint; }
 
    //Take over the interaction
    virtual void mousePressEvent(QMouseEvent* event);
    virtual void mouseReleaseEvent(QMouseEvent* event);
    virtual void mouseMoveEvent(QMouseEvent* event);
    virtual void wheelEvent(QWheelEvent* event);
    virtual void resizeEvent(QResizeEvent* event);
	virtual void keyPressEvent(QKeyEvent* event);
	virtual void keyReleaseEvent(QKeyEvent* event);

private:
	float m_zoom;
};

#endif // WFSCANVAS_H
