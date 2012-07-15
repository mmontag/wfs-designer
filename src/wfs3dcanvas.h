#ifndef WFS3DCANVAS_H
#define WFS3DCANVAS_H
#include <QGLWidget>

// forward declaration
class Loudspeaker; 
class PhysicalModel;
class VirtualSourceModel;

class WFS3DCanvas : public QGLWidget
{
	Q_OBJECT
public:
	WFS3DCanvas(QWidget *parent = 0);
	~WFS3DCanvas();
	PhysicalModel* physicalModel;
	VirtualSourceModel* virtualSourceModel;
protected:  
    void initializeGL();
	void resizeGL(int width, int height);
    void paintGL();
	void mousePressEvent(QMouseEvent *event);
	void mouseMoveEvent(QMouseEvent *event);
	void wheelEvent(QWheelEvent *event);
	void setXRotation(float angle);
	void setYRotation(float angle);
	void drawAxes(int lineWidth, int length);
	void drawFloorGrid(int width, int height, int major, int minor);
	void drawListener();
	void drawLoudspeakers();
	void drawSources();
	void drawStilt(float x, float y, float z);
	void blendColor(GLfloat* base, GLfloat* target, GLfloat* result, float ratio);
	//void setMaterial(materialStruct * mat);
private:
	float camSwing;
    float camElev;
    QPoint lastPos;
	static GLfloat specularColor[4];
	static GLfloat listenerColor[4];
	static GLfloat loudspeakerColor[4];
	static GLfloat vbapColor[4];
	static GLfloat wfsColor[4];
	static GLfloat sourceColor[4];
	static GLfloat lightGrayColor[4];
	static GLfloat grayColor[4];
	static GLfloat darkGrayColor[4];
	float camDist;
	float zoomSpeed;
	float rotationSpeed;
};

#endif // WFS3DCANVAS_H