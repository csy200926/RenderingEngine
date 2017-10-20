#pragma once

#include "RenderingGame.h"

#include <QGLWidget>

#include <qtimer.h>

class MyQGLWidget : public QGLWidget
{
	Q_OBJECT

		
public:
	MyQGLWidget(QWidget *parent);
	~MyQGLWidget();

protected:
	void initializeGL();
	void paintGL();
	void resizeGL(int width, int height);
	
private:

	RenderingGame game;

	void draw();

	int xRot;
	int yRot;
	int zRot;

	QPoint lastPos;

	QTimer myTimer;

private slots:
	void Update();


};
