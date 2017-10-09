#pragma once

#include <QGLWidget>

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
	void draw();

	int xRot;
	int yRot;
	int zRot;

	QPoint lastPos;
};