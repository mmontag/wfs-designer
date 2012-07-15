#include <QtGui>
#include <QtOpenGL>
#include <QDebug>
#include "wfs3dcanvas.h"
#include "qtglut.h"
#include "wfsmodel.h"

#ifndef GL_MULTISAMPLE
#define GL_MULTISAMPLE  0x809D
#endif

static float degToRad(float deg) {
	float rad = deg/(180/M_PI);
	return rad;
}

WFS3DCanvas::WFS3DCanvas(QWidget *parent): QGLWidget(QGLFormat(QGL::SampleBuffers), parent)
{
	camSwing = 45;
	camElev = 30;
	camDist = 200;
	zoomSpeed = 1.1; // x factor
	rotationSpeed = 0.3; // degrees per pixel
	setFormat(QGL::DoubleBuffer | QGL::DepthBuffer);
}
GLfloat WFS3DCanvas::specularColor[] = {.5,.5,.5,1};
GLfloat WFS3DCanvas::listenerColor[] = {.5,.5,.5,1};
GLfloat WFS3DCanvas::loudspeakerColor[] = {.6,.6,.6,1};
GLfloat WFS3DCanvas::vbapColor[] = {.9,.5,0,1};
GLfloat WFS3DCanvas::wfsColor[] = {.3,.9,0,1};
GLfloat WFS3DCanvas::sourceColor[] = {.3,.9,0,1};
GLfloat WFS3DCanvas::lightGrayColor[] = {.8,.8,.8,1};
GLfloat WFS3DCanvas::grayColor[] = {.5,.5,.5,1};
GLfloat WFS3DCanvas::darkGrayColor[] = {.3,.3,.3,1};

WFS3DCanvas::~WFS3DCanvas(){};

//void WFS3DCanvas::setMaterial(materialStruct * mat) {  //handy shortcut from an example in Angel
//	glMaterialfv(GL_FRONT, GL_AMBIENT, mat->ambient); 
//	glMaterialfv(GL_FRONT, GL_DIFFUSE, mat->diffuse); 
//	glMaterialfv(GL_FRONT, GL_SPECULAR, mat->specular); 
//	glMaterialf(GL_FRONT, GL_SHININESS, mat->shininess); 
//}

void WFS3DCanvas::initializeGL()  
{
	qglClearColor(Qt::gray);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	glShadeModel(GL_SMOOTH);
	glEnable(GL_LIGHTING);
    glEnable(GL_MULTISAMPLE);
	glMaterialfv(GL_FRONT, GL_SPECULAR, specularColor);
	glMaterialf(GL_FRONT, GL_SHININESS, 25);
	//position lights
    glEnable(GL_LIGHT0);
    glEnable(GL_LIGHT1);
    glEnable(GL_LIGHT2);
	GLfloat light0_pos[] = {50,100,100,1}; //key light
	glLightfv(GL_LIGHT0, GL_POSITION, light0_pos);
	GLfloat light1_pos[] = {-50,-100,-100,1}; //fill light
	glLightfv(GL_LIGHT1, GL_POSITION, light1_pos);
	GLfloat light2_pos[] = {-50,-100,100,1}; //key light
	glLightfv(GL_LIGHT2, GL_POSITION, light2_pos);
}   
void WFS3DCanvas::resizeGL(int width, int height)
{
	glViewport(0,0,width,height);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(45, (float)width/height, .5, 5000);
	glMatrixMode(GL_MODELVIEW);
}

void WFS3DCanvas::mousePressEvent(QMouseEvent *event)
{
	lastPos = event->pos();
}

void WFS3DCanvas::mouseMoveEvent(QMouseEvent *event)
{
	int dx = event->x() - lastPos.x();
	int dy = event->y() - lastPos.y();

	if (event->buttons() & Qt::LeftButton) {
		camSwing = (camSwing + rotationSpeed * dx);
		if(camSwing > 360) camSwing -= 360;
		if(camSwing < 0) camSwing += 360;
		camElev = (camElev + rotationSpeed * dy);
		if (camElev > 89.9) camElev = 89.9;
		if (camElev < -89.9) camElev = -89.9;
		updateGL();
	}
	lastPos = event->pos();
}

void WFS3DCanvas::wheelEvent(QWheelEvent *event) {
    if(event->delta() > 0) {
		//Zooming in
        camDist = camDist * 1/zoomSpeed;
    } else {
        //Zooming out
        camDist = camDist * zoomSpeed;
    }
	camDist = qMin((int)camDist, 1000);
	camDist = qMax((int)camDist, 10);
	updateGL();
}

void WFS3DCanvas::setXRotation(float angle)
{
	if (angle != camSwing) {
		camSwing = angle;
		//emit xRotationChanged(angle);
		updateGL();
	}
}

void WFS3DCanvas::setYRotation(float angle)
{
	if (angle != camElev) {
		camElev = angle;
		//emit yRotationChanged(angle);
		updateGL();
	}
}

void WFS3DCanvas::drawAxes(int lineWidth = 1, int length = 5) {
	//disable lighting
	glDisable(GL_LIGHTING);
		glColor3f(.1,.1,.1);
		glLineWidth(lineWidth);
		//axis lines
		glBegin(GL_LINES);
			glColor3f(.9,.2,0);
			glVertex3f(0,0,0);
			glVertex3f(length,0,0);
			glColor3f(.1,.3,.9);
			glVertex3f(0,0,0);
			glVertex3f(0,length,0);
			glColor3f(.1,.8,.2);
			glVertex3f(0,0,0);
			glVertex3f(0,0,length);
		glEnd();
	//enable lighting
	glEnable(GL_LIGHTING);
}

void WFS3DCanvas::drawFloorGrid(int width, int height, int major = 100, int minor = 10) {
	//disable lighting
	glDisable(GL_LIGHTING);

		//x/y axis lines
		glLineWidth(2);
		glColor4fv(darkGrayColor);
		glBegin(GL_LINES);
			glVertex3f(0,-height/2,0);
			glVertex3f(0,height/2,0);
			glVertex3f(-width/2,0,0);
			glVertex3f(width/2,0,0);
		glEnd();
		//minor grid lines
		glColor4fv(grayColor);
		glLineWidth(1);
		glBegin(GL_LINES);
			for(int i = -width/2; i <= width/2; i = i+minor) {
				if(i%major == 0) continue;
				glVertex3f(i,height/2,0);
				glVertex3f(i,-height/2,0);
			}
			for(int j = -height/2; j <= height/2; j = j+minor) {
				if(j%major == 0) continue;
				glVertex3f(width/2,j,0);
				glVertex3f(-width/2,j,0);
			};
		glEnd();

		//major grid lines
		glColor4fv(lightGrayColor);
		glBegin(GL_LINES);
			for(int i = -width/2; i <= width/2; i = i+major) {
				glVertex3f(i,height/2,0);
				glVertex3f(i,-height/2,0);
			}
			for(int j = -height/2; j <= height/2; j = j+major) {
				glVertex3f(width/2,j,0);
				glVertex3f(-width/2,j,0);
			};
		glEnd();

	//enable lighting
	glEnable(GL_LIGHTING);
}

void WFS3DCanvas::drawLoudspeakers() {
	foreach(Loudspeaker* lspk, physicalModel->loudspeakerList) {
		float x = lspk->x();
		float y = -lspk->y();
		float z = lspk->z();
		drawStilt(x, y, z);
		glPushMatrix();
		glTranslatef(x, y, z);
		if(lspk->srcVBAPGain > 0) { 
			GLfloat tempColor[4];
			blendColor(wfsColor, vbapColor, tempColor, lspk->srcVBAPGain);
			glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, tempColor);
			glutSolidSphere(4 + 1*lspk->srcWFSGain, 10, 10);
		} else if (lspk->srcWFSGain > 0) {
			GLfloat tempColor[4];
			blendColor(loudspeakerColor, wfsColor, tempColor, lspk->srcWFSGain);
			glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, tempColor);
			glutSolidSphere(4 + 1*lspk->srcWFSGain, 10, 10);
		} else {
			glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, loudspeakerColor);
			glutSolidSphere(4,10,10);
		}
		glPopMatrix();
		//qDebug() << "Loudspeaker: " << lspk->id << " - " << lspk->x() << ", " << lspk->y(); 
	}
}

void WFS3DCanvas::drawListener() {
	Listener* lstnr = physicalModel->listener;
	float x = lstnr->x();
	float y = -lstnr->y();
	float z = lstnr->z();
	drawStilt(x, y, z);
	glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, listenerColor);
	glPushMatrix();
	glTranslatef(x, y, z);
	//glutSolidSphere(4,8,8);
	glutSolidCube(10);
	glPopMatrix();
}

void WFS3DCanvas::drawSources() {
	foreach(VirtualSource *src, virtualSourceModel->virtualSourceList) {
		float x = src->x();
		float y = -src->y();
		float z = src->z();
		drawStilt(x, y, z);
		glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, sourceColor);
		glPushMatrix();
		glTranslatef(x, y, z);
		glutSolidSphere(4,8,8);
		glPopMatrix();
		//qDebug() << "Loudspeaker: " << lspk->id << " - " << lspk->x() << ", " << lspk->y(); 
	}
}

void WFS3DCanvas::drawStilt(float x, float y, float z) {
	glDisable(GL_LIGHTING);
	glColor4fv(lightGrayColor);
	glLineWidth(1);
	glBegin(GL_LINES);
		glVertex3f(x,y,0);
		glVertex3f(x,y,z);
	glEnd();
	glPushMatrix();
	glTranslatef(x,y,0);
	GLUquadric* quad = gluNewQuadric();
	gluDisk(quad, 0, 4, 8, 1);
	glPopMatrix();
	glEnable(GL_LIGHTING);
}

void WFS3DCanvas::blendColor(GLfloat *base, GLfloat *target, GLfloat *result, float ratio) {
	for(int i = 0; i < 4; i++) {
		result[i] = base[i] * (1-ratio) + target[i] * ratio;
	}
}

void WFS3DCanvas::paintGL()  
{  
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glLoadIdentity();

	float camX = camDist * cos(degToRad(-camSwing)) * cos(degToRad(camElev));
	float camZ = camDist * sin(degToRad(camElev)) + physicalModel->listener->z();
	float camY = camDist * sin(degToRad(-camSwing)) * cos(degToRad(camElev));	
	gluLookAt(camX, camY, camZ,						//eye position
				0, 0, physicalModel->listener->z(),	//target position
				0, 0, 1);							//up vector

	//glTranslatef(0.0, 0.0, -10.0);
	////glRotatef(xRot / 16.0, 1.0, 0.0, 0.0);
	////glRotatef(yRot / 16.0, 0.0, 1.0, 0.0);
	////glRotatef(zRot / 16.0, 0.0, 0.0, 1.0);
	
	drawFloorGrid(1000,1000,100,10); //width, height, major interval, minor interval
	//drawAxes(2, 10); //thicker lines; 10 units for global origin

	// draw using glut primitives :)
	drawListener();
	drawLoudspeakers();
	drawSources();
} 